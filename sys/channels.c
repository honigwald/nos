#include "channels.h"

/* Channel structure */
#define NUM_OF_CHANNELS 56
#define CHANNEL_ELEMENTS 4095
#define CHANNEL_SIZE (CHANNEL_ELEMENTS + 1)

static uint8_t Channels[NUM_OF_CHANNELS][CHANNEL_SIZE];
static uint8_t ChannelIn[NUM_OF_CHANNELS];
static uint8_t ChannelOut[NUM_OF_CHANNELS];
static int cat[NUM_OF_CHANNELS];	// channel allocation table

void init_channels()
{
	int id;
	for (id = 0; id < NUM_OF_CHANNELS; ++id) {
		ChannelIn[id] = 0;
		ChannelOut[id] = 0;
	}

	for (id = 0; id < NUM_OF_CHANNELS; ++id) {
		cat[id] = -1;
	}
}

static int channel_put(uint8_t new, int id)
{
	if (ChannelIn[id] == ((ChannelOut[id] - 1 + CHANNEL_SIZE) % CHANNEL_SIZE)) {
		return -1; /* Channel Full*/
	}
	
	Channels[id][ChannelIn[id]] = new;
	
	ChannelIn[id] = (ChannelIn[id] + 1) % CHANNEL_SIZE;
	
	return 0; // No errors
}

static int channel_get(uint8_t *old, int id)
{
	if(ChannelIn[id] == ChannelOut[id]) {
		return -1; /* Channel Empty - nothing to get*/
	}
	
	*old = Channels[id][ChannelOut[id]];
	
	ChannelOut[id] = (ChannelOut[id] + 1) % CHANNEL_SIZE;
	
	return 0; // No errors
}


int channel_open(int channel_id)
{
	if (channel_id > NUM_OF_CHANNELS-1) {
		printf("channel_open() error: id out of range\n");
		return -1;
	}

	if (cat[channel_id] != -1) {
		return 0;
	}

	ChannelIn[channel_id] = 0;
	ChannelOut[channel_id] = 0;

	cat[channel_id] = get_tid_k();

	return 1;
}

/**
 * returns amount of transmitted bytes
 */
int channel_send(int channel_id, uint8_t *data, uint32_t length)
{
	uint32_t bytes_send;
	for (bytes_send = 0; bytes_send < length; ++bytes_send) {
		if (channel_put(data[bytes_send], channel_id) == -1)
			break;
	}

	return bytes_send;
}

/**
 * returns amount of recieved bytes
 */
int channel_read(int channel_id, uint8_t *data, uint32_t length)
{
	//if ((unsigned int)cat[channel_id] != get_tid_k()) {
	//	printf("channel belongs to other thread\n");
	//	return -1;
	//}

	uint32_t bytes_read;
	for (bytes_read = 0; bytes_read < length; ++bytes_read) {
		if (channel_get(&(data[bytes_read]), channel_id) == -1)
			break;
	}

	return bytes_read;
}
