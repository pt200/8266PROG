// Bondarenko Andrey

#include <esp8266.h>
#include "esp8266_prog_platform.h"
#include "esp8266_prog.h"
#include "AVR_prog.h"


int ICACHE_FLASH_ATTR _PROG_AVR_ConnRecvCb(char *data, unsigned short len);
//int ICACHE_FLASH_ATTR _PROG_AVR_ConnSentDoneCb();
int ICACHE_FLASH_ATTR _PROG_AVR_DeInit();
int ICACHE_FLASH_ATTR _PROG_AVR_Init();

ESP8266_PROG_Instance PROG_AVR = {
		MAKE_PROG_ID('A', 'V', 'R', ' '),
		_PROG_AVR_Init,
		_PROG_AVR_DeInit,
		_PROG_AVR_ConnRecvCb,
		NULL,//_PROG_AVR_ConnSentDoneCb
		};

#define min(a,b) ((a)<(b)?(a):(b))


#define _MAGIC_CMD_AVR_STREAM_IO	0x32345678
#define _MAGIC_CMD_AVR_SET_RST		0x32345677
#define _MAGIC_CMD_NONE         	0

#define DATA_BUF_SIZE  16384
static uint8_t* data_buf = NULL;


//ToDo: support fragmentation
int ICACHE_FLASH_ATTR _PROG_AVR_ConnRecvCb(char *data,	unsigned short len) {
if( data_buf == NULL)
return -1;
	if (len < sizeof(PROG_CMD))
		return -1;

	PROG_CMD* cmd = (PROG_CMD*) data;
httpd_printf("AVR RX %d %d %d %d\r\n", len, cmd->magic_cmd, cmd->wr_len,
	cmd->rd_len);

	if (len < (sizeof(PROG_CMD) + cmd->wr_len))
		return -1;
	switch (cmd->magic_cmd) {
	case _MAGIC_CMD_AVR_STREAM_IO:
		// Send DATA
		for (uint32_t q = 0; q < cmd->wr_len; q++)
			data_buf[q] = AVR_IO8(cmd->data[q]);
		break;
	case _MAGIC_CMD_AVR_SET_RST:
		if( cmd->wr_len > 0)
		{
			AVR_SET_RST( cmd->data[0]);
			AVR_CLKI_PULSES( 100);
			data_buf[0] = cmd->data[0];
		}
		break;
	default:
		return -1;
	}
	// Send readed data
	if (cmd->rd_len > 0)
		progSendData((uint8_t*) data_buf, cmd->rd_len);
	return 0;
}

/*
int ICACHE_FLASH_ATTR _PROG_AVR_ConnSentDoneCb() {
	return 0;
}
*/

int ICACHE_FLASH_ATTR _PROG_AVR_DeInit( void) {
	ASSERT( data_buf == NULL);
	free( data_buf);
	data_buf = NULL;

	AVR_DEINIT();

	return 0;
}

//Initialize listening socket, do general initialization
int ICACHE_FLASH_ATTR _PROG_AVR_Init() {
httpd_printf( "conn AVR\n");

	//Alloc memory buf
	ASSERT( data_buf != NULL);
	data_buf = malloc( DATA_BUF_SIZE);
	if( data_buf == NULL)
		return -1;

	//Send Hi
	char* msg = "AVR_PROG_v1";
	progSendData( (uint8_t*) msg, 11);

    AVR_INIT();

	return 0;
}
