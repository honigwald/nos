.section .text

.global _trigger_undefined
_trigger_undefined:
	push	{r4-r12, lr}
	udf
	pop	{r4-r12, pc}


.global _trigger_data_abort
_trigger_data_abort:
	push	{r4-r12, lr}
	ldr	r0, =#0x00000001
	ldrex	r0, [r0]
	pop	{r4-r12, pc}

.global _trigger_software_interrupt
_trigger_software_interrupt:
	push	{r4-r12, lr}
	svc	#23
	pop	{r4-r12, pc}
