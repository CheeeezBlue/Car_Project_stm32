#include "CarControl.h"
#include "../Driver/PID.h"
#include "../Driver/Encoder.h"
#include "../Hardware/Motor.h"

/* ---- 车辆状态 ---- */
static PID_t  pid_L, pid_R;
static float  pid_target_L, pid_target_R;
static float  pid_max_pulse = 135.0f;   /* 100% PWM 对应 ≈135 pulses/10ms */
static u8     pid_direct;         /* 1=直接脉冲目标(TGT), 0=Speed%换算 */
static s8     Speed_L, Speed_R;
static s16    Enc_L, Enc_R;       /* 原始增量（VOFA+/OLED 显示用） */
static float  enc_filt_L, enc_filt_R; /* IIR 滤波后的速度（PID 用） */
static float  ff_coeff_L = 0.758f;   /* 左轮前馈系数: base_PWM = target × coeff */
static float  ff_coeff_R = 0.713f;   /* 右轮前馈系数 */
static u8     open_loop;          /* 1=开环测试模式，固定PWM */
static s8     open_loop_pwm;      /* 开环PWM值 */

#define ENC_FILT_ALPHA  0.2f     /* IIR 系数：0~1，越小滤波越强 */

/* ================================================================
   初始化
   ================================================================ */
void CarControl_Init(void)
{
	PID_Init(&pid_L, 1.5f, 2.0f, 0.03f, -30.0f, 30.0f);
	PID_Init(&pid_R, 1.5f, 2.0f, 0.03f, -30.0f, 30.0f);
	Speed_L   = 0;
	Speed_R   = 0;
	pid_target_L = 0.0f;
	pid_target_R = 0.0f;
	pid_direct   = 0;
	enc_filt_L   = 0.0f;
	enc_filt_R   = 0.0f;
	open_loop    = 0;
	open_loop_pwm = 0;
}

/* ================================================================
   核心：每10ms一次的控制循环
   ================================================================ */
void CarControl_Update(void)
{
	/* 1. 编码器采样 — M/T 法测速 + 原始增量 */
	s16 raw_L, raw_R;
	float spd_L = Encoder_Update(ENC_LEFT,  &raw_L);
	float spd_R = Encoder_Update(ENC_RIGHT, &raw_R);

	/* 左轮编码器相位校正（A/B 交叉） */
	spd_L = -spd_L;
	Enc_L = -raw_L;
	Enc_R =  raw_R;

	/* 2. IIR 低通滤波 → PID 反馈（不污染 VOFA+/OLED 原始值） */
	enc_filt_L += ENC_FILT_ALPHA * (spd_L - enc_filt_L);
	enc_filt_R += ENC_FILT_ALPHA * (spd_R - enc_filt_R);

	/* 3. 目标值换算（Speed% → pulses/10ms） */
	if (!pid_direct) {
		pid_target_L = ((float)Speed_L / 100.0f) * pid_max_pulse;
		pid_target_R = ((float)Speed_R / 100.0f) * pid_max_pulse;
	}

	/* 4. 电机输出 */
	if (open_loop) {
		Motor_SetSpeed(MOTOR_LEFT,  open_loop_pwm);
		Motor_SetSpeed(MOTOR_RIGHT, open_loop_pwm);
	} else {
		/* 前馈：base PWM = target × coeff，吃掉80%+的驱动需求 */
		float base_L = pid_target_L * ff_coeff_L;
		float base_R = pid_target_R * ff_coeff_R;
		/* PID 仅修正残差，限幅 ±30 */
		float pid_L_out = PID_Compute(&pid_L, pid_target_L, enc_filt_L, 0.01f);
		float pid_R_out = PID_Compute(&pid_R, pid_target_R, enc_filt_R, 0.01f);
		float out_L = base_L + pid_L_out;
		float out_R = base_R + pid_R_out;
		if (out_L >  100.0f) out_L =  100.0f;
		if (out_L < -100.0f) out_L = -100.0f;
		if (out_R >  100.0f) out_R =  100.0f;
		if (out_R < -100.0f) out_R = -100.0f;
		Motor_SetSpeed(MOTOR_LEFT,  (s8)out_L);
		Motor_SetSpeed(MOTOR_RIGHT, (s8)out_R);
	}
}

/* ================================================================
   命令接口
   ================================================================ */
void Car_SetL(s8 left)
{
	Speed_L = left;
	pid_direct = 0;
}

void Car_SetR(s8 right)
{
	Speed_R = right;
	pid_direct = 0;
}

void Car_Stop(void)
{
	Speed_L = 0;
	Speed_R = 0;
	pid_direct = 0;
	PID_Reset(&pid_L);
	PID_Reset(&pid_R);
	open_loop = 0;
}

void Car_OpenLoop(s8 pwm)
{
	open_loop = 1;
	open_loop_pwm = pwm;
	pid_target_L = pid_target_R = (float)pwm;  /* VOFA+ 显示用 */
	pid_direct = 1;
}

void Car_SetTarget(s8 target)
{
	pid_target_L = pid_target_R = (float)target;
	pid_direct = 1;
}

void Car_SetPID(float kp, float ki, float kd)
{
	pid_L.Kp = pid_R.Kp = kp;
	pid_L.Ki = pid_R.Ki = ki;
	pid_L.Kd = pid_R.Kd = kd;
}

void Car_SetFF(float l, float r)
{
	ff_coeff_L = l;
	ff_coeff_R = r;
}

/* ================================================================
   状态查询（OLED 显示用）
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
	return motor ? pid_target_R : pid_target_L;
}

float Car_GetDisplayTarget(u8 motor)
{
	if (pid_direct)
		return motor ? pid_target_R : pid_target_L;   /* TGT/OL: 脉冲值 */
	else
		return (float)(motor ? Speed_R : Speed_L);    /* SPD: 百分比 */
}

float Car_GetKp(void) { return pid_L.Kp; }
float Car_GetKi(void) { return pid_L.Ki; }
float Car_GetKd(void) { return pid_L.Kd; }
float Car_GetFF_L(void) { return ff_coeff_L; }
float Car_GetFF_R(void) { return ff_coeff_R; }
