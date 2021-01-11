#ifndef _PRINTF_H
#define _PRINTF_H

#include <stdarg.h>

#include "../driver/uart.h"

int printf(char* fmt, ...);

#endif // _PRINTF_H
