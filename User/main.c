#include "fml_types.h"
#include "GPIO.h"
#include "Motor.h"
#include "UART.h"
#include "System.h"
#include "Delay.h"
#include "Encoder.h"
#if 0  /* ======== 硬件测试模式 ======== */
#include "../Hardware/Grayscale.h"
#include "../Hardware/VL53L0X.h"
#include "../Hardware/Menu.h"
#endif
#if 1  /* ======== 正常模式（电机/PID/菜单）======== */
#include "../App/CarControl.h"
#include "../App/Command.h"
#include "../App/RunMode.h"
#include "../Hardware/Menu.h"
#include "../App/YawControl.h"
#include "../App/LineControl.h"
#include "../Hardware/MPU6050.h"
#endif

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

#if 0  /* ======== 硬件测试模式 ======== */
	Grayscale_Init();
	VL53L0X_Init();
	UART_Printf("\r\n=== Hardware Test ===\r\n");
	UART_Printf("Grayscale + VL53L0X\r\n\r\n");
	Menu_Init();
#endif
#if 1  /* ======== 正常模式 ======== */
	Motor_Init();
	Encoder_Init();
	CarControl_Init();
	RunMode_Init();
	// Menu_Init();
#if 0
	MPU6050_Init();
	Delay_ms(100);
	MPU6050_CalibrateGyroZ(300);
	YawControl_Init();
	Grayscale_Init();
	LineControl_Init();
#endif
	UART_Printf(" Car Ready!\r\n");
#endif

#if 1  /* 正常模式 VOFA+ 用 */
	u16 v_tick = 0;
#endif

	while (1)
	{
#if 0  /* ======== 硬件测试模式 ======== */
		u16 gray[GRAY_CHANNELS];
		Grayscale_ReadAll(gray);
		UART_Printf("GRAY:");
		for (u8 i = 0; i < GRAY_CHANNELS; i++) {
			UART_Printf("%d", gray[i]);
			if (i < GRAY_CHANNELS - 1) UART_Printf(",");
		}
		{
			u8 raw[64];
			u8 n = UART_PeekBuf(UART_ID_3, raw, sizeof(raw));
			if (n > 0) {
				UART_Printf("  |  R3[%d]:", n);
				for (u8 i = 0; i < n; i++) {
					if (raw[i] >= 32 && raw[i] <= 126)
						UART_Printf("%c", raw[i]);
					else if (raw[i] == '\r')
						UART_Printf("\\r");
					else if (raw[i] == '\n')
						UART_Printf("\\n");
					else
						UART_Printf(".");
				}
			}
		}
		VL53L0X_Update();
		s16 dist = VL53L0X_GetDistance();
		UART_Printf("  |  D:%dmm\r\n", dist);
		Delay_ms(100);
		Menu_Update();
#endif
#if 1  /* ======== 正常模式 ======== */
		char line[UART_LINE_SIZE];
		if (UART_GetLine(UART_ID_1, line, sizeof(line))) {
			Command_Handle(line);
		}
		if (UART_GetLine(UART_ID_3, line, sizeof(line))) {
			Command_Handle(line);
		}

#if 0
		if (YawControl_IsEnabled()) {
			float yaw_rate = MPU6050_ReadGyroZ();
			YawControl_Update(yaw_rate, 0.01f);
			Car_ApplyYawDiff(YawControl_GetDiff());
			yaw_was_enabled = 1;
		} else if (yaw_was_enabled) {
			s8 base = Car_GetSpeed(0);
			Car_SetL(base);
			Car_SetR(base);
			yaw_was_enabled = 0;
		}
		Menu_Update();
#endif
		RunMode_Update();
#if 0
		{
			u16 gray[8];
			Grayscale_ReadAll(gray);
			LineControl_Update(gray);
		}
#endif

		if (++v_tick >= 5) {
			v_tick = 0;
			UART_Printf("%.0f,%.0f,%.1f,%.1f,%.0f,%.0f\r\n",
			            Car_GetTarget(0), Car_GetTarget(1),
			            Car_GetFiltSpeed(0), Car_GetFiltSpeed(1),
			            Motor_GetLastPWM(MOTOR_LEFT),
			            Motor_GetLastPWM(MOTOR_RIGHT));
		}

		Delay_ms(10);
#endif
	}
}
