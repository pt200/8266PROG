#ifndef __SPI_MASTER_H__
#define __SPI_MASTER_H__

#include "driver/spi_register.h"

/*SPI number define*/
#define SPI         0
#define HSPI        1

void spi_master_init(uint8 spi_no);

#endif
