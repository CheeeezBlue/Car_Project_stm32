#include "IMU660RA.h"
#include "../Driver/I2C.h"
#include "../System/Delay.h"

/* 软件 I2C 总线: 与 OLED 共享 PB8(SCL)/PB9(SDA) */
static I2C_Bus_t imu_i2c = { GPIOB, 8, GPIOB, 9 };

static float gyro_x = 0.0f, gyro_y = 0.0f, gyro_z = 0.0f;
static float accel_x = 0.0f, accel_y = 0.0f, accel_z = 0.0f;
static float gyro_bias_z = 0.0f;
static u8    valid = 0;

/* 逐飞官方固件配置文件 (8192 bytes) */
static const u8 imu660ra_config_file[8192] = {
#include "imu660ra_config.inc"
};

static void imu_write_reg(u8 reg, u8 data)
{
	I2C_WriteReg(&imu_i2c, IMU660RA_ADDR, reg, data);
}

static u8 imu_read_reg(u8 reg)
{
	return I2C_ReadReg(&imu_i2c, IMU660RA_ADDR, reg);
}

static void imu_read_regs(u8 reg, u8* buf, u8 len)
{
	I2C_ReadBuf(&imu_i2c, IMU660RA_ADDR, reg, buf, len);
}

static void imu_write_regs(u8 reg, const u8* data, u32 len)
{
	/* I2C 无硬件连续写，逐字节发送 */
	I2C_Start(&imu_i2c);
	I2C_SendByte(&imu_i2c, IMU660RA_ADDR << 1);
	I2C_SendByte(&imu_i2c, reg);
	while (len--) I2C_SendByte(&imu_i2c, *data++);
	I2C_Stop(&imu_i2c);
}

static u8 imu_self_check(void)
{
	u8 dat = 0;
	u16 timeout = 0;
	do {
		if (++timeout > 0x00FF) return 1;
		dat = imu_read_reg(IMU660RA_CHIP_ID);
		Delay_ms(1);
	} while (dat != 0x24);
	return 0;
}

void IMU660RA_Init(void)
{
	I2C_SoftInit(&imu_i2c, I2C_SPEED_FAST);
	gyro_x = 0.0f; gyro_y = 0.0f; gyro_z = 0.0f;
	accel_x = 0.0f; accel_y = 0.0f; accel_z = 0.0f;
	gyro_bias_z = 0.0f;
	valid = 0;

	Delay_ms(20);

	if (imu_self_check()) return;

	imu_write_reg(IMU660RA_PWR_CONF, 0x00);       /* 关闭高级省电模式 */
	Delay_ms(10);
	imu_write_reg(IMU660RA_INIT_CTRL, 0x00);       /* 开始初始化配置 */
	imu_write_regs(IMU660RA_INIT_DATA, imu660ra_config_file, sizeof(imu660ra_config_file));
	imu_write_reg(IMU660RA_INIT_CTRL, 0x01);       /* 初始化配置结束 */
	Delay_ms(20);

	{
		u8 sta = imu_read_reg(IMU660RA_INT_STA);
		if (sta != 1) {
			/* 配置未完成, 但继续配置传感器 (兼容某些情况) */
		}
	}

	imu_write_reg(IMU660RA_PWR_CTRL, 0x0E);        /* 性能模式, 使能陀螺+加计+温度 */
	Delay_ms(10);
	imu_write_reg(IMU660RA_ACC_CONF, 0xA7);         /* 50Hz 采样 */
	imu_write_reg(IMU660RA_GYR_CONF, 0xA9);         /* 200Hz 采样 */
	imu_write_reg(IMU660RA_ACC_RANGE, IMU660RA_ACC_FS);
	imu_write_reg(IMU660RA_GYR_RANGE, IMU660RA_GYR_FS);
}

void IMU660RA_Update(void)
{
	u8 buf[6];
	s16 raw;

	/* 读陀螺仪 */
	imu_read_regs(IMU660RA_GYRO_ADDR, buf, 6);
	raw = (s16)((u16)buf[1] << 8 | buf[0]);
	gyro_x = raw / 16.4f;
	raw = (s16)((u16)buf[3] << 8 | buf[2]);
	gyro_y = raw / 16.4f;
	raw = (s16)((u16)buf[5] << 8 | buf[4]);
	gyro_z = raw / 16.4f - gyro_bias_z;

	/* 读加速度计 */
	imu_read_regs(IMU660RA_ACC_ADDR, buf, 6);
	raw = (s16)((u16)buf[1] << 8 | buf[0]);
	accel_x = raw / 4096.0f * 9.81f;
	raw = (s16)((u16)buf[3] << 8 | buf[2]);
	accel_y = raw / 4096.0f * 9.81f;
	raw = (s16)((u16)buf[5] << 8 | buf[4]);
	accel_z = raw / 4096.0f * 9.81f;

	valid = 1;
}

void IMU660RA_CalibrateGyroZ(u16 samples)
{
	s32 sum = 0;
	s16 raw;
	u8 buf[2];

	for (u16 i = 0; i < samples; i++) {
		imu_read_regs(IMU660RA_GYRO_ADDR + 4, buf, 2);
		raw = (s16)((u16)buf[1] << 8 | buf[0]);
		sum += raw;
		Delay_ms(2);
	}
	gyro_bias_z = (float)sum / (float)samples / 16.4f;
}

float IMU660RA_ReadGyroZ(void)  { return gyro_z; }
float IMU660RA_ReadGyroX(void)  { return gyro_x; }
float IMU660RA_ReadGyroY(void)  { return gyro_y; }
float IMU660RA_ReadAccelX(void) { return accel_x; }
float IMU660RA_ReadAccelY(void) { return accel_y; }
float IMU660RA_ReadAccelZ(void) { return accel_z; }
u8    IMU660RA_IsValid(void)    { return valid; }
