#include "ADC.h"

static u8 adc_ready = 0;

/**
 * @brief  初始化ADC1（单次转换模式，软件触发）
 * @param  无
 * @retval 无
 */
void ADCx_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	ADC_InitTypeDef cfg;
	cfg.ADC_Mode = ADC_Mode_Independent;
	cfg.ADC_ScanConvMode = DISABLE;
	cfg.ADC_ContinuousConvMode = DISABLE;
	cfg.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	cfg.ADC_DataAlign = ADC_DataAlign_Right;
	cfg.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &cfg);
	ADC_Cmd(ADC1, ENABLE);

	/* 校准 */
	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));
}

/**
 * @brief  启动单通道ADC转换
 * @param  ch: ADC_CH_t — ADC通道
 * @retval 无
 */
void ADC_StartConv(ADC_CH_t ch)
{
	ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_55Cycles5);
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	adc_ready = 0;
}

/**
 * @brief  检查ADC转换是否完成
 * @param  无
 * @retval u8 — 1=完成, 0=未完成
 */
u8 ADC_ConvDone(void)
{
	if (adc_ready) return 1;
	if (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)) {
		adc_ready = 1;
		return 1;
	}
	return 0;
}

/**
 * @brief  获取ADC转换结果
 * @param  无
 * @retval u16 — ADC值 (0~4095)
 */
u16 ADC_GetValue(void)
{
	return ADC_GetConversionValue(ADC1);
}

/**
 * @brief  读取ADC通道值（阻塞单次）
 * @param  ch: ADC_CH_t — ADC通道
 * @retval u16 — ADC值 (0~4095)
 */
u16 ADC_Read(ADC_CH_t ch)
{
	ADC_StartConv(ch);
	while (!ADC_ConvDone());
	return ADC_GetValue();
}

/**
 * @brief  多次采样取平均值
 * @param  ch: ADC_CH_t — ADC通道
 * @param  times: u8 — 采样次数 (1~255)
 * @retval u16 — 平均值
 */
u16 ADC_ReadAvg(ADC_CH_t ch, u8 times)
{
	u32 sum = 0;
	for (u8 i = 0; i < times; i++) sum += ADC_Read(ch);
	return (u16)(sum / times);
}
