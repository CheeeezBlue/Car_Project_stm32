#include "YawControl.h"
#include "../Driver/PID.h"

#define YAW_FILT_ALPHA  0.15f

static PID_t heading_pid;
static PID_t rate_pid;
static float heading_deg;
static float heading_target;
static float rate_target;
static float yaw_diff;
static float filt_rate;
static float rate_limit;
static u8 yaw_enabled;
static u8 rate_ready;

void YawControl_Init(void)
{
	PID_Init(&heading_pid, 1.0f, 0.0f, 0.0f, -45.0f, 45.0f);
	PID_Init(&rate_pid, 0.7f, 0.005f, 0.01f, -40.0f, 40.0f);
	heading_deg = 0.0f;
	heading_target = 0.0f;
	rate_target = 0.0f;
	yaw_diff = 0.0f;
	filt_rate = 0.0f;
	rate_limit = 45.0f;
	yaw_enabled = 0;
	rate_ready = 0;
}

void YawControl_Update(float yaw_rate, float dt)
{
	if (!yaw_enabled) {
		yaw_diff = 0.0f;
		return;
	}

	if (!rate_ready) {
		filt_rate = yaw_rate;
		rate_ready = 1;
	} else {
		filt_rate += YAW_FILT_ALPHA * (yaw_rate - filt_rate);
	}
	heading_deg += filt_rate * dt;
	rate_target = PID_Compute(&heading_pid, heading_target, heading_deg, dt);
	if (rate_target > rate_limit) rate_target = rate_limit;
	if (rate_target < -rate_limit) rate_target = -rate_limit;
	yaw_diff = PID_Compute(&rate_pid, rate_target, filt_rate, dt);
}

void YawControl_Enable(void)
{
	PID_Reset(&heading_pid);
	PID_Reset(&rate_pid);
	heading_deg = 0.0f;
	heading_target = 0.0f;
	rate_target = 0.0f;
	yaw_diff = 0.0f;
	filt_rate = 0.0f;
	rate_ready = 0;
	yaw_enabled = 1;
}

void YawControl_Disable(void)
{
	yaw_enabled = 0;
	heading_deg = 0.0f;
	heading_target = 0.0f;
	rate_target = 0.0f;
	yaw_diff = 0.0f;
	filt_rate = 0.0f;
	rate_ready = 0;
	PID_Reset(&heading_pid);
	PID_Reset(&rate_pid);
}

void YawControl_SetHeadingKp(float kp) { heading_pid.Kp = kp; }
void YawControl_SetHeadingKi(float ki) { heading_pid.Ki = ki; }
void YawControl_SetHeadingKd(float kd) { heading_pid.Kd = kd; }
void YawControl_SetRateKp(float kp) { rate_pid.Kp = kp; }
void YawControl_SetRateKi(float ki) { rate_pid.Ki = ki; }
void YawControl_SetRateKd(float kd) { rate_pid.Kd = kd; }

void YawControl_SetRateLimit(float max_deg_per_s)
{
	if (max_deg_per_s < 0.0f) max_deg_per_s = -max_deg_per_s;
	rate_limit = max_deg_per_s;
	heading_pid.out_max = max_deg_per_s;
	heading_pid.out_min = -max_deg_per_s;
	heading_pid.integral_max = max_deg_per_s;
}

void YawControl_SetLimit(float max_diff)
{
	if (max_diff < 0.0f) max_diff = -max_diff;
	rate_pid.out_max = max_diff;
	rate_pid.out_min = -max_diff;
	rate_pid.integral_max = max_diff;
}

float YawControl_GetDiff(void) { return yaw_diff; }
float YawControl_GetHeading(void) { return heading_deg; }
float YawControl_GetHeadingTarget(void) { return heading_target; }
float YawControl_GetRateTarget(void) { return rate_target; }
float YawControl_GetLastRate(void) { return filt_rate; }
float YawControl_GetHeadingKp(void) { return heading_pid.Kp; }
float YawControl_GetHeadingKi(void) { return heading_pid.Ki; }
float YawControl_GetHeadingKd(void) { return heading_pid.Kd; }
float YawControl_GetRateKp(void) { return rate_pid.Kp; }
float YawControl_GetRateKi(void) { return rate_pid.Ki; }
float YawControl_GetRateKd(void) { return rate_pid.Kd; }
float YawControl_GetRateLimit(void) { return rate_limit; }
u8 YawControl_IsEnabled(void) { return yaw_enabled; }
