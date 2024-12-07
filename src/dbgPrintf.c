#include "dbgPrintf.h"

// Define dbgPrintf as a wrapper
void dbgPrintf(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    // Send output to the console (stdout) for now
    vprintf(format, args);

    va_end(args);
}
