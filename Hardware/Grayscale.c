#include "Grayscale.h"
#include "../Driver/GPIO.h"
#include "../System/Delay.h"

/* 引脚 (与 GPIO_InitAll 一致)：
   OUT → PA5 (浮空输入)
   AD0 → PA4 (推挽输出)
   AD1 → PA0 (推挽输出)
   AD2 → PA1 (推挽输出) */
#define AD0_P  GPIO_PA
#define AD0_B  4
#define AD1_P  GPIO_PA
#define AD1_B  0
#define AD2_P  GPIO_PA
#define AD2_B  1
#define OUT_P  GPIO_PA
#define OUT_B  5

void Grayscale_Init(void)
{
	/* 引脚已在 GPIO_InitAll() 中初始化 */
	GPIO_WritePin(AD0_P, AD0_B, 0);
	GPIO_WritePin(AD1_P, AD1_B, 0);
	GPIO_WritePin(AD2_P, AD2_B, 0);
}
/* 选择通道 */
static void select_ch(u8 ch)
{
	GPIO_WritePin(AD0_P, AD0_B, (ch >> 0) & 1);
	GPIO_WritePin(AD1_P, AD1_B, (ch >> 1) & 1);
	GPIO_WritePin(AD2_P, AD2_B, (ch >> 2) & 1);
}

void Grayscale_ReadAll(u16* values)
{
	for (u8 i = 0; i < GRAY_CHANNELS; i++) {
		select_ch(i);
		Delay_us(100);                  /* 多路选择器 + 传感器稳定 */
		values[i] = GPIO_ReadPin(OUT_P, OUT_B) ? 1 : 0;  /* 1=黑,0=白 */
	}
}

float Grayscale_Calculate_Error(const u16* values)
{
	float sum = 0.0f;
	u8 count = 0;

	for (u8 i = 0; i < GRAY_CHANNELS; i++) {
		if (values[i] == 1) {   /* 1 = 黑线 */
			sum += (float)i;
			count++;
		}
	}

	if (count == 0) return 0.0f;          /* 没线：直行 */
	return (sum / count) - 3.5f;          /* 负=偏左, 正=偏右 */
}
