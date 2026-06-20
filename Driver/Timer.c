#include "Timer.h"

static TIM_TypeDef* const tim_map[] = { TIM2, TIM3, TIM4 };

/**
 * @brief  初始化定时器基础计时功能
 * @param  tim_id: TIM_ID_t — 定时器标识
 * @param  freq_hz: u16 — 计数频率（Hz），影响定时精度
 * @retval 无
 */
void Timer_Init(TIM_ID_t tim_id, u16 freq_hz)
{
	TIM_TypeDef* tim = tim_map[tim_id];
	u16 psc = (72000000UL / freq_hz) - 1;

	if (tim_id == TIM_ID_2)  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	else if (tim_id == TIM_ID_3) RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	else                     RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	TIM_TimeBaseInitTypeDef tb;
	tb.TIM_ClockDivision = TIM_CKD_DIV1;
	tb.TIM_CounterMode = TIM_CounterMode_Up;
	tb.TIM_Period = 65535;
	tb.TIM_Prescaler = psc;
	TIM_TimeBaseInit(tim, &tb);
}

void Timer_Start(TIM_ID_t tim_id) { TIM_Cmd(tim_map[tim_id], ENABLE); }
void Timer_Stop(TIM_ID_t tim_id)  { TIM_Cmd(tim_map[tim_id], DISABLE); }
void Timer_Reset(TIM_ID_t tim_id) { TIM_SetCounter(tim_map[tim_id], 0); }
u32  Timer_GetCount(TIM_ID_t tim_id) { return TIM_GetCounter(tim_map[tim_id]); }

/**
 * @brief  定时器微秒延时（阻塞轮询）
 * @param  tim_id: TIM_ID_t — 定时器标识
 * @param  us: u32 — 延时微秒数
 * @retval 无
 */
void Timer_Delay_us(TIM_ID_t tim_id, u32 us)
{
	u16 psc_val = 72 - 1;  /* 1MHz计数频率 */

	TIM_TypeDef* tim = tim_map[tim_id];

	TIM_TimeBaseInitTypeDef tb;
	tb.TIM_ClockDivision = TIM_CKD_DIV1;
	tb.TIM_CounterMode = TIM_CounterMode_Up;
	tb.TIM_Period = 65535;
	tb.TIM_Prescaler = psc_val;
	TIM_TimeBaseInit(tim, &tb);
	TIM_SetCounter(tim, 0);
	TIM_Cmd(tim, ENABLE);

	while (TIM_GetCounter(tim) < us);
	TIM_Cmd(tim, DISABLE);
}

/**
 * @brief  超声波测距：触发并测量回波脉宽（TIM4_CH2/PB7输入捕获）
 * @param  无
 * @retval u32 — 回波脉宽（微秒），0=超时
 */
u32 Ultrasonic_Measure_us(void)
{
	/* 触发：PB6输出10us高脉冲 */
	GPIO_WriteBit(GPIOB, GPIO_Pin_6, Bit_SET);
	Timer_Delay_us(TIM_ID_4, 15);
	GPIO_WriteBit(GPIOB, GPIO_Pin_6, Bit_RESET);

	/* TIM4初始化输入捕获（PB7 -> TIM4_CH2） */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	TIM_ICInitTypeDef ic;
	ic.TIM_Channel = TIM_Channel_2;
	ic.TIM_ICPolarity = TIM_ICPolarity_Rising;
	ic.TIM_ICSelection = TIM_ICSelection_DirectTI;
	ic.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	ic.TIM_ICFilter = 0;
	TIM_ICInit(TIM4, &ic);

	TIM_SelectInputTrigger(TIM4, TIM_TS_TI2FP2);
	TIM_SelectSlaveMode(TIM4, TIM_SlaveMode_Reset);
	TIM_Cmd(TIM4, ENABLE);

	/* 等待上升沿（超时25ms ≈ 4m距离上限） */
	u32 timeout = 25000;
	while (!GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7) && --timeout);
	if (!timeout) { TIM_Cmd(TIM4, DISABLE); return 0; }

	TIM_SetCounter(TIM4, 0);

	/* 等待下降沿 */
	timeout = 25000;
	while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7) && --timeout);
	u32 pulse = TIM_GetCounter(TIM4);
	TIM_Cmd(TIM4, DISABLE);

	return pulse ? pulse : 0;
}
