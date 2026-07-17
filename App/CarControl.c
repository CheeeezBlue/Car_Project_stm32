#include "CarControl.h"
#include "../Driver/PID.h"
#include "../Driver/Encoder.h"
#include "../Hardware/Motor.h"

/* ================================================================
   车辆状态
   ================================================================ */
static PID_t  pid_L, pid_R;
static float  pid_max_pulse = 135.0f;   /* 100% PWM 对应 ≈135 pulses/10ms */
static u8     pid_direct;               /* 1=直接脉冲目标(TGT), 0=Speed%换算 */
static s8     Speed_L, Speed_R;

/* 编码器 — 原始增量 + IIR 滤波 */
static s16    Enc_L, Enc_R;
static float  enc_filt_L, enc_filt_R;
#define ENC_FILT_ALPHA  0.2f

/* 速度斜坡规划 */
static float  cmd_target_L, cmd_target_R;   /* 用户指令目标 (pulses/10ms) */
static float  ramp_L, ramp_R;               /* 斜坡输出 */
static float  ramp_max_step = 5.0f;         /* 每周期最大变化量 */

/* 仿射前馈: PWM_FF = k * |v| + b * sign(v),  |v| ≤ deadband 时置零 */
static float  ff_k_L = 0.745f, ff_k_R = 0.690f;
static float  ff_b_L = 4.55f, ff_b_R = 4.55f;
static float  ff_deadband = 1.0f;           /* 低于此值前馈置零 */

/* Yaw 差速补偿 (由 Car_SetYawDiff 写入, CarControl_Update 消费) */
static float  yaw_diff;

/* 开环模式 */
static u8     open_loop, open_loop_was;
static s8     open_loop_pwm;

/* 刹车 */
static u8     brake_timer;

/* ================================================================
   初始化
   ================================================================ */
void CarControl_Init(void)
{
	PID_Init(&pid_L, 0.5f, 0.5f, 0.07f, -100.0f, 100.0f);
	PID_Init(&pid_R, 0.5f, 0.5f, 0.07f, -100.0f, 100.0f);
	Speed_L        = 0;
	Speed_R        = 0;
	pid_direct     = 0;
	enc_filt_L     = 0.0f;
	enc_filt_R     = 0.0f;
	cmd_target_L   = 0.0f;
	cmd_target_R   = 0.0f;
	ramp_L         = 0.0f;
	ramp_R         = 0.0f;
	yaw_diff       = 0.0f;
	open_loop      = 0;
	open_loop_was  = 0;
	open_loop_pwm  = 0;
	brake_timer    = 0;
}

/* ================================================================
   核心：每10ms一次的控制循环
   ================================================================ */
void CarControl_Update(void)
{
	/* ---- 1. 编码器采样 ---- */
	s16 raw_L, raw_R;
	float spd_L = Encoder_Update(ENC_LEFT,  &raw_L);
	float spd_R = Encoder_Update(ENC_RIGHT, &raw_R);
	spd_R = -spd_R;
	Enc_L = raw_L;
	Enc_R = -raw_R;

	/* ---- 2. IIR 低通滤波 ---- */
	enc_filt_L += ENC_FILT_ALPHA * (spd_L - enc_filt_L);
	enc_filt_R += ENC_FILT_ALPHA * (spd_R - enc_filt_R);

	/* ---- 3. 用户指令 → cmd_target ---- */
	if (!pid_direct) {
		cmd_target_L = ((float)Speed_L / 100.0f) * pid_max_pulse;
		cmd_target_R = ((float)Speed_R / 100.0f) * pid_max_pulse;
	}

	/* ---- 4. 速度斜坡规划 ---- */
	float step = ramp_max_step;
	float diff;
	diff = cmd_target_L - ramp_L;
	if      (diff >  step) ramp_L += step;
	else if (diff < -step) ramp_L -= step;
	else                   ramp_L  = cmd_target_L;

	diff = cmd_target_R - ramp_R;
	if      (diff >  step) ramp_R += step;
	else if (diff < -step) ramp_R -= step;
	else                   ramp_R  = cmd_target_R;

	/* ---- 5. Yaw 差速叠加 → 最终 PID 目标 ---- */
	float y2 = yaw_diff / 2.0f;
	float tgt_L = ramp_L - y2;
	float tgt_R = ramp_R + y2;

	/* ---- 6. 刹车 ---- */
	if (brake_timer) {
		Motor_SetSpeed(MOTOR_LEFT,  0);
		Motor_SetSpeed(MOTOR_RIGHT, 0);
		enc_filt_L = 0.0f;
		enc_filt_R = 0.0f;
		brake_timer--;
		return;
	}

	/* ---- 7. 开环模式 ---- */
	if (open_loop) {
		Motor_SetSpeed(MOTOR_LEFT,  open_loop_pwm);
		Motor_SetSpeed(MOTOR_RIGHT, open_loop_pwm);
		open_loop_was = 1;
		return;
	}
	/* 刚退出开环：斜坡同步到当前实际速度，平滑过渡 */
	if (open_loop_was) {
		ramp_L = enc_filt_L;
		ramp_R = enc_filt_R;
		open_loop_was = 0;
	}

	/* ---- 8. 仿射前馈 ---- */
	float ff_L = 0.0f, ff_R = 0.0f;
	float absL = (tgt_L > 0) ? tgt_L : -tgt_L;
	float absR = (tgt_R > 0) ? tgt_R : -tgt_R;
	if (absL > ff_deadband)
		ff_L = absL * ff_k_L + (tgt_L > 0 ? ff_b_L : -ff_b_L);
	if (absR > ff_deadband)
		ff_R = absR * ff_k_R + (tgt_R > 0 ? ff_b_R : -ff_b_R);

	/* ---- 9. PID 闭环修正 ---- */
	float pid_L_out = PID_Compute(&pid_L, tgt_L, enc_filt_L, 0.01f);
	float pid_R_out = PID_Compute(&pid_R, tgt_R, enc_filt_R, 0.01f);

	/* ---- 10. 叠加输出 + 限幅 ---- */
	float out_L = ff_L + pid_L_out;
	float out_R = ff_R + pid_R_out;
	if (out_L >  100.0f) out_L =  100.0f;
	if (out_L < -100.0f) out_L = -100.0f;
	if (out_R >  100.0f) out_R =  100.0f;
	if (out_R < -100.0f) out_R = -100.0f;
	Motor_SetSpeed(MOTOR_LEFT,  (s8)out_L);
	Motor_SetSpeed(MOTOR_RIGHT, (s8)out_R);
}

/* ================================================================
   命令接口
   ================================================================ */
void Car_SetL(s8 left)
{
	Speed_L = left;
	pid_direct = 0;
	open_loop = 0;
}

void Car_SetR(s8 right)
{
	Speed_R = right;
	pid_direct = 0;
	open_loop = 0;
}

void Car_Stop(void)
{
	Speed_L   = 0;
	Speed_R   = 0;
	pid_direct = 0;
	cmd_target_L = 0.0f;
	cmd_target_R = 0.0f;
	ramp_L    = 0.0f;
	ramp_R    = 0.0f;
	yaw_diff  = 0.0f;
	PID_Reset(&pid_L);
	PID_Reset(&pid_R);
	open_loop = 0;
	brake_timer = 0;
}

void Car_OpenLoop(s8 pwm)
{
	open_loop     = 1;
	open_loop_pwm = pwm;
	pid_direct    = 1;
}

void Car_SetTarget(s8 target)
{
	cmd_target_L = cmd_target_R = (float)target;
	pid_direct = 1;
	open_loop  = 0;
}

void Car_SetLRPulse(float L, float R)
{
	cmd_target_L = L;
	cmd_target_R = R;
	pid_direct = 1;
	open_loop  = 0;
}

void Car_SetYawDiff(float diff)
{
	yaw_diff = diff;
}

s8 Car_GetSpeed(u8 motor) { return motor ? Speed_R : Speed_L; }

/* ================================================================
   PID 参数
   ================================================================ */
void Car_SetPID(float kp, float ki, float kd)
{
	pid_L.Kp = pid_R.Kp = kp;
	pid_L.Ki = pid_R.Ki = ki;
	pid_L.Kd = pid_R.Kd = kd;
}

void Car_SetKp(float kp) { pid_L.Kp = pid_R.Kp = kp; }
void Car_SetKi(float ki) { pid_L.Ki = pid_R.Ki = ki; }
void Car_SetKd(float kd) { pid_L.Kd = pid_R.Kd = kd; }

/* ================================================================
   前馈参数
   ================================================================ */
void Car_SetFF(float l, float r)
{
	ff_k_L = l;
	ff_k_R = r;
}

void Car_SetFFOffset(float offset)
{
	ff_b_L = offset;
	ff_b_R = offset;
}

void Car_SetFFBias(float l, float r)
{
	ff_b_L = l;
	ff_b_R = r;
}

void Car_SetDeadband(float db)
{
	ff_deadband = db;
}

void Car_SetRampStep(float step)
{
	ramp_max_step = step;
}

/* ================================================================
   状态查询
   ================================================================ */
s16 Car_GetEnc(u8 motor)
{
	return motor ? Enc_R : Enc_L;
}

float Car_GetFiltSpeed(u8 motor)
{
	return motor ? enc_filt_R : enc_filt_L;
}

float Car_GetTarget(u8 motor)
{
	/* 返回 PID 正在跟踪的目标值（斜坡 ± yaw） */
	float y2 = yaw_diff / 2.0f;
	return motor ? (ramp_R + y2) : (ramp_L - y2);
}

float Car_GetDisplayTarget(u8 motor)
{
	if (pid_direct)
		return motor ? cmd_target_R : cmd_target_L;
	else
		return (float)(motor ? Speed_R : Speed_L);
}

float Car_GetKp(void)        { return pid_L.Kp; }
float Car_GetKi(void)        { return pid_L.Ki; }
float Car_GetKd(void)        { return pid_L.Kd; }
float Car_GetFF_L(void)      { return ff_k_L; }
float Car_GetFF_R(void)      { return ff_k_R; }
float Car_GetFFOffset(void)  { return ff_b_L; }
float Car_GetFFBias_L(void)  { return ff_b_L; }
float Car_GetFFBias_R(void)  { return ff_b_R; }
float Car_GetDeadband(void)  { return ff_deadband; }
float Car_GetRampStep(void)  { return ramp_max_step; }
