#include "SPI.h"
#include "GPIO.h"

static SPI_TypeDef* const spi_map[] = { SPI1, SPI2 };

/**
 * @brief  初始化SPI（全双工主机模式）
 * @param  spi_id: SPI_ID_t — SPI标识
 * @param  mode: SPI_Mode_t — SPI模式（CPOL/CPHA）
 * @param  prescaler: u16 — 波特率预分频，传入ST库常数如 SPI_BaudRatePrescaler_128
 * @retval 无
 */
void SPIx_Init(SPI_ID_t spi_id, SPI_Mode_t mode, u16 prescaler)
{
	SPI_TypeDef* spi = spi_map[spi_id];

	if (spi_id == SPI_ID_1) {
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
	} else {
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
	}

	SPI_InitTypeDef cfg;
	cfg.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	cfg.SPI_Mode = SPI_Mode_Master;
	cfg.SPI_DataSize = SPI_DataSize_8b;
	cfg.SPI_CPOL = (mode == SPI_MODE2 || mode == SPI_MODE3) ? SPI_CPOL_High : SPI_CPOL_Low;
	cfg.SPI_CPHA = (mode == SPI_MODE1 || mode == SPI_MODE3) ? SPI_CPHA_2Edge : SPI_CPHA_1Edge;
	cfg.SPI_NSS = SPI_NSS_Soft;
	cfg.SPI_BaudRatePrescaler = prescaler;
	cfg.SPI_FirstBit = SPI_FirstBit_MSB;
	cfg.SPI_CRCPolynomial = 7;
	SPI_Init(spi, &cfg);
	SPI_Cmd(spi, ENABLE);
}

/**
 * @brief  SPI单字节收发
 * @param  spi_id: SPI_ID_t — SPI标识
 * @param  data: u8 — 发送字节
 * @retval u8 — 接收字节
 */
u8 SPI_Transfer(SPI_ID_t spi_id, u8 data)
{
	SPI_TypeDef* spi = spi_map[spi_id];
	while (!SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_TXE));
	SPI_I2S_SendData(spi, data);
	while (!SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_RXNE));
	return (u8)SPI_I2S_ReceiveData(spi);
}

/**
 * @brief  SPI批量收发
 * @param  spi_id: SPI_ID_t — SPI标识
 * @param  tx: u8* — 发送缓冲区，NULL=发送0xFF
 * @param  rx: u8* — 接收缓冲区，NULL=丢弃
 * @param  len: u16 — 数据长度
 * @retval 无
 */
void SPI_TransferBuf(SPI_ID_t spi_id, u8* tx, u8* rx, u16 len)
{
	for (u16 i = 0; i < len; i++) {
		u8 d = SPI_Transfer(spi_id, tx ? tx[i] : 0xFF);
		if (rx) rx[i] = d;
	}
}

/**
 * @brief  软件CS片选控制
 * @param  port: GPIO_Port_t — CS引脚端口
 * @param  pin: u8 — CS引脚号
 * @param  val: u8 — 0=拉低选中, 1=拉高释放
 * @retval 无
 */
void SPI_SetCS(GPIO_Port_t port, u8 pin, u8 val)
{
	GPIO_WritePin(port, pin, val);
}
