#ifndef __SPI_H__
#define __SPI_H__

#include "fml_types.h"

typedef enum { SPI_MODE0 = 0, SPI_MODE1, SPI_MODE2, SPI_MODE3 } SPI_Mode_t;

void SPIx_Init(SPI_ID_t spi_id, SPI_Mode_t mode, u16 prescaler);
u8   SPI_Transfer(SPI_ID_t spi_id, u8 data);
void SPI_TransferBuf(SPI_ID_t spi_id, u8* tx, u8* rx, u16 len);
void SPI_SetCS(GPIO_Port_t port, u8 pin, u8 val);

#endif
