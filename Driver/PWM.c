#include "PWM.h"

static TIM_TypeDef* const tim_map[] = { TIM2, TIM3, TIM4 };
static u16 period_arr[4];

/**
 * @brief  初始化定时器PWM输出（4通道全部使能，PWM1模式）
 * @param  tim_id: TIM_ID_t — 定时器标识
 * @param  freq_hz: u16 — PWM频率（Hz），定时器时钟均72MHz
 * @retval 无
 */
void PWM_Init(TIM_ID_t tim_id, u16 freq_hz)
{
	TIM_TypeDef* tim = tim_map[tim_id];
	u32 clk = 72000000;

	u16 psc, arr;
	if (freq_hz < 1) freq_hz = 1;
	if (freq_hz <= 100)      { psc = 7200; arr = clk / (7200UL * freq_hz) - 1; }
	else if (freq_hz <= 1000){ psc = 720;  arr = clk / (720UL * freq_hz)  - 1; }
	else if (freq_hz <= 10000){psc = 72;   arr = clk / (72UL * freq_hz)   - 1; }
	else                     { psc = 0;    arr = clk / freq_hz             - 1; }
	if (arr > 65535) arr = 65535;
	period_arr[tim_id] = arr;

	if (tim_id == TIM_ID_2)      RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	else if (tim_id == TIM_ID_3) RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	else                         RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	TIM_TimeBaseInitTypeDef tb;
	tb.TIM_ClockDivision = TIM_CKD_DIV1;
	tb.TIM_CounterMode = TIM_CounterMode_Up;
	tb.TIM_Period = arr;
	tb.TIM_Prescaler = psc;
	TIM_TimeBaseInit(tim, &tb);

	TIM_OCInitTypeDef oc;
	oc.TIM_OCMode = TIM_OCMode_PWM1;
	oc.TIM_OCPolarity = TIM_OCPolarity_High;
	oc.TIM_OutputState = TIM_OutputState_Enable;
	oc.TIM_Pulse = 0;

	TIM_OC1Init(tim, &oc);
	TIM_OC2Init(tim, &oc);
	TIM_OC3Init(tim, &oc);
	TIM_OC4Init(tim, &oc);

	TIM_OC1PreloadConfig(tim, TIM_OCPreload_Enable);
	TIM_OC2PreloadConfig(tim, TIM_OCPreload_Enable);
	TIM_OC3PreloadConfig(tim, TIM_OCPreload_Enable);
	TIM_OC4PreloadConfig(tim, TIM_OCPreload_Enable);
	TIM_ARRPreloadConfig(tim, ENABLE);
	TIM_CtrlPWMOutputs(tim, ENABLE);
	TIM_Cmd(tim, ENABLE);
}

/**
 * @brief  设置PWM占空比（绝对值0 ~ ARR）
 * @param  tim_id: TIM_ID_t — 定时器标识
 * @param  ch: PWM_CH_t — PWM通道
 * @param  duty: u16 — 占空比值
 * @retval 无
 */
void PWM_SetDuty(TIM_ID_t tim_id, PWM_CH_t ch, u16 duty)
{
	TIM_TypeDef* tim = tim_map[tim_id];
	if (duty > period_arr[tim_id]) duty = period_arr[tim_id];
	switch (ch) {
	case PWM_CH1: TIM_SetCompare1(tim, duty); break;
	case PWM_CH2: TIM_SetCompare2(tim, duty); break;
	case PWM_CH3: TIM_SetCompare3(tim, duty); break;
	case PWM_CH4: TIM_SetCompare4(tim, duty); break;
	}
}

/**
 * @brief  设置PWM占空比（百分比0~100）
 * @param  tim_id: TIM_ID_t — 定时器标识
 * @param  ch: PWM_CH_t — PWM通道
 * @param  percent: u8 — 百分比
 * @retval 无
 */
void PWM_SetDutyPercent(TIM_ID_t tim_id, PWM_CH_t ch, u8 percent)
{
	if (percent > 100) percent = 100;
	PWM_SetDuty(tim_id, ch, (u32)period_arr[tim_id] * percent / 100);
}

/**
 * @brief  双极性PWM占空比设置（-100~100），用于电机正反转
 *         正数时 ch_fwd 输出PWM、ch_rev 输出0；负数时反之
 * @param  tim_id: 定时器标识
 * @param  ch_fwd: 正转通道
 * @param  ch_rev: 反转通道
 * @param  duty: 占空比百分比（-100~100），正数正转，负数反转，0停止
 */
void PWM_SetDutyBipolar(TIM_ID_t tim_id, PWM_CH_t ch_fwd, PWM_CH_t ch_rev, s8 duty)
{
	if (duty > 100) duty = 100;
	if (duty < -100) duty = -100;

	if (duty >= 0) {
		PWM_SetDutyPercent(tim_id, ch_fwd, (u8)duty);
		PWM_SetDutyPercent(tim_id, ch_rev, 0);
	} else {
		PWM_SetDutyPercent(tim_id, ch_fwd, 0);
		PWM_SetDutyPercent(tim_id, ch_rev, (u8)(-duty));
	}
}
