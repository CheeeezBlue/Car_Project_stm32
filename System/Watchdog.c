#include "Watchdog.h"

/**
 * @brief  初始化独立看门狗
 * @param  ms: u16 — 超时时间（ms），最大约4096ms，实际值受LSI精度影响
 * @retval 无
 */
void IWDG_Init(u16 ms)
{
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

	/* LSI≈40kHz, 预分频32 → 1.25kHz → 0.8ms/tick */
	u16 ticks;
	u8 pr;
	if (ms <= 3200)      { pr = IWDG_Prescaler_32;  ticks = ms * 10 / 8; }
	else if (ms <= 6400) { pr = IWDG_Prescaler_64;  ticks = ms * 10 / 16; }
	else                 { pr = IWDG_Prescaler_128; ticks = ms * 10 / 32; }

	if (ticks > 4095) ticks = 4095;
	IWDG_SetPrescaler(pr);
	IWDG_SetReload(ticks);
	IWDG_ReloadCounter();
	IWDG_Enable();
}

/**
 * @brief  喂狗（重装载计数器）
 * @param  无
 * @retval 无
 */
void IWDG_Feed(void)
{
	IWDG_ReloadCounter();
}
