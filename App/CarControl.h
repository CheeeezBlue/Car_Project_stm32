#ifndef __CAR_CONTROL_H__
#define __CAR_CONTROL_H__

#include "fml_types.h"

/* 初始化PID控制器 */
void CarControl_Init(void);

/* 每10ms调用：读编码器 → 斜坡规划 → 仿射前馈+PID → 电机 */
void CarControl_Update(void);

/* ---- 速度/目标设置 ---- */
void Car_SetL(s8 left);
void Car_SetR(s8 right);
void Car_Stop(void);
void Car_SetTarget(s8 target);
void Car_SetLRPulse(float L, float R);      /* yaw 环用：独立设置左右脉冲目标 */
void Car_SetYawDiff(float diff);            /* yaw 环用：存储差速值，下周期叠加到斜坡输出 */
s8   Car_GetSpeed(u8 motor);               /* 0=左, 1=右：当前 Speed% */

/* ---- 开环测试 ---- */
void Car_OpenLoop(s8 pwm);

/* ---- PID参数 ---- */
void Car_SetPID(float kp, float ki, float kd);
void Car_SetKp(float kp);
void Car_SetKi(float ki);
void Car_SetKd(float kd);

/* ---- 仿射前馈: PWM_FF = k * |v| + b * sign(v) ---- */
void  Car_SetFF(float l, float r);          /* k 增益（左/右） */
void  Car_SetFFOffset(float offset);        /* b 偏置（左右同值） */
void  Car_SetFFBias(float l, float r);      /* b 偏置（左右独立） */
void  Car_SetDeadband(float db);            /* 前馈死区阈值 */
void  Car_SetRampStep(float step);          /* 斜坡每周期最大步长 */

/* ---- 状态查询 ---- */
s16   Car_GetEnc(u8 motor);
float Car_GetFiltSpeed(u8 motor);
float Car_GetTarget(u8 motor);
float Car_GetDisplayTarget(u8 motor);
float Car_GetKp(void);
float Car_GetKi(void);
float Car_GetKd(void);
float Car_GetFF_L(void);        /* k 左 */
float Car_GetFF_R(void);        /* k 右 */
float Car_GetFFOffset(void);    /* b (兼容旧接口, 返回左轮b) */
float Car_GetFFBias_L(void);    /* b 左 */
float Car_GetFFBias_R(void);    /* b 右 */
float Car_GetDeadband(void);
float Car_GetRampStep(void);

#endif
