#include "GPIO.h"

static GPIO_TypeDef* const port_map[] = { GPIOA, GPIOB, GPIOC };

/**
 * @brief  初始化单个GPIO引脚模式
 * @param  port: GPIO_Port_t — 端口号 (GPIO_PA/GPIO_PB/GPIO_PC)
 * @param  pin: u8 — 引脚号 (0~15)
 * @param  mode: GPIO_Mode_t — 引脚模式
 * @retval 无
 */
void GPIO_InitPin(GPIO_Port_t port, u8 pin, GPIO_Mode_t mode)
{
	GPIO_TypeDef* gpio = port_map[port];
	u32 pin_mask = 1 << pin;

	RCC_APB2PeriphClockCmd((port == GPIO_PA) ? RCC_APB2Periph_GPIOA :
	                       (port == GPIO_PB) ? RCC_APB2Periph_GPIOB :
	                       RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitTypeDef cfg;
	cfg.GPIO_Pin = pin_mask;
	cfg.GPIO_Speed = GPIO_Speed_50MHz;

	switch (mode) {
	case GPIO_OUT_PP:  cfg.GPIO_Mode = GPIO_Mode_Out_PP; break;
	case GPIO_OUT_OD:  cfg.GPIO_Mode = GPIO_Mode_Out_OD; break;
	case GPIO_IN_PU:   cfg.GPIO_Mode = GPIO_Mode_IPU;    break;
	case GPIO_IN_PD:   cfg.GPIO_Mode = GPIO_Mode_IPD;    break;
	case GPIO_IN_FLOAT:cfg.GPIO_Mode = GPIO_Mode_IN_FLOATING; break;
	case GPIO_AF_PP:   cfg.GPIO_Mode = GPIO_Mode_AF_PP;  break;
	case GPIO_AF_OD:   cfg.GPIO_Mode = GPIO_Mode_AF_OD;  break;
	case GPIO_AIN:     cfg.GPIO_Mode = GPIO_Mode_AIN;    break;
	}
	GPIO_Init(gpio, &cfg);
}

/**
 * @brief  写GPIO引脚电平
 * @param  port: GPIO_Port_t — 端口号
 * @param  pin: u8 — 引脚号 (0~15)
 * @param  val: u8 — 0=低电平, 非0=高电平
 * @retval 无
 */
void GPIO_WritePin(GPIO_Port_t port, u8 pin, u8 val)
{
	if (val) GPIO_SetBits(port_map[port], 1 << pin);
	else     GPIO_ResetBits(port_map[port], 1 << pin);
}

/**
 * @brief  读GPIO引脚电平
 * @param  port: GPIO_Port_t — 端口号
 * @param  pin: u8 — 引脚号 (0~15)
 * @retval 0=低电平, 1=高电平
 */
u8 GPIO_ReadPin(GPIO_Port_t port, u8 pin)
{
	return (GPIO_ReadInputDataBit(port_map[port], 1 << pin)) ? 1 : 0;
}

/**
 * @brief  翻转GPIO引脚电平
 * @param  port: GPIO_Port_t — 端口号
 * @param  pin: u8 — 引脚号 (0~15)
 * @retval 无
 */
void GPIO_TogglePin(GPIO_Port_t port, u8 pin)
{
	GPIO_WriteBit(port_map[port], 1 << pin,
	              (BitAction)(1 - GPIO_ReadOutputDataBit(port_map[port], 1 << pin)));
}

/**
 * @brief  一次性初始化所有项目固定引脚（含JTAG禁用）
 * @param  无
 * @retval 无
 */
void GPIO_InitAll(void)
{
	/* 禁用JTAG，保留SWD（SWDIO=PA13, SWCLK=PA14仍可用于调试） */
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

	/* --- 电机A (IN1→PA4, IN2→PA5) --- */
	GPIO_InitPin(GPIO_PA, 0, GPIO_AF_PP);    /* MotorA PWM  */
	GPIO_InitPin(GPIO_PA, 4, GPIO_OUT_PP);   /* MotorA IN1  */
	GPIO_InitPin(GPIO_PA, 5, GPIO_OUT_PP);   /* MotorA IN2  */

	/* --- 电机B --- */
	GPIO_InitPin(GPIO_PA, 1, GPIO_AF_PP);    /* MotorB PWM  */
	GPIO_InitPin(GPIO_PB, 0, GPIO_OUT_PP);   /* MotorB IN1  */
	GPIO_InitPin(GPIO_PB, 1, GPIO_OUT_PP);   /* MotorB IN2  */

	/* --- 电机编码器A (TIM3 Default → PA6/PA7) --- */
	GPIO_InitPin(GPIO_PA, 6, GPIO_IN_FLOAT); /* EncA E1A    */
	GPIO_InitPin(GPIO_PA, 7, GPIO_IN_FLOAT); /* EncA E1B    */

	/* --- 灰度传感器 --- */
	GPIO_InitPin(GPIO_PB, 12, GPIO_IN_FLOAT);
	GPIO_InitPin(GPIO_PB, 13, GPIO_IN_FLOAT);
	GPIO_InitPin(GPIO_PB, 14, GPIO_IN_FLOAT);
	GPIO_InitPin(GPIO_PB, 15, GPIO_IN_FLOAT);
	GPIO_InitPin(GPIO_PA,  8, GPIO_IN_FLOAT);
	GPIO_InitPin(GPIO_PC, 14, GPIO_IN_FLOAT);
	GPIO_InitPin(GPIO_PC, 15, GPIO_IN_FLOAT);

	/* --- 红外避障（PB3/PB4/PB5，JTAG已禁用） --- */
	GPIO_InitPin(GPIO_PB, 3, GPIO_IN_PU);
	GPIO_InitPin(GPIO_PB, 4, GPIO_IN_PU);
	GPIO_InitPin(GPIO_PB, 5, GPIO_IN_PU);

	/* --- 电机编码器B (TIM4_CH1/CH2, 非电机驱动) --- */
	GPIO_InitPin(GPIO_PB, 6, GPIO_IN_FLOAT); /* EncB E2A    */
	GPIO_InitPin(GPIO_PB, 7, GPIO_IN_FLOAT); /* EncB E2B    */

	/* --- 超声波模块 (Trig→PB10, Echo→PA15) --- */
	GPIO_InitPin(GPIO_PB, 10, GPIO_OUT_PP);  /* Trig */
	GPIO_InitPin(GPIO_PA, 15, GPIO_IN_FLOAT); /* Echo */

	/* --- 舵机 --- */
	GPIO_InitPin(GPIO_PA, 2, GPIO_OUT_PP);

	/* --- OLED I2C (SCL→PB8, SDA→PB9, 与OLED.c一致) --- */
	GPIO_InitPin(GPIO_PB, 8, GPIO_OUT_OD);
	GPIO_InitPin(GPIO_PB, 9, GPIO_OUT_OD);
	/* PB11 被按键(SPEED_DOWN)复用，此处由 Key_Init() 配置 */

	/* --- 蓝牙 UART --- */
	GPIO_InitPin(GPIO_PA,  9, GPIO_AF_PP);    /* USART1 TX */
	GPIO_InitPin(GPIO_PA, 10, GPIO_IN_FLOAT); /* USART1 RX */

	/* --- MPU6050 I2C (软件模拟) --- */
	GPIO_InitPin(GPIO_PA, 11, GPIO_OUT_OD);   /* I2C SCL */
	GPIO_InitPin(GPIO_PA, 12, GPIO_OUT_OD);   /* I2C SDA */

	/* --- 按键 (A3=SPEED_UP, B11=SPEED_DOWN, 由 Key_Init() 配置) --- */

	/* --- 板载LED --- */
	GPIO_InitPin(GPIO_PC, 13, GPIO_OUT_PP);
}
