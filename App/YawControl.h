#ifndef __YAW_CONTROL_H__
#define __YAW_CONTROL_H__

#include "../Driver/fml_types.h"

void  YawControl_Init(void);
void  YawControl_Update(float yaw_rate, float dt);
void  YawControl_Enable(void);
void  YawControl_Disable(void);

void  YawControl_SetHeadingKp(float kp);
void  YawControl_SetHeadingKi(float ki);
void  YawControl_SetHeadingKd(float kd);
void  YawControl_SetRateKp(float kp);
void  YawControl_SetRateKi(float ki);
void  YawControl_SetRateKd(float kd);
void  YawControl_SetRateLimit(float max_deg_per_s);
void  YawControl_SetLimit(float max_diff);

float YawControl_GetDiff(void);
float YawControl_GetHeading(void);
float YawControl_GetHeadingTarget(void);
float YawControl_GetRateTarget(void);
float YawControl_GetLastRate(void);
float YawControl_GetHeadingKp(void);
float YawControl_GetHeadingKi(void);
float YawControl_GetHeadingKd(void);
float YawControl_GetRateKp(void);
float YawControl_GetRateKi(void);
float YawControl_GetRateKd(void);
float YawControl_GetRateLimit(void);
u8    YawControl_IsEnabled(void);

#endif
