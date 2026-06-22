#include "fml_types.h"
#include "GPIO.h"
#include "Motor.h"
#include "UART.h"
#include "System.h"
#include "Delay.h"
#include "Encoder.h"
#include "../App/CarControl.h"
#include "../App/Command.h"
#include "../App/YawControl.h"
#include "../Hardware/MPU6050.h"

int main(void)
{
	System_Init();
	GPIO_InitAll();
	Motor_Init();
	Encoder_Init();
	UART_Init(UART_ID_1, 115200);
	UART_EnableIRQ(UART_ID_1);
	UART_Init(UART_ID_2, 115200);
	UART_EnableIRQ(UART_ID_2);
	CarControl_Init();

	/* MPU6050 初始化 + 零偏校准 */
	MPU6050_Init();
	Delay_ms(100);
	if (MPU6050_Check())
		UART_PrintfAll("# MPU6050 OK\r\n");
	else
		UART_PrintfAll("# MPU6050 FAIL\r\n");

	MPU6050_CalibrateGyroZ(300);
	UART_PrintfAll("# Gyro Bias: %.3f deg/s\r\n", MPU6050_GetGyroZBias());

	YawControl_Init();

	UART_PrintfAll("# Car Ready.\r\n");

	u16 v_tick = 0;
	u8  yaw_was_enabled = 0;

	while (1)
	{
		/* 命令：USART1(PC) 和 USART2(HC05蓝牙) 双路输入 */
		char line[UART_LINE_SIZE];
		if (UART_GetLine(UART_ID_1, line, sizeof(line))) {
			Command_Handle(line);
		}
		if (UART_GetLine(UART_ID_2, line, sizeof(line))) {
			Command_Handle(line);
		}

		/* Yaw 控制：读取陀螺仪 → PID → 差分应用到左右轮 */
		if (YawControl_IsEnabled()) {
			float yaw_rate = MPU6050_ReadGyroZ();
			YawControl_Update(yaw_rate, 0.01f);
			Car_ApplyYawDiff(YawControl_GetDiff());
			yaw_was_enabled = 1;
		} else if (yaw_was_enabled) {
			/* Yaw 刚关闭：恢复到纯 SPD 模式 */
			s8 base = Car_GetSpeed(0);
			Car_SetL(base);
			Car_SetR(base);
			yaw_was_enabled = 0;
		}

		/* 速度环：编码器 → PID → 电机 */
		CarControl_Update();

		/* VOFA+ 每 50ms — 8通道 */
		if (++v_tick >= 5) {
			v_tick = 0;
			UART_PrintfAll("%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f\r\n",
			            Car_GetDisplayTarget(0), Car_GetDisplayTarget(1),
			            Car_GetFiltSpeed(0),    Car_GetFiltSpeed(1),
			            YawControl_GetTarget(),  YawControl_GetLastRate(),
			            YawControl_GetDiff(),    YawControl_GetOutput());
		}

		Delay_ms(10);
	}
}
