#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <mqueue.h>
#include "ratelimit.h"
#include "socket.h"
#include "common.h"

STATIC Bucket *bucket_init(double burst, double rate) {

	Bucket *bkt;
	int rate_integer;
	double rate_decimal;

	bkt = MALLOC_W(sizeof(*bkt));
	bkt->burst_capacity = burst;
	bkt->tokens = burst;
	bkt->fill_rate = rate;
	bkt->timestamp = 0;

	rate = 1 / rate;
	rate_integer = rate;
	rate_decimal = rate - rate_integer;
	bkt->sleep_time.tv_sec = rate_integer;
	bkt->sleep_time.tv_nsec = rate_decimal * NANOSECS;

	return bkt;
}

STATIC double get_tokens(Bucket *bkt) {

	double new_tokens;
	time_t now = time(NULL);

	if (bkt->tokens < bkt->burst_capacity) {
		new_tokens = bkt->fill_rate * difftime(now, bkt->timestamp);
		bkt->tokens = MIN(bkt->burst_capacity, bkt->tokens + new_tokens);
	}

	bkt->timestamp = now;
	return bkt->tokens;
}

STATIC bool consume_tokens(Bucket *bkt, double tokens) {

	if (tokens <= get_tokens(bkt)) {
		bkt->tokens -= tokens;
		return true;
	}

	return false;
}

mqd_t mqueue_create(const char *name, long maxlines, long linelen) {

	mqd_t mqd;
	struct mq_attr attr;

	mq_unlink(name); // Ensure we start with a fresh queue each time
	attr.mq_maxmsg = maxlines;
	attr.mq_msgsize = linelen;
	mqd = mq_open(name, O_CREAT | O_EXCL | O_WRONLY | O_NONBLOCK, S_IRUSR | S_IWUSR, &attr);
	if (mqd == -1)
		exit_msg("%s: %s\n", __func__, strerror(errno));

	return mqd;
}

struct rate_limit ratelimit_init(void) {

	struct rate_limit rtl;

	rtl.mqdfd = mq_open(MQUEUE_NAME, O_RDONLY);
	if (rtl.mqdfd == -1)
		exit_msg("mqueue open failed\n");

	rtl.bkt = bucket_init(BKT_BURST, BKT_RATE);
	if (!rtl.bkt)
		exit_msg("bucket init failed\n");

	return rtl;
}

void *ratelimit_loop(void *arg) {

	ssize_t n;
	char buf[MQUEUE_LINELEN];
	struct rate_limit rtl = *(struct rate_limit *) arg;

	while (true) {
		if (!consume_tokens(rtl.bkt, BKT_CONSUME)) {
			nanosleep(&rtl.bkt->sleep_time, NULL);
			rtl.bkt->tokens -= BKT_CONSUME;
		}
		n = mq_receive(rtl.mqdfd, buf, MQUEUE_LINELEN, NULL);
		if (n == -1)
			exit_msg("%s: %s\n", __func__, strerror(errno));

		if (sock_write(rtl.ircfd, buf, n) != n)
			exit_msg("error while sending to irc\n");
	}

	return NULL;
}

void ratelimit_destroy(struct rate_limit rtl) {

	free(rtl.bkt);
	mq_close(rtl.mqdfd);
	mq_unlink(MQUEUE_NAME);
}
