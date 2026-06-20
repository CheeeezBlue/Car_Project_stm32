#include "Key.h"

void Key_Init(void)
{
	GPIO_InitPin(GPIO_PA,  3, GPIO_IN_PU);  /* A3  = Speed+ */
	GPIO_InitPin(GPIO_PB, 11, GPIO_IN_PU);  /* B11 = Speed- */
}

uint8_t Key_GetNum(void)
{
	static uint8_t prev_up = 1, prev_dn = 1;
	uint8_t result = 0;
	uint8_t now;

	now = GPIO_ReadPin(GPIO_PA, 3);
	if (now == 0 && prev_up == 1) result = KEY_SPEED_UP;
	prev_up = now;

	now = GPIO_ReadPin(GPIO_PB, 11);
	if (now == 0 && prev_dn == 1) result = KEY_SPEED_DOWN;
	prev_dn = now;

	return result;
}
