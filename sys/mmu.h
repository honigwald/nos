#ifndef MMU_H
#define MMU_H

#include "../lib/printf.h"
#include "../lib/initstd.h"

void update_logical_address_space(int *tsa_table, int pid);
unsigned int translate_stack_addr(unsigned int addr, int tid);

#endif // MMU_H
