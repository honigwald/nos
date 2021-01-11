#include "fifo.h"

// SOURCE: https://stackoverflow.com/questions/215557/how-do-i-implement-a-circular-list-ring-buffer-in-c
/* Very simple queue
 * These are FIFO queues which discard the new data when full.
 *
 * Queue is empty when in == out.
 * If in != out, then 
 *  - items are placed into in before incrementing in
 *  - items are removed from out before incrementing out
 * Queue is full when in == (out-1 + QUEUE_SIZE) % QUEUE_SIZE;
 *
 * The queue will hold QUEUE_ELEMENTS number of items before the
 * calls to QueuePut fail.
 */

/* Queue structure */
#define QUEUE_ELEMENTS 1024
#define QUEUE_SIZE (QUEUE_ELEMENTS + 1)

static unsigned char Queue[QUEUE_SIZE];
static unsigned char QueueIn;
static unsigned char QueueOut;

void fifo_init(void)
{
    QueueIn = 0;
    QueueOut = 0;
}

int fifo_put(unsigned char new)
{
    if (QueueIn == ((QueueOut - 1 + QUEUE_SIZE) % QUEUE_SIZE)) {
        return -1; /* Queue Full*/
    }

    Queue[QueueIn] = new;

    QueueIn = (QueueIn + 1) % QUEUE_SIZE;

    return 0; // No errors
}

int fifo_get(unsigned char *old)
{
    if(QueueIn == QueueOut) {
        return -1; /* Queue Empty - nothing to get*/
    }

    *old = Queue[QueueOut];

    QueueOut = (QueueOut + 1) % QUEUE_SIZE;

    return 0; // No errors
}
