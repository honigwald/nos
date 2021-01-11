#include "thread_control.h"

//#define DEBUG

// thread status "enum"
#define READY       0
#define RUNNING     1
#define WAITING     2
#define TERMINATED  3

// process status "enum"
#define INACTIVE 0
#define ACTIVE   1

#define PID_MIN 0
#define PID_MAX 7

#define MIB 0x100000

// thread control block array linked list
typedef struct tcb_t tcb_t;
struct tcb_t {
	unsigned int r[13];
	unsigned int sp;
	unsigned int lr;
	unsigned int pc;
	unsigned int cpsr;
	int status;
	int tid;
	int pid;
	int wait_mode;
	unsigned int wait_value;
	tcb_t *prev;
	tcb_t *next;
};
static tcb_t tcb[_tcb_size];

// process control block
typedef struct pcb_t pcb_t;
struct pcb_t {
	int status;
	int tsa_table[_tcb_size];	// threat stack allocation table
};
static pcb_t pcb[_pcb_size];

static tcb_t *runqueue;
static context_t idle_context;
static int idle_running;

/**
 * iterates over tcb[] and returns first terminated element
 * return -1 if no terminated element is found
 */
static int allocate_tcb_element()
{
	int id;
	for (id = 0; id < _tcb_size; id++) {
		if (tcb[id].status == TERMINATED)
			return id;
	}
	return -1; // array is full
}

static void *allocate_thread_stack(int pid, int tid)
{
	if (pid < PID_MIN || pid > PID_MAX) {
		printf("allocate_thread_stack() error (1)\n");
		return NULL;
	}

	int entry;
	for (entry = 0; entry < _pcb_size; ++entry) {
		if (pcb[pid].tsa_table[entry] == -1) {
			pcb[pid].tsa_table[entry] = tid;
			return (void *)_tcb_sp_base + (entry * MIB);
		}
	}

	printf("allocate_thread_stack() error (2)\n");
	return NULL;
}

/**
 * append new element to linked-list runqueue.
 */
static void add_tcb_element(tcb_t *new)
{
	if (new == NULL) {
		printf("add_tcb_element() error: arg points to NULL");
		return;
	}

	if (runqueue == NULL) {       // list is empty
		runqueue = new;
		new->prev = NULL;
		new->next = NULL;
	} else {                      // list is not empty
		// go to end of list
		tcb_t *curr = runqueue;
		while (curr->next != NULL)
			curr = curr->next;

		// append new element
		curr->next = new;
		new->prev = curr;
		new->next = NULL;
	}

	return;
}

/**
 * changes next and prev pointer of adjacent elements to remove element revered by id
 */
static void remove_tcb_head_element(tcb_t *elem)
{
	if (elem->prev != NULL)
		elem->prev->next = elem->next;
	else
		runqueue = elem->next;
	if (elem->next != NULL)
		elem->next->prev = elem->prev;
}

/**
 * rotate head element in queue to tail
 */
static void rotate_tcb_queue()
{
	if (runqueue == NULL)
		return;
	if (runqueue->next == NULL)
		return;

	tcb_t *first = runqueue;
	tcb_t *last = first;
	while (last->next != NULL)
		last = last->next;

	runqueue = first->next;
	runqueue->prev = NULL;
	last->next = first;
	first->next = NULL;
	first->prev = last;
}

/**
 * search for first thread in READY state and return queue index
 * return -1 if not found
 */
static int index_of_first_ready()
{
	int index;
	tcb_t *curr = runqueue;
	for (index = 0; curr != NULL; ++index) {
		if (curr->status == READY)
			return index;
		curr = curr->next;
	}

	return -1;
}

/**
 * reset tcb[id] to initial/default state
 */
static void reset_tcb_element(int id)
{
	int i;
	for (i = 0; i < 13; ++i)
		tcb[id].r[i] = 0;
	tcb[id].sp = 0;
	tcb[id].lr = 0;
	tcb[id].pc = 0;
	tcb[id].cpsr = _usr_mode; // all flags empty and mode set to user
	tcb[id].status = TERMINATED;
	tcb[id].pid = -1;
	tcb[id].prev = NULL;
	tcb[id].next = NULL;

	return;
}
static void reset_pcb_element(int pid)
{
	pcb[pid].status = INACTIVE;
	int i;
	for (i = 0; i < _tcb_size; ++i) {
		pcb[pid].tsa_table[i] = -1;
	}
}

static void idle_thread();
/**
 * initiate all tcb[] elements to default state
 */
void init_tcb()
{
	int i, j;
	// init tcb
	for (i = 0; i < _tcb_size; ++i) {
		for (j = 0; j < 13; ++j)
			tcb[i].r[j] = 0;
		tcb[i].sp = 0;
		tcb[i].lr = 0;
		tcb[i].pc = 0;
		tcb[i].cpsr = 0;
		tcb[i].status = TERMINATED;
		tcb[i].tid = i;
		tcb[i].pid = -1;
		tcb[i].wait_mode = 0;
		tcb[i].wait_value = 0;
		tcb[i].prev = NULL;
		tcb[i].next = NULL;
	}

	// init runqueue
	runqueue = NULL;

	// init pcb
	int pid;
	for (pid = 0; pid < _pcb_size; ++pid) {
		pcb[pid].status = INACTIVE;
		reset_pcb_element(pid);
	}

	// init idle context
	idle_context.cpsr = 0;
	idle_context.lr = (unsigned int)(&idle_thread);
	idle_context.sp = 0;
	for (i = 0; i < 13; ++i)
		idle_context.r[i] = 0;
	idle_context.pc = (unsigned int)(&idle_thread);
	idle_running = 1;
}

/**
 * prints all elements of tcb[]
 */
void print_tcb_array()
{
	printf("+=+=+=+=+=+=+=+=+=+=+ tcb array +=+=+=+=+=+=+=+=+=+=+");
	printf("\n");

	char *ready_str = "READY";
	char *running_str = "RUNNING";
	char *waiting_str = "WAITING";
	char *terminated_str = "TERMINATED";
	char *error_str = "state error";

	int i;
	char *status_str = NULL;
	for (i = 0; i < _tcb_size; i++) {
		switch (tcb[i].status) {
		case READY:
			status_str = ready_str;
			break;
		case RUNNING:
			status_str = running_str;
			break;
		case WAITING:
			status_str = waiting_str;
			break;
		case TERMINATED:
			status_str = terminated_str;
			break;
		default:
			status_str = error_str;
			break;
		}
		printf("%2u", i);
		printf(" <%p>:", &(tcb[i]));
		if (tcb[i].status != TERMINATED) {
			printf(" r0:%u", tcb[i].r[0]);
			printf(" sp:%p", tcb[i].sp);
			printf(" lr:%p", tcb[i].lr);
			printf(" pc:%p", tcb[i].pc);
			printf(" cpsr:%p", tcb[i].cpsr);
			printf(" status:%s",status_str);
			printf(" prev:%p", tcb[i].prev);
			printf(" next:%p\n", tcb[i].next);
		} else {
			printf(" EMPTY\n");
		}
	}
	printf("+=+=+=+=+=+=+=+=+=+=+\n");

	return;
}

/**
 * prints all elements of runqueue
 */
void print_tcb_queue()
{
	printf("+=+=+=+=+=+=+=+=+=+=+ tcb queue +=+=+=+=+=+=+=+=+=+=+");
	printf("\n");

	char *ready_str = "READY";
	char *running_str = "RUNNING";
	char *waiting_str = "WAITING";
	char *terminated_str = "TERMINATED";
	char *error_str = "state error";
	char *status_str = NULL;

	tcb_t *curr = runqueue;
	int i = 0;
	while(curr != NULL) {
		switch (curr->status) {
		case READY:
			status_str = ready_str;
			break;
		case RUNNING:
			status_str = running_str;
			break;
		case WAITING:
			status_str = waiting_str;
			break;
		case TERMINATED:
			status_str = terminated_str;
			break;
		default:
			status_str = error_str;
			break;
		}

		printf("%2u", i);
		printf(" <%p>:", curr);
		printf(" pid:%u", curr->pid);
		printf(" tid:%u", curr->tid);
		printf(" sp:%p", curr->sp);
		printf(" lr:%p", curr->lr);
		printf(" pc:%p", curr->pc);
		printf(" cpsr:%p", curr->cpsr);
		printf(" status:%s",status_str);
		printf(" prev:%p", curr->prev);
		printf(" next:%p\n", curr->next);

		curr = curr->next;
		i++;
	}
	printf("+=+=+=+=+=+=+=+=+=+=+\n");
}

/**
 * creates a new thread by
 * 1. searching a free tcb[]-element
 * 2. resetting found element
 * 3. initializing found element w/ given parameters
 * 4. changing state TERMINATED->READY
 * 5. set lr to t_term
 * 6. appending to runqueue
 *
 * returns -1 if error occours
 */
int thread_create(void *(*fptr)(void *), void *data, size_t len, void *(*t_term))
{
	// allocate thread and appant to runqueue
	int thread_id = allocate_tcb_element();
	if (thread_id == -1) {
		printf("thread_create() error: thread limit reached\n");
		return -1;
	}

	// initiate found element
	tcb_t *thread = &(tcb[thread_id]);

	reset_tcb_element(thread_id);
	thread->pc = (unsigned int)fptr;
	thread->lr = (unsigned int)t_term;

	// set pid
	if (runqueue == NULL) {
		thread->pid = 0;
		pcb[0].status = ACTIVE;
	} else {
		thread->pid = runqueue->pid;
	}

	// allocate stack
	thread->sp = (unsigned int)allocate_thread_stack(thread->pid, thread_id);

	if (data != NULL) {
		thread->sp -= 8;
		int align = 8;
		if (len % align > 0)				// if unaligned
			thread->sp -= align - (len % align);	// waste some space
		thread->sp -= len;
		void *am_addr = (void *)translate_stack_addr(thread->sp, thread->tid);
		memcpy(am_addr, data, len);
		thread->r[0] = thread->sp;
	} else {
		thread->r[0] = 0;
	}
	thread->status = READY;

	add_tcb_element(thread);

	return 0;
}

int process_create(void *(*fptr)(void *), void *data, size_t len, void *(*t_term))
{
	// allocate thread and appant to runqueue
	int thread_id = allocate_tcb_element();
	if (thread_id == -1) {
		printf("process_create() error: thread limit reached\n");
		return -1;
	}

	// initiate found element
	tcb_t *thread = &(tcb[thread_id]);

	reset_tcb_element(thread_id);
	thread->pc = (unsigned int)fptr;
	thread->lr = (unsigned int)t_term;

	// set pid
	int pid;
	for (pid = 0; pid < _pcb_size; ++pid) {
		if (pcb[pid].status == INACTIVE) {
			thread->pid = pid;
			pcb[pid].status = ACTIVE;
			break;
		}
	}
	if (thread->pid == -1) {
		printf("process_create() error: process limit reached\n");
		return -1;
	}

	// allocate stack
	thread->sp = (unsigned int)allocate_thread_stack(thread->pid, thread_id);

	if (data != NULL) {
		thread->sp -= 8;
		int align = 8;
		if (len % align > 0)				// if unaligned
			thread->sp -= align - (len % align);	// waste some space
		thread->sp -= len;
		void *am_addr = (void *)translate_stack_addr(thread->sp, thread->tid);
		memcpy(am_addr, data, len);
		thread->r[0] = thread->sp;
	} else {
		thread->r[0] = 0;
	}
	thread->status = READY;

	add_tcb_element(thread);

	return 0;
}

void get_tid(context_t *curr_context)
{
	curr_context->r[0] = runqueue->tid;
}

unsigned int get_tid_k()
{
	return runqueue->tid;
}

void get_pid(context_t *curr_context)
{
	curr_context->r[0] = runqueue->pid;
}

void thread_wait(int mode, int value)
{
	if (runqueue != NULL) {
		runqueue->status = WAITING;
		runqueue->wait_mode = mode;
		runqueue->wait_value = value;
	}
}

/**
 * simulates timer clock for thread which are waiting in WAIT_FOR_COUNTER mode.
 * if wait value less than 0 thread is set to READY
 */
void thread_wait_clock()
{
	tcb_t *iter_ptr = runqueue;
	while (iter_ptr != NULL) {
		if (iter_ptr->status == WAITING \
			&& iter_ptr->wait_mode == WAIT_FOR_COUNTER) {
			if (iter_ptr->wait_value > 0)
				iter_ptr->wait_value -= 1;
			else
				iter_ptr->status = READY;
		}
		iter_ptr = iter_ptr->next;
	}
}

/**
 * sets first foundend thread in runqueue which is waiting in
 * WAIT_FOR_UART mode to READY
 */
void thread_awake_for_uart()
{
	tcb_t *iter_ptr = runqueue;
	while (iter_ptr != NULL) {
		if (iter_ptr->status == WAITING \
			&& iter_ptr->wait_mode == WAIT_FOR_UART) {
			iter_ptr->status = READY;
			unsigned char letter;
			uart_getc(&letter);
			(iter_ptr->r[0]) = letter;
			break;
		}
		iter_ptr = iter_ptr->next;
	}
}

void awake_for_channel_send(unsigned int id)
{
	tcb_t *iter_ptr = runqueue;
	while (iter_ptr != NULL) {
		if (iter_ptr->status == WAITING \
			&& iter_ptr->wait_mode == WAIT_FOR_CHANNEL_READ \
			&& iter_ptr->wait_value == id) {
			break;
		}
		iter_ptr = iter_ptr->next;
	}
	if (iter_ptr != NULL) {		// element found
		uint8_t *base_address = (uint8_t *)translate_stack_addr(iter_ptr->r[1], iter_ptr->tid);
		size_t offset = (iter_ptr->r[0]) ? (iter_ptr->r[0]-1) :  (iter_ptr->r[0]);
		iter_ptr->r[0] = channel_send((int)id, base_address + offset, (uint32_t)(iter_ptr->r[2])); 
		if (iter_ptr->r[0] == (iter_ptr->r[2])) {
			iter_ptr->status = READY;
		}
	} 
}

void awake_for_channel_read(unsigned int id)
{
	tcb_t *iter_ptr = runqueue;
	while (iter_ptr != NULL) {
		if (iter_ptr->status == WAITING \
			&& iter_ptr->wait_mode == WAIT_FOR_CHANNEL_SEND \
			&& iter_ptr->wait_value == id) {
			break;
		}
		iter_ptr = iter_ptr->next;
	}
	if (iter_ptr != NULL) {		// element found
		uint8_t *base_address = (uint8_t *)translate_stack_addr(iter_ptr->r[1], iter_ptr->tid);
		size_t offset = (iter_ptr->r[0]) ? (iter_ptr->r[0]-1) :  (iter_ptr->r[0]);
		iter_ptr->r[0] = channel_read((int)id, base_address + offset, (uint32_t)(iter_ptr->r[2])); 
		if (iter_ptr->r[0] == (iter_ptr->r[2])) {
			iter_ptr->status = READY;
		}
	} 
}

void thread_terminate()
{
#ifdef DEBUG
	printf("\nTERMINATED\n");
#endif
	if (runqueue != NULL)
		runqueue->status = TERMINATED;

	// clear thread stack allocation in process tsa_table
	int pid = runqueue->pid;
	int tid = runqueue->tid;
	int entry;
	for (entry = 0; entry < _tcb_size; ++entry) {
		if (pcb[pid].tsa_table[entry] == tid) {
			pcb[pid].tsa_table[entry] = -1;
			break;
		}
	}

	// check if process needs to be set to INACTIVE and do so
	for (entry = 0; entry < _tcb_size; ++entry) {
		if (pcb[pid].tsa_table[entry] != -1)
			break;
	}
	if (entry == _tcb_size) // loop ran until array end - process empty
		reset_pcb_element(pid);

	return;
}

static void idle_thread()
{
#ifdef DEBUG
	printf("IDLE\n");
#endif
	while (1);
}

static void context_copy(context_t *dst, context_t *src)
{
	int i;
	dst->cpsr = src->cpsr;
	dst->lr = src->lr;
	dst->sp = src->sp;
	for (i = 0; i < 13; ++i)
		dst->r[i] = src->r[i];
	dst->pc= src->pc;
}

static void context_copy_tcb(context_t *dst, tcb_t *src)
{
	int i;
	dst->cpsr = src->cpsr;
	dst->lr = src->lr;
	dst->sp = src->sp;
	for (i = 0; i < 13; ++i)
		dst->r[i] = src->r[i];
	dst->pc= src->pc;
}

static void tcb_copy_context(tcb_t *dst, context_t *src)
{
	int i;
	dst->cpsr = src->cpsr;
	dst->lr = src->lr;
	dst->sp = src->sp;
	for (i = 0; i < 13; ++i)
		dst->r[i] = src->r[i];
	dst->pc= src->pc;
}

void scheduler(context_t *curr_context)
{
#ifdef DEBUG
	printf("\n");
	printf(" <>-><-<>-><-<>-><-<>-><-<>-><-<>-><-<>-><-<>-><-<>-><-<>\n");
	printf(" <> scheduler - - -\n");
	printf("\n");
	printf("pre:\n");
	print_tcb_queue();
	printf("\n");
#endif

	// check for inconsistency
	tcb_t *curr = runqueue;
	if (curr != NULL) {
		curr = curr->next;
		while (curr != NULL) {
			if (curr->status == RUNNING)
				printf("scheduler() error: non-head elem running\n");
			if (curr->status == TERMINATED)
				printf("scheduler() error: non-head elem terminated\n");
			curr = curr->next;
		}
	}

	if (runqueue == NULL) { 		// runqeue is empty
		if (curr_context->lr != (unsigned int)&idle_thread)
			if (context_change_print_f)
				printf("\n");
		context_copy(curr_context, &idle_context);
		idle_running = 1;
	} else {				// runqueue not empty
		// remove possibly terminated thread from queue
		if (runqueue->status == TERMINATED) {
			remove_tcb_head_element(runqueue);
		} else if (!idle_running) {
			// save context in case idle thread is currently not running
			tcb_copy_context(runqueue, curr_context);
			if (runqueue->status == RUNNING)
				runqueue->status = READY;
			else if (runqueue->status == WAITING)
				runqueue->status = WAITING;
			rotate_tcb_queue();
		}

		int index = index_of_first_ready();
		if (index >= 0) {	// thread found - rotate to index
			for (; index > 0; --index)
				rotate_tcb_queue();
			context_copy_tcb(curr_context, runqueue);
			// update l1_table
			int pid = runqueue->pid;
			int *tsa = pcb[pid].tsa_table;
			update_logical_address_space(tsa, pid);

			runqueue->status = RUNNING;
			idle_running = 0;
			if (context_change_print_f)
				printf("\n");
		} else {		// not found - run idle thread
			context_copy(curr_context, &idle_context);
			idle_running = 1;
		}
	}
#ifdef DEBUG
	printf("post\n");
	print_tcb_queue();
	printf("\n");
	printf(" <> scheduler - - -\n");
	printf(" <>-><-<>-><-<>-><-<>-><-<>-><-<>-><-<>-><-<>-><-<>-><-<>\n");
	printf("\n");
#endif

}
