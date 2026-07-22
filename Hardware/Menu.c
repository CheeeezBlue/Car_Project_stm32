#include "Menu.h"
#include "OLED.h"
#include "Key.h"
#include "../App/CarControl.h"
#include "../App/RunMode.h"
#include "../App/LineControl.h"
#include "../Hardware/Motor.h"
#include "../Driver/GPIO.h"
#include "../Driver/UART.h"
#include "../System/Delay.h"
#include <stdio.h>

/* ---- 模式槽位 ---- */
const u8 mode_slots[MODE_SLOT_COUNT] = {
	MODE_STATIONARY,
	MODE_STRAIGHT,
	MODE_LINE,
};

/* ---- 菜单状态 ---- */
static struct {
	MenuLevel_t level;
	u8  cursor;        /* 当前层选中项 */
	u8  data_cat;      /* DATA_CAT: 类别下标 */
	u8  data_page;     /* DATA_PAGE: 页号 */
	u8  pid_cat;       /* PID_CAT: 0=速度,1=灰度 */
	u8  pid_param;     /* PID_TUNE: 0=Kp,1=Ki,2=Kd */
	u8  key_held;      /* 当前持续按下的键 0=无 */
	u16 hold_ticks;
	u16 confirm_tick;  /* 启停确认倒计时 */
	u8  dirty;         /* 需要重绘 */
} menu;

/* ---- 前向声明 ---- */
static void Render(void);
static void RenderDataPage(u8 cat, u8 pg);
static u8   KeyRaw(u8 id);
static void Menu_Dispatch(u8 key, u8 act);
static void KeyScan(void);

/* ================================================================
   初始化
   ================================================================ */
void Menu_Init(void)
{
	OLED_Init();
	OLED_ShowString(1, 3, "Car  Ready!");

	menu.level        = MENU_MAIN;
	menu.cursor       = 0;
	menu.data_cat     = 0;
	menu.data_page    = 0;
	menu.pid_cat      = 0;
	menu.pid_param    = 0;
	menu.key_held     = 0;
	menu.hold_ticks   = 0;
	menu.confirm_tick = 0;
	menu.dirty        = 1;

	Delay_ms(1500);

	/* 将 prev 状态同步到当前引脚电平，避免上电瞬间误触发边沿 */
	Key_GetNum();

	OLED_Clear();
	Render();
}

/* ================================================================
   每 10ms 主循环调用
   ================================================================ */
void Menu_Update(void)
{
	KeyScan();

	/* 启停确认倒计时 */
	if (menu.level == MENU_RUN_CONFIRM) {
		if (++menu.confirm_tick > 200) {  /* 2s 超时 */
			menu.level = MENU_MAIN;
			menu.confirm_tick = 0;
			OLED_Clear();
			Render();
			return;
		}
	}

	/* 需要重绘时刷新 */
	if (menu.dirty) {
		menu.dirty = 0;
		Render();
	}
}

/* ================================================================
   按键扫描：仅负责物理按键检测 → 交给 Menu_Dispatch
   ================================================================ */
static void KeyScan(void)
{
	/* 检查当前按键是否仍按下 */
	if (menu.key_held && KeyRaw(menu.key_held)) {
		menu.hold_ticks++;
		if (menu.hold_ticks == 1) {
			Menu_Dispatch(menu.key_held, 1);       /* 首次短按 */
		} else if (menu.hold_ticks > LONG_PRESS_TICKS &&
		           (menu.hold_ticks % REPEAT_INTERVAL == 0)) {
			Menu_Dispatch(menu.key_held, 2);       /* 长按连发 */
		}
	} else {
		/* 按键已松开，检查新按下 */
		menu.key_held   = 0;
		menu.hold_ticks = 0;

		u8 key = Key_GetNum();  /* 边沿检测 */
		if (key) {
			menu.key_held   = key;
			menu.hold_ticks = 1;
			Menu_Dispatch(key, 1);
		}
	}
}

/* ================================================================
   动作分发：物理按键和串口命令共用
   ================================================================ */
static void Menu_Dispatch(u8 key, u8 act)
{
	UART_Printf("[KEY] L%d key=%d act=%d\r\n", menu.level, key, act);

	switch (menu.level) {

	/* ---- MENU_MAIN ---- */
	case MENU_MAIN:
		if (act != 1) break;
		switch (key) {
		case 1:
			menu.level = MENU_MODE_SELECT;
			menu.cursor = 0;
			OLED_Clear(); menu.dirty = 1;
			break;
		case 2:
			menu.level = MENU_DATA_CAT;
			menu.cursor = 0;
			menu.data_cat = 0;
			menu.data_page = 0;
			OLED_Clear(); menu.dirty = 1;
			break;
		case 3:
			menu.level = MENU_PID_CAT;
			menu.cursor = 0;
			menu.pid_cat = 0;
			menu.pid_param = 0;
			OLED_Clear(); menu.dirty = 1;
			break;
		case 4:
			menu.level = MENU_RUN_CONFIRM;
			menu.confirm_tick = 0;
			OLED_Clear(); menu.dirty = 1;
			break;
		}
		break;

	/* ---- MENU_MODE_SELECT ---- */
	case MENU_MODE_SELECT:
		if (act != 1) break;
		switch (key) {
		case 1: case 2: case 3:
			RunMode_Set((RunMode_t)mode_slots[key - 1]);
			menu.level = MENU_MAIN;
			OLED_Clear(); menu.dirty = 1;
			break;
		case 4:
			menu.level = MENU_MAIN;
			OLED_Clear(); menu.dirty = 1;
			break;
		}
		break;

	/* ---- MENU_DATA_CAT ---- */
	case MENU_DATA_CAT:
		if (act != 1) break;
		switch (key) {
		case 1:
			menu.data_cat = (menu.data_cat == 0)
				? DATA_CAT_COUNT - 1 : menu.data_cat - 1;
			menu.data_page = 0;
			menu.dirty = 1;
			break;
		case 2:
			menu.data_cat = (menu.data_cat + 1) % DATA_CAT_COUNT;
			menu.data_page = 0;
			menu.dirty = 1;
			break;
		case 3:
			menu.level = MENU_DATA_PAGE;
			OLED_Clear(); menu.dirty = 1;
			break;
		case 4:
			menu.level = MENU_MAIN;
			OLED_Clear(); menu.dirty = 1;
			break;
		}
		break;

	/* ---- MENU_DATA_PAGE ---- */
	case MENU_DATA_PAGE:
		if (act != 1) break;
		switch (key) {
		case 1:
			if (menu.data_page > 0) {
				menu.data_page--;
				menu.dirty = 1;
			}
			break;
		case 2: {
			u8 max_pg = (menu.data_cat == DATA_SPEED) ? 3 : 0;
			if (menu.data_page < max_pg) {
				menu.data_page++;
				menu.dirty = 1;
			}
			break;
		}
		case 4:
			menu.level = MENU_DATA_CAT;
			OLED_Clear(); menu.dirty = 1;
			break;
		}
		break;

	/* ---- MENU_PID_CAT ---- */
	case MENU_PID_CAT:
		if (act != 1) break;
		switch (key) {
		case 1:
			menu.pid_cat = (menu.pid_cat == 0) ? PIDCAT_COUNT - 1 : menu.pid_cat - 1;
			menu.dirty = 1;
			break;
		case 2:
			menu.pid_cat = (menu.pid_cat + 1) % PIDCAT_COUNT;
			menu.dirty = 1;
			break;
		case 3:
			menu.pid_param = 0;
			menu.level = MENU_PID_TUNE;
			OLED_Clear(); menu.dirty = 1;
			break;
		case 4:
			menu.level = MENU_MAIN;
			OLED_Clear(); menu.dirty = 1;
			break;
		}
		break;

	/* ---- MENU_PID_TUNE ---- */
	case MENU_PID_TUNE: {
		float step = (act == 2) ? PID_STEP_COARSE : PID_STEP_FINE;
		int dir = (key == 1) ? 1 : (key == 2) ? -1 : 0;

		if (key == 3 && act == 1) {
			menu.pid_param = (menu.pid_param + 1) % 3;
			menu.dirty = 1;
		} else if (dir != 0 && menu.pid_cat == PIDCAT_SPEED) {
			switch (menu.pid_param) {
			case 0: Car_SetKp(Car_GetKp() + step * dir); break;
			case 1: Car_SetKi(Car_GetKi() + step * dir); break;
			case 2: Car_SetKd(Car_GetKd() + step * dir); break;
			}
			menu.dirty = 1;
		} else if (dir != 0 && menu.pid_cat == PIDCAT_LINE) {
			switch (menu.pid_param) {
			case 0: LineControl_SetKp(LineControl_GetKp() + step * dir); break;
			case 1: LineControl_SetKi(LineControl_GetKi() + step * dir); break;
			case 2: LineControl_SetKd(LineControl_GetKd() + step * dir); break;
			}
			menu.dirty = 1;
		} else if (key == 4 && act == 1) {
			menu.level = MENU_PID_CAT;
			OLED_Clear(); menu.dirty = 1;
		}
		break;
	}

	/* ---- MENU_RUN_CONFIRM ---- */
	case MENU_RUN_CONFIRM:
		if (act != 1) break;
		if (key == 4) {
			RunMode_Run();
		} else {
			RunMode_Stop();
		}
		menu.level = MENU_MAIN;
		OLED_Clear(); menu.dirty = 1;
		break;
	}
}

/* ================================================================
   原始按键读取（检测是否按下，不消抖）
   ================================================================ */
static u8 KeyRaw(u8 id)
{
	switch (id) {
	case 1: return GPIO_ReadPin(GPIO_PB,  3) == 0;
	case 2: return GPIO_ReadPin(GPIO_PB,  4) == 0;
	case 3: return GPIO_ReadPin(GPIO_PB,  5) == 0;
	case 4: return GPIO_ReadPin(GPIO_PC, 13) == 0;
	}
	return 0;
}

/* ================================================================
   串口命令直接操作菜单（不再走 KeyScan）
   ================================================================ */
void Menu_InjectKey(u8 key)
{
	if (key < 1 || key > 4) return;
	Menu_Dispatch(key, 1);
}

/* ================================================================
   完整页面渲染（按当前 menu 状态）
   ================================================================ */
static void Render(void)
{
	char buf[17];

	switch (menu.level) {

	/* ---- 主菜单 ---- */
	case MENU_MAIN:
		OLED_ShowString(1, 2, "===  MAIN  ===");
		OLED_ShowString(2, 1, "1.Mode  2.Data ");
		OLED_ShowString(3, 1, "3.PID   4.Run   ");
		OLED_ShowString(4, 1, "   ");
		break;

	/* ---- 模式切换 ---- */
	case MENU_MODE_SELECT: {
		OLED_ShowString(1, 1, "  MODE SELECT");
		for (u8 i = 0; i < MODE_SLOT_COUNT; i++) {
			RunMode_t cur = RunMode_Get();
			char mark = ((RunMode_t)mode_slots[i] == cur) ? '>' : ' ';
			sprintf(buf, "%c%d.%-13s",
			        mark, i + 1, RunMode_GetName((RunMode_t)mode_slots[i]));
			OLED_ShowString(2 + i, 1, buf);
		}
		break;
	}

	/* ---- 数据类别 ---- */
	case MENU_DATA_CAT: {
		static const char* cat_names[DATA_CAT_COUNT] = {
			"SPEED LOOP",
			"GRAYSCALE",
			"YAW ANGLE",
			"SYSTEM",
		};
		OLED_ShowString(1, 2, " DATA  SELECT");
		sprintf(buf, "> %-14s", cat_names[menu.data_cat]);
		OLED_ShowString(2, 1, buf);
		OLED_ShowString(3, 1, "   KEY1/2:switch");
		OLED_ShowString(4, 1, "KEY3:enter 4:back");
		break;
	}

	/* ---- 数据翻页 ---- */
	case MENU_DATA_PAGE:
		RenderDataPage(menu.data_cat, menu.data_page);
		break;

	/* ---- PID 类别 ---- */
	case MENU_PID_CAT:
		OLED_ShowString(1, 2, " PID  SELECT");
		if (menu.pid_cat == PIDCAT_SPEED)
			OLED_ShowString(2, 1, "> SPEED PID     ");
		else
			OLED_ShowString(2, 1, "> LINE  PID     ");
		OLED_ShowString(3, 1, "1/2:SW 3:ENTER ");
		OLED_ShowString(4, 1, "   4.BACK      ");
		break;

	/* ---- PID 调参 ---- */
	case MENU_PID_TUNE: {
		float kp, ki, kd;
		if (menu.pid_cat == PIDCAT_SPEED) {
			kp = Car_GetKp(); ki = Car_GetKi(); kd = Car_GetKd();
		} else {
			kp = LineControl_GetKp(); ki = LineControl_GetKi(); kd = LineControl_GetKd();
		}

		const char* title = (menu.pid_cat == PIDCAT_SPEED)
			? " SPEED PID" : " LINE  PID";
		OLED_ShowString(1, 1, title);

		sprintf(buf, "%cKp:%-10.3f", menu.pid_param == 0 ? '*' : ' ', kp);
		OLED_ShowString(2, 1, buf);
		sprintf(buf, "%cKi:%-10.3f", menu.pid_param == 1 ? '*' : ' ', ki);
		OLED_ShowString(3, 1, buf);
		sprintf(buf, "%cKd:%-10.3f", menu.pid_param == 2 ? '*' : ' ', kd);
		OLED_ShowString(4, 1, buf);
		break;
	}

	/* ---- 启停确认 ---- */
	case MENU_RUN_CONFIRM:
		OLED_ShowString(1, 2, " RUN  MODE ?");
		OLED_ShowString(2, 1, "                ");
		sprintf(buf, "  %-12s", RunMode_GetName(RunMode_Get()));
		OLED_ShowString(3, 1, buf);
		OLED_ShowString(4, 1, "4=RUN 1/2/3=STOP");
		break;
	}
}

/* ================================================================
   数据翻页渲染
   ================================================================ */
static void RenderDataPage(u8 cat, u8 pg)
{
	char buf[17];

	switch (cat) {

	case DATA_SPEED:
		switch (pg) {
		case 0:  /* 总览 */
			OLED_ShowString(1, 2, "SPEED  LOOP");
			sprintf(buf, "LT:%-4d RT:%-4d",
			        (int)Car_GetTarget(0), (int)Car_GetTarget(1));
			OLED_ShowString(2, 1, buf);
			sprintf(buf, "LS:%-4.0f RS:%-4.0f",
			        Car_GetFiltSpeed(0), Car_GetFiltSpeed(1));
			OLED_ShowString(3, 1, buf);
			sprintf(buf, "LP:%-4.0f RP:%-4.0f",
			        Motor_GetLastPWM(MOTOR_LEFT),
			        Motor_GetLastPWM(MOTOR_RIGHT));
			OLED_ShowString(4, 1, buf);
			return;
		case 1:  /* PID */
			OLED_ShowString(1, 2, "PID  MONITOR");
			sprintf(buf, "Kp:%.3f Ki:%.3f",
			        Car_GetKp(), Car_GetKi());
			OLED_ShowString(2, 1, buf);
			sprintf(buf, "Kd:%.3f", Car_GetKd());
			OLED_ShowString(3, 1, buf);
			sprintf(buf, "T:%-4.0f T:%-4.0f",
			        Car_GetTarget(0), Car_GetTarget(1));
			OLED_ShowString(4, 1, buf);
			return;
		case 2:  /* 左电机 */
			OLED_ShowString(1, 3, "MOTOR LEFT");
			sprintf(buf, "SPD:%-4d ENC:%-4d",
			        (int)Car_GetTarget(0), Car_GetEnc(0));
			OLED_ShowString(2, 1, buf);
			sprintf(buf, "PWM:%-4.0f",
			        Motor_GetLastPWM(MOTOR_LEFT));
			OLED_ShowString(3, 1, buf);
			sprintf(buf, "FLT:%-5.1f",
			        Car_GetFiltSpeed(0));
			OLED_ShowString(4, 1, buf);
			return;
		case 3:  /* 右电机 */
			OLED_ShowString(1, 3, "MOTOR RIGHT");
			sprintf(buf, "SPD:%-4d ENC:%-4d",
			        (int)Car_GetTarget(1), Car_GetEnc(1));
			OLED_ShowString(2, 1, buf);
			sprintf(buf, "PWM:%-4.0f",
			        Motor_GetLastPWM(MOTOR_RIGHT));
			OLED_ShowString(3, 1, buf);
			sprintf(buf, "FLT:%-5.1f",
			        Car_GetFiltSpeed(1));
			OLED_ShowString(4, 1, buf);
			return;
		}
		break;

	case DATA_GRAY:
		OLED_ShowString(1, 2, "GRAYSCALE");
		OLED_ShowString(2, 2, "   (disabled)");
		OLED_ShowString(3, 1, "                ");
		OLED_ShowString(4, 1, "                ");
		return;

	case DATA_YAW:
		OLED_ShowString(1, 2, "YAW ANGLE");
		OLED_ShowString(2, 2, "   (disabled)");
		OLED_ShowString(3, 1, "                ");
		OLED_ShowString(4, 1, "                ");
		return;

	case DATA_SYS:
		OLED_ShowString(1, 2, "   SYSTEM");
		sprintf(buf, "MODE:%-11s",
		        RunMode_GetName(RunMode_Get()));
		OLED_ShowString(2, 1, buf);
		OLED_ShowString(3, 1, "UART1/3  OK    ");
		OLED_ShowString(4, 1, "    4:BACK      ");
		return;
	}

	/* 越界 */
	OLED_ShowString(1, 2, "   NO DATA");
	OLED_ShowString(2, 1, "                ");
	OLED_ShowString(3, 1, "                ");
	OLED_ShowString(4, 1, "                ");
}
