#include "uwait.h"

// asm procedure to be masked w/ c function
void _uwait(unsigned int cycles);

void uwait(unsigned int ms)
{
	unsigned int cycles = ms * (1775367/1000);
	_uwait(cycles);
}
