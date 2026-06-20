#ifndef __TIMER_H__
#define __TIMER_H__

#include "fml_types.h"

void Timer_Init(TIM_ID_t tim_id, u16 freq_hz);
void Timer_Start(TIM_ID_t tim_id);
void Timer_Stop(TIM_ID_t tim_id);
void Timer_Reset(TIM_ID_t tim_id);
u32  Timer_GetCount(TIM_ID_t tim_id);

/* 输入捕获：超声波回波测距 */
u32  Ultrasonic_Measure_us(void);

/* 软件延时，使用定时器轮询 */
void Timer_Delay_us(TIM_ID_t tim_id, u32 us);

#endif
