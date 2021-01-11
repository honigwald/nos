#include "stdlib.h"

void *memcpy(void* dst, void* src, size_t n)
{
	char *csrc = (char*)src;
	char *cdst = (char*)dst;
	unsigned i;
	for (i = 0; i < n; ++i)
		cdst[i] = csrc[i];
	return dst;
}

size_t strlen(const char *str)
{
	size_t length;

	for (length = 0; str[length] != '\0'; ++length);

	return length;
}
