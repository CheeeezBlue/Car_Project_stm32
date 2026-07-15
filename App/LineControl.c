#include "LineControl.h"
#include "CarControl.h"
#include "../Driver/PID.h"
#include "../Hardware/Grayscale.h"

static PID_t pid;
static float  error;
static float  output;
static u8     enabled;
static s8     base_speed;

void LineControl_Init(void)
{
	PID_Init(&pid, 3.0f, 0.0f, 0.0f, -40.0f, 40.0f);
	error      = 0.0f;
	output     = 0.0f;
	enabled    = 0;
	base_speed = 30;
}

void LineControl_Update(const u16* gray)
{
	if (!enabled) return;

	error  = Grayscale_Calculate_Error(gray);
	output = PID_Compute(&pid, 0.0f, error, 0.01f);

	float left  = (float)base_speed + output;
	float right = (float)base_speed - output;

	/* 限幅 */
	if (left  >  100.0f) left  =  100.0f;
	if (left  < -100.0f) left  = -100.0f;
	if (right >  100.0f) right =  100.0f;
	if (right < -100.0f) right = -100.0f;

	Car_SetL((s8)left);
	Car_SetR((s8)right);
}

void LineControl_SetSpeed(s8 base)  { base_speed = base; }
void LineControl_SetKp(float kp)    { pid.Kp = kp; }
void LineControl_SetKi(float ki)    { pid.Ki = ki; }
void LineControl_SetKd(float kd)    { pid.Kd = kd; }

void LineControl_Enable(void)       { PID_Reset(&pid); enabled = 1; }
void LineControl_Disable(void)      { enabled = 0; Car_Stop(); }

u8   LineControl_IsEnabled(void)    { return enabled; }
float LineControl_GetKp(void)       { return pid.Kp; }
float LineControl_GetKi(void)       { return pid.Ki; }
float LineControl_GetKd(void)       { return pid.Kd; }
float LineControl_GetError(void)    { return error; }
float LineControl_GetOutput(void)   { return output; }
s8   LineControl_GetBaseSpeed(void) { return base_speed; }
