#ifndef RATELIMIT_H
#define RATELIMIT_H

/**
 * @file ratelimit.h
 * Create queue (using unix sockets) to send all messages to. Token bucket algorithm is employed,
 * in order to throttle the sending of messages if needed. That way we avoid getting
 * kicked by the server for flooding. Token bucket algorithm supports bursting before
 * throttling kicks in. If lines arrive while queue is full, they are dropped.
 */

#include <time.h>
#include "irc.h"

#define BUCKET_BURST   6.0
#define BUCKET_RATE    1.0
#define BUCKET_CONSUME 1.0
#define QUEUE_MAXSIZE  30 * IRCLEN

typedef struct token_bucket {
	double burst_capacity;
	double tokens;
	double fill_rate;
	time_t timestamp;
	struct timespec sleep_time;
} Bucket;

struct ratelimit {
	int irc;
	int queue;
	Bucket *bucket;
};

/** Initialize token bucket algorithm and setup unix sockets */
struct ratelimit *ratelimit_init(Irc server);

/* Read's messages from queue according to priority and send them to IRC rate-limited
 * @param arg  struct ratelimit is expected with all the values filled in */
void *ratelimit_loop(void *arg);

/** Free resources created in init() */
void ratelimit_destroy(struct ratelimit *rtl);

#endif

