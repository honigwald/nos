#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>
#include "../lib/printf.h"

void *memcpy(void* dst, void* src, size_t n);
size_t strlen(const char *str);

#endif // _STDLIB_H
