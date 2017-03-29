// Bondarenko Andrey

#ifndef __ESP8266_PROG_PLATFORM_H__
#define __ESP8266_PROG_PLATFORM_H__



#define PLATFORM_SOCK8_PIN1			4
#define PLATFORM_SOCK8_PIN2			12
#define PLATFORM_SOCK8_PIN3_PIN7	5
//#define PLATFORM_SOCK8_PIN4 ?GND pin NOT config?
#define PLATFORM_SOCK8_PIN5			13
#define PLATFORM_SOCK8_PIN6			14
#define PLATFORM_SOCK8_PIN7_PIN3	5
//#define PLATFORM_SOCK8_PIN8 ?+3V3 pin NOT config?

#define PLATFORM_PIN_LED 2
#define PLATFORM_PIN_BTN 0


#define PLATFORM_SOCK8_PIN1_SET_FUNC_GPIO()			{ PIN_FUNC_SELECT( PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);}
#define PLATFORM_SOCK8_PIN2_SET_FUNC_GPIO()			{ PIN_FUNC_SELECT( PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);}
#define PLATFORM_SOCK8_PIN3_PIN7_SET_FUNC_GPIO()	{ PIN_FUNC_SELECT( PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);}
#define PLATFORM_SOCK8_PIN5_SET_FUNC_GPIO()			{ PIN_FUNC_SELECT( PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);}
#define PLATFORM_SOCK8_PIN6_SET_FUNC_GPIO()			{ PIN_FUNC_SELECT( PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);}
#define PLATFORM_SOCK8_PIN7_PIN3_SET_FUNC_GPIO()	{ PLATFORM_SOCK8_PIN3_PIN7_SET_FUNC_GPIO();}

#define PLATFORM_SOCK8_PIN1_SET_FUNC_GPIO_INPUT()		{ GPIO_DIS_OUTPUT( PLATFORM_SOCK8_PIN1);}
#define PLATFORM_SOCK8_PIN2_SET_FUNC_GPIO_INPUT()		{ GPIO_DIS_OUTPUT( PLATFORM_SOCK8_PIN2);}
#define PLATFORM_SOCK8_PIN3_PIN7_SET_FUNC_GPIO_INPUT()	{ GPIO_DIS_OUTPUT( PLATFORM_SOCK8_PIN3_PIN7);}
#define PLATFORM_SOCK8_PIN5_SET_FUNC_GPIO_INPUT()		{ GPIO_DIS_OUTPUT( PLATFORM_SOCK8_PIN5);}
#define PLATFORM_SOCK8_PIN6_SET_FUNC_GPIO_INPUT()		{ GPIO_DIS_OUTPUT( PLATFORM_SOCK8_PIN6);}
#define PLATFORM_SOCK8_PIN7_PIN3_SET_FUNC_GPIO_INPUT()	{ PLATFORM_SOCK8_PIN3_PIN7_SET_FUNC_GPIO_INPUT();}

#define PLATFORM_SOCK8_PIN1_SET( q)			{ GPIO_OUTPUT_SET( PLATFORM_SOCK8_PIN1, q);}
#define PLATFORM_SOCK8_PIN2_SET( q)			{ GPIO_OUTPUT_SET( PLATFORM_SOCK8_PIN2, q);}
#define PLATFORM_SOCK8_PIN3_PIN7_SET( q)	{ GPIO_OUTPUT_SET( PLATFORM_SOCK8_PIN3_PIN7, q);}
#define PLATFORM_SOCK8_PIN5_SET( q)			{ GPIO_OUTPUT_SET( PLATFORM_SOCK8_PIN5, q);}
#define PLATFORM_SOCK8_PIN6_SET( q)			{ GPIO_OUTPUT_SET( PLATFORM_SOCK8_PIN6, q);}
#define PLATFORM_SOCK8_PIN7_PIN3_SET( q)	{ PLATFORM_SOCK8_PIN3_PIN7_SET( q);}

#define PLATFORM_SOCK8_PIN1_GET()		( GPIO_INPUT_GET( PLATFORM_SOCK8_PIN1))
#define PLATFORM_SOCK8_PIN2_GET()		( GPIO_INPUT_GET( PLATFORM_SOCK8_PIN2))
#define PLATFORM_SOCK8_PIN3_PIN7_GET()	( GPIO_INPUT_GET( PLATFORM_SOCK8_PIN3_PIN_7))
#define PLATFORM_SOCK8_PIN5_GET()		( GPIO_INPUT_GET( PLATFORM_SOCK8_PIN5))
#define PLATFORM_SOCK8_PIN6_GET()		( GPIO_INPUT_GET( PLATFORM_SOCK8_PIN6))
#define PLATFORM_SOCK8_PIN7_PIN3_GET()	( PLATFORM_SOCK8_PIN3_PIN7_GET())

#endif
