#ifndef _CHANNEL_H
#define _CHANNELS_H
#include "stdint.h"
#include "../lib/printf.h"
#include "../sys/thread_control.h"

void init_channels();
int channel_open(int channel_id);
int channel_send(int channel_id, uint8_t *data, uint32_t length);
int channel_read(int channel_id, uint8_t *data, uint32_t length);

#endif // _CHANNELS_H
