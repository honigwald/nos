#ifndef _THREAD_CONTROL_H
#define _THREAD_CONTROL_H

// wait mode definitions
#define WAIT_FOR_COUNTER	0
#define WAIT_FOR_UART		1
#define WAIT_FOR_CHANNEL_READ	2
#define WAIT_FOR_CHANNEL_SEND	3


#include <stddef.h>
#include "stdint.h"
#include "../lib/initstd.h"
#include "../lib/printf.h"
#include "../lib/flags.h"
#include "../lib/stdlib.h"
#include "../sys/mmu.h"
#include "../sys/channels.h"

typedef struct context_t context_t;
struct context_t {
	unsigned int cpsr;
	unsigned int lr;
	unsigned int sp;
	unsigned int r[13];
	unsigned int pc;
};

void init_tcb();
void print_tcb_array();
void print_tcb_queue();
int thread_create(void *(*fptr)(void *), void *data, size_t len, void *(*t_term));
int process_create(void *(*fptr)(void *), void *data, size_t len, void *(*t_term));
void get_tid(context_t *curr_context);
unsigned int get_tid_k();
void get_pid(context_t *curr_context);
void thread_wait(int mode, int value);
void thread_wait_clock();
void thread_awake_for_uart();
void awake_for_channel_read(unsigned int id);
void awake_for_channel_send(unsigned int id);
void thread_terminate();
void _thread_terminate();
void _start_thread_control();

void scheduler(context_t *curr_context);

#endif // _THREAD_CONTROL_H
