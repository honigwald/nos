#include "uart.h"

// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// flag register bits
#define TXFE (1 << 7)
#define RXFF (1 << 6)
#define TXFF (1 << 5)
#define RXFE (1 << 4)
#define BUSY (1 << 3)
#define CTS  (1 << 0)
// dr register bits
#define OE (1 << 11)
#define BE (1 << 10)
#define PE (1 << 9)
#define FE (1 << 8)
// cr register bits
#define UARTEN (1 << 0)
// lcrh register bits
#define FEN (1 << 4)
// imsc register bits
#define RXIM (1 << 4)
// irc register bits
#define RXIC (1 << 4)

// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// fifo
#define QUEUE_ELEMENTS 1024
#define QUEUE_SIZE (QUEUE_ELEMENTS + 1)

static unsigned char Queue[QUEUE_SIZE];
static unsigned char QueueIn;
static unsigned char QueueOut;

// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// uart register access
#define UART_BASE (0X7E201000 - 0X3F000000)
struct uart_t {
	unsigned int dr;
	unsigned int rsrecr;
	unsigned int unused0[4];
	unsigned int fr;
	unsigned int unused1;
	unsigned int ilpr;
	unsigned int ibrd;
	unsigned int fbrd;
	unsigned int lcrh;
	unsigned int cr;
	unsigned int ifls;
	unsigned int imsc;
	unsigned int ris;
	unsigned int mis;
	unsigned int icr;
};
static volatile struct uart_t *const uart = (struct uart_t *)UART_BASE;

void init_uart()
{
	wait(8388608/10);
	uart->cr &= ~UARTEN;	// disable uart
	wait(8388608/10);

	// configure UART
	uart->lcrh &= ~FEN;	// disable hardware FIFOs
	uart->imsc |= RXIM;	// enable RXIM (recieve interrupt mask)

	uart->cr |= UARTEN;	// enable uart
	wait(8388608/10);

	// fifo init
	QueueIn = 0;
	QueueOut = 0;
}

/*
 * write argument char c into data reg if TXFF flag is 0
 */
void uart_putc(unsigned char c)
{
	while (uart->fr & TXFF)
		continue;
	uart->dr = c;
}

/*
 * read a character from data register
 * returns 0 on success - -1 if queue is empty
 */
int uart_getc(unsigned char *c)
{
	// fifo get(c)
	if(QueueIn == QueueOut)
		return -1; // queue empty - nothing to get
	*c = Queue[QueueOut];
	QueueOut = (QueueOut + 1) % QUEUE_SIZE;

	return 0;
}

void uart_rx_irq_handler()
{
	unsigned int dr = uart->dr;

	// error checking
	if (dr & (OE | BE | PE | FE))
		return;

	// fifo put(dr)
	if (QueueIn == ((QueueOut - 1 + QUEUE_SIZE) % QUEUE_SIZE))
		return; // queue full
	Queue[QueueIn] = (unsigned char)dr;
	QueueIn = (QueueIn + 1) % QUEUE_SIZE;

	return;
}

void uart_rx_irq_ack()
{
	uart->icr |= RXIC;  // clears the interrupt (UARTRXINTR)
}
