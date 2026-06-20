#include "PID.h"

/**
 * @brief  初始化 PID 控制器
 * @param  pid:      PID 实例指针
 * @param  Kp/Ki/Kd: PID 系数
 * @param  out_min:  输出下限（如 -100）
 * @param  out_max:  输出上限（如 +100）
 */
void PID_Init(PID_t* pid, float Kp, float Ki, float Kd, float out_min, float out_max)
{
	pid->Kp = Kp;
	pid->Ki = Ki;
	pid->Kd = Kd;
	pid->integral    = 0.0f;
	pid->prev_error  = 0.0f;
	pid->out_min     = out_min;
	pid->out_max     = out_max;
	pid->integral_max = out_max;
}

/**
 * @brief  单次 PID 计算
 * @param  pid:      PID 实例
 * @param  setpoint: 目标值（编码器脉冲/10ms）
 * @param  actual:   实际值（编码器脉冲/10ms）
 * @param  dt:       控制周期（秒，0.01 = 100Hz）
 * @retval 输出值 (PWM 占空比，-100~100)
 */
float PID_Compute(PID_t* pid, float setpoint, float actual, float dt)
{
	float error = setpoint - actual;

	/* 比例项 */
	float p = pid->Kp * error;

	/* 积分项 */
	if (pid->Ki != 0.0f) {
		pid->integral += error * dt;
		if (pid->integral >  pid->integral_max) pid->integral =  pid->integral_max;
		if (pid->integral < -pid->integral_max) pid->integral = -pid->integral_max;
	}
	float i = pid->Ki * pid->integral;

	/* 微分项 */
	float d = 0.0f;
	if (dt > 0.0001f) {
		d = pid->Kd * (error - pid->prev_error) / dt;
	}
	pid->prev_error = error;

	/* 输出限幅 */
	float out = p + i + d;
	if (out > pid->out_max) out = pid->out_max;
	if (out < pid->out_min) out = pid->out_min;

	return out;
}

/**
 * @brief  复位 PID（清除积分和误差记忆）
 */
void PID_Reset(PID_t* pid)
{
	pid->integral   = 0.0f;
	pid->prev_error = 0.0f;
}
