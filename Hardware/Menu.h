#ifndef __MENU_H__
#define __MENU_H__

#include "fml_types.h"

/* 菜单层级 */
typedef enum {
	MENU_MAIN,           /* 主菜单 4 项 */
	MENU_MODE_SELECT,    /* 模式切换 */
	MENU_DATA_CAT,       /* 数据类别 */
	MENU_DATA_PAGE,      /* 数据翻页 */
	MENU_PID_CAT,        /* PID 类别 */
	MENU_PID_TUNE,       /* PID 调参 */
	MENU_RUN_CONFIRM,    /* 启停二次确认 */
} MenuLevel_t;

/* 数据类别索引 */
typedef enum {
	DATA_SPEED  = 0,
	DATA_GRAY   = 1,   /* 灰度启用后追加 */
	DATA_YAW    = 2,   /* MPU6050 启用后追加 */
	DATA_SYS    = 3,
	DATA_CAT_COUNT
} DataCat_t;

/* PID 类别索引 */
#define PIDCAT_SPEED  0
#define PIDCAT_LINE   1

/* 调参步长 */
#define PID_STEP_FINE    0.001f
#define PID_STEP_COARSE  0.010f
#define LONG_PRESS_TICKS 50      /* 0.5s @ 10ms */
#define REPEAT_INTERVAL  3       /* 每 30ms 连发 */

/* 模式槽位（模式切换页显示，0-based 索引） */
#define MODE_SLOT_COUNT  3
extern const u8 mode_slots[MODE_SLOT_COUNT];  /* RunMode_t 枚举值数组 */

void Menu_Init(void);
void Menu_Update(void);
void Menu_InjectKey(u8 key);

#endif
