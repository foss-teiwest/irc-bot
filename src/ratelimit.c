#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

	bucket = MALLOC_W(sizeof(*bucket));
	bucket->burst_capacity = burst;
	bucket->tokens = burst;
	bucket->fill_rate = rate;
	bucket->timestamp = 0;

	rate = 1 / rate;
	rate_integer = rate;
	rate_decimal = rate - rate_integer;
	bucket->sleep_time.tv_sec = rate_integer;
	bucket->sleep_time.tv_nsec = rate_decimal * NANOSECS;

	return bucket;
}

STATIC double get_tokens(Bucket *bucket) {

	double new_tokens;
	time_t now = time(NULL);

	if (bucket->tokens < bucket->burst_capacity) {
		new_tokens = bucket->fill_rate * difftime(now, bucket->timestamp);
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
	struct ratelimit *rtl = malloc(sizeof(*rtl));

	rtl->bucket = bucket_init(BUCKET_BURST, BUCKET_RATE);
	if (!rtl->bucket)
		return NULL;

	if (socketpair(AF_UNIX, SOCK_DGRAM, 0, queue)) {
		perror(__func__);
		goto cleanup;
	}
	if (setsockopt(queue[WR], SOL_SOCKET, SO_SNDBUF, &(socklen_t) {QUEUE_MAXSIZE}, sizeof(socklen_t))) {
		perror(__func__);
		goto cleanup;
	}
	if (fcntl(queue[WR], F_SETFL, O_NONBLOCK)) {
		perror(__func__);
		goto cleanup;
	}
	rtl->irc = get_socket(server);
	set_queue(server, queue[WR]);
	rtl->queue = queue[RD];

	return rtl;

cleanup:
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
			exit_msg("error while reading from queue\n");

		if (sock_write(rtl.irc, buf, n) != n)
			exit_msg("error while sending to irc\n");
	}
	return NULL;
}

void ratelimit_destroy(struct ratelimit *rtl) {

	close(rtl->queue);
	free(rtl->bucket);
	free(rtl);
}
