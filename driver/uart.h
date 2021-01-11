#ifndef _UART_H
#define _UART_H

#include <errno.h>
#include "../sys/wait.h"

void init_uart();
void uart_putc(unsigned char c);
int uart_getc(unsigned char *c);
void uart_rx_irq_handler();
void uart_rx_irq_ack();

#endif // _UART_H
