#include "userthread.h"

#define LIMIT 9

void thread_reader()
{
	open_channel(1);

	uint8_t data[4096];
	size_t i;
	for (i = 0; i < 4096; ++i) {
		data[i] = 0;
	}
	size_t size = 0;

	unsigned char c = '\0';
	do {
		while (c != '\n') {
			read_channel(1, &c, 1);
			data[size] = c;
			size++;
		}

		uprintf(">> thread_reader: ");
		for (i = 0; i < size; ++i) {
			uprintf("%c", data[i]);
		}
		uprintf(">> thread_writer: ");
		
		for (i = 0; i < 4096; ++i) {
			data[i] = 0;
		}
		size = 0;
		c = '\0';
	} while(1);
}

void thread_writer()
{
	uprintf(">> thread_writer: ");
	uint8_t data[4096];
	int i;
	for (i = 0; i < 4096; ++i) {
		data[i] = 0;
	}

	size_t last = 0;

	char c = '\0';
	do {
		while(c != '\n') {
			c = getc();
			putc(c);
			data[last] = c;
			last++;
		}
		send_channel(1, data, last);
		c = '\0';
		last = 0;
	} while(1);
}

void thread_maker()
{
	start_process((void *)(&thread_reader), NULL, 0);
	start_process((void *)(&thread_writer), NULL, 0);
}
