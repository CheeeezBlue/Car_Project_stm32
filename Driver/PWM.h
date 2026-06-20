#ifndef __PWM_H__
#define __PWM_H__

#include "fml_types.h"

void PWM_Init(TIM_ID_t tim_id, u16 freq_hz);
void PWM_SetDuty(TIM_ID_t tim_id, PWM_CH_t ch, u16 duty);
void PWM_SetDutyPercent(TIM_ID_t tim_id, PWM_CH_t ch, u8 percent);
void PWM_SetDutyBipolar(TIM_ID_t tim_id, PWM_CH_t ch_fwd, PWM_CH_t ch_rev, s8 duty);

#endif
