#include "../lib/initstd.h"

// DACR bits
#define CLIENT 0b01

.section .init

.global _start
_start:
	// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
	// + mmu

	// disable data and instruction caches
	mrc	p15, 0, r0, c1, c0, 0	// r0 = SCTLR
	// clear bit 2 and bit 12
	mvn	r1, #(1 << 2)		// r1 = ~(1 << 2)
	and	r0, r0, r1		// r0 = r0 & r1
	mvn	r1, #(1 << 12)		// r1 = ~(1 << 12)
	and	r0, r0, r1		// r0 = r0 & r1
	// write back
	mcr	p15, 0, r0, c1, c0, 0	// SCTLR = r0

	// create l1 table
	bl	create_l1_table
	bl	print_l1_table

	// propagate l1 table base adress (TTBR0)
	ldr	r0, =l1_table
	mcr	p15, 0, r0, c2, c0, 0

	// set domain_0 access control to client (DACR)
	mov	r0, #CLIENT
	mcr	p15, 0, r0, c3, c0, 0

	// using short descriptor format  (TTBCR)
	mov	r0, #0
	mcr	p15, 0, r0, c2, c0, 2

	// activate mmu (SCTLR)
	mov	r0, #1
	mcr	p15, 0, r0, c1, c0, 0

	mrc	p15, 0, r8, c1, c0, 0	// r0 = SCTLR

	// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
	// + cpu mode specific stacks

	// init stack pointer for all processor modes
	// we start in svc mode
	ldr	sp, =_svc_sp
	mov	r0, sp

	cps	#_fiq_mode
	ldr	sp, =_fiq_sp
	mov	r1, sp

	cps	#_irq_mode
	ldr	sp, =_irq_sp
	mov	r2, sp

	cps	#_abt_mode
	ldr	sp, =_abt_sp
	mov	r3, sp

	cps	#_und_mode
	ldr	sp, =_und_sp
	mov	r4, sp

	cps	#_sys_mode
	ldr	sp, =_sys_sp
	mov	r5, sp

	// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
	// + interrupt vector table

	ldr	r6, =_ivt_start
	mcr	p15, 0, r6, c12, c0, 0 	// init VBAR

	cps	#_svc_mode

	// enable interrupts arm kern
	cpsie	if

	// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
	// start kernel

	// reg init fo testing
	mov	r0, #42
	mov	r1, #11
	mov 	r2, #22
	mov	r3, #33
	mov	r4, #44
	mov	r5, #55
	mov	r6, #66
	mov	r7, #77
	//mov	r8, #88
	mov	r9, #99
	mov	r10, #110
	mov	r11, #111
	mov	r12, #122

	bl	kernel_main

.Lend:
	b       .Lend
