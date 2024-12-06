// Includes
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

// Hard defines
#define APP_VERSION 		0x01
#define APP_VERSION_STR		"0.01"

// Variable defines
typedef enum appStatus
{
	APP_STATUS_OK,
	APP_STATUS_FAIL,
	APP_STATUS_UNKNOWN,
	APP_STATUS_RESERVED,
}appStatus_t;

typedef struct appConfig
{
	uint32_t version;
}appConfig_t;

// Function defines
void appInit(void);
appStatus_t NvmDataWrite(void);


