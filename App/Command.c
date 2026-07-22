#include "Command.h"
#include "CarControl.h"
#include "YawControl.h"
#include "LineControl.h"
#include "RunMode.h"
#include "../Driver/UART.h"
#include "../Hardware/Motor.h"
#include "../Hardware/Menu.h"

static u8 speed_blocked(RunMode_t mode)
{
	return mode == MODE_STATIONARY;
}

void Command_Handle(const char* line)
{
	UART_Cmd_t cmd;
	RunMode_t mode;
	UART_ParseCmd(line, &cmd);
	mode = RunMode_Get();

	switch (cmd.type) {
	case CMD_MODE:
		if (RunMode_Set((RunMode_t)cmd.value)) UART_Printf("Mode: %s\r\n", RunMode_GetName(RunMode_Get()));
		else UART_Printf("ERR: Mode must be 1..3\r\n");
		break;
	case CMD_STOP:
		RunMode_Stop(); UART_Printf("STOP\r\n"); break;
	case CMD_QUERY:
		UART_Printf("%s %s HDG:%.2f RT:%.1f/%.1f DF:%.1f T:%.0f,%.0f ENC:%d,%d PWM:%.0f,%.0f\r\n",
			RunMode_GetName(mode), RunMode_IsRunning() ? "RUN" : "STOP",
			YawControl_GetHeading(), YawControl_GetRateTarget(), YawControl_GetLastRate(), YawControl_GetDiff(),
			Car_GetTarget(0), Car_GetTarget(1), Car_GetEnc(0), Car_GetEnc(1),
			Motor_GetLastPWM(MOTOR_LEFT), Motor_GetLastPWM(MOTOR_RIGHT));
		break;
	case CMD_SPD:
		if (speed_blocked(mode)) { UART_Printf("ERR: Speed blocked in STATIONARY\r\n"); break; }
		Car_SetL(cmd.value); Car_SetR(cmd.value); UART_Printf("Speed: %d\r\n", cmd.value); break;
	case CMD_LR:
		if (speed_blocked(mode)) { UART_Printf("ERR: Speed blocked in STATIONARY\r\n"); break; }
		if (cmd.left_set) Car_SetL(cmd.left);
		if (cmd.right_set) Car_SetR(cmd.right);
		UART_Printf("L:%d R:%d\r\n", cmd.left, cmd.right); break;
	case CMD_KP: Car_SetKp(cmd.value / 1000.0f); break;
	case CMD_KI: Car_SetKi(cmd.value / 1000.0f); break;
	case CMD_KD: Car_SetKd(cmd.value / 1000.0f); break;
	case CMD_TGT:
		if (!speed_blocked(mode)) Car_SetTarget(cmd.value);
		break;
	case CMD_OL:
		if (!speed_blocked(mode)) Car_OpenLoop(cmd.value);
		break;
	case CMD_FFL: Car_SetFF(cmd.value / 1000.0f, Car_GetFF_R()); break;
	case CMD_FFR: Car_SetFF(Car_GetFF_L(), cmd.value / 1000.0f); break;
	case CMD_FFO: Car_SetFFOffset(cmd.value / 1000.0f); break;
	case CMD_FDB: Car_SetDeadband(cmd.value / 1000.0f); break;
	case CMD_RMP: Car_SetRampStep(cmd.value / 1000.0f); break;
	case CMD_YHP:
		if (mode == MODE_STRAIGHT) YawControl_SetHeadingKp(cmd.value / 1000.0f);
		break;
	case CMD_YHI:
		if (mode == MODE_STRAIGHT) YawControl_SetHeadingKi(cmd.value / 1000.0f);
		break;
	case CMD_YHD:
		if (mode == MODE_STRAIGHT) YawControl_SetHeadingKd(cmd.value / 1000.0f);
		break;
	case CMD_YRL:
		if (mode == MODE_STRAIGHT) YawControl_SetRateLimit(cmd.value);
		break;
	case CMD_YWP:
		if (mode == MODE_STRAIGHT) YawControl_SetRateKp(cmd.value / 1000.0f);
		break;
	case CMD_YWI:
		if (mode == MODE_STRAIGHT) YawControl_SetRateKi(cmd.value / 1000.0f);
		break;
	case CMD_YWD:
		if (mode == MODE_STRAIGHT) YawControl_SetRateKd(cmd.value / 1000.0f);
		break;
	case CMD_YWM:
		if (mode == MODE_STRAIGHT) YawControl_SetLimit(cmd.value / 1000.0f);
		break;
	case CMD_LSPD:
		if (mode == MODE_LINE) LineControl_SetSpeed((s8)cmd.value);
		break;
	case CMD_LKP:
		if (mode == MODE_LINE) LineControl_SetKp(cmd.value / 1000.0f);
		break;
	case CMD_LKI:
		if (mode == MODE_LINE) LineControl_SetKi(cmd.value / 1000.0f);
		break;
	case CMD_LKD:
		if (mode == MODE_LINE) LineControl_SetKd(cmd.value / 1000.0f);
		break;
	case CMD_KEY1: Menu_InjectKey(1); break;
	case CMD_KEY2: Menu_InjectKey(2); break;
	case CMD_KEY3: Menu_InjectKey(3); break;
	case CMD_KEY4: Menu_InjectKey(4); break;
	default: break;
	}
}

void Command_HandleFW(u8 type, const u8* payload, u8 len)
{
	char line[64];
	u8 n;
	if (type != 0x01) return;
	n = (len < sizeof(line) - 1) ? len : (u8)(sizeof(line) - 1);
	for (u8 i = 0; i < n; i++) line[i] = (char)payload[i];
	line[n] = '\0';
	Command_Handle(line);
}
