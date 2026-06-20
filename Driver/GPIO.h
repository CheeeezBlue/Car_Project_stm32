#ifndef __GPIO_H__
#define __GPIO_H__

#include "fml_types.h"

void GPIO_InitPin(GPIO_Port_t port, u8 pin, GPIO_Mode_t mode);
void GPIO_WritePin(GPIO_Port_t port, u8 pin, u8 val);
u8   GPIO_ReadPin(GPIO_Port_t port, u8 pin);
void GPIO_TogglePin(GPIO_Port_t port, u8 pin);
void GPIO_InitAll(void);

#endif
