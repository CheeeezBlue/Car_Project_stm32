#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

#include "stm32f10x.h"

void IWDG_Init(u16 ms);
void IWDG_Feed(void);

#endif
