#ifndef __GRAYSCALE_H__
#define __GRAYSCALE_H__

#include "../Driver/fml_types.h"

#define GRAY_CHANNELS  8

void Grayscale_Init(void);
void Grayscale_ReadAll(u16* values);          /* values[0..7] = 0(黑) ~ 1(白) */
float Grayscale_Calculate_Error(const u16* values); /* 加权平均法: 相对中心的偏移(-3.5~3.5) */

#endif
