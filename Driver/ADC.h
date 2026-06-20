#ifndef __ADC_H__
#define __ADC_H__

#include "fml_types.h"

void ADCx_Init(void);
u16  ADC_Read(ADC_CH_t ch);
u16  ADC_ReadAvg(ADC_CH_t ch, u8 times);
void ADC_StartConv(ADC_CH_t ch);
u8   ADC_ConvDone(void);
u16  ADC_GetValue(void);

#endif
