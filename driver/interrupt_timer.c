#include "interrupt_timer.h"

// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// control register bits
#define COUNTER_23_BITS 1
#define PRESCALE_BIT0   2
#define PRESCALE_BIT1   3
#define TIMER_INT       5
#define TIMER_ENABLED   7

// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// timer (arm side) register access
/**
 *   Short description of timer struct
 *   - load: this register is written to value. it sets the time to count
 *   - value: holds current timer value. counted down when running.
 *            if 0 is reached interrupt pending bit is set
 *   - control(R/W): 31:10 unused
 *   		        9 free running counter enabled
 *       	        8 timers keeps running if ARM is in debug mode
 *       	        7 timer enabled
 *       	        6 NOT USED
 *       	        5 timer interrupt enabled
 *       	        4 NOT USED
 *       	      3:2  00 prescale is clock/1
 *                         01 prescale is clock/16
 *                         10 prescale is clock/256
 *                         11 prescale is clock/1
 *       	        1 23-bit counter (16-bit counter)
 *       	        0 NOT USED
 *   - irq_clear(W): When writting, interrupt pending bit is cleared
 *   - raw_irq(R): shows status of interrupt pending bit
 *   - masked_irq(R): interrupt line is asserted
 *   - reload: copy of load register. if writting no immediate effect occurs.
 *             value is loaded, when value register reached 0
 *   - predivider: 10bit wide. reset value is 0x7D.
 *                 timer_clock = apb_clock/(pre_divider+1)
 *   - free_running_counter: seems to be not needed
 */
#define TIMER_BASE (0x7E00B000 - 0X3F000000 + 0x400)
struct timer {
	unsigned int load;
	unsigned int value;
	unsigned int control;
	unsigned int irq_clear;
	unsigned int raw_irq;
	unsigned int masked_irq;
	unsigned int reload;
	unsigned int predivider;
	unsigned int free_running_counter;
};

static volatile struct timer *const tr = (struct timer *)TIMER_BASE;

void interrupt_timer_ack()
{
	tr->irq_clear = 1;
}

int init_interrupt_timer(unsigned int ms)
{
	if (ms > 8000*1000) {
		printf("init_interrupt_timer(%u) error: value to big", ms);
		return -1;
	}

	tr->control |= 1 << COUNTER_23_BITS;  // configure counter to be 23 bits
	tr->control |= 1 << PRESCALE_BIT1;    // set prescale to 256
	tr->control |= 1 << TIMER_INT;        // enable timer interrupts
	tr->predivider = 976;                 // to achive 1 MHz clock

	timer_irq_print_f = 0;

	tr->load = ms;                        // set timer start value (in ms)

	return 0;
}

void start_interrupt_timer()
{
	tr->control |= 1 << TIMER_ENABLED;  // start timer
}
