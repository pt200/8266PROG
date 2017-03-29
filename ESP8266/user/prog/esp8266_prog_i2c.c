// Bondarenko Andrey

#include <esp8266.h>
#include "driver/spi.h"
#include "esp8266_prog_platform.h"
#include "esp8266_prog.h"
#include "esp8266_prog_i2c.h"


int ICACHE_FLASH_ATTR _PROG_I2C_ConnRecvCb(char *data, unsigned short len);
//int ICACHE_FLASH_ATTR _PROG_I2C_ConnSentDoneCb();
int ICACHE_FLASH_ATTR _PROG_I2C_DeInit();
int ICACHE_FLASH_ATTR _PROG_I2C_Init();

ESP8266_PROG_Instance PROG_I2C = {
		MAKE_PROG_ID('I', '2', 'C', ' '),
		_PROG_I2C_Init,
		_PROG_I2C_DeInit,
		_PROG_I2C_ConnRecvCb,
		NULL /*_PROG_I2C_ConnSentDoneCb*/
		};



#define min(a,b) ((a)<(b)?(a):(b))

#define _MAGIC_CMD_I2C_IO		0x12345678
#define _MAGIC_CMD_I2C_START	0x12345668
#define _MAGIC_CMD_I2C_STOP		0x12345669
#define _MAGIC_CMD_I2C_RESET	0x12345667
#define _MAGIC_CMD_NONE         0


#define SDA_PIN PLATFORM_SOCK8_PIN5
#define SCL_PIN PLATFORM_SOCK8_PIN6

void ICACHE_FLASH_ATTR SOCK8_I2C_INIT()
{
	PLATFORM_SOCK8_PIN1_SET_FUNC_GPIO();
	PLATFORM_SOCK8_PIN2_SET_FUNC_GPIO();
	PLATFORM_SOCK8_PIN3_PIN7_SET_FUNC_GPIO();
	PLATFORM_SOCK8_PIN5_SET_FUNC_GPIO();
	PLATFORM_SOCK8_PIN6_SET_FUNC_GPIO();
	PLATFORM_SOCK8_PIN1_SET( 0);
	PLATFORM_SOCK8_PIN2_SET( 0);
	PLATFORM_SOCK8_PIN3_PIN7_SET( 0);
	PLATFORM_SOCK8_PIN5_SET( 0);  // SDA
	PLATFORM_SOCK8_PIN6_SET( 0);  // SCL
}
void ICACHE_FLASH_ATTR SOCK8_I2C_DEINIT()
{
//	GPIO_DIS_OUTPUT(5);
}

void ICACHE_FLASH_ATTR _I2C_WAIT()
{
	os_delay_us( 5);
}
void ICACHE_FLASH_ATTR _I2C_SCL_0()
{
	_I2C_WAIT();
	GPIO_OUTPUT_SET( SCL_PIN, 0);
}
void ICACHE_FLASH_ATTR _I2C_SCL_1()
{
	_I2C_WAIT();
	GPIO_DIS_OUTPUT( SCL_PIN);
}
void ICACHE_FLASH_ATTR _I2C_SDA_0()
{
	_I2C_WAIT();
	GPIO_OUTPUT_SET( SDA_PIN, 0);
}
void ICACHE_FLASH_ATTR _I2C_SDA_1()
{
	_I2C_WAIT();
	GPIO_DIS_OUTPUT( SDA_PIN);
}
int ICACHE_FLASH_ATTR _I2C_GET_SDA()
{
	return GPIO_INPUT_GET( SDA_PIN);
}



// ToDo : buffer ???
#define DATA_BUF_SIZE  16384
static uint8_t* data_buf = NULL;


int ICACHE_FLASH_ATTR _PROG_I2C_START() {
	_I2C_SDA_1();
	_I2C_SCL_1();
	_I2C_SDA_0();
	_I2C_SCL_0();
	return 1;
}
int ICACHE_FLASH_ATTR _PROG_I2C_STOP() {
	_I2C_SCL_0();
	_I2C_SDA_0();
	_I2C_SCL_1();
	_I2C_SDA_1();
	return 1;
}
void ICACHE_FLASH_ATTR _PROG_I2C_IO(uint8_t *data, uint32_t len) {
	uint8_t tmp;
	for( uint32_t addr = 0; addr < len; addr += 2)
	{
		tmp = data[ addr];
		for( uint32_t q = 0; q < 8; q++)
		{
			if( tmp & 0x80)
				_I2C_SDA_1();
			else
				_I2C_SDA_0();

			tmp <<= 1;

			_I2C_SCL_1();

			if(_I2C_GET_SDA())
				tmp |= 1;

			_I2C_SCL_0();
		}
		data[ addr] = tmp;

		// SET/GET ACK
		if( data[ addr+1])
			_I2C_SDA_1();
		else
			_I2C_SDA_0();

		_I2C_SCL_1();
		data[ addr+1] = _I2C_GET_SDA();
		_I2C_SCL_0();
	}
}
int ICACHE_FLASH_ATTR _PROG_I2C_RESET() {
	uint8_t tmp = 0xFF;
	_PROG_I2C_STOP();
	_PROG_I2C_IO( &tmp, 1);
	return 1;
}

int ICACHE_FLASH_ATTR _PROG_I2C_getCMDlenght(PROG_CMD* cmd,	unsigned short len) {
	if( cmd == NULL)
		return 0;

	if(len < sizeof(PROG_CMD))
		return 0;

	if(len < (sizeof(PROG_CMD) + cmd->wr_len))
		return 0;

	return (sizeof(PROG_CMD) + cmd->wr_len);
}

int ICACHE_FLASH_ATTR _PROG_I2C_ExecCMD(PROG_CMD* cmd,	unsigned short len) {
	if( cmd == NULL)
		return -1;

	if(len < sizeof(PROG_CMD))
		return -1;

//httpd_printf("I2C_cmd %d %d %d %d\r\n", len, cmd->magic_cmd, cmd->wr_len, cmd->rd_len);

	switch( cmd->magic_cmd)
	{
		case _MAGIC_CMD_I2C_START:
			data_buf[ 1] = _PROG_I2C_START();
			break;

		case _MAGIC_CMD_I2C_STOP:
			data_buf[ 1] = _PROG_I2C_STOP();
			break;

		case _MAGIC_CMD_I2C_IO:
			//ToDo: support fragmentation !!! before "exception"
			//ToDo : NOT assert
			ASSERT(len < (sizeof(PROG_CMD) + cmd->wr_len));
memcpy( data_buf, cmd->data, cmd->wr_len);
			_PROG_I2C_IO( data_buf, cmd->wr_len);
			break;

		case _MAGIC_CMD_I2C_RESET:
			data_buf[ 1] = _PROG_I2C_RESET();
			break;

//		default:
			//Err cmd. DO NOTHING
	}
	if( cmd->rd_len > 0)
		progSendData(data_buf, cmd->rd_len);

	return 0;
}


int ICACHE_FLASH_ATTR _PROG_I2C_ConnRecvCb(char *data,	unsigned short len) {
	unsigned short cmd_size;

	while( ( cmd_size = _PROG_I2C_getCMDlenght( ( PROG_CMD*)data, len)) > 0)
	{
		_PROG_I2C_ExecCMD( ( PROG_CMD*)data, cmd_size);

		data += cmd_size;
		len -= cmd_size;
	}


	return 0;
}

/*
int ICACHE_FLASH_ATTR _PROG_I2C_ConnSentDoneCb() {
	return 0;
}
*/
int ICACHE_FLASH_ATTR _PROG_I2C_DeInit( void) {
	ASSERT( data_buf == NULL);
	free( data_buf);
	data_buf = NULL;

	//I2C deinit funtion
	SOCK8_I2C_DEINIT();
	return 0;
}

//Initialize listening socket, do general initialization
int ICACHE_FLASH_ATTR _PROG_I2C_Init() {
httpd_printf( "conn I2C\n");

	//Alloc memory buf
	ASSERT( data_buf != NULL);
	data_buf = malloc( DATA_BUF_SIZE);
	if( data_buf == NULL)
		return -1;

	//Send Hi
	char* msg = "I2C_PROG_v1";
	progSendData( (uint8_t*) msg, 11);

	//I2C init funtion
	SOCK8_I2C_INIT();

	return 0;
}
