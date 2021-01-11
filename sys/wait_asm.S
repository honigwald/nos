.section .text

.global _wait
_wait:
	push	{r4-r11,lr}

	mov	r1, r0
1:
	subs	r1, r1, #1
	bne	1b

	pop	{r4-r11,pc}
