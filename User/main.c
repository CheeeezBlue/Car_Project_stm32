#include "fml_types.h"
#include "GPIO.h"
#include "Motor.h"
#include "UART.h"
#include "System.h"
#include "Delay.h"
#include "Encoder.h"
#include "../App/CarControl.h"
#include "../App/Command.h"

int main(void)
{
	System_Init();
	GPIO_InitAll();
	Motor_Init();
	Encoder_Init();
	UART_Init(UART_ID_1, 115200);
	UART_EnableIRQ(UART_ID_1);
	CarControl_Init();

	u16 v_tick = 0;

	while (1)
	{
		/* 命令 */
		char line[UART_LINE_SIZE];
		if (UART_GetLine(UART_ID_1, line, sizeof(line))) {
			Command_Handle(line);
		}

		/* 控制：编码器→PID→电机（O:xxx 进入开环测试） */
		CarControl_Update();

		/* VOFA （每 50ms / 5次循环）— 4通道CSV */
		if (++v_tick >= 5) {
			v_tick = 0;
			UART_Printf("%.1f,%.1f,%.1f,%.1f\r\n",
			            Car_GetDisplayTarget(0), Car_GetDisplayTarget(1),
			            Car_GetFiltSpeed(0), Car_GetFiltSpeed(1));
		}

		Delay_ms(10);
	}
}
