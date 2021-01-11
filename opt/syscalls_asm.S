.section .text

.global _getc
_getc:
	push	{lr}
	svc	11
	pop	{pc}

.global _putc
_putc:
	push	{lr}
	svc	12
	pop	{pc}

.global _thread_create
_thread_create:
	push	{lr}
	svc	21
	pop	{pc}

.global _process_create
_process_create:
	push	{lr}
	svc	24
	pop	{pc}

.global _exit
_exit:
	push	{lr}
	svc	22
	pop	{pc}

.global _thread_sleep
_thread_sleep:
	push	{lr}
	svc	23
	pop	{pc}

.global _tid
_tid:
	push	{lr}
	svc	25
	pop	{pc}

.global _pid
_pid:
	push	{lr}
	svc	26
	pop	{pc}

.global _open_channel
_open_channel:
	push	{lr}
	svc	27
	pop	{pc}

.global _send_channel
_send_channel:
	push	{lr}
	svc	28
	pop	{pc}

.global _read_channel
_read_channel:
	push	{lr}
	svc	29
	pop	{pc}
