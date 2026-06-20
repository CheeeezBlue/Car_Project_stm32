#ifndef __COMMAND_H__
#define __COMMAND_H__

#include "fml_types.h"

/* 文本命令入口（UART_GetLine → 解析 → 分发） */
void Command_Handle(const char* line);

/* FireWater 命令帧入口（FireWater_GetFrame → 转文本 → 复用解析器） */
void Command_HandleFW(u8 type, const u8* payload, u8 len);

#endif
