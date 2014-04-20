#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <time.h>
#include "ratelimit.h"
#include "socket.h"
#include "irc.h"
#include "common.h"

STATIC Bucket *bucket_init(double burst, double rate) {

	Bucket *bucket;
	int rate_integer;
	double rate_decimal;

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

STATIC double get_tokens(Bucket *bucket) {

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

STATIC bool consume_tokens(Bucket *bucket, double tokens) {

	if (tokens <= get_tokens(bucket)) {
		bucket->tokens -= tokens;
		return true;
	}
	return false;
}

struct ratelimit *ratelimit_init(Irc server) {

	int queue[RDWR];
	struct ratelimit *rtl = malloc_w(sizeof(*rtl));

	rtl->bucket = bucket_init(BUCKET_BURST, BUCKET_RATE);
	if (!rtl->bucket)
		return NULL;

	if (socketpair(AF_UNIX, SOCK_DGRAM, 0, queue))
		goto cleanup;

	// Reduce SEND-Q buffer to avoid delaying messages for too long
	if (setsockopt(queue[WR], SOL_SOCKET, SO_SNDBUF, &(socklen_t) {QUEUE_MAXSIZE}, sizeof(socklen_t)))
		goto cleanup2;

	if (fcntl(queue[WR], F_SETFL, O_NONBLOCK))
		goto cleanup2;

	rtl->irc = get_socket(server);
	rtl->queue = queue[RD];
	set_queue(server, queue[WR]);

	return rtl;

cleanup2:
	close(queue[RD]);
	close(queue[WR]);
cleanup:
	perror(__func__);
	free(rtl->bucket);
	free(rtl);
	return NULL;
}

void *ratelimit_loop(void *arg) {

	ssize_t n;
	char buf[IRCLEN];
	struct ratelimit rtl = *(struct ratelimit *) arg;

	while (true) {
		if (!consume_tokens(rtl.bucket, BUCKET_CONSUME)) {
			nanosleep(&rtl.bucket->sleep_time, NULL);
			rtl.bucket->tokens -= BUCKET_CONSUME;
		}
		n = sock_read(rtl.queue, buf, IRCLEN);
		if (n <= 0)
			exit_msg("error while reading from queue");

		if (sock_write(rtl.irc, buf, n) != n)
			exit_msg("error while sending to irc");
	}
	return NULL;
}

