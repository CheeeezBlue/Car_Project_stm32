#include "fml_types.h"
#include "GPIO.h"
#include "Motor.h"
#include "UART.h"
#include "System.h"
#include "Delay.h"
#include "Encoder.h"
#include "../App/CarControl.h"
#include "../App/Command.h"
#include "../App/RunMode.h"
#include "../Hardware/Menu.h"
#include "../App/YawControl.h"
#include "../App/LineControl.h"
#include "../Hardware/MPU6050.h"
#include "../Hardware/JY901S.h"
#include "../Hardware/Key.h"
#include "../Hardware/Grayscale.h"
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
	UART_Init(UART_ID_3, 115200);
	UART_EnableIRQ(UART_ID_3);

	CarControl_Init();
	Grayscale_Init();
	LineControl_Init();
	Key_Init();
	RunMode_Init();
	Menu_Init();

	JY901S_Init();
	Delay_ms(200);
	YawControl_Init();

	UART_Printf(" Car Ready!\r\n");

	u16 v_tick = 0;

	while (1) {
		char line[UART_LINE_SIZE];
		if (UART_GetLine(UART_ID_1, line, sizeof(line))) {
			Command_Handle(line);
		}
		if (UART_GetLine(UART_ID_3, line, sizeof(line))) {
			Command_Handle(line);
		}

		u16 gray[GRAY_CHANNELS];
		Grayscale_ReadAll(gray);
		JY901S_Update();
		float yaw_rate = JY901S_ReadGyroZ();

		switch (RunMode_Get()) {
		case MODE_IDLE:
		case MODE_MANUAL:
			Car_SetYawDiff(0.0f);
			break;
		case MODE_STRAIGHT:
			YawControl_Update(yaw_rate, 0.01f);
			Car_SetYawDiff(YawControl_GetDiff());
			break;
		case MODE_LINE:
			LineControl_Update(gray);
			break;
		}

		RunMode_Update();
		Menu_Update();

		if (++v_tick >= 5) {
			v_tick = 0;
			UART_Printf("%.0f,%.0f,%.1f,%.1f,%.2f,%.1f\r\n",
			            Car_GetTarget(0), Car_GetTarget(1),
			            Car_GetFiltSpeed(0), Car_GetFiltSpeed(1),
			            LineControl_GetError(),
			            LineControl_GetOutput());
		}

		Delay_ms(10);
	}
}
