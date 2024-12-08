#include "app.h"
#include "dbgPrintf.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "version.h"

// Variable defines
appConfig_t appConfig = {0};

// Functions defines and externs
extern appStatus_t screenInit(void);

// userInit -- Initialises all parameters
appStatus_t userInit(void)
{
	FILE *appFile;
	bool loadDefaults = false;
	appStatus_t sc = APP_STATUS_OK;

	appFile = fopen("appFiles.txt", "w+");
	
	if(appFile == 0)
	{
		loadDefaults = true;
	}

	if(loadDefaults == true)
	{
		dbgPrintf("Loading Factory Defaults");
		appConfig.version = APP_VERSION; // Make the defaults here, then make a function call to save
	}

	dbgPrintf("Loading App Version 0x0%u\r\n", appConfig.version);

	sc = screenInit();

	return sc;
}

appStatus_t NvmWriteData(void)
{
	appStatus_t sc = APP_STATUS_OK;

	dbgPrintf("Writing Data...\r\n");

	return sc;
}
