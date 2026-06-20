#include "I2C.h"
#include "GPIO.h"

static void i2c_delay(u8 speed)
{
	volatile u32 i = (speed == I2C_SPEED_FAST) ? 3 : 10;
	while (i--) __NOP();
}

#define SCL_PORT(b) ((b)->scl_port == GPIOA ? GPIO_PA : (b)->scl_port == GPIOB ? GPIO_PB : GPIO_PC)
#define SDA_PORT(b) ((b)->sda_port == GPIOA ? GPIO_PA : (b)->sda_port == GPIOB ? GPIO_PB : GPIO_PC)

#define SCL_H(b) GPIO_WritePin(SCL_PORT(b), (b)->scl_pin, 1)
#define SCL_L(b) GPIO_WritePin(SCL_PORT(b), (b)->scl_pin, 0)
#define SDA_H(b) GPIO_WritePin(SDA_PORT(b), (b)->sda_pin, 1)
#define SDA_L(b) GPIO_WritePin(SDA_PORT(b), (b)->sda_pin, 0)
#define SDA_IN(b) GPIO_ReadPin(SDA_PORT(b), (b)->sda_pin)

/**
 * @brief  初始化软件I2C总线
 * @param  bus: I2C_Bus_t* — 总线配置结构体指针
 * @param  speed: u8 — I2C_SPEED_STD(100kHz) 或 I2C_SPEED_FAST(400kHz)
 * @retval 无
 */
void I2C_SoftInit(I2C_Bus_t* bus, u8 speed)
{
	(void)speed;
	GPIO_InitPin(SCL_PORT(bus), bus->scl_pin, GPIO_OUT_OD);
	GPIO_InitPin(SDA_PORT(bus), bus->sda_pin, GPIO_OUT_OD);
	SCL_H(bus); SDA_H(bus);
}

/**
 * @brief  发送起始信号
 * @param  bus: I2C_Bus_t* — 总线指针
 * @retval u8 — 保留（兼容硬件I2C）
 */
u8 I2C_Start(I2C_Bus_t* bus)
{
	SDA_H(bus); SCL_H(bus); i2c_delay(1);
	SDA_L(bus); i2c_delay(1);
	SCL_L(bus);
	return 0;
}

/**
 * @brief  发送停止信号
 * @param  bus: I2C_Bus_t* — 总线指针
 * @retval 无
 */
void I2C_Stop(I2C_Bus_t* bus)
{
	SDA_L(bus); SCL_H(bus); i2c_delay(1);
	SDA_H(bus); i2c_delay(1);
}

/**
 * @brief  发送一个字节，返回从机ACK
 * @param  bus: I2C_Bus_t* — 总线指针
 * @param  data: u8 — 待发送字节
 * @retval u8 — 0=ACK, 1=NACK
 */
u8 I2C_SendByte(I2C_Bus_t* bus, u8 data)
{
	for (u8 i = 0; i < 8; i++) {
		if (data & 0x80) SDA_H(bus); else SDA_L(bus);
		data <<= 1;
		SCL_H(bus); i2c_delay(1);
		SCL_L(bus);
	}
	SDA_H(bus); SCL_H(bus); i2c_delay(1);
	u8 ack = SDA_IN(bus);
	SCL_L(bus);
	return ack;
}

/**
 * @brief  接收一个字节
 * @param  bus: I2C_Bus_t* — 总线指针
 * @param  ack: u8 — 0=发送ACK, 1=发送NACK
 * @retval u8 — 接收到的字节
 */
u8 I2C_RecvByte(I2C_Bus_t* bus, u8 ack)
{
	u8 data = 0;
	SDA_H(bus);
	for (u8 i = 0; i < 8; i++) {
		SCL_H(bus); i2c_delay(1);
		data = (data << 1) | (SDA_IN(bus) ? 1 : 0);
		SCL_L(bus);
	}
	if (ack) SDA_H(bus); else SDA_L(bus);
	SCL_H(bus); i2c_delay(1);
	SCL_L(bus);
	return data;
}

/**
 * @brief  向设备寄存器写入单字节
 * @param  bus: I2C_Bus_t* — 总线指针
 * @param  dev_addr: u8 — 7位设备地址（不含R/W位）
 * @param  reg: u8 — 寄存器地址
 * @param  val: u8 — 写入值
 * @retval 无
 */
void I2C_WriteReg(I2C_Bus_t* bus, u8 dev_addr, u8 reg, u8 val)
{
	I2C_Start(bus);
	I2C_SendByte(bus, dev_addr << 1);
	I2C_SendByte(bus, reg);
	I2C_SendByte(bus, val);
	I2C_Stop(bus);
}

/**
 * @brief  从设备寄存器读取单字节
 * @param  bus: I2C_Bus_t* — 总线指针
 * @param  dev_addr: u8 — 7位设备地址
 * @param  reg: u8 — 寄存器地址
 * @retval u8 — 读取值
 */
u8 I2C_ReadReg(I2C_Bus_t* bus, u8 dev_addr, u8 reg)
{
	u8 val;
	I2C_Start(bus);
	I2C_SendByte(bus, dev_addr << 1);
	I2C_SendByte(bus, reg);
	I2C_Start(bus);
	I2C_SendByte(bus, (dev_addr << 1) | 1);
	val = I2C_RecvByte(bus, 1);
	I2C_Stop(bus);
	return val;
}

/**
 * @brief  从设备连续读取多字节
 * @param  bus: I2C_Bus_t* — 总线指针
 * @param  dev_addr: u8 — 7位设备地址
 * @param  reg: u8 — 起始寄存器地址
 * @param  buf: u8* — 接收缓冲区
 * @param  len: u8 — 读取长度
 * @retval 无
 */
void I2C_ReadBuf(I2C_Bus_t* bus, u8 dev_addr, u8 reg, u8* buf, u8 len)
{
	I2C_Start(bus);
	I2C_SendByte(bus, dev_addr << 1);
	I2C_SendByte(bus, reg);
	I2C_Start(bus);
	I2C_SendByte(bus, (dev_addr << 1) | 1);
	for (u8 i = 0; i < len; i++)
		buf[i] = I2C_RecvByte(bus, i == len - 1);
	I2C_Stop(bus);
}
