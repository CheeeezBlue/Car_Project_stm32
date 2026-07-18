#include "JY901S.h"
#include "../Driver/UART.h"

/* JY901S 二进制协议: 0x55 + type + 8data + checksum = 11 bytes */
#define JY901_PKT_LEN  11
#define JY901_HEADER   0x55
#define JY901_GYRO     0x52   /* 角速度包 */
#define JY901_ANGLE    0x53   /* 角度包 */

static float gyro_z  = 0.0f;
static float yaw     = 0.0f;
static u8    valid   = 0;

void JY901S_Init(void)
{
	UART_Init(UART_ID_2, 115200);
	UART_EnableIRQ(UART_ID_2);
	gyro_z = 0.0f;
	yaw    = 0.0f;
	valid  = 0;
}

void JY901S_Update(void)
{
	static u8 buf[JY901_PKT_LEN];
	static u8 idx = 0;

	while (UART_Available(UART_ID_2)) {
		u8 byte = UART_ReadByte(UART_ID_2);

		/* 搜索帧头 0x55，收到即复位索引 */
		if (byte == JY901_HEADER) {
			idx = 0;
		}

		buf[idx++] = byte;
		if (idx < JY901_PKT_LEN) continue;

		/* 收满一帧，校验 */
		idx = 0;
		u8 sum = 0;
		for (u8 i = 0; i < 10; i++) sum += buf[i];
		if (sum != buf[10]) continue;

		u8 type = buf[1];
		switch (type) {
		case JY901_GYRO: {
			s16 wz_raw = (s16)((u16)buf[7] << 8 | buf[6]);
			gyro_z = wz_raw / 32768.0f * 2000.0f;  /* °/s */
			valid = 1;
			break;
		}
		case JY901_ANGLE: {
			s16 yaw_raw = (s16)((u16)buf[7] << 8 | buf[6]);
			yaw = yaw_raw / 32768.0f * 180.0f;     /* ° */
			break;
		}
		}
	}
}

float JY901S_ReadGyroZ(void)
{
	return gyro_z;
}

float JY901S_ReadYaw(void)
{
	return yaw;
}

u8 JY901S_IsValid(void)
{
	return valid;
}
