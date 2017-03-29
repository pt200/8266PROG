// Bondarenko Andrey

#include <esp8266.h>
#include "driver/ds18b20.h"
#include "esp8266_prog_platform.h"
#include "esp8266_prog.h"

int ICACHE_FLASH_ATTR _PROG_1w_ConnRecvCb(char *data, unsigned short len);
//void ICACHE_FLASH_ATTR _PROG_1w_ConnSentDoneCb();
int ICACHE_FLASH_ATTR _PROG_1w_DeInit();
int ICACHE_FLASH_ATTR _PROG_1w_Init();

ESP8266_PROG_Instance PROG_1w = {
		MAKE_PROG_ID('1', 'w', ' ', ' '),
		_PROG_1w_Init,
		_PROG_1w_DeInit,
		_PROG_1w_ConnRecvCb,
		NULL //_PROG_1w_ConnSentDoneCb;
		};

#define min(a,b) ((a)<(b)?(a):(b))

#define _MAGIC_CMD_RESET    0x12345671
#define _MAGIC_CMD_FIND_ID  0x12345672
#define _MAGIC_CMD_IO       0x12345673
#define _MAGIC_CMD_IO_POWER 0x12345674
#define _MAGIC_CMD_NONE     0

#define DATA_BUF_SIZE  16384
static uint8_t* data_buf = NULL;

//ToDo: support fragmentation
int ICACHE_FLASH_ATTR _PROG_1w_ConnRecvCb(char *data, unsigned short len) {
//httpd_printf("1w RX %d\r\n", len);
	int bus_1w_power = 0;
	if (len < sizeof(PROG_CMD))
		return -1;

	PROG_CMD* cmd = (PROG_CMD*) data;
	httpd_printf("1w RX %d %d %d %d\r\n", len, cmd->magic_cmd, cmd->wr_len,
			cmd->rd_len);

	if (len < (sizeof(PROG_CMD) + cmd->wr_len))
		return -1;
	switch (cmd->magic_cmd) {
	case _MAGIC_CMD_IO_POWER:
		bus_1w_power = 1;
	case _MAGIC_CMD_IO:
		// Send DATA
		for (uint32_t q = 0; q < cmd->wr_len; q++)
			ds_write(cmd->data[q], bus_1w_power);
// Receive DATA
		for (uint32_t q = 0; q < cmd->rd_len; q++)
			data_buf[q] = ds_read();
		break;
	case _MAGIC_CMD_FIND_ID:
		if( cmd->wr_len > 0)
			if(cmd->data[0] != 0)
				ds_reset_search();
		data_buf[0] = ds_search(&data_buf[4]);
		break;
	case _MAGIC_CMD_RESET:
		data_buf[0] = ds_reset();
		break;
	default:
		return -1;
	}
// Send data
	if (cmd->rd_len > 0)
		progSendData((uint8_t*) data_buf, cmd->rd_len);
	return 0;
}

//void ICACHE_FLASH_ATTR _PROG_1w_ConnSentDoneCb() {
//}

//DeInitialize
int ICACHE_FLASH_ATTR _PROG_1w_DeInit() {
	ASSERT( data_buf == NULL);
	free( data_buf);
	data_buf = NULL;

	return 0;
}

//Initialize
int ICACHE_FLASH_ATTR _PROG_1w_Init() {
httpd_printf("1wconn\n");

	//Alloc memory buf
	ASSERT( data_buf != NULL);
	data_buf = malloc( DATA_BUF_SIZE);
	if( data_buf == NULL)
		return -1;

	//Send Hi
	char* msg = "1w_PROG_v1";
	progSendData((uint8_t*) msg, 10);


	//1w bus init
	ds_init();

	return 0;
}
