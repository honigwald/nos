#ifndef _SYSCALLS_H
#define _SYSCALLS_H

#include <stddef.h>
#include <stdint.h>

/**
 * read character from UART - sets threat to WAITING until
 * character is available
 *
 * param: c - address to write read character to
 */
unsigned char getc();

/**
 * write character to UART
 *
 * param: c - char value to be written
 */
void putc(unsigned char c);

/**
 * create thread in tcb
 *
 * param: fptr - function pointer to function in form "void fptr(void *data)"
 * param: data - function parameter
 * param: len  - length of data array in bytes
 * return: 0 on success, -1 if thread couldn't be created due to thread limit
 */
int start_thread(void *(*fptr)(void *), void *data, size_t len);
int start_process(void *(*fptr)(void *), void *data, size_t len);

/**
 * terminate thread currently running in tcb
 */
void exit();

/**
 * set threat to WAITING depending on mode
 *
 * param: mode - WAIT_FOR_COUNTER lets thread sleep for n Zeitscheiben
 * 		 WAIT_FOR_UART lets thread sleep until next uart interrupt (if more
 * 		 than one threat waits for uart irq, first in runqueue is prioritized)
 */
void sleep(size_t n);

int mytid();
int mypid();

int open_channel(int channel_id);
int send_channel(int channel_id, uint8_t *data, uint32_t length);
int read_channel(int channel_id, uint8_t *data, uint32_t length);

#endif // _SYSCALLS_H

