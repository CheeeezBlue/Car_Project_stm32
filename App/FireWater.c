#include "FireWater.h"
#include "Command.h"
#include "../Driver/UART.h"

/* ================================================================
   CRC8 — 多项式 0x1D，支持增量计算
   ================================================================ */
static u8 crc8_byte(u8 crc, u8 byte)
{
	crc ^= byte;
	for (u8 i = 0; i < 8; i++)
		crc = (crc & 0x80) ? ((crc << 1) ^ 0x1D) : (crc << 1);
	return crc;
}

u8 FireWater_CRC8(const u8* data, u16 len)
{
	u8 crc = 0xFF;
	while (len--) crc = crc8_byte(crc, *data++);
	return crc;
}

/* ================================================================
   发送：组包 → UART
   帧格式: | 0xAF | Type | Len_L | Len_H | Payload | CRC8 |
   CRC 范围: Type + Length[2] + Payload（不含 0xAF）
   ================================================================ */
void FireWater_SendFrame(u8 type, const u8* payload, u16 len)
{
	/* 增量 CRC：type → len_L → len_H → payload */
	u8 crc = 0xFF;
	crc = crc8_byte(crc, type);
	crc = crc8_byte(crc, (u8)(len & 0xFF));
	crc = crc8_byte(crc, (u8)((len >> 8) & 0xFF));
	for (u16 i = 0; i < len; i++)
		crc = crc8_byte(crc, payload[i]);

	/* 逐字节发送 */
	UART_SendByte(UART_ID_1, 0xAF);
	UART_SendByte(UART_ID_1, type);
	UART_SendByte(UART_ID_1, (u8)(len & 0xFF));
	UART_SendByte(UART_ID_1, (u8)((len >> 8) & 0xFF));
	for (u16 i = 0; i < len; i++)
		UART_SendByte(UART_ID_1, payload[i]);
	UART_SendByte(UART_ID_1, crc);
}

/* ================================================================
   便捷：发送 float 数组数据帧 — type=0x00
   payload = count × 4 字节（float 原始字节流）
   ================================================================ */
void FireWater_SendData(const float* data, u8 count)
{
	FireWater_SendFrame(FW_TYPE_DATA, (const u8*)data, count * 4);
}

/* ================================================================
   便捷：发送文本响应帧 — type=0x02
   ================================================================ */
void FireWater_SendResponse(const char* text)
{
	u16 len = 0;
	while (text[len]) len++;
	if (len > FW_PAYLOAD_MAX) len = FW_PAYLOAD_MAX;
	FireWater_SendFrame(FW_TYPE_RESP, (const u8*)text, len);
}

/* ================================================================
   接收：从 UART 环形缓冲区提取一个 FireWater 帧
   仅在帧完整可用时才消费数据；否则不碰缓冲区
   返回 1=提取成功
   ================================================================ */
u8 FireWater_GetFrame(u8* type, u8* payload, u8* len)
{
	u8 avail = UART_Available(UART_ID_1);
	if (avail < 5) return 0;  /* 最小帧: 0xAF + type + len×2 + CRC */

	/* 探测帧头 */
	u8 first;
	if (!UART_PeekByte(UART_ID_1, &first)) return 0;
	if (first != 0xAF) return 0;

	/* 探测完整头（4字节），计算 payload 长度 */
	u8 header[4];
	UART_PeekBuf(UART_ID_1, header, 4);
	u16 payload_len = (u16)header[2] | ((u16)header[3] << 8);
	if (payload_len > FW_PAYLOAD_MAX) {
		/* 长度非法 → 丢弃帧头字节，恢复文本解析 */
		UART_ReadByte(UART_ID_1);
		return 0;
	}

	/* 检查完整帧是否到齐 */
	u16 total = 4 + payload_len + 1;  /* header + payload + CRC */
	if (avail < total) return 0;      /* 数据未到齐，下次再试 */

	/* 消费整帧 */
	UART_ReadByte(UART_ID_1);           /* 丢弃 0xAF */
	u8 fw_type   = UART_ReadByte(UART_ID_1);
	u8 fw_len_l  = UART_ReadByte(UART_ID_1);
	u8 fw_len_h  = UART_ReadByte(UART_ID_1);

	for (u16 i = 0; i < payload_len; i++)
		payload[i] = UART_ReadByte(UART_ID_1);
	u8 crc_rcvd = UART_ReadByte(UART_ID_1);

	/* 增量 CRC 校验：type → len_L → len_H → payload */
	u8 crc_calc = 0xFF;
	crc_calc = crc8_byte(crc_calc, fw_type);
	crc_calc = crc8_byte(crc_calc, fw_len_l);
	crc_calc = crc8_byte(crc_calc, fw_len_h);
	for (u16 i = 0; i < payload_len; i++)
		crc_calc = crc8_byte(crc_calc, payload[i]);

	if (crc_calc != crc_rcvd) return 0;   /* CRC 不匹配 */

	*type = fw_type;
	*len  = (u8)payload_len;
	return 1;
}

/* ================================================================
   主循环调用：处理接收到的 FireWater 命令帧
   ================================================================ */
void FireWater_Process(void)
{
	u8 type, payload[FW_PAYLOAD_MAX], len;
	if (FireWater_GetFrame(&type, payload, &len)) {
		if (type == FW_TYPE_CMD || type == FW_TYPE_RESP) {
			Command_HandleFW(type, payload, len);
		}
	}
}
