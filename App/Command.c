#include "Command.h"
#include "CarControl.h"
#include "../Driver/UART.h"
#include "../Hardware/Motor.h"

void Command_Handle(const char* line)
{
	UART_Cmd_t cmd;
	UART_ParseCmd(line, &cmd); //解析命令

	switch (cmd.type) {

	case CMD_SPD:
		Car_SetL(cmd.value);
		Car_SetR(cmd.value);
		UART_Printf("Speed: %d\r\n", cmd.value);
		break;

	case CMD_LR:
		if (cmd.left_set && cmd.right_set) {
			Car_SetL(cmd.left);
			Car_SetR(cmd.right);
		} else if (cmd.left_set) {
			Car_SetL(cmd.left);
		} else if (cmd.right_set) {
			Car_SetR(cmd.right);
		}
		UART_Printf("L:%d R:%d\r\n", cmd.left, cmd.right);
		break;

	case CMD_STOP:
		Car_Stop();
		UART_Printf("STOP\r\n");
		break;

	case CMD_QUERY:
		UART_Printf("TGT:%.0f,%.0f ENC:%d,%d PWM:%.0f,%.0f %s\r\n",
		            Car_GetTarget(0), Car_GetTarget(1),
		            Car_GetEnc(0), Car_GetEnc(1),
		            Motor_GetLastPWM(MOTOR_LEFT),
		            Motor_GetLastPWM(MOTOR_RIGHT),
		            "PID");
		break;

	case CMD_KP: {
		float v = cmd.value / 1000.0f;
		Car_SetPID(v, Car_GetKi(), Car_GetKd());
		UART_Printf("Kp=%.3f\r\n", v);
		break;
	}
	case CMD_KI: {
		float v = cmd.value / 1000.0f;
		Car_SetPID(Car_GetKp(), v, Car_GetKd());
		UART_Printf("Ki=%.3f\r\n", v);
		break;
	}
	case CMD_KD: {
		float v = cmd.value / 1000.0f;
		Car_SetPID(Car_GetKp(), Car_GetKi(), v);
		UART_Printf("Kd=%.3f\r\n", v);
		break;
	}
	case CMD_TGT:
		Car_SetTarget(cmd.value);
		UART_Printf("Target: %d pulse/10ms\r\n", cmd.value);
		break;

	case CMD_OL:
		Car_OpenLoop(cmd.value);
		UART_Printf("OpenLoop PWM: %d\r\n", cmd.value);
		break;

	case CMD_FFL: {
		float v = cmd.value / 1000.0f;
		Car_SetFF(v, Car_GetFF_R());
		UART_Printf("FF_L=%.3f\r\n", v);
		break;
	}
	case CMD_FFR: {
		float v = cmd.value / 1000.0f;
		Car_SetFF(Car_GetFF_L(), v);
		UART_Printf("FF_R=%.3f\r\n", v);
		break;
	}
	case CMD_FFO: {
		float v = cmd.value / 1000.0f;
		Car_SetFFOffset(v);
		UART_Printf("FF_Offset=%.1f\r\n", v);
		break;
	}

	default:
		break;
	}
}

/* ================================================================
   FireWater 命令帧入口：payload 转文本 → 复用 Command_Handle
   ================================================================ */
void Command_HandleFW(u8 type, const u8* payload, u8 len)
{
	if (type != 0x01) return;  /* 只处理命令帧，0x02 响应帧是 PC 方向的 */

	/* 二进制 payload → C 字符串 */
	char line[64];
	u8 n = (len < sizeof(line) - 1) ? len : (u8)(sizeof(line) - 1);
	for (u8 i = 0; i < n; i++) line[i] = (char)payload[i];
	line[n] = '\0';

	/* 复用现有文本命令解析 + 分发 */
	Command_Handle(line);
}
