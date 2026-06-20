#ifndef __MOTOR_H__
#define __MOTOR_H__

#include "fml_types.h"

typedef enum { MOTOR_LEFT = 0, MOTOR_RIGHT } Motor_ID_t;

void  Motor_Init(void);
void  Motor_SetSpeed(Motor_ID_t id, s8 duty);
float Motor_GetLastPWM(Motor_ID_t id);

#endif
