#ifndef __PID_H__
#define __PID_H__

typedef struct {
	float Kp;            /* 比例系数 */
	float Ki;            /* 积分系数 */
	float Kd;            /* 微分系数 */
	float integral;      /* 积分累加值 */
	float prev_error;    /* 上次误差（微分用） */
	float out_min;       /* 输出下限 (PWM: -100) */
	float out_max;       /* 输出上限 (PWM: +100) */
	float integral_max;  /* 积分限幅（防饱和） */
} PID_t;

void  PID_Init(PID_t* pid, float Kp, float Ki, float Kd, float out_min, float out_max);
float PID_Compute(PID_t* pid, float setpoint, float actual, float dt);
void  PID_Reset(PID_t* pid);

#endif
