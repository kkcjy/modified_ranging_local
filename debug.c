#include "debug.h"


void DEBUG_PRINT(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}