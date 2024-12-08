// Includes
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

// Variable defines
typedef enum appStatus
{
	APP_STATUS_OK,
	APP_STATUS_FAIL,
	APP_STATUS_SCREEN_FAILURE,
	APP_STATUS_UNKNOWN,
	APP_STATUS_RESERVED,
}appStatus_t;

typedef struct appConfig
{
	uint32_t version;
}appConfig_t;

// Function defines
appStatus_t appInit(void);
appStatus_t NvmDataWrite(void);


