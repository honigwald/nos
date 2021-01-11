#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#include "../lib/flags.h"
#include "../lib/printf.h"

int timer_irq_pending();
int uart_irq_pending();
void init_interrupt_registers();

#endif // _INTERRUPTS_H
