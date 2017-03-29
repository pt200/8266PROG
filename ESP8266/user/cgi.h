#ifndef CGI_H
#define CGI_H

#include "httpd.h"

int ICACHE_FLASH_ATTR cgiLed(HttpdConnData *connData);
int ICACHE_FLASH_ATTR tplLed(HttpdConnData *connData, char *token, void **arg);

#endif