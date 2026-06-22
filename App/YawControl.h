#ifndef __YAW_CONTROL_H__
#define __YAW_CONTROL_H__

#include "../Driver/fml_types.h"

/* Yaw PID 初始化 */
void YawControl_Init(void);

/* 每控制周期调用：输入当前角速度 → PID → 更新左右轮转速差 */
void YawControl_Update(float yaw_rate, float dt);

/* ---- 目标/参数设置 ---- */
void YawControl_SetTarget(float deg_per_s);   /* 目标角速度 (°/s)，+CCW/-CW */
void YawControl_SetPID(float kp, float ki, float kd);
void YawControl_SetKp(float kp);
void YawControl_SetKi(float ki);
void YawControl_SetKd(float kd);
void YawControl_SetLimit(float max_diff);     /* 输出差分上限 (pulses/10ms) */

/* ---- 状态查询 ---- */
float YawControl_GetDiff(void);      /* 转速差: 左=-diff/2, 右=+diff/2 */
float YawControl_GetTarget(void);    /* 当前目标角速度 */
float YawControl_GetOutput(void);    /* PID 原始输出 */
float YawControl_GetLastRate(void);  /* 最近一次角速度 (°/s) */
float YawControl_GetKp(void);
float YawControl_GetKi(void);
float YawControl_GetKd(void);
u8    YawControl_IsEnabled(void);

void YawControl_Enable(void);
void YawControl_Disable(void);

#endif
