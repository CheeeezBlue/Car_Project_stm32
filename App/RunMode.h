#ifndef __RUNMODE_H__
#define __RUNMODE_H__

#include "fml_types.h"

/* 运行模式 */
typedef enum {
	MODE_IDLE     = 0,  /* 停车等待 */
	MODE_MANUAL   = 1,  /* 蓝牙命令直控（调参） */
	MODE_STRAIGHT = 2,  /* 直走 + 角度环稳定 */
	MODE_LINE     = 3,  /* 灰度循迹 */
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
