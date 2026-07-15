#ifndef __RUNMODE_H__
#define __RUNMODE_H__

#include "fml_types.h"

/* 运行模式 */
typedef enum {
	MODE_IDLE,        /* 停车等待 */
	MODE_MANUAL,      /* 蓝牙命令直控（调参） */
	MODE_STRAIGHT,    /* 直走定距（编码器）   [TODO] */
	MODE_LINE,        /* 灰度循迹              [TODO] */
	MODE_YAW_LOCK,    /* 角度环锁定            [TODO] */
	MODE_LINE_PARK,   /* 循迹 + 定点停车       [TODO] */
	MODE_COUNT
} RunMode_t;

void      RunMode_Init(void);
void      RunMode_Update(void);
void      RunMode_Set(RunMode_t mode);
RunMode_t RunMode_Get(void);
const char* RunMode_GetName(RunMode_t mode);
void      RunMode_Run(void);
void      RunMode_Stop(void);

#endif
