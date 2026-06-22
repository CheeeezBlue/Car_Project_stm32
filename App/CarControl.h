#ifndef __CAR_CONTROL_H__
#define __CAR_CONTROL_H__

#include "fml_types.h"

/* 初始化PID控制器 */
void CarControl_Init(void);

/* 每10ms调用：读编码器 → 换算目标 → PID → 电机 */
void CarControl_Update(void);

/* ---- 速度/目标设置 ---- */
void Car_SetL(s8 left);
void Car_SetR(s8 right);
void Car_Stop(void);
void Car_SetTarget(s8 target);
void Car_SetLRPulse(float L, float R);      /* yaw 环用：独立设置左右脉冲目标 */
void Car_ApplyYawDiff(float diff); /* yaw 环用：当前 pid_target 均值 ± diff/2 */
s8   Car_GetSpeed(u8 motor);               /* 0=左, 1=右：当前 Speed% */

/* ---- 开环测试（临时：直驱PWM，编码器照常采样） ---- */
void Car_OpenLoop(s8 pwm);

/* ---- PID参数 ---- */
void Car_SetPID(float kp, float ki, float kd);
void Car_SetKp(float kp);
void Car_SetKi(float ki);
void Car_SetKd(float kd);

/* ---- 前馈系数 ---- */
void  Car_SetFF(float l, float r);
void  Car_SetFFOffset(float offset);
float Car_GetFF_L(void);
float Car_GetFF_R(void);
float Car_GetFFOffset(void);

/* ---- 状态查询（OLED用） ---- */
s16   Car_GetEnc(u8 motor);       /* 0=左, 1=右 */
float Car_GetFiltSpeed(u8 motor); /* IIR滤波后速度 */
float Car_GetTarget(u8 motor);
float Car_GetDisplayTarget(u8 motor); /* SPD模式返回Speed%/ TGT模式返回脉冲 */
float Car_GetKp(void);
float Car_GetKi(void);
float Car_GetKd(void);

#endif
