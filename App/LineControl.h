#ifndef __LINE_CONTROL_H__
#define __LINE_CONTROL_H__

#include "../Driver/fml_types.h"

void  LineControl_Init(void);
void  LineControl_Update(const u16* gray);

void  LineControl_SetSpeed(s8 base);   /* LSPD: 同时设置两轮速度% */
void  LineControl_SetKp(float kp);     /* LKP */
void  LineControl_SetKi(float ki);     /* LKI */
void  LineControl_SetKd(float kd);     /* LKD */

void  LineControl_Enable(void);        /* 启用巡线 */
void  LineControl_Disable(void);       /* 关闭巡线 */
u8    LineControl_IsEnabled(void);

float LineControl_GetKp(void);
float LineControl_GetKi(void);
float LineControl_GetKd(void);
float LineControl_GetError(void);      /* 灰度位置误差 (-3.5~+3.5) */
float LineControl_GetOutput(void);     /* 差速输出 (pulses/10ms) */
s8    LineControl_GetBaseSpeed(void);

#endif
