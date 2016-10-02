#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <poll.h>
#include <fcntl.h>
#include "socket.h"
#include "irc.h"
#include "mpd.h"
#include "common.h"
#include "init.h"

STATIC bool mpd_announce(bool on);

// Be careful to only access & change announce & random members through the atomic macros defined in common.h
extern struct mpd_info *mpd;

STATIC char **parse_mpd_play_query(char *query) {

	size_t size;
	char *temp, **cmd, *count = "3";

	if (strstr(query, "youtu")) {
		size = sizeof(char *) * 4;
		cmd = malloc_w(size);
		memcpy(cmd, CMD(SCRIPTDIR "youtube2mp3.sh", cfg.mpd_database, query), size);
	} else {
		if (*query == '-' && isdigit(*(query + 1))) {
			temp = null_terminate(query + 2, ' ');
			if (!temp)
				return NULL;

			count = query + 1;
			query = temp  + 1;
			while (isspace(*query))
				query++;
		}
		size = sizeof(char *) * 5;
		cmd = malloc_w(size);
		memcpy(cmd, CMD(SCRIPTDIR "mpd_search.sh", cfg.mpd_database, count, query), size);
	}
	return cmd;
}

void bot_play(Irc server, struct parsed_data pdata) {

	char **cmd;

	pdata.message = trim_whitespace(pdata.message);
	if (!pdata.message) {
		print_cmd_output_unsafe(server, pdata.target, SCRIPTDIR "mpd_random.sh");
		TRUE(mpd->random);
		return;
	}
	cmd = parse_mpd_play_query(pdata.message);
	if (!cmd)
		return;

	if (print_cmd_output(server, pdata.target, cmd) == EXIT_SUCCESS) {
		mpd_announce(OFF);
		FALSE(mpd->random);
	}
	free(cmd);
}

void bot_current(Irc server, struct parsed_data pdata) {

	print_cmd_output_unsafe(server, pdata.target, "mpc | head -2");
}

void bot_playlist(Irc server, struct parsed_data pdata) {

	print_cmd_output_unsafe(server, pdata.target, "mpc playlist | head");
}

void bot_history(Irc server, struct parsed_data pdata) {

	char cmd[CMDLEN];

	if (FETCH(mpd->random))
		send_message(server, pdata.target, "%s", "history disabled in random mode");
	else {
		snprintf(cmd, CMDLEN, "ls -t1 %s | head | tac |" REMOVE_EXTENSION, cfg.mpd_database);
		print_cmd_output_unsafe(server, pdata.target, cmd);
	}
}

void bot_stop(Irc server, struct parsed_data pdata) {

	if (FETCH(mpd->random)) {
		FALSE(mpd->random);
		mpd_announce(OFF);
		if (remove(cfg.mpd_random_state))
			perror(__func__);
	}
	print_cmd_output_unsafe(server, pdata.target, "mpc -q clear");
}

void bot_next(Irc server, struct parsed_data pdata) {

	if (FETCH(mpd->announce))
		print_cmd_output_unsafe(server, pdata.target, "mpc -q next");
	else
		print_cmd_output_unsafe(server, pdata.target, "mpc next | head -n1");
}

void bot_shuffle(Irc server, struct parsed_data pdata) {

	print_cmd_output_unsafe(server, pdata.target, "mpc shuffle >/dev/null");
}

void bot_seek(Irc server, struct parsed_data pdata) {

	int argc;
	char **argv;

	argc = extract_params(pdata.message, &argv);
	if (!argc)
		send_message(server, pdata.target, "%s", "usage: [+-][HH:MM:SS]|<0-100>%");
	else
		print_cmd_output(server, pdata.target, CMD(SCRIPTDIR "mpd_seek.sh", argv[0]));

	free(argv);
}

STATIC bool mpd_announce(bool on) {

	if (on) {
		TRUE(mpd->announce);
		return (sock_write(mpd->fd, "noidle\n", 7) == 7 &&
			sock_write(mpd->fd, "idle player\n", 12) == 12);
	} else {
		FALSE(mpd->announce);
		return (sock_write(mpd->fd, "noidle\n", 7) == 7 &&
			sock_write(mpd->fd, "idle sticker\n", 13) == 13);
	}
}

void bot_announce(Irc server, struct parsed_data pdata) {

	int argc;
	char **argv;

	(void) server; // Silence unused variable warning

	if (!FETCH(mpd->random))
		return;

	argc = extract_params(pdata.message, &argv);
	if (!argc)
		return;

	if (strcase_eq(argv[0], "on") && !FETCH(mpd->announce))
		mpd_announce(ON);
	else if (strcase_eq(argv[0], "off") && FETCH(mpd->announce))
		mpd_announce(OFF);

	free(argv);
}

int mpd_connect(const char *port) {

	char buf[64];
	bool status;

	mpd->fd = sock_connect(LOCALHOST, port);
	if (mpd->fd < 0)
		return -1;

	if (sock_read(mpd->fd, buf, sizeof(buf)) <= 0)
		goto cleanup;

	if (!starts_with(buf, "OK"))
		goto cleanup;

	if (fcntl(mpd->fd, F_SETFL, O_NONBLOCK)) {
		perror(__func__);
		goto cleanup;
	}
	if (FETCH(mpd->announce))
		status = mpd_announce(ON);
	else
		status = mpd_announce(OFF);

	if (!status)
		goto cleanup;

	return mpd->fd; // Success

cleanup:
	close(mpd->fd);
	return -1;
}

STATIC char *get_title(void) {

	int ready;
	ssize_t n = 0;
	char *song_title, buf[SONG_INFO_LEN + 1];

	struct pollfd pfd = {
		.fd = mpd->fd,
		.events = POLLIN
	};
	ready = poll(&pfd, 1, 4 * MILLISECS);
	if (ready == 1) {
		if (pfd.revents & POLLIN) {
			n = sock_read(mpd->fd, buf, SONG_INFO_LEN);
			if (n <= 0)
				return NULL;
		}
	} else if (ready == -1) {
		perror(__func__);
		return NULL;
	} else {
		fprintf(stderr, "%s:Timeout limit reached\n", __func__);
		return NULL;
	}
	buf[n] = '\0'; // terminate reply
	song_title = strstr(buf, "Title");
	if (!song_title)
		return NULL;

	song_title += 7; // advance to song_title start
	if (!null_terminate(song_title, '\n'))
		return NULL;

	// Don't forget to free
	return strndup(song_title, SONG_TITLE_LEN);
}

bool print_song(Irc server, const char *target) {

	static char old_song[SONG_TITLE_LEN];
	char *song_title, buf[128 + 1];

	if (sock_read(mpd->fd, buf, 128) <= 0)
		goto cleanup;

	// Preserve the connection if "noidle" was issued
	if (!starts_with(buf, "changed"))
		return true;

	// Ask for current song
	if (sock_write(mpd->fd, "currentsong\n", 12) != 12)
		goto cleanup;

	song_title = get_title();
	if (!song_title)
		goto cleanup;

	if (!streq(old_song, song_title)) {
		send_message(server, target, "♪ %s ♪", song_title);
		snprintf(old_song, SONG_TITLE_LEN, "%s", song_title);
	}
	// Free title and restart query
	free(song_title);
	if (mpd_announce(ON))
		return true;

cleanup:
	close(mpd->fd);
	return false;
}
