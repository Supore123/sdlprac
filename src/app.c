#include "app.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

appConfig_t appConfig = {0};

// userInit -- Initialises all parameters
void userInit(void)
{
	FILE *appFile;
	bool loadDefaults = false;
	appFile = fopen("appFiles.txt", "w+");
	
	if(appFile == 0)
	{
		loadDefaults = true;
	}

	if(loadDefaults == true)
	{
		printf("Loading Factory Defaults");
		appConfig.version = APP_VERSION; // Make the defaults here, then make a function call to save
	}

	printf("Loading App Version 0x0%u\r\n", appConfig.version);
}

appStatus_t NvmWriteData(void)
{
	appStatus_t sc = APP_STATUS_OK;

	printf("Writing Data...\r\n");

	return sc;
}
