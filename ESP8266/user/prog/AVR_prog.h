// Bondarenko Andrey

#ifndef __AVR_PROG_H__
#define __AVR_PROG_H__


#include <esp8266.h>
#include "esp8266_prog_platform.h"
#include "esp8266_prog.h"

int ICACHE_FLASH_ATTR AVR_INIT();
int ICACHE_FLASH_ATTR AVR_DEINIT();
uint8_t ICACHE_FLASH_ATTR AVR_IO8( uint8_t in);
uint32_t ICACHE_FLASH_ATTR AVR_IO32( uint32_t in);
int ICACHE_FLASH_ATTR AVR_SET_RST( int q);
int ICACHE_FLASH_ATTR AVR_CLKI_PULSES( int q);

#endif
