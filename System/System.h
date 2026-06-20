#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include "stm32f10x.h"

void System_Init(void);
void System_NVIC_Config(u8 irq_ch, u8 pre_prio, u8 sub_prio);

#endif
