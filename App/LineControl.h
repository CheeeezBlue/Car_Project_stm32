#ifndef __LINE_CONTROL_H__
#define __LINE_CONTROL_H__

#include "../Driver/fml_types.h"

void  LineControl_Init(void);
void  LineControl_Update(const u16* gray);

void  LineControl_SetSpeed(s8 base);
void  LineControl_SetKp(float kp);
void  LineControl_SetKi(float ki);
void  LineControl_SetKd(float kd);

void  LineControl_Enable(void);
void  LineControl_Disable(void);
u8    LineControl_IsEnabled(void);

float LineControl_GetKp(void);
float LineControl_GetKi(void);
float LineControl_GetKd(void);
float LineControl_GetError(void);
float LineControl_GetOutput(void);
s8    LineControl_GetBaseSpeed(void);

#endif
