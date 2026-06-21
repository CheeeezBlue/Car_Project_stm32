#include "YawControl.h"
#include "../Driver/PID.h"

/* ---- Yaw 状态 ---- */
static PID_t  yaw_pid;
static float  yaw_target;       /* 目标角速度 (°/s) */
static float  yaw_diff;         /* 当前转速差 (pulses/10ms) */
static float  yaw_raw_out;      /* PID 原始输出 (调试用) */
static u8     yaw_enabled;

/* ================================================================
   初始化
   ================================================================ */
void YawControl_Init(void)
{
	/* 保守初值: Kp=1.0, Ki=0.2, Kd=0.01, 输出限幅 ±40 */
	PID_Init(&yaw_pid, 1.0f, 0.2f, 0.01f, -40.0f, 40.0f);
	yaw_target  = 0.0f;
	yaw_diff    = 0.0f;
	yaw_raw_out = 0.0f;
	yaw_enabled = 0;
}

/* ================================================================
   核心：每控制周期调用
   ================================================================ */
void YawControl_Update(float yaw_rate, float dt)
{
	if (!yaw_enabled) {
		yaw_diff    = 0.0f;
		yaw_raw_out = 0.0f;
		PID_Reset(&yaw_pid);
		return;
	}

	yaw_raw_out = PID_Compute(&yaw_pid, yaw_target, yaw_rate, dt);
	yaw_diff    = yaw_raw_out;
}

/* ================================================================
   命令接口
   ================================================================ */
void YawControl_SetTarget(float deg_per_s)
{
	yaw_target = deg_per_s;
}

void YawControl_SetPID(float kp, float ki, float kd)
{
	yaw_pid.Kp = kp;
	yaw_pid.Ki = ki;
	yaw_pid.Kd = kd;
}

void YawControl_SetLimit(float max_diff)
{
	yaw_pid.out_max =  max_diff;
	yaw_pid.out_min = -max_diff;
	yaw_pid.integral_max = max_diff;
}

void YawControl_Enable(void)
{
	PID_Reset(&yaw_pid);
	yaw_enabled = 1;
}

void YawControl_Disable(void)
{
	yaw_enabled = 0;
	yaw_diff    = 0.0f;
	PID_Reset(&yaw_pid);
}

/* ================================================================
   状态查询
   ================================================================ */
float YawControl_GetDiff(void)    { return yaw_diff; }
float YawControl_GetTarget(void)  { return yaw_target; }
float YawControl_GetOutput(void)  { return yaw_raw_out; }
float YawControl_GetKp(void)      { return yaw_pid.Kp; }
float YawControl_GetKi(void)      { return yaw_pid.Ki; }
float YawControl_GetKd(void)      { return yaw_pid.Kd; }
u8    YawControl_IsEnabled(void)  { return yaw_enabled; }
