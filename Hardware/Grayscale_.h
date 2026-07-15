#ifndef __GRAYSCALE_H__
#define __GRAYSCALE_H__

#include "fml_types.h"
#include "stm32f10x.h"

void Grayscale_Sensor_Init(void);
static void select_ch(u8 ch);
void Grayscale_ReadAll(u16* values);

#endif



