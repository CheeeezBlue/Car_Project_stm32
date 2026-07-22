#include "RunMode.h"
#include "CarControl.h"
#include "LineControl.h"
#include "YawControl.h"

static RunMode_t current_mode = MODE_STATIONARY;
static u8 running;

static const char* const mode_names[MODE_COUNT] = {
	"INVALID",
	"STATIONARY",
	"STRAIGHT",
	"LINE",
};

void RunMode_Init(void)
{
	current_mode = MODE_STATIONARY;
	running = 0;
}

void RunMode_Update(void)
{
	CarControl_Update();
}

u8 RunMode_Set(RunMode_t mode)
{
	if (mode < MODE_STATIONARY || mode >= MODE_COUNT) return 0;

	RunMode_Stop();
	current_mode = mode;
	return 1;
}

RunMode_t RunMode_Get(void)
{
	return current_mode;
}

const char* RunMode_GetName(RunMode_t mode)
{
	if (mode < MODE_STATIONARY || mode >= MODE_COUNT) return "???";
	return mode_names[mode];
}

void RunMode_Run(void)
{
	if (current_mode == MODE_STATIONARY) return;

	Car_SetYawDiff(0.0f);
	if (current_mode == MODE_LINE)
		LineControl_Enable();
	else if (current_mode == MODE_STRAIGHT)
		YawControl_Enable();
	running = 1;
}

void RunMode_Stop(void)
{
	running = 0;
	Car_Stop();
	Car_SetYawDiff(0.0f);
	LineControl_Disable();
	YawControl_Disable();
}

u8 RunMode_IsRunning(void)
{
	return running;
}
