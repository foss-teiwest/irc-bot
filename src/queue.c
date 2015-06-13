#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include "queue.h"
#include "socket.h"
#include "irc.h"
#include "common.h"

struct fifo_queue {
	int head;
	int tail;
	char lines[QUEUE_MAXLINES][IRCLEN + 1];
};

struct token_bucket {
	double tokens;
	double burst_capacity;
	double fill_rate;
	struct timespec timestamp;
	struct timespec sleep_time;
};

struct message_queue {
	int ircfd;
	pthread_mutex_t *mtx;
	pthread_cond_t *cond;
	size_t numlines;
	struct fifo_queue *queue;
	struct token_bucket *bucket;
};

STATIC struct token_bucket *bucket_init(double burst, double rate) {

	int rate_integer;
	double rate_decimal;
	struct token_bucket *bucket;

	bucket = malloc_w(sizeof(*bucket));
	bucket->burst_capacity = burst;
	bucket->tokens = burst;
	bucket->fill_rate = rate;

	rate = 1 / rate;
	rate_integer = rate;
	rate_decimal = rate - rate_integer;
	bucket->sleep_time.tv_sec = rate_integer;
	bucket->sleep_time.tv_nsec = rate_decimal * NANOSECS;

	return bucket;
}

STATIC double timediff(struct timespec now, struct timespec old) {

	struct timespec diff;

	diff.tv_sec  = now.tv_sec  - old.tv_sec;
	diff.tv_nsec = now.tv_nsec - old.tv_nsec;
	if (diff.tv_nsec < 0) {
		diff.tv_sec--;
		diff.tv_nsec += NANOSECS;
	}
	return diff.tv_sec + ((double) diff.tv_nsec / NANOSECS);
}

STATIC double get_tokens(struct token_bucket *bucket) {

	double new_tokens;
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	if (bucket->tokens < bucket->burst_capacity) {
		new_tokens = bucket->fill_rate * timediff(now, bucket->timestamp);
		bucket->tokens = MIN(bucket->burst_capacity, bucket->tokens + new_tokens);
	}
	bucket->timestamp = now;
	return bucket->tokens;
}

STATIC bool consume_tokens(struct token_bucket *bucket, double tokens) {

	if (tokens <= get_tokens(bucket)) {
		bucket->tokens -= tokens;
		return true;
	}
	return false;
}

Mqueue mqueue_init(int fd) {

	Mqueue mq  = malloc_w(sizeof(*mq));
	mq->mtx    = malloc_w(sizeof(*mq->mtx));
	mq->cond   = malloc_w(sizeof(*mq->cond));
	mq->queue  = calloc_w(sizeof(*mq->queue));
	mq->bucket = bucket_init(QUEUE_BURST_CAPACITY, QUEUE_FILL_RATE);

	mq->ircfd = fd;
	mq->numlines = 0;
	if (!pthread_mutex_init(mq->mtx, NULL) && !pthread_cond_init(mq->cond, NULL))
		return mq;

	perror(__func__);
	free(mq->mtx);
	free(mq->cond);
	free(mq->queue);
	free(mq->bucket);
	free(mq);
	return NULL;
}

void mqueue_destroy(Mqueue mq) {

	if (!mq)
		return;

	pthread_mutex_destroy(mq->mtx);
	pthread_cond_destroy(mq->cond);
	free(mq->mtx);
	free(mq->cond);
	free(mq->queue);
	free(mq->bucket);
	free(mq);
}

bool mqueue_send(Mqueue mq, const char *line) {

	if (!line)
		return false;

	pthread_mutex_lock(mq->mtx);
	if (mq->numlines == QUEUE_MAXLINES) {
		pthread_mutex_unlock(mq->mtx);
		return false;
	}
	strncpy(mq->queue->lines[mq->queue->tail], line, IRCLEN);
	mq->queue->tail = (mq->queue->tail + 1) % QUEUE_MAXLINES;
	mq->numlines++;

	pthread_mutex_unlock(mq->mtx);
	pthread_cond_signal(mq->cond);

	return true;
}

char *mqueue_recv(Mqueue mq) {

	char *line;

	pthread_mutex_lock(mq->mtx);
	while (!mq->numlines)
		pthread_cond_wait(mq->cond, mq->mtx);

	line = mq->queue->lines[mq->queue->head];
	mq->queue->head = (mq->queue->head + 1) % QUEUE_MAXLINES;
	mq->numlines--;

	pthread_mutex_unlock(mq->mtx);
	return line;
}

void *mqueue_start(void *mqueue) {

	int len;
	char *line;
	Mqueue mq = mqueue;

	for (;;) {
		if (!consume_tokens(mq->bucket, QUEUE_CONSUME_RATE)) {
			nanosleep(&mq->bucket->sleep_time, NULL);
			mq->bucket->tokens -= QUEUE_CONSUME_RATE;
		}
		line = mqueue_recv(mq);
		assert(line);
		len = strlen(line);

		if (sock_write(mq->ircfd, line, len) != len)
			exit_msg("error while sending to irc");
	}
	// NOT REACHED
	return NULL;
}
