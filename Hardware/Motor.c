#include "Motor.h"
#include "../Driver/PWM.h"
#include "../Driver/GPIO.h"

#define MOTOR_TIM      TIM_ID_1
#define MOTOR_FREQ_HZ  1000

/* 硬件引脚映射（与GPIO_InitAll一致）：
   MotorA: PWM=PA8(TIM1_CH1), IN1=PB12, IN2=PB13
   MotorB: PWM=PA11(TIM1_CH4), IN1=PB14, IN2=PB15 */
#define MA_PWM_CH  PWM_CH1
#define MA_IN1_P   GPIO_PB
#define MA_IN1_B   12
#define MA_IN2_P   GPIO_PB
#define MA_IN2_B   13

#define MB_PWM_CH  PWM_CH4
#define MB_IN1_P   GPIO_PB
#define MB_IN1_B   14
#define MB_IN2_P   GPIO_PB
#define MB_IN2_B   15

void Motor_Init(void)
{
	PWM_Init(MOTOR_TIM, MOTOR_FREQ_HZ);
	GPIO_WritePin(MA_IN1_P, MA_IN1_B, 0);
	GPIO_WritePin(MA_IN2_P, MA_IN2_B, 0);
	GPIO_WritePin(MB_IN1_P, MB_IN1_B, 0);
	GPIO_WritePin(MB_IN2_P, MB_IN2_B, 0);
}

static void motorA_set(s8 duty)
{
	if (duty > 100) duty = 100;
	if (duty < -100) duty = -100;

	if (duty > 0) {
		GPIO_WritePin(MA_IN1_P, MA_IN1_B, 0);
		GPIO_WritePin(MA_IN2_P, MA_IN2_B, 1);
		PWM_SetDutyPercent(MOTOR_TIM, MA_PWM_CH, (u8)duty);
	} else if (duty < 0) {
		GPIO_WritePin(MA_IN1_P, MA_IN1_B, 1);
		GPIO_WritePin(MA_IN2_P, MA_IN2_B, 0);
		PWM_SetDutyPercent(MOTOR_TIM, MA_PWM_CH, (u8)(-duty));
	} else {
		GPIO_WritePin(MA_IN1_P, MA_IN1_B, 0);
		GPIO_WritePin(MA_IN2_P, MA_IN2_B, 0);
		PWM_SetDutyPercent(MOTOR_TIM, MA_PWM_CH, 0);
	}
}

static void motorB_set(s8 duty)
{
	if (duty > 100) duty = 100;
	if (duty < -100) duty = -100;

	if (duty > 0) {
		GPIO_WritePin(MB_IN1_P, MB_IN1_B, 0);
		GPIO_WritePin(MB_IN2_P, MB_IN2_B, 1);
		PWM_SetDutyPercent(MOTOR_TIM, MB_PWM_CH, (u8)duty);
	} else if (duty < 0) {
		GPIO_WritePin(MB_IN1_P, MB_IN1_B, 1);
		GPIO_WritePin(MB_IN2_P, MB_IN2_B, 0);
		PWM_SetDutyPercent(MOTOR_TIM, MB_PWM_CH, (u8)(-duty));
	} else {
		GPIO_WritePin(MB_IN1_P, MB_IN1_B, 0);
		GPIO_WritePin(MB_IN2_P, MB_IN2_B, 0);
		PWM_SetDutyPercent(MOTOR_TIM, MB_PWM_CH, 0);
	}
}

static s8 last_pwm[2];  /* 记录最近一次 PWM 输出，供查询 */

void Motor_SetSpeed(Motor_ID_t id, s8 duty)
{
	last_pwm[id] = duty;
	if (id == MOTOR_LEFT)
		motorA_set(duty);
	else
		motorB_set(duty);
}

float Motor_GetLastPWM(Motor_ID_t id)
{
	return (float)last_pwm[id];
}
