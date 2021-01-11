#include "../lib/initstd.h"

.section .text

.global _thread_terminate_abort
_thread_terminate_abort:
	// save context from current thread
	push	{r0-r12, lr}	// save r0 to r12 and pc from where we came
	cps	#_sys_mode
	mov	r0, lr
	mov	r1, sp
	cps	#_abt_mode
	push	{r0-r1}		// save lr and sp from where we came
	mrs	r0, spsr
	push	{r0}		// save cpsr from where we came

	mov	r0, sp	// curr_context

	push	{r0}
	bl	thread_terminate
	pop	{r0}
	bl	scheduler

	mcr	p15, 0, r0, c8, c7, 0	// invalidate data-tlb

	// restore context - may have been changed due to scheduling
	pop	{r0}		// restore cpsr
	msr	spsr, r0
	pop	{r0-r1}		// restore lr and sp
	cps	#_sys_mode
	mov	lr, r0
	mov	sp, r1
	cps	#_abt_mode
	pop	{r0-r12, lr}	// restore r0 to r12 and pc

	subs	pc, lr, #0


.global _thread_terminate_undefined
_thread_terminate_undefined:
	// save context from current thread
	push	{r0-r12, lr}	// save r0 to r12 and pc from where we came
	cps	#_sys_mode
	mov	r0, lr
	mov	r1, sp
	cps	#_und_mode
	push	{r0-r1}		// save lr and sp from where we came
	mrs	r0, spsr
	push	{r0}		// save cpsr from where we came

	mov	r0, sp	// curr_context

	push	{r0}
	bl	thread_terminate
	pop	{r0}
	bl	scheduler

	mcr	p15, 0, r0, c8, c7, 0	// invalidate data-tlb

	// restore context - may have been changed due to scheduling
	pop	{r0}		// restore cpsr
	msr	spsr, r0
	pop	{r0-r1}		// restore lr and sp
	cps	#_sys_mode
	mov	lr, r0
	mov	sp, r1
	cps	#_und_mode
	pop	{r0-r12, lr}	// restore r0 to r12 and pc

	subs	pc, lr, #0
