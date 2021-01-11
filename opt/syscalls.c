#include "syscalls.h"

// asm svc functions
int _getc();
void _putc(unsigned char c);
int _thread_create(void *(*fptr)(void *), void *data, size_t len, void *(*t_term));
int _process_create(void *(*fptr)(void *), void *data, size_t len, void *(*t_term));
void _exit();
void _thread_sleep(size_t n);
int _tid();
int _pid();
int _open_channel(int channel_id);
int _send_channel(int channel_id, uint8_t *data, uint32_t length);
int _read_channel(int channel_id, uint8_t *data, uint32_t length);

unsigned char getc()
{
	return _getc();
}

void putc(unsigned char c)
{
	_putc(c);
}

int start_thread(void *(*fptr)(void *), void *data, size_t len)
{
	return _thread_create(fptr, data, len, (void *)_exit);
}

int start_process(void *(*fptr)(void *), void *data, size_t len)
{
	return _process_create(fptr, data, len, (void *)_exit);
}

void exit()
{
	_exit();
}

void sleep(size_t n)
{
	_thread_sleep(n);
}

int mytid()
{
	return _tid();
}

int mypid()
{
	return _pid();
}

int open_channel(int channel_id)
{
	return _open_channel(channel_id);
}

int send_channel(int channel_id, uint8_t *data, uint32_t length)
{
	return _send_channel(channel_id, data, length);
}

int read_channel(int channel_id, uint8_t *data, uint32_t length)
{
	return _read_channel(channel_id, data, length);
}
