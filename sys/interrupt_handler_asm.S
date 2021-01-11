#include "../lib/initstd.h"

.section .data

.global _create_reg_dump_error
_create_reg_dump_error:  .ascii  "_create_reg_dump error: mode [%u] not supported\n"
.global _debug_val
_debug_val: .ascii	"_debug_val: %u\n"
.global _debug_adr
_debug_adr: .ascii	"_debug_adr: %p\n"

.section .text

.global _ivt_start
.balign 32
_ivt_start:
	b	_reset
	b	_undefined
	b	_software_interrupt
	b	_prefetch_abort
	b	_data_abort
	nop
	b	_irq
	b	_fiq

_reset:
	bl	_reset_print
	b	.Lend

_create_reg_dump:
	// dfar and dfsr
	mrc	p15, 0, r1, c6, c0, 0
	mrc	p15, 0, r2, c5, c0, 0
	push	{r1-r2}

	// cpsr and spsr
	mrs	r1, cpsr
	mrs	r2, spsr
	push	{r1-r2}
//if
	cmp	r0, #_und_mode
	bne	elif
						// case undefined mode start
	// undefined lr and sp
	mov	r1, lr
	mov	r2, sp
	push	{r1-r2}

	// irq lr and sp
	cps	#_irq_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_und_mode
	push	{r1-r2}

	// fiq lr and sp
	cps	#_fiq_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_und_mode
	push	{r1-r2}

	// abort lr and sp
	cps	#_abt_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_und_mode
	push	{r1-r2}

	// supervisor lr and sp
	cps	#_svc_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_und_mode
	push	{r1-r2}

	// user/system lr and sp
	cps	#_sys_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_und_mode
	push	{r1-r2}
						// case undefined mode end
	b	endif
elif:
	cmp	r0, #_abt_mode
	bne	else
						// case data abort mode start
	// undefined lr and sp
	cps	#_und_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_abt_mode
	push	{r1-r2}

	// irq lr and sp
	cps	#_irq_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_abt_mode
	push	{r1-r2}

	// fiq lr and sp
	cps	#_fiq_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_abt_mode
	push	{r1-r2}

	// abort lr and sp
	mov	r1, lr
	mov	r2, sp
	push	{r1-r2}

	// supervisor lr and sp
	cps	#_svc_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_abt_mode
	push	{r1-r2}

	// user/system lr and sp
	cps	#_sys_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_abt_mode
	push	{r1-r2}
						// case data abort mode end
	b	endif
else:
	mov	r1, r0
	ldr	r0, =_create_reg_dump_error
	bl	printf
endif:
	mov	pc, lr

_undefined:
	// register snapshot
	stmfd	sp, {r0-r12, sp, lr, pc}
	sub	sp, sp, #(16*4)

	// dfar and dfsr
	mrc	p15, 0, r1, c6, c0, 0
	mrc	p15, 0, r2, c5, c0, 0
	push	{r1-r2}

	// cpsr and spsr
	mrs	r1, cpsr
	mrs	r2, spsr
	push	{r1-r2}

	// undefined lr and sp
	mov	r1, lr
	mov	r2, sp
	push	{r1-r2}

	// irq lr and sp
	cps	#_irq_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_und_mode
	push	{r1-r2}

	// fiq lr and sp
	cps	#_fiq_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_und_mode
	push	{r1-r2}

	// abort lr and sp
	cps	#_abt_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_und_mode
	push	{r1-r2}

	// supervisor lr and sp
	cps	#_svc_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_und_mode
	push	{r1-r2}

	// user/system lr and sp
	cps	#_sys_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_und_mode
	push	{r1-r2}

	// configure struct-pointer for function call
	mov	r0, sp

	bl	_undefined_print

	b	.Lend

_software_interrupt:
	// save context from current thread
	push	{r0-r12, lr}	// save r0 to r12 and pc from where we came
	cps	#_sys_mode
	mov	r0, lr
	mov	r1, sp
	cps	#_svc_mode
	push	{r0-r1}		// save lr and sp from where we came
	mrs	r0, spsr
	push	{r0}		// save cpsr from where we came

	ldr	r0, [lr,#-4]
	mrs	r1, cpsr
	mov	r2, sp	// curr_context

	bl	_software_interrupt_handler

	mcr	p15, 0, r0, c8, c7, 0	// invalidate data-tlb

	// restore context - may have been changed due to scheduling
	pop	{r0}		// restore cpsr
	msr	spsr, r0
	pop	{r0-r1}		// restore lr and sp
	cps	#_sys_mode
	mov	lr, r0
	mov	sp, r1
	cps	#_svc_mode
	pop	{r0-r12, lr}	// restore r0 to r12 and pc

	subs	pc, lr, #0

_prefetch_abort:
	// register snapshot
	stmfd	sp, {r0-r12, sp, lr, pc}
	sub	sp, sp, #(16*4)

	// ifar and ifsr
	mrc	p15, 0, r1, c6, c0, 2
	mrc	p15, 0, r2, c5, c0, 1
	push	{r1-r2}

	// cpsr and spsr
	mrs	r1, cpsr
	mrs	r2, spsr
	push	{r1-r2}

	// undefined lr and sp
	cps	#_und_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_abt_mode
	push	{r1-r2}

	// irq lr and sp
	cps	#_irq_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_abt_mode
	push	{r1-r2}

	// fiq lr and sp
	cps	#_fiq_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_abt_mode
	push	{r1-r2}

	// abort lr and sp
	mov	r1, lr
	mov	r2, sp
	push	{r1-r2}

	// supervisor lr and sp
	cps	#_svc_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_abt_mode
	push	{r1-r2}

	// user/system lr and sp
	cps	#_sys_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_abt_mode
	push	{r1-r2}

	// configure struct-pointer for function call
	mov	r0, sp

	bl	_prefetch_abort_print

	b	.Lend

_data_abort:
	// register snapshot
	stmfd	sp, {r0-r12, sp, lr, pc}
	sub	sp, sp, #(16*4)

	// dfar and dfsr
	mrc	p15, 0, r1, c6, c0, 0
	mrc	p15, 0, r2, c5, c0, 0
	push	{r1-r2}

	// cpsr and spsr
	mrs	r1, cpsr
	mrs	r2, spsr
	push	{r1-r2}

	// undefined lr and sp
	cps	#_und_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_abt_mode
	push	{r1-r2}

	// irq lr and sp
	cps	#_irq_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_abt_mode
	push	{r1-r2}

	// fiq lr and sp
	cps	#_fiq_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_abt_mode
	push	{r1-r2}

	// abort lr and sp
	mov	r1, lr
	mov	r2, sp
	push	{r1-r2}

	// supervisor lr and sp
	cps	#_svc_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_abt_mode
	push	{r1-r2}

	// user/system lr and sp
	cps	#_sys_mode
	mov	r1, lr
	mov	r2, sp
	cps	#_abt_mode
	push	{r1-r2}

	// configure struct-pointer for function call
	mov	r0, sp

	bl	_data_abort_print

	b	.Lend

_irq:
	// save context from current thread
	sub	lr, lr, #4
	push	{r0-r12, lr}	// save r0 to r12 and pc from where we came
	cps	#_sys_mode
	mov	r0, lr
	mov	r1, sp
	cps	#_irq_mode
	push	{r0-r1}		// save lr and sp from where we came
	mrs	r0, spsr
	push	{r0}		// save cpsr from where we came

	mov	r0, sp
	bl	_irq_handler

	mcr	p15, 0, r0, c8, c7, 0	// invalidate data-tlb

	// restore context - may have been changed due to scheduling
	pop	{r0}		// restore cpsr
	msr	spsr, r0
	pop	{r0-r1}		// restore lr and sp
	cps	#_sys_mode
	mov	lr, r0
	mov	sp, r1
	cps	#_irq_mode
	pop	{r0-r12, lr}	// restore r0 to r12 and pc

	subs	pc, lr, #0

_fiq:
	bl	_fiq_print
	b	.Lend

.Lend:
	b	.Lend
