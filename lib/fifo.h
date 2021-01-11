#ifndef _FIFO_H
#define _FIFO_H

void fifo_init(void);
int fifo_put(unsigned char new);
int fifo_get(unsigned char *old);

#endif // _FIFO_H
