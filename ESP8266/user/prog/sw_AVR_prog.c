// Bondarenko Andrey

#include <esp8266.h>
#include "driver/spi.h"
#include "esp8266_prog_platform.h"
#include "AVR_prog.h"


#define PIN_RST		PLATFORM_SOCK8_PIN1
#define PIN_CLKI	PLATFORM_SOCK8_PIN2
#define PIN_MOSI	PLATFORM_SOCK8_PIN5
#define PIN_MISO	PLATFORM_SOCK8_PIN6
#define PIN_SCK		PLATFORM_SOCK8_PIN7_PIN3
#define PIN_RST_SET( q)		PLATFORM_SOCK8_PIN1_SET( q);
#define PIN_CLKI_SET( q)	PLATFORM_SOCK8_PIN2_SET( q);
#define PIN_MOSI_SET( q)	PLATFORM_SOCK8_PIN5_SET( q);
#define PIN_MISO_GET()		PLATFORM_SOCK8_PIN6_GET()
#define PIN_SCK_SET( q)		PLATFORM_SOCK8_PIN7_PIN3_SET( q);

#define CLKI_3PULSES() \
		{ \
			PIN_CLKI_SET( 1); \
			PIN_CLKI_SET( 0); \
			PIN_CLKI_SET( 1); \
			PIN_CLKI_SET( 0); \
			PIN_CLKI_SET( 1); \
			PIN_CLKI_SET( 0); \
		}

uint32_t ICACHE_FLASH_ATTR AVR_IO32( uint32_t in)
{
	uint32_t out = 0;

	CLKI_3PULSES();

	for( int q = 0; q < 32/*( sizeof( in) * 8)*/; q++)
	{
		PIN_MOSI_SET( ((in&0x80)==0x80));

		PIN_SCK_SET( 1); CLKI_3PULSES();

		in <<= 1;
		out <<= 1;

		if( PIN_MISO_GET())
			out |= 1;

		PIN_SCK_SET( 0); CLKI_3PULSES();
	}
	return out;
}

uint8_t ICACHE_FLASH_ATTR AVR_IO8( uint8_t in)
{
	uint8_t out = 0;

	CLKI_3PULSES();

	for( int q = 0; q < 8/*( sizeof( in) * 8)*/; q++)
	{
		PIN_MOSI_SET( ((in&0x80)==0x80));

		PIN_SCK_SET( 1); CLKI_3PULSES();

		in <<= 1;
		out <<= 1;

		if( PIN_MISO_GET())
			out |= 1;

		PIN_SCK_SET( 0); CLKI_3PULSES();
	}
	return out;
}

int ICACHE_FLASH_ATTR AVR_SET_RST( int q)
{
//ToTo: if reset=1 - free all socket pins !!!
	PIN_RST_SET( q);
	return 0;
}

int ICACHE_FLASH_ATTR AVR_CLKI_PULSES( int q)
{
	for( ; q; q--)
	{
		PIN_CLKI_SET( 1);
		PIN_CLKI_SET( 0);
	}
	return 0;
}



int ICACHE_FLASH_ATTR AVR_INIT()
{
	PIN_RST_SET( 1);
	PLATFORM_SOCK8_PIN1_SET_FUNC_GPIO();
	PLATFORM_SOCK8_PIN2_SET_FUNC_GPIO();
	PLATFORM_SOCK8_PIN5_SET_FUNC_GPIO();
	PLATFORM_SOCK8_PIN6_SET_FUNC_GPIO(); PLATFORM_SOCK8_PIN6_SET_FUNC_GPIO_INPUT();
	PLATFORM_SOCK8_PIN7_PIN3_SET_FUNC_GPIO();

	return 0;
};

int ICACHE_FLASH_ATTR AVR_DEINIT()
{
	PLATFORM_SOCK8_PIN2_SET_FUNC_GPIO_INPUT();
	PLATFORM_SOCK8_PIN5_SET_FUNC_GPIO_INPUT();
	PLATFORM_SOCK8_PIN6_SET_FUNC_GPIO_INPUT();
	PLATFORM_SOCK8_PIN7_PIN3_SET_FUNC_GPIO_INPUT();

	// NOT free RESET PIN - because MCU start on socket
	//PLATFORM_SOCK8_PIN1_SET_FUNC_GPIO_INPUT();

	return 0;
};
