#include "MPU6050.h"
#include "../Driver/I2C.h"
#include "../System/Delay.h"
#include <stdio.h>

/* 软件 I2C 总线: 与 OLED 共享 PB8(SCL)/PB9(SDA) */
static I2C_Bus_t mpu_i2c = { GPIOB, 8, GPIOB, 9 };

static MPU6050_t mpu = {
	.gyro_bias_z     = 0.0f,
	.gyro_sensitivity = 131.0f,   /* ±250°/s 默认 */
	.initialized     = 0
};

/* ================================================================
   初始化
   ================================================================ */

/**
 * @brief  初始化 MPU6050
 * @note   I2C 引脚 PB8/PB9 与 OLED 共享，已在 GPIO_InitAll() 中配置为 OD
 *         默认: 陀螺仪 ±250°/s, 加速度计 ±2g, DLPF 42Hz, 采样率 1kHz
 */
void MPU6050_Init(void)
{
	I2C_SoftInit(&mpu_i2c, I2C_SPEED_FAST);

	/* 1. 唤醒 (清除 SLEEP 位) */
	I2C_WriteReg(&mpu_i2c, MPU6050_ADDR, MPU6050_REG_PWR_MGMT_1, 0x00);
	Delay_ms(50);

	/* 2. 采样率分频 = 0 → 1kHz 内部采样 */
	I2C_WriteReg(&mpu_i2c, MPU6050_ADDR, MPU6050_REG_SMPRT_DIV, 0x00);

	/* 3. DLPF = 3 → 陀螺仪 42Hz 带宽, 4.8ms 延迟 */
	I2C_WriteReg(&mpu_i2c, MPU6050_ADDR, MPU6050_REG_CONFIG, DLPF_42HZ);

	/* 4. 陀螺仪 ±250°/s */
	I2C_WriteReg(&mpu_i2c, MPU6050_ADDR, MPU6050_REG_GYRO_CFG, GYRO_FS_250);
	mpu.gyro_sensitivity = 131.0f;

	/* 5. 加速度计 ±2g */
	I2C_WriteReg(&mpu_i2c, MPU6050_ADDR, MPU6050_REG_ACCEL_CFG, ACCEL_FS_2G);

	mpu.initialized = 1;
}

/* ================================================================
   硬件检查
   ================================================================ */

/**
 * @brief  读取 WHO_AM_I 寄存器，验证通信
 * @retval 1=MPU6050 应答正常, 0=通信失败
 */
u8 MPU6050_Check(void)
{
	u8 whoami = I2C_ReadReg(&mpu_i2c, MPU6050_ADDR, MPU6050_REG_WHO_AM_I);
	return (whoami == 0x68) ? 1 : 0;
}

/* ================================================================
   参数配置
   ================================================================ */

void MPU6050_SetDLPF(MPU6050_DLPF_t dlpf)
{
	I2C_WriteReg(&mpu_i2c, MPU6050_ADDR, MPU6050_REG_CONFIG, (u8)dlpf);
}

void MPU6050_SetGyroFS(MPU6050_GyroFS_t fs)
{
	I2C_WriteReg(&mpu_i2c, MPU6050_ADDR, MPU6050_REG_GYRO_CFG, (u8)fs);
	switch (fs) {
	case GYRO_FS_250:  mpu.gyro_sensitivity = 131.0f; break;
	case GYRO_FS_500:  mpu.gyro_sensitivity = 65.5f;  break;
	case GYRO_FS_1000: mpu.gyro_sensitivity = 32.8f;  break;
	case GYRO_FS_2000: mpu.gyro_sensitivity = 16.4f;  break;
	}
}

void MPU6050_SetAccelFS(MPU6050_AccelFS_t fs)
{
	I2C_WriteReg(&mpu_i2c, MPU6050_ADDR, MPU6050_REG_ACCEL_CFG, (u8)fs);
}

/* ================================================================
   原始数据读取
   ================================================================ */

/**
 * @brief  读取陀螺仪三轴原始值
 * @param  gx/gy/gz: 输出原始 ADC 值（有符号）
 */
void MPU6050_ReadGyroRaw(s16* gx, s16* gy, s16* gz)
{
	u8 buf[6];
	I2C_ReadBuf(&mpu_i2c, MPU6050_ADDR, MPU6050_REG_GYRO_XOUT_H, buf, 6);
	*gx = (s16)((buf[0] << 8) | buf[1]);
	*gy = (s16)((buf[2] << 8) | buf[3]);
	*gz = (s16)((buf[4] << 8) | buf[5]);
}

/**
 * @brief  读取加速度计三轴原始值
 */
void MPU6050_ReadAccelRaw(s16* ax, s16* ay, s16* az)
{
	u8 buf[6];
	I2C_ReadBuf(&mpu_i2c, MPU6050_ADDR, MPU6050_REG_ACCEL_XOUT_H, buf, 6);
	*ax = (s16)((buf[0] << 8) | buf[1]);
	*ay = (s16)((buf[2] << 8) | buf[3]);
	*az = (s16)((buf[4] << 8) | buf[5]);
}

/**
 * @brief  一次性读取全部 6 轴数据
 * @param  data: 输出物理量 (°/s, m/s²)
 */
void MPU6050_ReadAll(MPU6050_Data_t* data)
{
	u8 buf[14];
	s16 raw_ax, raw_ay, raw_az, raw_gx, raw_gy, raw_gz;

	I2C_ReadBuf(&mpu_i2c, MPU6050_ADDR, MPU6050_REG_ACCEL_XOUT_H, buf, 14);

	raw_ax = (s16)((buf[0]  << 8) | buf[1]);
	raw_ay = (s16)((buf[2]  << 8) | buf[3]);
	raw_az = (s16)((buf[4]  << 8) | buf[5]);
	/* buf[6..7] = 温度，跳过 */
	raw_gx = (s16)((buf[8]  << 8) | buf[9]);
	raw_gy = (s16)((buf[10] << 8) | buf[11]);
	raw_gz = (s16)((buf[12] << 8) | buf[13]);

	data->accel_x = raw_ax / 16384.0f * 9.81f;
	data->accel_y = raw_ay / 16384.0f * 9.81f;
	data->accel_z = raw_az / 16384.0f * 9.81f;

	data->gyro_x = raw_gx / mpu.gyro_sensitivity - 0.0f;  /* 预留 X 偏置位 */
	data->gyro_y = raw_gy / mpu.gyro_sensitivity - 0.0f;
	data->gyro_z = raw_gz / mpu.gyro_sensitivity - mpu.gyro_bias_z;
}

/* ================================================================
   快捷读取
   ================================================================ */

/**
 * @brief  只读陀螺仪 Z 轴 (°/s)，yaw 控制环用
 * @retval Z 轴角速度 (°/s)，已扣除零偏
 */
float MPU6050_ReadGyroZ(void)
{
	u8 buf[2];
	s16 raw;
	I2C_ReadBuf(&mpu_i2c, MPU6050_ADDR, MPU6050_REG_GYRO_XOUT_H + 4, buf, 2);
	raw = (s16)((buf[0] << 8) | buf[1]);
	return raw / mpu.gyro_sensitivity - mpu.gyro_bias_z;
}

/* ================================================================
   零偏校准
   ================================================================ */

/**
 * @brief  静止状态下采集 N 次陀螺仪 Z 轴，计算零偏均值
 * @param  samples: 采样次数（建议 200~500）
 * @note   调用前小车必须保持静止
 */
void MPU6050_CalibrateGyroZ(u16 samples)
{
	s32 sum = 0;
	s16 raw;

	for (u16 i = 0; i < samples; i++) {
		u8 buf[2];
		I2C_ReadBuf(&mpu_i2c, MPU6050_ADDR, MPU6050_REG_GYRO_XOUT_H + 4, buf, 2);
		raw = (s16)((buf[0] << 8) | buf[1]);
		sum += raw;
		Delay_ms(2);  /* 2ms 间隔 ≈ 500Hz 采样 */
	}

	mpu.gyro_bias_z = (float)sum / (float)samples / mpu.gyro_sensitivity;
}

/**
 * @brief  获取当前零偏值
 * @retval 零偏值
 */
float MPU6050_GetGyroZBias(void)
{
	return mpu.gyro_bias_z;
}
