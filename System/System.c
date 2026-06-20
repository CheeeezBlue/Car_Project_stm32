#include "System.h"

/**
 * @brief  系统时钟初始化：优先HSE(8MHz)->PLL(x9)->72MHz，HSE失败则用HSI(8MHz)->PLL(x12)->48MHz
 * @param  无
 * @retval 无
 */
void System_Init(void)
{
	u32 timeout;
	u8 hse_ok = 0;

	RCC_HSEConfig(RCC_HSE_ON);
	for (timeout = 0; timeout < 0xFFFF; timeout++) {
		if (RCC_GetFlagStatus(RCC_FLAG_HSERDY) != RESET) { hse_ok = 1; break; }
	}

	if (hse_ok) {
		FLASH_SetLatency(FLASH_Latency_2);
		RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
	} else {
		/* HSE不可用，退回HSI */
		RCC_HSEConfig(RCC_HSE_OFF);
		FLASH_SetLatency(FLASH_Latency_1);
		RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_12);  /* 8/2*12 = 48MHz */
	}

	FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
	RCC_PLLCmd(ENABLE);
	while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

	RCC_HCLKConfig(RCC_SYSCLK_Div1);
	RCC_PCLK1Config(RCC_HCLK_Div2);
	RCC_PCLK2Config(RCC_HCLK_Div1);

	RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
	while (RCC_GetSYSCLKSource() != 0x08);

	SystemCoreClockUpdate();  /* 同步时钟变量，否则USART波特率计算错误 */
}

/**
 * @brief  NVIC中断优先级配置
 * @param  irq_ch: u8 — 中断通道号
 * @param  pre_prio: u8 — 抢占优先级 (0~15)
 * @param  sub_prio: u8 — 响应优先级 (0~15)
 * @retval 无
 */
void System_NVIC_Config(u8 irq_ch, u8 pre_prio, u8 sub_prio)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitTypeDef nvic;
	nvic.NVIC_IRQChannel = irq_ch;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	nvic.NVIC_IRQChannelPreemptionPriority = pre_prio;
	nvic.NVIC_IRQChannelSubPriority = sub_prio;
	NVIC_Init(&nvic);
}
