#include "Command.h"
#include "CarControl.h"
#include "YawControl.h"
#include "LineControl.h"
#include "RunMode.h"
#include "../Driver/UART.h"
#include "../Hardware/Motor.h"
#include "../Hardware/Menu.h"

void Command_Handle(const char* line)
{
	UART_Cmd_t cmd;
	UART_ParseCmd(line, &cmd);

	RunMode_t mode = RunMode_Get();

	switch (cmd.type) {

	case CMD_MODE:
		RunMode_Set((RunMode_t)cmd.value);
		UART_Printf("Mode: %s\r\n", RunMode_GetName(RunMode_Get()));
		break;

	/* ---- 速度指令 (禁止在 IDLE 模式) ---- */
	case CMD_SPD:
		if (mode == MODE_IDLE) { UART_Printf("ERR: Speed blocked in IDLE\r\n"); break; }
		Car_SetL(cmd.value);
		Car_SetR(cmd.value);
		UART_Printf("Speed: %d\r\n", cmd.value);
		break;

	case CMD_LR:
		if (mode == MODE_IDLE) { UART_Printf("ERR: Speed blocked in IDLE\r\n"); break; }
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
		YawControl_Disable();
		UART_Printf("STOP\r\n");
		break;

	case CMD_QUERY:
		UART_Printf("TGT:%.0f,%.0f ENC:%d,%d PWM:%.0f,%.0f %s\r\n",
		            Car_GetTarget(0), Car_GetTarget(1),
		            Car_GetEnc(0), Car_GetEnc(1),
		            Motor_GetLastPWM(MOTOR_LEFT),
		            Motor_GetLastPWM(MOTOR_RIGHT),
		            RunMode_GetName(mode));
		break;

	case CMD_KP:
		if (mode == MODE_IDLE) { UART_Printf("ERR: Speed blocked in IDLE\r\n"); break; }
		{ float v = cmd.value / 1000.0f; Car_SetKp(v); UART_Printf("Vel Kp=%.3f\r\n", v); }
		break;

	case CMD_KI:
		if (mode == MODE_IDLE) { UART_Printf("ERR: Speed blocked in IDLE\r\n"); break; }
		{ float v = cmd.value / 1000.0f; Car_SetKi(v); UART_Printf("Vel Ki=%.3f\r\n", v); }
		break;

	case CMD_KD:
		if (mode == MODE_IDLE) { UART_Printf("ERR: Speed blocked in IDLE\r\n"); break; }
		{ float v = cmd.value / 1000.0f; Car_SetKd(v); UART_Printf("Vel Kd=%.3f\r\n", v); }
		break;

	case CMD_TGT:
		if (mode == MODE_IDLE) { UART_Printf("ERR: Speed blocked in IDLE\r\n"); break; }
		Car_SetTarget(cmd.value);
		UART_Printf("Target: %d pulse/10ms\r\n", cmd.value);
		break;

	case CMD_OL:
		if (mode == MODE_IDLE) { UART_Printf("ERR: Speed blocked in IDLE\r\n"); break; }
		Car_OpenLoop(cmd.value);
		UART_Printf("OpenLoop PWM: %d\r\n", cmd.value);
		break;

	case CMD_FFL:
		if (mode == MODE_IDLE) { UART_Printf("ERR: Speed blocked in IDLE\r\n"); break; }
		{ float v = cmd.value / 1000.0f; Car_SetFF(v, Car_GetFF_R()); UART_Printf("FF_L=%.3f\r\n", v); }
		break;

	case CMD_FFR:
		if (mode == MODE_IDLE) { UART_Printf("ERR: Speed blocked in IDLE\r\n"); break; }
		{ float v = cmd.value / 1000.0f; Car_SetFF(Car_GetFF_L(), v); UART_Printf("FF_R=%.3f\r\n", v); }
		break;

	case CMD_FFO:
		if (mode == MODE_IDLE) { UART_Printf("ERR: Speed blocked in IDLE\r\n"); break; }
		{ float v = cmd.value / 1000.0f; Car_SetFFOffset(v); UART_Printf("FF_Bias=%.1f\r\n", v); }
		break;

	case CMD_FDB:
		if (mode == MODE_IDLE) { UART_Printf("ERR: Speed blocked in IDLE\r\n"); break; }
		{ float v = (float)cmd.value / 1000.0f; Car_SetDeadband(v); UART_Printf("FF_Deadband=%.2f\r\n", v); }
		break;

	case CMD_RMP:
		if (mode == MODE_IDLE) { UART_Printf("ERR: Speed blocked in IDLE\r\n"); break; }
		{ float v = (float)cmd.value / 1000.0f; Car_SetRampStep(v); UART_Printf("Ramp_Step=%.1f\r\n", v); }
		break;

	/* ---- Yaw 指令 (仅 MODE_STRAIGHT) ---- */
	case CMD_YAW:
		if (mode != MODE_STRAIGHT) { UART_Printf("ERR: Yaw needs STRAIGHT mode\r\n"); break; }
		YawControl_SetTarget((float)cmd.value);
		YawControl_Enable();
		UART_Printf("Yaw TGT: %d deg/s\r\n", cmd.value);
		break;

	case CMD_YWP:
		if (mode != MODE_STRAIGHT) { UART_Printf("ERR: Yaw needs STRAIGHT mode\r\n"); break; }
		{ float v = cmd.value / 1000.0f; YawControl_SetKp(v); UART_Printf("Yaw Kp=%.3f\r\n", v); }
		break;

	case CMD_YWI:
		if (mode != MODE_STRAIGHT) { UART_Printf("ERR: Yaw needs STRAIGHT mode\r\n"); break; }
		{ float v = cmd.value / 1000.0f; YawControl_SetKi(v); UART_Printf("Yaw Ki=%.3f\r\n", v); }
		break;

	case CMD_YWD:
		if (mode != MODE_STRAIGHT) { UART_Printf("ERR: Yaw needs STRAIGHT mode\r\n"); break; }
		{ float v = cmd.value / 1000.0f; YawControl_SetKd(v); UART_Printf("Yaw Kd=%.3f\r\n", v); }
		break;

	case CMD_YWM:
		if (mode != MODE_STRAIGHT) { UART_Printf("ERR: Yaw needs STRAIGHT mode\r\n"); break; }
		{ float v = cmd.value / 1000.0f; YawControl_SetLimit(v); UART_Printf("Yaw MaxDiff=%.1f\r\n", v); }
		break;

	/* ---- 巡线指令 (仅 MODE_LINE) ---- */
	case CMD_LSPD:
		if (mode != MODE_LINE) { UART_Printf("ERR: Line needs LINE mode\r\n"); break; }
		LineControl_SetSpeed((s8)cmd.value);
		UART_Printf("Line Speed: %d\r\n", cmd.value);
		break;

	case CMD_LKP:
		if (mode != MODE_LINE) { UART_Printf("ERR: Line needs LINE mode\r\n"); break; }
		{ float v = cmd.value / 1000.0f; LineControl_SetKp(v); UART_Printf("Line Kp=%.3f\r\n", v); }
		break;

	case CMD_LKI:
		if (mode != MODE_LINE) { UART_Printf("ERR: Line needs LINE mode\r\n"); break; }
		{ float v = cmd.value / 1000.0f; LineControl_SetKi(v); UART_Printf("Line Ki=%.3f\r\n", v); }
		break;

	case CMD_LKD:
		if (mode != MODE_LINE) { UART_Printf("ERR: Line needs LINE mode\r\n"); break; }
		{ float v = cmd.value / 1000.0f; LineControl_SetKd(v); UART_Printf("Line Kd=%.3f\r\n", v); }
		break;

	case CMD_KEY1: Menu_InjectKey(1); break;
	case CMD_KEY2: Menu_InjectKey(2); break;
	case CMD_KEY3: Menu_InjectKey(3); break;
	case CMD_KEY4: Menu_InjectKey(4); break;

	default:
		break;
	}
}

/* ================================================================
   FireWater 命令帧入口：payload 转文本 → 复用 Command_Handle
   ================================================================ */
void Command_HandleFW(u8 type, const u8* payload, u8 len)
{
	if (type != 0x01) return;

	char line[64];
	u8 n = (len < sizeof(line) - 1) ? len : (u8)(sizeof(line) - 1);
	for (u8 i = 0; i < n; i++) line[i] = (char)payload[i];
	line[n] = '\0';

	Command_Handle(line);
}
