#ifndef __IMU660RA_H__
#define __IMU660RA_H__

#include "../Driver/fml_types.h"

#define IMU660RA_ADDR       0x69   /* SA0上拉: 0x69, SA0接地: 0x68 */

/* 寄存器映射 */
#define IMU660RA_CHIP_ID    0x00
#define IMU660RA_PWR_CONF   0x7C
#define IMU660RA_PWR_CTRL   0x7D
#define IMU660RA_INIT_CTRL  0x59
#define IMU660RA_INIT_DATA  0x5E
#define IMU660RA_INT_STA    0x21
#define IMU660RA_ACC_ADDR   0x0C
#define IMU660RA_GYRO_ADDR  0x12
#define IMU660RA_ACC_CONF   0x40
#define IMU660RA_ACC_RANGE  0x41
#define IMU660RA_GYR_CONF   0x42
#define IMU660RA_GYR_RANGE  0x43

/* 默认量程: 陀螺±2000dps(÷16.4), 加计±8g(÷4096) */
#define IMU660RA_ACC_FS     0x02
#define IMU660RA_GYR_FS     0x00

void  IMU660RA_Init(void);
void  IMU660RA_Update(void);
float IMU660RA_ReadGyroZ(void);
float IMU660RA_ReadGyroX(void);
float IMU660RA_ReadGyroY(void);
float IMU660RA_ReadAccelX(void);
float IMU660RA_ReadAccelY(void);
float IMU660RA_ReadAccelZ(void);
u8    IMU660RA_IsValid(void);
void  IMU660RA_CalibrateGyroZ(u16 samples);

#endif
