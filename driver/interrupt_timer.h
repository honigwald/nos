#ifndef _TIMER_H
#define _TIMER_H

#include "../lib/printf.h"
#include "../lib/flags.h"

void interrupt_timer_ack();
int init_interrupt_timer(unsigned int ms);
void start_interrupt_timer();

#endif // _TIMER_H
