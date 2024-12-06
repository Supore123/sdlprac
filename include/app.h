// Includes
#include <stdio.h>
#include <stdlib.h>

// Hard defines
#define APP_VERSION 		0x01
#define APP_VERSION_STR		"0.01"

// Function defines
void appInit(void);

// Variable defines
typedef enum appStatus
{
	APP_STATUS_OK,
	APP_STATUS_FAIL,
	APP_STATUS_UNKNOWN,
	APP_STATUS_RESERVED,
}appStatus_t;
