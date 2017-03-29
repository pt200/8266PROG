#ifndef SPI_APP_H
#define SPI_APP_H

#include "spi_register.h"
#include "ets_sys.h"
#include "osapi.h"
//#include "uart.h"
#include "os_type.h"
#include "gpio.h"


/*SPI number define*/
#define SPI 			0
#define HSPI			1



//spi master init funtion
void ICACHE_FLASH_ATTR spi_master_init(uint8 spi_no, unsigned cpol, unsigned cpha, unsigned databits, uint32_t clock);
//spi master deinit funtion
void ICACHE_FLASH_ATTR spi_master_deinit(uint8 spi_no);
//use spi send 8bit data
void spi_mast_byte_write(uint8 spi_no,uint8 *data);
//use spi send 8bit data
uint8 HSPI_byte_io(uint8 data);


#endif

