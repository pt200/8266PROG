// Bondarenko Andrey

#ifndef __ESP8266_PROG_H__
#define __ESP8266_PROG_H__

#define ASSERT(x) if( x){ os_printf( "assert %s:%d", __FILE__, __LINE__); while(1){}}

void ICACHE_FLASH_ATTR progSendData(uint8_t *buff, int len);

/** A callback prototype to inform about events for a espconn */
//typedef void (* espconn_recv_callback)(void *arg, char *pdata, unsigned short len);
typedef int (* _PROG_INIT)();
typedef int (* _PROG_DEINIT)();
typedef int (* _PROG_RECEIVE_CALLBACK)(char *pdata, unsigned short len);
typedef int (* _PROG_SEND_DONE_CALLBACK)();

#define MAKE_PROG_ID( a, b, c, d) ( ((( uint32_t)d) << 24) | ((( uint32_t)c) << 16) | \
                                    ((( uint32_t)b) << 8) |(( uint32_t)a))

typedef struct {
	uint32_t magic_cmd;
	uint32_t wr_len;
	uint32_t rd_len;
	uint8_t data[0];
}PROG_CMD;

typedef struct
{
	uint32_t id;
	_PROG_INIT init;
	_PROG_DEINIT deinit;
	_PROG_RECEIVE_CALLBACK recv_callback;
	_PROG_SEND_DONE_CALLBACK sent_callback;
//  espconn_connect_callback write_finish_fn;
}ESP8266_PROG_Instance;


//Initialize listening socket, do general initialization
void ICACHE_FLASH_ATTR ESP8266_PROG_Init( int port);

#endif
