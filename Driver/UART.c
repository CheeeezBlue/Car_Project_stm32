#include "UART.h"
#include "System.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static USART_TypeDef* const usart_map[] = { USART1, USART2, USART3 };

/* --- 环形缓冲区（每个UART一个） --- */
static volatile u8  rx_buf[3][UART_RXBUF_SIZE];
static volatile u16 rx_head[3];  /* ISR写入位置 */
static volatile u16 rx_tail[3];  /* 主循环读取位置 */

static __inline void ring_put(UART_ID_t id, u8 byte)
{
	u16 next = (rx_head[id] + 1) & (UART_RXBUF_SIZE - 1);
	if (next != rx_tail[id]) {       /* 未满 */
		rx_buf[id][rx_head[id]] = byte;
		rx_head[id] = next;
	}
}

static __inline u8 ring_get(UART_ID_t id)
{
	if (rx_head[id] == rx_tail[id]) return 0;  /* 空 */
	u8 byte = rx_buf[id][rx_tail[id]];
	rx_tail[id] = (rx_tail[id] + 1) & (UART_RXBUF_SIZE - 1);
	return byte;
}

static __inline u8 ring_avail(UART_ID_t id)
{
	return (rx_head[id] - rx_tail[id]) & (UART_RXBUF_SIZE - 1);
}

/* ================================================================
   基础驱动（阻塞）
   ================================================================ */

/**
 * @brief  初始化UART（8N1）
 */
void UART_Init(UART_ID_t uart_id, u32 baud)
{
	USART_TypeDef* uart = usart_map[uart_id];

	if (uart_id == UART_ID_1)
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	else if (uart_id == UART_ID_2)
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	else
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	USART_InitTypeDef cfg;
	cfg.USART_BaudRate = baud;
	cfg.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	cfg.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	cfg.USART_Parity = USART_Parity_No;
	cfg.USART_StopBits = USART_StopBits_1;
	cfg.USART_WordLength = USART_WordLength_8b;
	USART_Init(uart, &cfg);
	USART_Cmd(uart, ENABLE);

	/* 清空环形缓冲区 */
	rx_head[uart_id] = 0;
	rx_tail[uart_id] = 0;
}

/**
 * @brief  使能RXNE中断
 */
void UART_EnableIRQ(UART_ID_t uart_id)
{
	USART_TypeDef* uart = usart_map[uart_id];
	USART_ITConfig(uart, USART_IT_RXNE, ENABLE);

	if (uart_id == UART_ID_1)
		System_NVIC_Config(USART1_IRQn, 1, 0);
	else if (uart_id == UART_ID_2)
		System_NVIC_Config(USART2_IRQn, 1, 0);
	else
		System_NVIC_Config(USART3_IRQn, 1, 0);
}

void UART_SendByte(UART_ID_t uart_id, u8 data)
{
	USART_TypeDef* uart = usart_map[uart_id];
	while (!USART_GetFlagStatus(uart, USART_FLAG_TXE));
	USART_SendData(uart, data);
}

void UART_SendString(UART_ID_t uart_id, const char* str)
{
	while (*str) UART_SendByte(uart_id, *str++);
}

void UART_SendBuf(UART_ID_t uart_id, const u8* buf, u16 len)
{
	for (u16 i = 0; i < len; i++) UART_SendByte(uart_id, buf[i]);
}

void UART_Printf(const char* fmt, ...)
{
	char buf[128];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	UART_SendString(UART_ID_1, buf);      /* CH340 USB (USART1) */
	UART_SendString(UART_ID_3, buf);      /* 蓝牙 (USART3)   */
}

/* ================================================================
   非阻塞接收
   ================================================================ */

u8 UART_ReadByte(UART_ID_t uart_id)
{
	return ring_get(uart_id);
}

u8 UART_Available(UART_ID_t uart_id)
{
	return ring_avail(uart_id);
}

/**
 * @brief  窥探环形缓冲区首字节，不消费
 * @retval 1=成功读取, 0=缓冲区空
 */
u8 UART_PeekByte(UART_ID_t uart_id, u8* byte)
{
	if (rx_head[uart_id] == rx_tail[uart_id]) return 0;
	*byte = rx_buf[uart_id][rx_tail[uart_id]];
	return 1;
}

/**
 * @brief  窥探环形缓冲区最多 n 字节到 buf，不消费
 * @retval 实际读到的字节数
 */
u8 UART_PeekBuf(UART_ID_t uart_id, u8* buf, u8 n)
{
	u8 avail = ring_avail(uart_id);
	if (n > avail) n = avail;
	u16 tail = rx_tail[uart_id];
	for (u8 i = 0; i < n; i++) {
		buf[i] = rx_buf[uart_id][(tail + i) & (UART_RXBUF_SIZE - 1)];
	}
	return n;
}

/**
 * @brief  中断服务函数中调用，将接收到的字节写入环形缓冲区
 */
void UART_RxISR(UART_ID_t uart_id)
{
	if (USART_GetITStatus(usart_map[uart_id], USART_IT_RXNE) != RESET) {
		ring_put(uart_id, (u8)USART_ReceiveData(usart_map[uart_id]));
	}
}

/* ================================================================
   行解析（非阻塞）
   ================================================================ */

/**
 * @brief  从环形缓冲区提取一行（遇到'\n'返回），无阻塞
 * @retval 1=取到完整一行, 0=还没有
 */
u8 UART_GetLine(UART_ID_t uart_id, char* buf, u8 maxlen)
{
	u8 i = 0;
	while (ring_avail(uart_id) && i < maxlen - 1) {
		char c = (char)ring_get(uart_id);
		buf[i++] = c;
		if (c == '\n') { buf[i] = '\0'; return 1; }
	}
	buf[i] = '\0';
	return i > 0 ? 1 : 0;  /* 即使没换行也返回已有内容 */
}

/* ================================================================
   命令解析
   支持的格式：
     SPD:50        设置两轮速度
     L:50 R:30     分别设置左右轮
     STOP          停止
     ?             查询状态
     PID:100       设定PID目标
   ================================================================ */

void UART_ParseCmd(const char* line, UART_Cmd_t* cmd)
{
	cmd->type     = CMD_NONE;
	cmd->left     = 0;
	cmd->right    = 0;
	cmd->value    = 0;
	cmd->left_set = 0;
	cmd->right_set= 0;

	if (!line || !*line) return;

	if (strncmp(line, "SPD:", 4) == 0) {
		cmd->type = CMD_SPD;
		cmd->value = (s16)atoi(line + 4);
	} else if (strncmp(line, "L:", 2) == 0) {
		cmd->type = CMD_LR;
		cmd->left = (s8)atoi(line + 2);
		cmd->left_set = 1;
		const char* r = strstr(line, "R:");
		if (r) { cmd->right = (s8)atoi(r + 2); cmd->right_set = 1; }
	} else if (strncmp(line, "R:", 2) == 0) {
		cmd->type = CMD_LR;
		cmd->right = (s8)atoi(line + 2);
		cmd->right_set = 1;
		const char* l = strstr(line, "L:");
		if (l) { cmd->left = (s8)atoi(l + 2); cmd->left_set = 1; }
	} else if (strncmp(line, "STOP", 4) == 0) {
		cmd->type = CMD_STOP;
	} else if (line[0] == '?') {
		cmd->type = CMD_QUERY;
	} else if (strncmp(line, "KP:", 3) == 0) {
		cmd->type = CMD_KP;
		cmd->value = (s16)atoi(line + 3);
	} else if (strncmp(line, "KI:", 3) == 0) {
		cmd->type = CMD_KI;
		cmd->value = (s16)atoi(line + 3);
	} else if (strncmp(line, "KD:", 3) == 0) {
		cmd->type = CMD_KD;
		cmd->value = (s16)atoi(line + 3);
	} else if (strncmp(line, "TGT:", 4) == 0) {
		cmd->type = CMD_TGT;
		cmd->value = (s16)atoi(line + 4);
	} else if (strncmp(line, "O:", 2) == 0) {
		cmd->type = CMD_OL;
		cmd->value = (s16)atoi(line + 2);
	} else if (strncmp(line, "FFL:", 4) == 0) {
		cmd->type = CMD_FFL;
		cmd->value = (s16)atoi(line + 4);
	} else if (strncmp(line, "FFR:", 4) == 0) {
		cmd->type = CMD_FFR;
		cmd->value = (s16)atoi(line + 4);
	} else if (strncmp(line, "FFO:", 4) == 0) {
		cmd->type = CMD_FFO;
		cmd->value = (s16)atoi(line + 4);
	} else if (strncmp(line, "FDB:", 4) == 0) {
		cmd->type = CMD_FDB;
		cmd->value = (s16)atoi(line + 4);
	} else if (strncmp(line, "RMP:", 4) == 0) {
		cmd->type = CMD_RMP;
		cmd->value = (s16)atoi(line + 4);
		} else if (strncmp(line, "YHP:", 4) == 0) {
			cmd->type = CMD_YHP;
			cmd->value = (s16)atoi(line + 4);
		} else if (strncmp(line, "YHI:", 4) == 0) {
			cmd->type = CMD_YHI;
			cmd->value = (s16)atoi(line + 4);
		} else if (strncmp(line, "YHD:", 4) == 0) {
			cmd->type = CMD_YHD;
			cmd->value = (s16)atoi(line + 4);
		} else if (strncmp(line, "YRL:", 4) == 0) {
			cmd->type = CMD_YRL;
			cmd->value = (s16)atoi(line + 4);
		} else if (strncmp(line, "YAWP:", 5) == 0) {
		cmd->type = CMD_YWP;
		cmd->value = (s16)atoi(line + 5);
	} else if (strncmp(line, "YAWI:", 5) == 0) {
		cmd->type = CMD_YWI;
		cmd->value = (s16)atoi(line + 5);
	} else if (strncmp(line, "YAWD:", 5) == 0) {
		cmd->type = CMD_YWD;
		cmd->value = (s16)atoi(line + 5);
	} else if (strncmp(line, "YAWM:", 5) == 0) {
		cmd->type = CMD_YWM;
		cmd->value = (s16)atoi(line + 5);
	} else if (strncmp(line, "MD:", 3) == 0) {
		cmd->type = CMD_MODE;
		cmd->value = (s16)atoi(line + 3);
	} else if (strncmp(line, "YAW:", 4) == 0) {
		cmd->type = CMD_YAW;
		cmd->value = (s16)atoi(line + 4);
	} else if (strncmp(line, "LSPD:", 5) == 0) {
		cmd->type = CMD_LSPD;
		cmd->value = (s16)atoi(line + 5);
		} else if (strncmp(line, "LKP:", 4) == 0) {
		cmd->type = CMD_LKP;
		cmd->value = (s16)atoi(line + 4);
		} else if (strncmp(line, "LKI:", 4) == 0) {
		cmd->type = CMD_LKI;
		cmd->value = (s16)atoi(line + 4);
		} else if (strncmp(line, "LKD:", 4) == 0) {
		cmd->type = CMD_LKD;
		cmd->value = (s16)atoi(line + 4);
		} else if (strncmp(line, "KEY1", 4) == 0) {
			cmd->type = CMD_KEY1;
		} else if (strncmp(line, "KEY2", 4) == 0) {
			cmd->type = CMD_KEY2;
		} else if (strncmp(line, "KEY3", 4) == 0) {
			cmd->type = CMD_KEY3;
		} else if (strncmp(line, "KEY4", 4) == 0) {
			cmd->type = CMD_KEY4;
		}
	}
