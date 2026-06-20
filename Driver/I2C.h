#ifndef __I2C_H__
#define __I2C_H__

#include "fml_types.h"

#define I2C_SPEED_STD  0  /* 100kHz */
#define I2C_SPEED_FAST 1  /* 400kHz */

void I2C_SoftInit(I2C_Bus_t* bus, u8 speed);
u8   I2C_Start(I2C_Bus_t* bus);
void I2C_Stop(I2C_Bus_t* bus);
u8   I2C_SendByte(I2C_Bus_t* bus, u8 data);
u8   I2C_RecvByte(I2C_Bus_t* bus, u8 ack);
void I2C_WriteReg(I2C_Bus_t* bus, u8 dev_addr, u8 reg, u8 val);
u8   I2C_ReadReg(I2C_Bus_t* bus, u8 dev_addr, u8 reg);
void I2C_ReadBuf(I2C_Bus_t* bus, u8 dev_addr, u8 reg, u8* buf, u8 len);

#endif
