#ifndef __MPU6050_H__
#define __MPU6050_H__

#include "../Driver/fml_types.h"

/* MPU6050 7位 I2C 地址 (AD0=0) */
#define MPU6050_ADDR        0x68

/* ---- 寄存器映射 ---- */
#define MPU6050_REG_WHO_AM_I   0x75
#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_REG_SMPRT_DIV  0x19
#define MPU6050_REG_CONFIG     0x1A
#define MPU6050_REG_GYRO_CFG   0x1B
#define MPU6050_REG_ACCEL_CFG  0x1C

/* 数据寄存器（从高字节开始，连续读 6 字节） */
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_GYRO_XOUT_H  0x43

/* ---- 量程枚举 ---- */
typedef enum {
	GYRO_FS_250  = 0x00,   /* ±250°/s,  灵敏度 131  LSB/°/s */
	GYRO_FS_500  = 0x08,   /* ±500°/s,  灵敏度 65.5 LSB/°/s */
	GYRO_FS_1000 = 0x10,   /* ±1000°/s, 灵敏度 32.8 LSB/°/s */
	GYRO_FS_2000 = 0x18    /* ±2000°/s, 灵敏度 16.4 LSB/°/s */
} MPU6050_GyroFS_t;

typedef enum {
	ACCEL_FS_2G  = 0x00,   /* ±2g,  灵敏度 16384 LSB/g */
	ACCEL_FS_4G  = 0x08,   /* ±4g,  灵敏度 8192  LSB/g */
	ACCEL_FS_8G  = 0x10,   /* ±8g,  灵敏度 4096  LSB/g */
	ACCEL_FS_16G = 0x18    /* ±16g, 灵敏度 2048  LSB/g */
} MPU6050_AccelFS_t;

/* 低通滤波 DLPF_CFG: 0~6，值越大带宽越低、延迟越大 */
typedef enum {
	DLPF_256HZ = 0,        /* Gyro BW 256Hz, 延迟 0.98ms */
	DLPF_188HZ = 1,        /* Gyro BW 188Hz, 延迟 1.9ms  */
	DLPF_98HZ  = 2,        /* Gyro BW 98Hz,  延迟 2.8ms  */
	DLPF_42HZ  = 3,        /* Gyro BW 42Hz,  延迟 4.8ms  */
	DLPF_20HZ  = 4,        /* Gyro BW 20Hz,  延迟 8.3ms  */
	DLPF_10HZ  = 5,        /* Gyro BW 10Hz,  延迟 13.4ms */
	DLPF_5HZ   = 6         /* Gyro BW 5Hz,   延迟 18.6ms */
} MPU6050_DLPF_t;

/* ---- 数据结构 ---- */
typedef struct {
	float gyro_x;   /* °/s */
	float gyro_y;   /* °/s */
	float gyro_z;   /* °/s — yaw 环用 */
	float accel_x;  /* m/s² */
	float accel_y;  /* m/s² */
	float accel_z;  /* m/s² */
} MPU6050_Data_t;

typedef struct {
	float gyro_bias_z;    /* 陀螺仪 Z 轴零偏 (°/s) */
	float gyro_sensitivity; /* 当前量程灵敏度 (LSB/°/s) */
	u8    initialized;
} MPU6050_t;

/* ---- API ---- */
void MPU6050_Init(void);
u8   MPU6050_Check(void);                     /* 读取 WHO_AM_I，返回 1=成功 */
void MPU6050_SetDLPF(MPU6050_DLPF_t dlpf);
void MPU6050_SetGyroFS(MPU6050_GyroFS_t fs);
void MPU6050_SetAccelFS(MPU6050_AccelFS_t fs);

void MPU6050_ReadGyroRaw(s16* gx, s16* gy, s16* gz);
void MPU6050_ReadAccelRaw(s16* ax, s16* ay, s16* az);
void MPU6050_ReadAll(MPU6050_Data_t* data);    /* 一次 I2C 突发读 14 字节 */

float MPU6050_ReadGyroZ(void);                 /* 快捷：只读 Z 轴 (°/s) */

void MPU6050_CalibrateGyroZ(u16 samples);      /* 静止状态下采集 N 次求均值 */
float MPU6050_GetGyroZBias(void);

#endif
