#ifndef QUEUE_H
#define QUEUE_H

/**
 * @file queue.h
 * Create message queue to send all messages to. Token bucket algorithm is employed,
 * in order to throttle the sending of messages if needed. That way we avoid getting
 * kicked by the server for flooding. Token bucket algorithm supports bursting before
 * throttling kicks in. If lines arrive while queue is full, they are dropped.
 */

#include <stdbool.h>

#define QUEUE_MAXLINES       30
#define QUEUE_BURST_CAPACITY 6.0
#define QUEUE_FILL_RATE      1.0
#define QUEUE_CONSUME_RATE   1.0

typedef struct message_queue *Mqueue;

/** Initialize token bucket algorithm and setup queue
 *  @param fd  Store the fd in the Mqueue struct so that mqueue_start() can access it
 *             because pthread_create() accepts a single argument */
Mqueue mqueue_init(int fd);

/** Reverse the steps in init */
void mqueue_destroy(Mqueue mq);

/** Read's messages from queue and send them to IRC rate limited */
void *mqueue_start(void *mqueue);

/** Send messages to the queue. Returns false if queue was full */
bool mqueue_send(Mqueue mq, const char *line);

/** Receive messages from queue. If queue is empty it will block until a message arrives */
char *mqueue_recv(Mqueue mq);

#endif

