#include "wait.h"

// asm procedure to be masked w/ c function
void _wait(unsigned int cycles);

void wait(unsigned int cycles)
{
	//unsigned int cycles = ms * (1775367/1000);
	_wait(cycles);
}
