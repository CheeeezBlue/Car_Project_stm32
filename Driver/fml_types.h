#ifndef __FML_TYPES_H__
#define __FML_TYPES_H__

#include "stm32f10x.h"

/* 通用整数类型别名 */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;

/* 通用返回值枚举 */
typedef enum { STATUS_OK = 0, STATUS_ERROR, STATUS_TIMEOUT, STATUS_BUSY } Status_t;

/* GPIO 端口枚举 */
typedef enum { GPIO_PA = 0, GPIO_PB, GPIO_PC } GPIO_Port_t;

/* GPIO 模式枚举 */
typedef enum {
	GPIO_OUT_PP = 0,  /* 推挽输出 */
	GPIO_OUT_OD,      /* 开漏输出 */
	GPIO_IN_PU,       /* 上拉输入 */
	GPIO_IN_PD,       /* 下拉输入 */
	GPIO_IN_FLOAT,    /* 浮空输入 */
	GPIO_AF_PP,       /* 复用推挽输出 */
	GPIO_AF_OD,       /* 复用开漏输出 */
	GPIO_AIN          /* 模拟输入 */
} GPIO_Mode_t;

/* 定时器标识枚举 */
typedef enum { TIM_ID_2 = 0, TIM_ID_3, TIM_ID_4, TIM_ID_1 } TIM_ID_t;

/* PWM 通道枚举 */
typedef enum { PWM_CH1 = 1, PWM_CH2, PWM_CH3, PWM_CH4 } PWM_CH_t;

/* I2C 总线结构体（支持多路软件 I2C） */
typedef struct {
	GPIO_TypeDef* scl_port; u16 scl_pin;
	GPIO_TypeDef* sda_port; u16 sda_pin;
	u8 speed;
} I2C_Bus_t;

/* UART 标识枚举 */
typedef enum { UART_ID_1 = 0, UART_ID_2, UART_ID_3 } UART_ID_t;

/* ADC 通道枚举 */
typedef enum {
	ADC_CH_0=0, ADC_CH_1, ADC_CH_2, ADC_CH_3, ADC_CH_4,
	ADC_CH_5, ADC_CH_6, ADC_CH_7, ADC_CH_8, ADC_CH_9,
	ADC_CH_10, ADC_CH_11, ADC_CH_12, ADC_CH_13, ADC_CH_14,
	ADC_CH_15, ADC_CH_16, ADC_CH_17
} ADC_CH_t;

/* SPI 标识枚举 */
typedef enum { SPI_ID_1 = 0, SPI_ID_2 } SPI_ID_t;

/* 编码器标识枚举 */
typedef enum { ENC_LEFT = 0, ENC_RIGHT } Encoder_ID_t;

#endif
