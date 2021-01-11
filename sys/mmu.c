#include "mmu.h"

#define L1_SIZE 4096

// L1 table entry bits
#define SEC	0b10   	<< 0		// set format to section
#define DOM_0	0b0000 	<< 5		// set domain 0
#define NA	0b00000000 << 10	// no Access
#define SA	0b00000001 << 10	// system access
#define SROA	0b10000001 << 10	// system read-only access
#define UROA	0b00000010 << 10	// user read-only access
#define FA	0b00000011 << 10	// Full Access

// permission enum
#define AP_NA   0	// no Access
#define AP_SA   1	// system access
#define AP_SROA 2	// system read-only access
#define AP_UROA 3	// user read-only access
#define AP_FA   4	// full Access

#define ENTRY_ADDR_OFFSET 20
#define MB_PAGE 1
#define MiB 0x100000
#define USPACE_START 0x004
#define PSTACK_BASE 0x00c
#define LSTACK_BASE 0x005
#define PHEAP_BASE 0x004
#define LHEAP_ADDR 0x004
#define LOWER_BITS 0x000fffff
#define SMAP_OFFSET (0x1 << 28)


unsigned int l1_table[L1_SIZE] __attribute__((aligned(L1_SIZE*4)));

static void fill_in_range(size_t *ec, int amount, int permission, size_t map_to)
{
	int entry_bits = 0;
	switch(permission) {
	case 0:					// no access
		entry_bits = NA | DOM_0 | SEC;
		break;
	case 1:					// system access
		entry_bits = SA | DOM_0 | SEC;
		break;
	case 2:					// system read-only access
		entry_bits = SROA | DOM_0 | SEC;
		break;
	case 3:					// user read-only access
		entry_bits = UROA | DOM_0 | SEC;
		break;
	case 4:					// full access
		entry_bits = FA | DOM_0 | SEC;
		break;
	default:
		printf("fill_in_range() error");
		return;
	}

	size_t i;
	size_t end;
	if (amount == -1)
		end = L1_SIZE;
	else
		end = *ec + amount;
	for (i = *ec; i < end; ++i) {
		if (i >= L1_SIZE) // compiler hat sonst wegen dem "<<" gewarnt
			break;

		l1_table[i] = (map_to++ << ENTRY_ADDR_OFFSET) + entry_bits;
	}

	*ec = i;
}

void create_l1_table()
{
	size_t ec = 0x000;	// entry counter like '.' in linker script
	// no access for unallocated memory
	fill_in_range(&ec, -1, AP_NA, ec);

	ec = 0x001;

	// system read-only for kernel sections
	fill_in_range(&ec, 1, AP_SROA, ec);

	// system access for kernel stacks
	fill_in_range(&ec, 1, AP_SA, ec);

	// user read-only for user sections
	fill_in_range(&ec, 1, AP_UROA, ec);

	// system access for process stacks
	fill_in_range(&ec, 40, AP_SA, ec);

	ec = 0x104;

	// system acces remap for kernel access
	// to user space (mapping independed)
	fill_in_range(&ec, 40, AP_SA, 0x004);

	ec = 0x3f0;

	// system access for mem-maped-i/o
	fill_in_range(&ec, 3, AP_SA, ec);
}

void print_l1_table()
{
	printf("+=+=+=+=+=+=+=+=+=+=+=+=+   l1  table   +=+=+=+=+=+=+=+=+=+=+=+=+\n");
	int i;
	for (i = 0; i < L1_SIZE; ++i) {
		char *str = "N/A";

		unsigned int entry = l1_table[i];
		unsigned int permission_bitmask = 0b11111111 << 10;
		unsigned int permission = entry & permission_bitmask;

		switch(permission) {
		case NA:				// no access
			str = "no access";
			break;
		case SA:				// system access
			str = "system access";
			break;
		case SROA:				// system read-only access
			str = "system read-only access";
			break;
		case UROA:				// user read-only access
			str = "user read-only access";
			break;
		case FA:				// full access
			str = "full access";
			break;
		default:
			printf("print_l1_table() error\n");
			continue;
		}

		if (i < 6)
			printf("entry %#4i: %p | %s\n", i, l1_table[i], str);
		if (i == 6)
			printf("[...]\n");
		if (i >= 42 && i < 46)
			printf("entry %#4i: %p | %s\n", i, l1_table[i], str);
		if (i == 46)
			printf("[...]\n");
		if (i >= 258 && i < 262)
			printf("entry %#4i: %p | %s\n", i, l1_table[i], str);
		if (i == 262)
			printf("[...]\n");
		if (i >= 298 && i < 302)
			printf("entry %#4i: %p | %s\n", i, l1_table[i], str);
		if (i == 302)
			printf("[...]\n");
		if (i >= 1006 && i < 1013)
			printf("entry %#4i: %p | %s\n", i, l1_table[i], str);
		if (i == 1013)
			printf("[...]\n");
		if (i >= 4094)
			printf("entry %#4i: %p | %s\n", i, l1_table[i], str);
	}
	printf("+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+\n\n");
}

void create_mapping_FA(size_t l_addr, size_t p_addr)
{
	int entry_bits = FA | DOM_0 | SEC;
	l1_table[l_addr] = (p_addr << ENTRY_ADDR_OFFSET) + entry_bits;
}

void update_logical_address_space(int *tsa_table, int pid)
{
	size_t ec = USPACE_START; 
	fill_in_range(&ec, 40, AP_SA, ec);

	// heap mapping
	size_t p_addr = PHEAP_BASE + (pid * MB_PAGE);
	size_t l_addr = LHEAP_ADDR;
	create_mapping_FA(l_addr, p_addr);

	// stacks mapping
	int i;
	for (i=0; i<_tcb_size; i++) {
		int tid = tsa_table[i];
		if (tid < 0)
			continue;
		p_addr = PSTACK_BASE + (tid * MB_PAGE);
		l_addr = LSTACK_BASE + (i * MB_PAGE);
		//printf(">>->tsa_table[%i]:%ipa:%i\tla:%i\n", i, tid, p_addr, l_addr);
		create_mapping_FA(l_addr, p_addr);
	}
}

unsigned int translate_stack_addr(unsigned int addr, int tid)
{
	unsigned int upper = SMAP_OFFSET + ((tid + PSTACK_BASE) * MiB);
	unsigned int lower = addr & LOWER_BITS;

	//printf("addr:%p\tam_addr:%p\n", addr, upper + lower);

	return upper + lower;
}
