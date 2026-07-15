#include "RunMode.h"
#include "CarControl.h"
#include "LineControl.h"

static RunMode_t current_mode = MODE_IDLE;

static const char* mode_names[] = {
	"IDLE",
	"MANUAL",
	"STRAIGHT",
	"LINE",
	"YAW LOCK",
	"LINE PARK",
};

/* ================================================================
   初始化
   ================================================================ */
void RunMode_Init(void)
{
	current_mode = MODE_MANUAL;
}

/* ================================================================
   每10ms调用：模式逻辑 + 速度闭环
   ================================================================ */
void RunMode_Update(void)
{
	switch (current_mode) {

	case MODE_IDLE:
		/* 停车，CarControl 已由 STOP 命令复位到 0 */
		CarControl_Update();
		break;

	case MODE_MANUAL:
		/* 蓝牙命令直接控制速度，不做额外干预 */
		CarControl_Update();
		break;

	case MODE_STRAIGHT:
		/* TODO: 编码器积分 → 直走 N cm 后自动停车 */
		CarControl_Update();
		break;

	case MODE_LINE:
		/* TODO: Grayscale_ReadAll → LineControl_Update */
		CarControl_Update();
		break;

	case MODE_YAW_LOCK:
		/* TODO: MPU6050 yaw → PID 锁定直行方向 */
		CarControl_Update();
		break;

	case MODE_LINE_PARK:
		/* TODO: 循迹 + 编码器/VL53L0X 定点停车 */
		CarControl_Update();
		break;
	}
}

/* ================================================================
   模式切换
   ================================================================ */
void RunMode_Set(RunMode_t mode)
{
	if (mode >= MODE_COUNT) return;

	/* 切模式时停车 + 复位 PID */
	if (mode != current_mode) {
		Car_Stop();
	}
	current_mode = mode;
}

RunMode_t RunMode_Get(void)
{
	return current_mode;
}

const char* RunMode_GetName(RunMode_t mode)
{
	if (mode >= MODE_COUNT) return "???";
	return mode_names[mode];
}

/* ================================================================
   一键启停（菜单调用）
   ================================================================ */
void RunMode_Run(void)
{
	switch (current_mode) {
	case MODE_LINE:
		LineControl_Enable();
		break;
	default:
		break;
	}
}

void RunMode_Stop(void)
{
	Car_Stop();
#if 0
	LineControl_Disable();
	YawControl_Disable();
#endif
}
