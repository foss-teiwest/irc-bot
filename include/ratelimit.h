#ifndef RATELIMIT_H
#define RATELIMIT_H

/**
 * @file ratelimit.h
 * Create Posix message queue to send all messages to. Token bucket algorithm is employed,
 * in order to throttle the sending of messages if needed. That way we avoid getting
 * kicked by the server for flooding. Token bucket algorithm supports some burst before
 * throttling kicks in. If lines are arrived while queue is full, they are dropped.
 * @warning You have to set the maximum allowed messages that Posix message queues allow
 * to match the value of MQUEUE_MAXLINES. You can do it by running the following as root:
 *     echo MQUEUE_MAXLINES > /proc/sys/fs/mqueue/msg_max
 */

#include <time.h>
#include <mqueue.h>
#include "irc.h"

typedef struct token_bucket {
	double burst_capacity;
	double tokens;
	double fill_rate;
	time_t timestamp;
	struct timespec sleep_time;
} Bucket;

struct rate_limit {
	int ircfd;
	mqd_t mqdfd;
	Bucket *bkt;
};

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define NANOSECS 1000 * 1000 * 1000

#define BKT_BURST 8.0
#define BKT_RATE 1.0
#define BKT_CONSUME 1.0
#define MQUEUE_NAME "/irc-bot"
#define MQUEUE_MAXLINES 40
#define MQUEUE_LINELEN IRCLEN

/**
 * Create write-only Posix message queue
 *
 * @param name      The name of queue. Posix requires that the first char is a slash. E.x. /bot
 * @param maxlines  The maximum amount of lines that can be in the queue
 * @param linelen   The maximum length a line can be
 * @return          File descriptor
 */
mqd_t mqueue_create(const char *name, long maxlines, long linelen);

/** Open mqueue in read-only mode and initialize the token bucket algorithm */
struct rate_limit ratelimit_init(void);

/**
 * Read's messages from queue according to priority and send them to IRC rate-limited
 *
 * @param arg  struct rate_limit is expected with all the values filled in
 */
void *ratelimit_loop(void *arg);

/** Destroy queue and token bucket constructs */
void ratelimit_destroy(struct rate_limit rtl);

#endif

