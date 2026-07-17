#include "LineControl.h"
#include "CarControl.h"
#include "../Hardware/Grayscale.h"
#include "../Driver/PID.h"

static PID_t line_pid;
static float error;
static float filt_err;     /* IIR 低通滤波状态 */
#define LINE_FILT_ALPHA  0.4f
static float output;       /* 差速输出 (pulses/10ms) */
static u8    enabled;
static s8    base_speed;
static float last_side = 1.0f;
static u16   lost_cnt;
static s8    lost_saved_spd;  /* 丢线前原始速度 */

/* 丢线策略常量 */
#define LOST_FULL_DIFF     40.0f   /* 满舵差速 (pulses/10ms) */
#define LOST_FAST_RATIO    0.5f    /* 0~500ms: 50% 速度 */
#define LOST_SLOW_RATIO    0.3f    /* >500ms: 30% 速度 */
#define LOST_FAST_MIN_SPD  15      /* 0~500ms: 最低速度 */
#define LOST_SLOW_MIN_SPD  10      /* >500ms: 最低速度 */
#define LOST_TIMEOUT       50      /* 500ms 切换慢速 */

void LineControl_Init(void)
{
	PID_Init(&line_pid, 5.0f, 0.0f, 0.0f, -40.0f, 40.0f);
	error      = 0.0f;
	filt_err   = 0.0f;
	output     = 0.0f;
	enabled    = 0;
	base_speed = 30;
	last_side      = 1.0f;
	lost_cnt       = 0;
	lost_saved_spd = 30;
}

void LineControl_Update(const u16* gray)
{
	if (!enabled) return;

	u8 has_line = 0;
	for (u8 i = 0; i < GRAY_CHANNELS; i++) {
		if (gray[i] == 1) { has_line = 1; break; }
	}

	if (has_line) {
		float raw = Grayscale_Calculate_Error(gray);

		/* 刚恢复连线：还原速度 + 复位滤波器和PID */
		if (lost_cnt > 0) {
			Car_SetL(lost_saved_spd);
			Car_SetR(lost_saved_spd);
			filt_err = raw;
			PID_Reset(&line_pid);
		} else {
			filt_err += LINE_FILT_ALPHA * (raw - filt_err);
		}

		if (raw >  0.1f) last_side =  1.0f;
		if (raw < -0.1f) last_side = -1.0f;
		lost_cnt = 0;
		error = filt_err;

		output = PID_Compute(&line_pid, 0.0f, error, 0.01f);
		Car_SetYawDiff(output);

	} else {
		/* 首次丢线：保存当前速度 */
		if (lost_cnt == 0) {
			lost_saved_spd = Car_GetSpeed(0);
			if (lost_saved_spd <= 0) lost_saved_spd = base_speed;
		}

		/* 降速：0~500ms 较快，>500ms 更慢 */
		s8 spd;
		if (lost_cnt <= LOST_TIMEOUT) {
			spd = (s8)((float)lost_saved_spd * LOST_FAST_RATIO);
			if (spd < LOST_FAST_MIN_SPD) spd = LOST_FAST_MIN_SPD;
		} else {
			spd = (s8)((float)lost_saved_spd * LOST_SLOW_RATIO);
			if (spd < LOST_SLOW_MIN_SPD) spd = LOST_SLOW_MIN_SPD;
		}
		Car_SetL(spd);
		Car_SetR(spd);

		/* 满舵绕过PID，同时清空PID状态防回线反冲 */
		error  = last_side * 3.5f;
		output = -LOST_FULL_DIFF * last_side;
		Car_SetYawDiff(output);
		PID_Reset(&line_pid);

		lost_cnt++;
	}
}

void LineControl_Enable(void)
{
	enabled        = 1;
	error          = 0.0f;
	filt_err       = 0.0f;
	output         = 0.0f;
	last_side      = 1.0f;
	lost_cnt       = 0;
	lost_saved_spd = base_speed;
	PID_Reset(&line_pid);
	Car_SetYawDiff(0.0f);
}

void LineControl_Disable(void)
{
	enabled = 0;
	error   = 0.0f;
	output  = 0.0f;
	Car_SetYawDiff(0.0f);
}

void LineControl_SetSpeed(s8 base)
{
	base_speed = base;
	Car_SetL(base);
	Car_SetR(base);
}

void LineControl_SetKp(float kp) { line_pid.Kp = kp; }
void LineControl_SetKi(float ki) { line_pid.Ki = ki; }
void LineControl_SetKd(float kd) { line_pid.Kd = kd; }

u8    LineControl_IsEnabled(void)    { return enabled; }
float LineControl_GetKp(void)        { return line_pid.Kp; }
float LineControl_GetKi(void)        { return line_pid.Ki; }
float LineControl_GetKd(void)        { return line_pid.Kd; }
float LineControl_GetError(void)     { return error; }
float LineControl_GetOutput(void)    { return output; }
s8    LineControl_GetBaseSpeed(void) { return base_speed; }
