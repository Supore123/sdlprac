#include "app.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct appConfig
{
	uint32_t version;
}appConfig_t;

appConfig_t appConfig = {0};

void userInit(void)
{
	appConfig.version = APP_VERSION;
	printf("Loading App Version 0x0%u\r\n", appConfig.version);
}
