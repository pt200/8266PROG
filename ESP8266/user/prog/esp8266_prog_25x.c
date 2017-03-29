// Bondarenko Andrey

#include <esp8266.h>
#include "driver/spi.h"
#include "esp8266_prog_platform.h"
#include "esp8266_prog.h"


int ICACHE_FLASH_ATTR _PROG_25x_ConnRecvCb(char *data, unsigned short len);
int ICACHE_FLASH_ATTR _PROG_25x_ConnSentDoneCb();
int ICACHE_FLASH_ATTR _PROG_25x_DeInit();
int ICACHE_FLASH_ATTR _PROG_25x_Init();

ESP8266_PROG_Instance PROG_25x = {
		MAKE_PROG_ID('2', '5', 'x', ' '),
		_PROG_25x_Init,
		_PROG_25x_DeInit,
		_PROG_25x_ConnRecvCb,
		_PROG_25x_ConnSentDoneCb
		};



#define min(a,b) ((a)<(b)?(a):(b))

#define _MAGIC_CMD_IO  			0x12345678
#define _MAGIC_CMD_PAGE_WRITE	0x12345668
#define _MAGIC_CMD_SERIAL_WRITE 0x12345679
#define _MAGIC_CMD_NONE         0

//#define _PROG_CS_PIN		PLATFORM_SOCK8_PIN1
//#define _PROG_CS_PIN_FUNK_GPIO	PLATFORM_SOCK8_PIN1_FUNC_GPIO
#define HSPI_SET_CS( q)		GPIO_OUTPUT_SET(4, q)
#define HSPI_CS_INIT() \
	{ \
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4); \
		HSPI_SET_CS(1); \
	}
#define HSPI_CS_DEINIT()	GPIO_DIS_OUTPUT(4);

#define SOCK8_INIT() \
	{ \
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5); \
		GPIO_OUTPUT_SET(5, 1); \
	}
#define SOCK8_DEINIT()		GPIO_DIS_OUTPUT(5);


PROG_CMD cmd;


#define DATA_BUF_SIZE  16384
static uint8_t* data_buf = NULL;


#define PAGE_SIZE    256
#define STATUS1_WEL  0x02
#define STATUS1_BUSY 0x01

int ICACHE_FLASH_ATTR _PROG_25x_write_page(uint8_t *data, uint32_t len) {
	uint8_t tmp;

	if( len < ( 4 + 1))
		return 1;

	uint32_t addr = *( ( uint32_t*)data);

	if( len > ( 4 + PAGE_SIZE - (addr&0xFF))) // ? Trim length
		len = ( 4 + PAGE_SIZE - (addr&0xFF));

    // Wait chip free
	do
	{
		HSPI_SET_CS(0);
		HSPI_byte_io( 0x05);
		tmp = HSPI_byte_io( 0xFF);
		HSPI_SET_CS(1);
	}while( tmp & STATUS1_BUSY);

	// CMD Write ENABLE
	HSPI_SET_CS(0);
    HSPI_byte_io( 0x06);
	HSPI_SET_CS(1);

	// Check WEL bit
	HSPI_SET_CS(0);
    HSPI_byte_io( 0x05);
    tmp = HSPI_byte_io( 0xFF);
	HSPI_SET_CS(1);
	if( !( tmp & STATUS1_WEL))
		return 2;

	// Write Data
	HSPI_SET_CS(0);
    HSPI_byte_io( 0x02);
    HSPI_byte_io( ( addr >> 16) & 0xFF);
    HSPI_byte_io( ( addr >> 8) & 0xFF);
    HSPI_byte_io( ( addr >> 0) & 0xFF);
    for( int q = 4; q < len; q++)
        HSPI_byte_io( ( uint8_t)*( data + q));
	HSPI_SET_CS(1);

    // Wait chip free
	do
	{
		HSPI_SET_CS(0);
		HSPI_byte_io( 0x05);
		tmp = HSPI_byte_io( 0xFF);
		HSPI_SET_CS(1);
	}while( tmp & STATUS1_BUSY);

    // Check write data
	HSPI_SET_CS(0);
    HSPI_byte_io( 0x0B); // Fast read
    HSPI_byte_io( ( addr >> 16) & 0xFF);
    HSPI_byte_io( ( addr >> 8) & 0xFF);
    HSPI_byte_io( ( addr >> 0) & 0xFF);
    HSPI_byte_io( 0xFF); // Dummy byte
    for( int q = 4; q < len; q++)
        if( HSPI_byte_io( 0xFF) != ( uint8_t)*( data + q))
       	{
        	HSPI_SET_CS(1);
        	return 3;
       	}
	HSPI_SET_CS(1);

	return 0;
}

	//ToDo: support fragmentation
int ICACHE_FLASH_ATTR _PROG_25x_ConnRecvCb(char *data,	unsigned short len) {
//httpd_printf("25xRX %d\r\n", len);
	if (cmd.magic_cmd == _MAGIC_CMD_NONE) {
		//ToDo: support fragmentation !!! before "exception"
		ASSERT(len < sizeof(PROG_CMD));


		if( ((PROG_CMD*)data)->magic_cmd == _MAGIC_CMD_PAGE_WRITE)
		{
			PROG_CMD* cmd = (PROG_CMD*)data;
			//ToDo: support fragmentation !!! before "exception"
			ASSERT(len < (sizeof(PROG_CMD) + cmd->wr_len));
			data_buf[ 0] = _PROG_25x_write_page( cmd->data, cmd->wr_len);
			progSendData(data_buf, cmd->rd_len);
			return 0;
		}
		memcpy(&cmd, data, sizeof(PROG_CMD));
httpd_printf("25xmRXcmd %d %d %d %d\r\n", len, cmd.magic_cmd, cmd.wr_len,
cmd.rd_len);
		//ToDo: support serial program !!! before "exception"
		ASSERT(cmd.magic_cmd != _MAGIC_CMD_IO); // NOT SUPPORTED

		//ToDo: support fragmentation !!! before "exception"
		ASSERT(len < (sizeof(PROG_CMD) + cmd.wr_len));

		// Start IO
		HSPI_SET_CS(0);

		// Send DATA
		uint32_t _tx_len = min( cmd.wr_len, (len - sizeof(PROG_CMD)));

		for (uint32_t q = 0; q < _tx_len; q++)
			HSPI_byte_io( *((((PROG_CMD*) data)->data) + q));

		cmd.wr_len -= _tx_len;
	} else {
		// Send DATA
		uint32_t _tx_len = min(len, cmd.wr_len);

		for (uint32_t q = 0; q < _tx_len; q++)
			HSPI_byte_io( *( data + q));

		cmd.wr_len -= _tx_len;
	}

	if (cmd.wr_len == 0) {
		// Receive data

		uint32_t _rd_len = min(DATA_BUF_SIZE, cmd.rd_len);
		for (uint32_t q = 0; q < _rd_len; q++)
			*((uint8_t*) (data_buf + q)) = HSPI_byte_io( 0xFF);
		cmd.rd_len -= _rd_len;

		// send readed data
//httpd_printf("25xmTX %d\r\n", _rd_len);
		if (_rd_len > 0)
			progSendData(data_buf, _rd_len);
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		if (cmd.rd_len == 0) {
			HSPI_SET_CS(1);
			cmd.magic_cmd = _MAGIC_CMD_NONE; // IO Done
		}
	}
	return 0;
}

int ICACHE_FLASH_ATTR _PROG_25x_ConnSentDoneCb() {
//httpd_printf("25xTXdonecb\r\n");
	if (cmd.wr_len != 0)
		return -1;
	//ToDo: support serial program !!! before "exception"
//	if (cmd.magic_cmd != _MAGIC_CMD_IO) // NOT SUPPORTED
//		espconn_disconnect(espconn);

	uint32_t _rd_len = min( DATA_BUF_SIZE, cmd.rd_len);
	for (uint32_t q = 0; q < _rd_len; q++)
//		spi_mast_byte_write( HSPI, (uint8_t*) (data_buf + q));
		*((uint8_t*) (data_buf + q)) = HSPI_byte_io( 0xFF);
	cmd.rd_len -= _rd_len;

	// send readed data
//httpd_printf("25xsTX %d\r\n", _rd_len);
	if (_rd_len > 0)
		progSendData( data_buf, _rd_len);

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	if (cmd.rd_len == 0) {
		HSPI_SET_CS(1);
		cmd.magic_cmd = _MAGIC_CMD_NONE; // IO Done
	}
	return 0;
}

int ICACHE_FLASH_ATTR _PROG_25x_DeInit( void) {
	ASSERT( data_buf == NULL);
	free( data_buf);
	data_buf = NULL;

	//spi master deinit funtion
	HSPI_CS_DEINIT();
	SOCK8_DEINIT();
	spi_master_deinit( HSPI);
	return 0;
}

//Initialize listening socket, do general initialization
int ICACHE_FLASH_ATTR _PROG_25x_Init() {
httpd_printf( "conn 25x\n");

	//Alloc memory buf
	ASSERT( data_buf != NULL);
	data_buf = malloc( DATA_BUF_SIZE);
	if( data_buf == NULL)
		return -1;

	// Reset cmd
	cmd.magic_cmd = _MAGIC_CMD_NONE;
	cmd.rd_len = 0;
	cmd.wr_len = 0;

	//Send Hi
	char* msg = "25x_PROG_v1";
	progSendData( (uint8_t*) msg, 11);

	//spi master init funtion
	HSPI_CS_INIT();
	SOCK8_INIT();
	spi_master_init( HSPI, 0, 0, 8, 16000000);

return 0;
}
