#ifndef __RUNMODE_H__
#define __RUNMODE_H__

#include "fml_types.h"

typedef enum {
	MODE_STATIONARY = 1,
	MODE_STRAIGHT   = 2,
	MODE_LINE       = 3,
	MODE_COUNT
} RunMode_t;

void        RunMode_Init(void);
void        RunMode_Update(void);
u8          RunMode_Set(RunMode_t mode);
RunMode_t   RunMode_Get(void);
const char* RunMode_GetName(RunMode_t mode);
void        RunMode_Run(void);
void        RunMode_Stop(void);
u8          RunMode_IsRunning(void);

#endif
