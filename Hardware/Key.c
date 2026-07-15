#include "Key.h"

void Key_Init(void)
{
	GPIO_InitPin(GPIO_PB,  3, GPIO_IN_PU);  /* KEY1 */
	GPIO_InitPin(GPIO_PB,  4, GPIO_IN_PU);  /* KEY2 */
	GPIO_InitPin(GPIO_PB,  5, GPIO_IN_PU);  /* KEY3 */
	GPIO_InitPin(GPIO_PC, 13, GPIO_IN_PU);  /* KEY4 */
}

uint8_t Key_GetNum(void)
{
	static uint8_t prev1 = 1, prev2 = 1, prev3 = 1, prev4 = 1;
	uint8_t result = 0;
	uint8_t now;

	now = GPIO_ReadPin(GPIO_PB, 3);
	if (now == 0 && prev1 == 1) result = KEY_SPEED_UP;
	prev1 = now;

	now = GPIO_ReadPin(GPIO_PB, 4);
	if (now == 0 && prev2 == 1) result = KEY_SPEED_DOWN;
	prev2 = now;

	now = GPIO_ReadPin(GPIO_PB, 5);
	if (now == 0 && prev3 == 1) result = 3;
	prev3 = now;

	now = GPIO_ReadPin(GPIO_PC, 13);
	if (now == 0 && prev4 == 1) result = 4;
	prev4 = now;

	return result;
}
