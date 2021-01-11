#include <stddef.h>

#include "../driver/interrupt_timer.h"
#include "../driver/interrupt_registers.h"
#include "../driver/uart.h"
#include "../lib/initstd.h"
#include "../lib/printf.h"
#include "../lib/flags.h"
#include "../sys/thread_control.h"
#include "../sys/channels.h"

//#define DEBUG

#define USR "User"
#define FIQ "Fast Interrupt"
#define IRQ "Interrupt"
#define SVC "Supervisor"
#define ABT "Abort"
#define UND "Undefined"
#define SYS "System"
#define ERR "N/A - fault status: "

void _thread_terminate_abort();
void _thread_terminate_undefined();

struct dump_t {
	int system[2];
	int supervisor[2];
	int abort[2];
	int fiq[2];
	int irq[2];
	int undefined[2];
	int cpsr;
	int spsr;
	int far;
	int fsr;
	int r[13];
	int sp;
	int lr;
	int pc;
};

struct dump2_t {
	int system[2];
	int supervisor[2];
	int abort[2];
	int fiq[2];
	int irq[2];
	int undefined[2];
	int cpsr;
	int spsr;
};

void _reset_print()
{
	printf("_reset_print()\n");
}

char *_parse_psr_flags(int psr)
{
	static char flags[9];
	flags[0] = (psr & (1 << 31)) ? 'N' : '_';
	flags[1] = (psr & (1 << 30)) ? 'Z' : '_';
	flags[2] = (psr & (1 << 29)) ? 'C' : '_';
	flags[3] = (psr & (1 << 28)) ? 'V' : '_';
	flags[4] = ' ';
	flags[5] = (psr & (1 <<  7)) ? 'I' : '_';
	flags[6] = (psr & (1 <<  6)) ? 'F' : '_';
	flags[7] = (psr & (1 <<  5)) ? 'T' : '_';
	flags[8] = '\0';

	return flags;
}

char *_parse_psr_modus(int psr)
{
	static char *modus;
	switch (psr & 0b11111) {
	case _usr_mode:
		modus = USR;
		break;
	case _fiq_mode:
		modus = FIQ;
		break;
	case _irq_mode:
		modus = IRQ;
		break;
	case _svc_mode:
		modus = SVC;
		break;
	case _abt_mode:
		modus = ABT;
		break;
	case _und_mode:
		modus = UND;
		break;
	case _sys_mode:
		modus = SYS;
		break;
	default:
		modus = ERR;
		break;
	}

	return modus;
}

void _undefined_print(struct dump_t *dump)
{
	// process cpsr
	char *cpsr_flags = _parse_psr_flags(dump->cpsr);
	char *cpsr_modus = _parse_psr_modus(dump->cpsr);

	// process spsr
	char *spsr_flags = _parse_psr_flags(dump->spsr);
	char *spsr_modus = _parse_psr_modus(dump->spsr);

	printf("############################################################");
	printf("###############\n");
	printf("Undefined Instruction an Adresse %p\n", dump->lr-4);
	printf("\n");
	printf(">>> Registerschnappschuss (aktueller Modus) <<<\n");
	printf("R0: %p    R8:  %p\n", dump->r[0], dump->r[8]);
	printf("R1: %p    R9:  %p\n", dump->r[1], dump->r[9]);
	printf("R2: %p    R10: %p\n", dump->r[2], dump->r[10]);
	printf("R3: %p    R11: %p\n", dump->r[3], dump->r[11]);
	printf("R4: %p    R12: %p\n", dump->r[4], dump->r[12]);
	printf("R5: %p    SP:  %p\n", dump->r[5], dump->sp);
	printf("R6: %p    LR:  %p\n", dump->r[6], dump->lr);
	printf("R7: %p    PC:  %p\n", dump->r[7], dump->pc);
	printf("\n");
	printf(">>> Aktuelle Statusregister (SPSR des aktuellen Modus) <<<\n");
	printf("CPSR: %s %s\t(%p)\n", cpsr_flags, cpsr_modus, dump->cpsr);
	printf("SPSR: %s %s\t(%p)\n", spsr_flags, spsr_modus, dump->spsr);
	printf("\n");
	printf(">>> Aktuelle modusspezifischer Register (außer SPSR und R8-R");
	printf("12) <<<\n");
	printf("\t\tLR\t   SP\n");
	printf("User/System:\t%p %p\n", dump->system[0], dump->system[1]);
	printf("Supervisor:\t%p %p\n", dump->supervisor[0],dump->supervisor[1]);
	printf("Abort:\t\t%p %p\n", dump->abort[0], dump->abort[1]);
	printf("FIQ:\t\t%p %p\n", dump->fiq[0], dump->fiq[1]);
	printf("IRQ:\t\t%p %p\n", dump->irq[0], dump->irq[1]);
	printf("Undefined:\t%p %p\n", dump->undefined[0], dump->undefined[1]);
	printf("\n");

	// return system halt boolean
	unsigned mode_bitmask = 0b11111;
	if ((dump->spsr & mode_bitmask) == _usr_mode) {
		printf("##########################################################");
		printf("#################\n\n");
		_thread_terminate_undefined();
	} else {
		printf("System angehalten.\n");
		return;
	}
}

void _software_interrupt_handler(unsigned int svc_inst, unsigned error_cpsr, context_t *curr_context)
{
	// convert svc instruction to svc value
	unsigned int svc_val = svc_inst & 0x00ffffff;

#ifdef DEBUG
	printf(">> svc_val: %u\n", svc_val);
#endif

	unsigned int id;
	unsigned mode_bitmask = 0b11111;
	if ((curr_context->cpsr & mode_bitmask) == _usr_mode) {
		switch (svc_val) {
		// basic syscalls
		case 11:		// get character
			;
			int rv = uart_getc((unsigned char *)(&curr_context->r[0]));
			if (rv == -1) {
				thread_wait(WAIT_FOR_UART, 0);
			}
			scheduler(curr_context);
			break;
		case 12:		// put character
			uart_putc(curr_context->r[0]);
			break;

		// thread syscalls
		case 21:		// start thread
			thread_create((void *)(curr_context->r[0]), \
					(void *)(curr_context->r[1]), \
					(size_t)(curr_context->r[2]), \
					(void *)(curr_context->r[3]));
			break;
		case 22:		// terminate thread
			thread_terminate();
			scheduler(curr_context);
			break;
		case 23:		// hibernate thread
			thread_wait(WAIT_FOR_COUNTER, curr_context->r[1]);
			scheduler(curr_context);
			break;
		case 24:		// start process 
			process_create((void *)(curr_context->r[0]), \
					(void *)(curr_context->r[1]), \
					(size_t)(curr_context->r[2]), \
					(void *)(curr_context->r[3]));
			break;
		case 25:
			get_tid(curr_context);
			break;
		case 26:
			get_pid(curr_context);
			break;
		case 27:
			channel_open((int)(curr_context->r[0]));
			break;
		case 28:		// channel send
			id = curr_context->r[0];
			curr_context->r[0] = channel_send((int)(curr_context->r[0]), \
							(uint8_t *)(curr_context->r[1]), \
							(uint32_t)(curr_context->r[2]));
			if (curr_context->r[0] < (curr_context->r[2])) {
				thread_wait(WAIT_FOR_CHANNEL_READ, id);	
			}
			awake_for_channel_read(id);
			scheduler(curr_context);
			break;
		case 29:		// channel read
			id = curr_context->r[0];
			curr_context->r[0] = channel_read((int)(curr_context->r[0]), \
							(uint8_t *)(curr_context->r[1]), \
							(uint32_t)(curr_context->r[2]));
			if (curr_context->r[0] < (curr_context->r[2])) {
				thread_wait(WAIT_FOR_CHANNEL_SEND, id);
			}
			awake_for_channel_send(id);
			scheduler(curr_context);
			break;
		default:
			printf("supervisor call error: %u not defined", svc_val);
			break;
		}
	} else {
		// process cpsr
		char *cpsr_flags = _parse_psr_flags(error_cpsr);
		char *cpsr_modus = _parse_psr_modus(error_cpsr);

		// process spsr
		char *spsr_flags = _parse_psr_flags(curr_context->cpsr);
		char *spsr_modus = _parse_psr_modus(curr_context->cpsr);

		printf("\n");
		printf("############################################################");
		printf("###############\n");
		printf("Software Interrupt vom Kernel\n");
		printf("\n");
		printf(">>> Registerschnappschuss (aktueller Modus) <<<\n");
		printf("R0: %p    R8:  %p\n", curr_context->r[0], curr_context->r[8]);
		printf("R1: %p    R9:  %p\n", curr_context->r[1], curr_context->r[9]);
		printf("R2: %p    R10: %p\n", curr_context->r[2], curr_context->r[10]);
		printf("R3: %p    R11: %p\n", curr_context->r[3], curr_context->r[11]);
		printf("R4: %p    R12: %p\n", curr_context->r[4], curr_context->r[12]);
		printf("R5: %p    SP:  %p\n", curr_context->r[5], curr_context->sp);
		printf("R6: %p    LR:  %p\n", curr_context->r[6], curr_context->lr);
		printf("R7: %p    PC:  %p\n", curr_context->r[7], curr_context->pc);
		printf("\n");
		printf(">>> Aktuelle Statusregister (SPSR des aktuellen Modus) <<<\n");
		printf("CPSR: %s %s\t(%p)\n", cpsr_flags, cpsr_modus, error_cpsr);
		printf("SPSR: %s %s\t(%p)\n", spsr_flags, spsr_modus, curr_context->cpsr);
		printf("\n");

		printf("System angehalten.\n");
		// halt system
		while (1);
	}
}

int switch_case_of_doom(int fsr, char **data_fault)
{
	int level = -1;
	switch (fsr & 0b111111) {
	case 0b000111:	// translation fault [third level]
		level++;
	case 0b000110:	// translation fault [second level]
		level++;
	case 0b000101:	// translation fault [first level]
		level++;
	case 0b000100:	// translation fault [reserved]
		level++;
		*data_fault = "Translation fault on level ";
		break;

	case 0b001011:	// access flag fault [third level]
		level++;
	case 0b001010:	// access flag fault [second level]
		level++;
	case 0b001001:	// access flag fault [first level]
		level++;
	case 0b001000:	// access flag fault [reserved]
		level++;
		*data_fault = "Access flag fault on level ";
		break;

	case 0b001111:	// permission fault [third level]
		level++;
	case 0b001110:	// permission fault [second level]
		level++;
	case 0b001101:	// permission fault [first level]
		level++;
	case 0b001100:	// permission fault [reserved]
		level++;
		*data_fault = "Permission fault on level ";
		break;

	case 0b010000:	// synchronous external abort
		*data_fault = "Synchronous external abort";
		break;
	case 0b011000:	// synchronous parity error on memory access
		*data_fault = "Synchronous parity error on memory access";
		break;
	case 0b010001:	// asynchronous external abort
		*data_fault = "Asynchronous external abort";
		break;
	case 0b011001:	// asynchronous parity error on memory access
		*data_fault = "Asynchronous parity error on memory access";
		break;

	case 0b010111:	// synchronous external on translation table walk [third level]
		level++;
	case 0b010110:	// synchronous external on translation table walk [second level]
		level++;
	case 0b010101:	// synchronous external on translation table walk [first level]
		level++;
	case 0b010100:	// synchronous external on translation table walk [reserved]
		level++;
		*data_fault = "Synchronous external on translation table walk on level ";
		break;

	case 0b011111:	// synchronous parity error on memory access on translation table walk [third level]
		level++;
	case 0b011110:	// synchronous parity error on memory access on translation table walk [second level]
		level++;
	case 0b011101:	// synchronous parity error on memory access on translation table walk [first level]
		level++;
	case 0b011100:	// synchronous parity error on memory access on translation table walk [reserved]
		level++;
		*data_fault = "Synchronous parity error on memory access on translation table walk on level ";
		break;

	case 0b100001:	// alignment fault
		*data_fault = "Alignment fault";
		break;
	case 0b100010:	// debug event
		*data_fault = "Debug event";
		break;
	case 0b110000:	// TLB conflict abort
		*data_fault = "TLB conflict abort";
		break;
	case 0b110100:	// IMPLEMENTATION DEFINED
		*data_fault = "IMPLEMENTATION DEFINED";
		break;
	case 0b111010:	// IMPLEMENTATION DEFINED
		*data_fault = "IMPLEMENTATION DEFINED";
		break;

	case 0b111111:	// domain fault [third level]
		level++;
	case 0b111110:	// domain fault [second level]
		level++;
	case 0b111101:	// domain fault [first level]
		level++;
	case 0b111100:	// domain fault [reserved]
		level++;
		*data_fault = "Domain fault on level ";
		break;

	default:
		*data_fault = ERR;
		level = fsr & 0b111111;
		break;
	}

	return level;
}

void _prefetch_abort_print(struct dump_t *dump)
{
	// process cpsr
	char *cpsr_flags = _parse_psr_flags(dump->cpsr);
	char *cpsr_modus = _parse_psr_modus(dump->cpsr);

	// process spsr
	char *spsr_flags = _parse_psr_flags(dump->spsr);
	char *spsr_modus = _parse_psr_modus(dump->spsr);

	// process intruction fault status register
	char *data_fault;
	int level = switch_case_of_doom(dump->fsr, &data_fault);

	printf("############################################################");
	printf("###############\n");
	printf("Prefetch abort an Adresse %p\n", dump->far);
	if (level > -1)
		printf("Fehler: %s%u\n", data_fault, level);
	else
		printf("Fehler: %s\n", data_fault);
	printf("\n");
	printf(">>> Registerschnappschuss (aktueller Modus) <<<\n");
	printf("R0: %p    R8:  %p\n", dump->r[0], dump->r[8]);
	printf("R1: %p    R9:  %p\n", dump->r[1], dump->r[9]);
	printf("R2: %p    R10: %p\n", dump->r[2], dump->r[10]);
	printf("R3: %p    R11: %p\n", dump->r[3], dump->r[11]);
	printf("R4: %p    R12: %p\n", dump->r[4], dump->r[12]);
	printf("R5: %p    SP:  %p\n", dump->r[5], dump->sp);
	printf("R6: %p    LR:  %p\n", dump->r[6], dump->lr);
	printf("R7: %p    PC:  %p\n", dump->r[7], dump->pc);
	printf("\n");
	printf(">>> Aktuelle Statusregister (SPSR des aktuellen Modus) <<<\n");
	printf("CPSR: %s %s\t(%p)\n", cpsr_flags, cpsr_modus, dump->cpsr);
	printf("SPSR: %s %s\t(%p)\n", spsr_flags, spsr_modus, dump->spsr);
	printf("\n");
	printf(">>> Aktuelle modusspezifischer Register (außer SPSR und R8-R");
	printf("12) <<<\n");
	printf("\t\tLR\t   SP\n");
	printf("User/System:\t%p %p\n", dump->system[0], dump->system[1]);
	printf("Supervisor:\t%p %p\n", dump->supervisor[0],dump->supervisor[1]);
	printf("Abort:\t\t%p %p\n", dump->abort[0], dump->abort[1]);
	printf("FIQ:\t\t%p %p\n", dump->fiq[0], dump->fiq[1]);
	printf("IRQ:\t\t%p %p\n", dump->irq[0], dump->irq[1]);
	printf("Undefined:\t%p %p\n", dump->undefined[0], dump->undefined[1]);
	printf("\n");

	// return system halt boolean
	unsigned mode_bitmask = 0b11111;
	if ((dump->spsr & mode_bitmask) == _usr_mode) {
		printf("####################################################");
		printf("#######################\n\n");
		_thread_terminate_abort();
	} else {
		printf("System angehalten.\n");
		return;
	}
}

void _data_abort_print(struct dump_t *dump)
{
	// process cpsr
	char *cpsr_flags = _parse_psr_flags(dump->cpsr);
	char *cpsr_modus = _parse_psr_modus(dump->cpsr);

	// process spsr
	char *spsr_flags = _parse_psr_flags(dump->spsr);
	char *spsr_modus = _parse_psr_modus(dump->spsr);

	// process data fault status register
	char *data_fault;
	int level = switch_case_of_doom(dump->fsr, &data_fault);

	printf("############################################################");
	printf("###############\n");
	printf("Data Abort an Adresse %p\n", dump->lr-8);
	printf("Zugriff: lesend auf Adresse %p\n", dump->far);
	if (level > -1)
		printf("Fehler: %s%u\n", data_fault, level);
	else
		printf("Fehler: %s\n", data_fault);
	printf("\n");
	printf(">>> Registerschnappschuss (aktueller Modus) <<<\n");
	printf("R0: %p    R8:  %p\n", dump->r[0], dump->r[8]);
	printf("R1: %p    R9:  %p\n", dump->r[1], dump->r[9]);
	printf("R2: %p    R10: %p\n", dump->r[2], dump->r[10]);
	printf("R3: %p    R11: %p\n", dump->r[3], dump->r[11]);
	printf("R4: %p    R12: %p\n", dump->r[4], dump->r[12]);
	printf("R5: %p    SP:  %p\n", dump->r[5], dump->sp);
	printf("R6: %p    LR:  %p\n", dump->r[6], dump->lr);
	printf("R7: %p    PC:  %p\n", dump->r[7], dump->pc);
	printf("\n");
	printf(">>> Aktuelle Statusregister (SPSR des aktuellen Modus) <<<\n");
	printf("CPSR: %s %s\t(%p)\n", cpsr_flags, cpsr_modus, dump->cpsr);
	printf("SPSR: %s %s\t(%p)\n", spsr_flags, spsr_modus, dump->spsr);
	printf("\n");
	printf(">>> Aktuelle modusspezifischer Register (außer SPSR und R8-R");
	printf("12) <<<\n");
	printf("\t\tLR\t   SP\n");
	printf("User/System:\t%p %p\n", dump->system[0], dump->system[1]);
	printf("Supervisor:\t%p %p\n", dump->supervisor[0],dump->supervisor[1]);
	printf("Abort:\t\t%p %p\n", dump->abort[0], dump->abort[1]);
	printf("FIQ:\t\t%p %p\n", dump->fiq[0], dump->fiq[1]);
	printf("IRQ:\t\t%p %p\n", dump->irq[0], dump->irq[1]);
	printf("Undefined:\t%p %p\n", dump->undefined[0], dump->undefined[1]);
	printf("\n");

	// return system halt boolean
	unsigned mode_bitmask = 0b11111;
	if ((dump->spsr & mode_bitmask) == _usr_mode) {
		printf("##########################################################");
		printf("#################\n\n");
		_thread_terminate_abort();
	} else {
		printf("System angehalten.\n");
		return;
	}
}

void _irq_handler(context_t *curr_context)
{
	if(timer_irq_pending()) {
		thread_wait_clock();
		scheduler(curr_context);
		interrupt_timer_ack();
	} else if (uart_irq_pending()) {
		uart_rx_irq_handler();	// write hardware fifo to software fifo
		thread_awake_for_uart();
		scheduler(curr_context);
		uart_rx_irq_ack();	// activate irq in uart register again
	}
}

void _fiq_print()
{
	printf("_fiq_print()\n");
}
