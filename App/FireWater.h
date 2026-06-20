#ifndef __FIREWATER_H__
#define __FIREWATER_H__

#include "fml_types.h"

/* FireWater 帧类型 */
#define FW_TYPE_DATA     0x00   /* 数据帧（波形浮点数组） */
#define FW_TYPE_CMD      0x01   /* 命令帧（PC → MCU） */
#define FW_TYPE_RESP     0x02   /* 响应帧（MCU → PC） */

/* 最大 payload 长度（总帧 ≤ 256 字节，header4 + CRC1 = 5，留 251） */
#define FW_PAYLOAD_MAX   251

/* ---- 发送 API ---- */

/* 发送完整 FireWater 帧（type + payload[len] → 组包 + CRC8 → UART发送） */
void FireWater_SendFrame(u8 type, const u8* payload, u16 len);

/* 便捷：发送 float 数组数据帧（type=0x00） */
void FireWater_SendData(const float* data, u8 count);

/* 便捷：发送文本响应帧（type=0x02） */
void FireWater_SendResponse(const char* text);

/* ---- 接收 API ---- */

/* 尝试从 UART 环形缓冲区提取一个完整 FireWater 帧
 * 仅在帧头 0xAF + 完整帧可用时才消费数据，否则不动缓冲区
 * 返回 1=提取成功（填充 type/payload/len），0=暂无数据或帧不完整 */
u8 FireWater_GetFrame(u8* type, u8* payload, u8* len);

/* 循环调用：处理接收到的 FireWater 命令帧，分发到 Command_HandleFW */
void FireWater_Process(void);

/* ---- CRC8 ---- */
u8 FireWater_CRC8(const u8* data, u16 len);

#endif
