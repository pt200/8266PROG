// Bondarenko Andrey

//#include <assert.h>
#include <esp8266.h>
#include "esp8266_prog.h"
#include "esp8266_prog_25x.h"
#include "esp8266_prog_1w.h"
#include "esp8266_prog_AVR.h"
#include "esp8266_prog_i2c.h"


ESP8266_PROG_Instance* prog = NULL;

ESP8266_PROG_Instance* progs[] = {
		&PROG_25x,
		&PROG_AVR,
		&PROG_1w,
		&PROG_I2C
};


//Listening connection data
static espconn progConn;
static esp_tcp progTcp;


static void ICACHE_FLASH_ATTR progConnRecvCb(void *espconn, char *data,
		unsigned short len) {
	if (prog != NULL) {
		if( prog->recv_callback != NULL)
			prog->recv_callback(data, len);
	} else {
		if (len < 4)
			return;
//httpd_printf("mRX %d %08x %08x %08x\r\n", len, *((uint32_t*) data), progs[ 0]->id,progs[ 1]->id);

		prog = NULL;
		for( int q = 0; q < ( sizeof( progs) / sizeof(  progs[ 0])); q++)
		{
			if( progs[ q]->id == *((uint32_t*) data))
			{
				if( progs[ q]->init)
				{
					if( progs[ q]->init() == 0)
						prog = progs[ q];
					else
						prog = NULL;
				}
				break;
			}
		}
		if( prog == NULL)
		{
			espconn_disconnect(espconn);
		}
	}
}

static void ICACHE_FLASH_ATTR progConnSentDoneCb(void *espconn) {
	if (prog != NULL)
		if (prog->sent_callback)
			prog->sent_callback();
}

static void ICACHE_FLASH_ATTR progConnCb(void *espconn) {
//	ASSERT(prog.ispresent);

	char* msg = "8266_PROG_v1";
	espconn_sent(espconn, (uint8_t*) msg, 12);
}

static void ICACHE_FLASH_ATTR progConnReconCb(void *espconn, sint8 err) {
	if (prog != NULL)
		if (prog->deinit)
			prog->deinit();
	prog = NULL;
}
static void ICACHE_FLASH_ATTR progConnDisconCb(void *espconn) {
	if (prog != NULL)
		if (prog->deinit)
			prog->deinit();
	prog = NULL;
}

void ICACHE_FLASH_ATTR progSendData(uint8_t *buff, int len) {
	espconn_sent(&progConn, (uint8_t*) buff, len);
}

//Initialize listening socket, do general initialization
void ICACHE_FLASH_ATTR ESP8266_PROG_Init(int port) {

	prog = NULL;

	progConn.type = ESPCONN_TCP;
	progConn.state = ESPCONN_NONE;
	progConn.proto.tcp = &progTcp;

	progTcp.local_port = port;

	espconn_tcp_set_max_con_allow(&progConn, 1);
	espconn_regist_connectcb(&progConn, progConnCb);
	espconn_regist_disconcb(&progConn, progConnDisconCb);
    espconn_regist_reconcb(&progConn, progConnReconCb);
	espconn_regist_recvcb(&progConn, progConnRecvCb);
	espconn_regist_sentcb(&progConn, progConnSentDoneCb);

	espconn_accept(&progConn);
}

//	httpdRecvCb(conn, (char*)conn->proto.tcp->remote_ip, conn->proto.tcp->remote_port, data, len);
/*
  void ICACHE_FLASH_ATTR _progConnDisconnect(ConnTypePtr conn) {
 espconn_disconnect(conn);
 }

 void ICACHE_FLASH_ATTR _progConnDisableTimeout(ConnTypePtr conn) {
 //Can't disable timeout; set to 2 hours instead.
 espconn_regist_time(conn, 7199, 1);
 }
 */
