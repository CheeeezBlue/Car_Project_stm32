#ifndef __UART_H__
#define __UART_H__

#include "fml_types.h"

/* 环形缓冲区大小（必须是2的幂） */
#define UART_RXBUF_SIZE  256
#define UART_LINE_SIZE   64

/* 解析后的命令结构体 */
typedef enum {
	CMD_NONE, CMD_SPD, CMD_LR, CMD_STOP, CMD_QUERY,
	CMD_KP, CMD_KI, CMD_KD, CMD_TGT, CMD_OL,   /* OL=开环测试 */
	CMD_FFL, CMD_FFR, CMD_FFO                       /* 前馈系数 + 死区偏移 */
} CmdType_t;

typedef struct {
	CmdType_t type;
	s8 left;         /* 左轮速度 (-100~100) */
	s8 right;        /* 右轮速度 (-100~100) */
	s16 value;       /* 通用值 */
	u8 left_set :1;  /* L: 是否出现在命令中 */
	u8 right_set:1;  /* R: 是否出现在命令中 */
} UART_Cmd_t;

/* --- 基础 UART 驱动（阻塞） --- */
void UART_Init(UART_ID_t uart_id, u32 baud);
void UART_EnableIRQ(UART_ID_t uart_id);
void UART_SendByte(UART_ID_t uart_id, u8 data);
void UART_SendString(UART_ID_t uart_id, const char* str);
void UART_SendBuf(UART_ID_t uart_id, const u8* buf, u16 len);
void UART_Printf(const char* fmt, ...);

/* --- 非阻塞接收（中断 + 环形缓冲区） --- */
u8   UART_ReadByte(UART_ID_t uart_id);          /* 读一个字节，无数据返回0 */
u8   UART_Available(UART_ID_t uart_id);         /* 缓冲区可用字节数 */
u8   UART_PeekByte(UART_ID_t uart_id, u8* byte); /* 窥探首字节，不消费 */
u8   UART_PeekBuf(UART_ID_t uart_id, u8* buf, u8 n); /* 窥探最多 n 字节，不消费 */
void UART_RxISR(UART_ID_t uart_id);             /* 中断服务中调用 */

/* --- 行解析 --- */
u8   UART_GetLine(UART_ID_t uart_id, char* buf, u8 maxlen); /* 提取一行（无阻塞） */

/* --- 命令解析 --- */
void UART_ParseCmd(const char* line, UART_Cmd_t* cmd); /* 解析文本行为命令 */

#endif
