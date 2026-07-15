#include "Menu.h"
#include "OLED.h"
#include "Key.h"
#include "../App/CarControl.h"
#include "../App/RunMode.h"
#include "../App/LineControl.h"
#include "../Hardware/Motor.h"
#include "../Driver/GPIO.h"
#include "../System/Delay.h"
#include <stdio.h>

/* ---- 模式槽位 ---- */
const u8 mode_slots[MODE_SLOT_COUNT] = {
	MODE_IDLE,       /* KEY1: 暂停小车 */
	MODE_MANUAL,     /* KEY2: 正常调试 */
	MODE_LINE,       /* KEY3: 循迹模式 */
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
	u8  pending;       /* 待执行动作: 0=无, 1=短按, 2=长按连发 */        
	u16 confirm_tick;  /* 启停确认倒计时 */
	u8  dirty;         /* 需要重绘 */
} menu;

/* ---- 前向声明 ---- */
static void Render(void);
static void RenderDataPage(u8 cat, u8 pg);
static u8   KeyRaw(u8 id);
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
	menu.pending      = 0;
	menu.confirm_tick = 0;
	menu.dirty        = 1;

	Delay_ms(1500);
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
   按键扫描：短按 + 长按检测
   ================================================================ */
static void KeyScan(void)
{
	/* 检查当前按键是否仍按下 */
	if (menu.key_held && KeyRaw(menu.key_held)) {
		menu.hold_ticks++;
		if (menu.hold_ticks == 1) {
			menu.pending = 1;  /* 首次短按 */
		} else if (menu.hold_ticks > LONG_PRESS_TICKS &&
		           (menu.hold_ticks % REPEAT_INTERVAL == 0)) {
			menu.pending = 2;  /* 长按连发 */
		}
	} else {
		/* 按键已松开，检查新按下 */
		menu.key_held   = 0;
		menu.hold_ticks = 0;

		u8 key = Key_GetNum();  /* 边沿检测 */
		if (key) {
			menu.key_held   = key;
			menu.hold_ticks = 1;
			menu.pending    = 1;  /* 首次短按 */
			return;
		}
	}

	if (!menu.pending) return;

	u8 act = menu.pending;
	menu.pending = 0;

	switch (menu.level) {

	/* ================================================================
	   MENU_MAIN — 主菜单
	   ================================================================ */
	case MENU_MAIN:
		if (act != 1) break;
		switch (menu.key_held) {
		case 1:  /* KEY1: 模式切换 */
			menu.level = MENU_MODE_SELECT;
			menu.cursor = 0;
			OLED_Clear(); menu.dirty = 1;
			break;
		case 2:  /* KEY2: 数据显示 */
			menu.level = MENU_DATA_CAT;
			menu.cursor = 0;
			menu.data_cat = 0;
			menu.data_page = 0;
			OLED_Clear(); menu.dirty = 1;
			break;
		case 3:  /* KEY3: PID调参 */
			menu.level = MENU_PID_CAT;
			menu.cursor = 0;
			menu.pid_cat = 0;
			menu.pid_param = 0;
			OLED_Clear(); menu.dirty = 1;
			break;
		case 4:  /* KEY4: 一键启停 */
			menu.level = MENU_RUN_CONFIRM;
			menu.confirm_tick = 0;
			OLED_Clear(); menu.dirty = 1;
			break;
		}
		break;

	/* ================================================================
	   MENU_MODE_SELECT — 模式切换
	   ================================================================ */
	case MENU_MODE_SELECT:
		if (act != 1) break;
		switch (menu.key_held) {
		case 1: case 2: case 3:
			RunMode_Set((RunMode_t)mode_slots[menu.key_held - 1]);
			menu.dirty = 1;
			break;
		case 4:  /* 返回 */
			menu.level = MENU_MAIN;
			OLED_Clear(); menu.dirty = 1;
			break;
		}
		break;

	/* ================================================================
	   MENU_DATA_CAT — 数据类别
	   ================================================================ */
	case MENU_DATA_CAT:
		if (act != 1) break;
		switch (menu.key_held) {
		case 1:  /* 上翻类别 */
			menu.data_cat = (menu.data_cat == 0)
				? DATA_CAT_COUNT - 1 : menu.data_cat - 1;
			menu.data_page = 0;
			menu.dirty = 1;
			break;
		case 2:  /* 下翻类别 */
			menu.data_cat = (menu.data_cat + 1) % DATA_CAT_COUNT;
			menu.data_page = 0;
			menu.dirty = 1;
			break;
		case 3:  /* 进入该类别翻页 */
			menu.level = MENU_DATA_PAGE;
			OLED_Clear(); menu.dirty = 1;
			break;
		case 4:  /* 返回 */
			menu.level = MENU_MAIN;
			OLED_Clear(); menu.dirty = 1;
			break;
		}
		break;

	/* ================================================================
	   MENU_DATA_PAGE — 数据翻页
	   ================================================================ */
	case MENU_DATA_PAGE:
		if (act != 1) break;
		switch (menu.key_held) {
		case 1:  /* 上页 */
			if (menu.data_page > 0) {
				menu.data_page--;
				menu.dirty = 1;
			}
			break;
		case 2:  /* 下页 */
			menu.data_page++;
			menu.dirty = 1;
			break;
		case 4:  /* 返回类别选择 */
			menu.level = MENU_DATA_CAT;
			OLED_Clear(); menu.dirty = 1;
			break;
		}
		break;

	/* ================================================================
	   MENU_PID_CAT — PID 类别
	   ================================================================ */
	case MENU_PID_CAT:
		if (act != 1) break;
		switch (menu.key_held) {
		case 1:  /* 速度环 PID */
			menu.pid_cat = PIDCAT_SPEED;
			menu.pid_param = 0;
			menu.level = MENU_PID_TUNE;
			OLED_Clear(); menu.dirty = 1;
			break;
		case 2:  /* 灰度 PID */
			menu.pid_cat = PIDCAT_LINE;
			menu.pid_param = 0;
			menu.level = MENU_PID_TUNE;
			OLED_Clear(); menu.dirty = 1;
			break;
		case 4:  /* 返回 */
			menu.level = MENU_MAIN;
			OLED_Clear(); menu.dirty = 1;
			break;
		}
		break;

	/* ================================================================
	   MENU_PID_TUNE — PID 调参
	   ================================================================ */
	case MENU_PID_TUNE: {
		float step = (act == 2) ? PID_STEP_COARSE : PID_STEP_FINE;
		int dir = (menu.key_held == 1) ? 1 :
		          (menu.key_held == 2) ? -1 : 0;

		if (menu.key_held == 3 && act == 1) {
			/* KEY3: 切换 Kp→Ki→Kd */
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
		} else if (menu.key_held == 4 && act == 1) {
			/* KEY4: 返回 */
			menu.level = MENU_PID_CAT;
			OLED_Clear(); menu.dirty = 1;
		}
		break;
	}

	/* ================================================================
	   MENU_RUN_CONFIRM — 启停二次确认
	   ================================================================ */
	case MENU_RUN_CONFIRM:
		if (act != 1) break;
		if (menu.key_held == 4) {
			/* 确认执行 */
			RunMode_Run();
			menu.level = MENU_MAIN;
			OLED_Clear(); menu.dirty = 1;
		} else {
			/* 其他键取消 */
			menu.level = MENU_MAIN;
			OLED_Clear(); menu.dirty = 1;
		}
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
   完整页面渲染（按当前 menu 状态）
   ================================================================ */
static void Render(void)
{
	char buf[17];

	switch (menu.level) {

	/* ---- 主菜单 ---- */
	case MENU_MAIN:
		OLED_ShowString(1, 2, "===  MAIN  ===");
		OLED_ShowString(2, 0, "1.Mode  2.Data ");
		OLED_ShowString(3, 0, "3.PID   4.Run   ");
		OLED_ShowString(4, 0, "   ");
		break;

	/* ---- 模式切换 ---- */
	case MENU_MODE_SELECT: {
		OLED_ShowString(1, 1, "  MODE SELECT");
		for (u8 i = 0; i < MODE_SLOT_COUNT; i++) {
			RunMode_t cur = RunMode_Get();
			char mark = ((RunMode_t)mode_slots[i] == cur) ? '>' : ' ';
			sprintf(buf, "%c%d.%-13s",
			        mark, i + 1, RunMode_GetName((RunMode_t)mode_slots[i]));
			OLED_ShowString(2 + i, 0, buf);
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
		OLED_ShowString(1, 1, " DATA  SELECT");
		sprintf(buf, "> %-14s", cat_names[menu.data_cat]);
		OLED_ShowString(2, 0, buf);
		OLED_ShowString(3, 0, "   KEY1/2:switch");
		OLED_ShowString(4, 0, "KEY3:enter 4:back");
		break;
	}

	/* ---- 数据翻页 ---- */
	case MENU_DATA_PAGE:
		RenderDataPage(menu.data_cat, menu.data_page);
		break;

	/* ---- PID 类别 ---- */
	case MENU_PID_CAT:
		OLED_ShowString(1, 2, " PID  SELECT");
		OLED_ShowString(2, 0, "1.SPEED PID    ");
		OLED_ShowString(3, 0, "2.LINE  PID    ");
		OLED_ShowString(4, 0, "   4.BACK      ");
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
		OLED_ShowString(1, 0, title);

		sprintf(buf, "%cKp:%-10.3f", menu.pid_param == 0 ? '*' : ' ', kp);
		OLED_ShowString(2, 0, buf);
		sprintf(buf, "%cKi:%-10.3f", menu.pid_param == 1 ? '*' : ' ', ki);
		OLED_ShowString(3, 0, buf);
		sprintf(buf, "%cKd:%-10.3f", menu.pid_param == 2 ? '*' : ' ', kd);
		OLED_ShowString(4, 0, buf);
		break;
	}

	/* ---- 启停确认 ---- */
	case MENU_RUN_CONFIRM:
		OLED_ShowString(1, 2, " RUN  MODE ?");
		OLED_ShowString(2, 0, "                ");
		sprintf(buf, "  %-12s", RunMode_GetName(RunMode_Get()));
		OLED_ShowString(3, 0, buf);
		OLED_ShowString(4, 0, "KEY4=GO  ANY=NO");
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
			sprintf(buf, "L:%-4d R:%-4d",
			        (int)Car_GetTarget(0), (int)Car_GetTarget(1));
			OLED_ShowString(2, 0, buf);
			sprintf(buf, "E:%-4d E:%-4d",
			        Car_GetEnc(0), Car_GetEnc(1));
			OLED_ShowString(3, 0, buf);
			sprintf(buf, "P:%-4.0f P:%-4.0f",
			        Motor_GetLastPWM(MOTOR_LEFT),
			        Motor_GetLastPWM(MOTOR_RIGHT));
			OLED_ShowString(4, 0, buf);
			return;
		case 1:  /* PID */
			OLED_ShowString(1, 2, "PID  MONITOR");
			sprintf(buf, "Kp:%.3f Ki:%.3f",
			        Car_GetKp(), Car_GetKi());
			OLED_ShowString(2, 0, buf);
			sprintf(buf, "Kd:%.3f", Car_GetKd());
			OLED_ShowString(3, 0, buf);
			sprintf(buf, "T:%-4.0f T:%-4.0f",
			        Car_GetTarget(0), Car_GetTarget(1));
			OLED_ShowString(4, 0, buf);
			return;
		case 2:  /* 左电机 */
			OLED_ShowString(1, 3, "MOTOR LEFT");
			sprintf(buf, "SPD:%-4d ENC:%-4d",
			        (int)Car_GetTarget(0), Car_GetEnc(0));
			OLED_ShowString(2, 0, buf);
			sprintf(buf, "PWM:%-4.0f",
			        Motor_GetLastPWM(MOTOR_LEFT));
			OLED_ShowString(3, 0, buf);
			sprintf(buf, "FLT:%-5.1f",
			        Car_GetFiltSpeed(0));
			OLED_ShowString(4, 0, buf);
			return;
		case 3:  /* 右电机 */
			OLED_ShowString(1, 3, "MOTOR RIGHT");
			sprintf(buf, "SPD:%-4d ENC:%-4d",
			        (int)Car_GetTarget(1), Car_GetEnc(1));
			OLED_ShowString(2, 0, buf);
			sprintf(buf, "PWM:%-4.0f",
			        Motor_GetLastPWM(MOTOR_RIGHT));
			OLED_ShowString(3, 0, buf);
			sprintf(buf, "FLT:%-5.1f",
			        Car_GetFiltSpeed(1));
			OLED_ShowString(4, 0, buf);
			return;
		}
		break;

	case DATA_GRAY:
		OLED_ShowString(1, 2, "GRAYSCALE");
		OLED_ShowString(2, 2, "   (disabled)");
		OLED_ShowString(3, 0, "                ");
		OLED_ShowString(4, 0, "                ");
		return;

	case DATA_YAW:
		OLED_ShowString(1, 2, "YAW ANGLE");
		OLED_ShowString(2, 2, "   (disabled)");
		OLED_ShowString(3, 0, "                ");
		OLED_ShowString(4, 0, "                ");
		return;

	case DATA_SYS:
		OLED_ShowString(1, 2, "   SYSTEM");
		sprintf(buf, "MODE:%-11s",
		        RunMode_GetName(RunMode_Get()));
		OLED_ShowString(2, 0, buf);
		OLED_ShowString(3, 0, "UART1/3  OK    ");
		OLED_ShowString(4, 0, "KEY1/2:pg 4:back");
		return;
	}

	/* 越界 */
	OLED_ShowString(1, 2, "   NO DATA");
	OLED_ShowString(2, 0, "                ");
	OLED_ShowString(3, 0, "                ");
	OLED_ShowString(4, 0, "                ");
}

/* ================================================================
   远程注入按键（串口命令 KEY1~KEY4 → 模拟短按）
   ================================================================ */
void Menu_InjectKey(u8 key)
{
	if (key < 1 || key > 4) return;
	menu.key_held   = key;
	menu.hold_ticks = 1;
	menu.pending    = 1;
}
