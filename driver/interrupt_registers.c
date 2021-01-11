#include "interrupt_registers.h"


// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// basic register bits
#define TIMER_INT 0
// irq2 register bits
#define UART_INT 25

// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// interrupt maskregister access
#define INTERRUPT_BASE (0x7E00B000 - 0X3F000000 + 0x200)
struct interrupt {
	unsigned int irq_basic_pending;
	unsigned int irq_pending_1;
	unsigned int irq_pending_2;
	unsigned int fiq_control;
	unsigned int enable_irq_1;
	unsigned int enable_irq_2;
	unsigned int enable_basic_irq;
	unsigned int disable_irq_1;
	unsigned int disable_irq_2;
	unsigned int disable_basic_irq;
};
static volatile struct interrupt *const ir = (struct interrupt *)INTERRUPT_BASE;


int timer_irq_pending()
{
	return (ir->irq_basic_pending & (1 << TIMER_INT));
}
int uart_irq_pending()
{
	return (ir->irq_pending_2 & (1 << UART_INT));
}

void init_interrupt_registers()
{
	ir->enable_basic_irq |= 1 << TIMER_INT;  // enable timer_irq
	ir->enable_irq_2 |= 1 << UART_INT;       // enable uart_irq
	timer_irq_print_f = 0;                   // don't print '!' by default
}
