#include "RunMode.h"
#include "CarControl.h"
#include "LineControl.h"
#include "YawControl.h"

static RunMode_t current_mode = MODE_LINE;

static const char* mode_names[] = {
	"IDLE",
	"MANUAL",
	"STRAIGHT",
	"LINE",
};

/* ================================================================
   初始化
   ================================================================ */
void RunMode_Init(void)
{
	current_mode = MODE_LINE;
	LineControl_Enable();
}

/* ================================================================
   每10ms调用：速度闭环
   ================================================================ */
void RunMode_Update(void)
{
	CarControl_Update();
}

/* ================================================================
   模式切换：自动关闭旧模式环，开启新模式环
   ================================================================ */
void RunMode_Set(RunMode_t mode)
{
	if (mode >= MODE_COUNT) return;
	if (mode == current_mode) return;

	/* 关闭旧模式环 */
	switch (current_mode) {
	case MODE_STRAIGHT:
		YawControl_Disable();
		Car_SetYawDiff(0.0f);
		break;
	case MODE_LINE:
		LineControl_Disable();
		break;
	default:
		Car_SetYawDiff(0.0f);
		break;
	}

	Car_Stop();

	/* 开启新模式环 */
	switch (mode) {
	case MODE_STRAIGHT:
		YawControl_Enable();
		break;
	case MODE_LINE:
		LineControl_Enable();
		break;
	default:
		break;
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
	case MODE_STRAIGHT:
		YawControl_Enable();
		break;
	default:
		break;
	}
}

void RunMode_Stop(void)
{
	Car_Stop();
	LineControl_Disable();
	YawControl_Disable();
}
