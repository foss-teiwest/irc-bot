#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include "socket.h"
#include "irc.h"
#include "mpd.h"
#include "common.h"

extern int mpdfd;
extern struct mpd_status_type *mpd_status;

STATIC bool mpd_announce(bool on) {

	if (on) {
		mpd_status->announce = ON;
		return sock_write_non_blocking(mpdfd, "idle player\n", 12) == 12;
	} else {
		if (!mpd_status->announce)
			return true;

		mpd_status->announce = OFF;
		return sock_write_non_blocking(mpdfd, "noidle\n", 7) == 7;
	}
}

void play(Irc server, Parsed_data pdata) {

	char *prog;

	if (!pdata.message)
		return;

	mpd_announce(OFF);
	mpd_status->random = OFF;

	if (strstr(pdata.message, "youtu"))
		prog = SCRIPTDIR "youtube2mp3.sh";
	else
		prog = SCRIPTDIR "mpd_search.sh";

	print_cmd_output(server, pdata.target, CMD(prog, cfg.mpd_database, pdata.message));
}

void current(Irc server, Parsed_data pdata) {

	print_cmd_output_unsafe(server, pdata.target, "mpc current");
}

void playlist(Irc server, Parsed_data pdata) {

	print_cmd_output_unsafe(server, pdata.target, "mpc playlist | head");
}

void history(Irc server, Parsed_data pdata) {

	char cmd[CMDLEN];

	if (mpd_status->random)
		send_message(server, pdata.target, "%s", "history disabled in random mode");
	else {
		snprintf(cmd, CMDLEN, "ls -t1 %s | head | tac |" REMOVE_EXTENSION, cfg.mpd_database);
		print_cmd_output_unsafe(server, pdata.target, cmd);
	}
}

void random_mode(Irc server, Parsed_data pdata) {

	if (mpd_status->random)
		send_message(server, pdata.target, "%s", "already in random mode");
	else {
		mpd_status->random = ON;
		print_cmd_output_unsafe(server, pdata.target, SCRIPTDIR "mpd_random.sh");
	}
}

void stop(Irc server, Parsed_data pdata) {

	if (mpd_status->random) {
		mpd_status->random = OFF;
		mpd_announce(OFF);
		remove(cfg.mpd_random_file);
	}

	print_cmd_output_unsafe(server, pdata.target, "mpc -q clear");
}

void next(Irc server, Parsed_data pdata) {

	// TODO Only print the result to the one who send the command on channel / prive
	if (mpd_status->announce)
		print_cmd_output_unsafe(server, pdata.target, "mpc -q next");
	else
		print_cmd_output_unsafe(server, pdata.target, "mpc next | head -n1");
}

void seek(Irc server, Parsed_data pdata) {

	int argc;
	char **argv;

	argc = extract_params(pdata.message, &argv);
	if (!argc)
		send_message(server, pdata.target, "%s", "usage: [+-][HH:MM:SS]|<0-100>%");
	else if (argc == 1) {
		print_cmd_output(server, pdata.target, CMD(SCRIPTDIR "mpd_seek.sh", argv[0]));
		free(argv);
	}
}

void announce(Irc server, Parsed_data pdata) {

	int argc;
	char **argv;

	(void) server; // Silence unused variable warning

	if (!mpd_status->random)
		return;

	argc = extract_params(pdata.message, &argv);
	if (!argc)
		return;

	if (starts_case_with(argv[0], "on") && !mpd_status->announce)
		mpd_announce(ON);
	else if (starts_case_with(argv[0], "off") && mpd_status->announce)
		mpd_announce(OFF);

	free(argv);
}

int mpd_connect(const char *port) {

	int mpd;
	char buf[64];

	mpd = sock_connect(LOCALHOST, port);
	if (mpd < 0)
		return -1;

	if (sock_read(mpd, buf, sizeof(buf) - 1) <= 0)
		goto cleanup;

	if (!starts_with(buf, "OK"))
		goto cleanup;

	if (mpd_status->announce)
		if (!mpd_announce(ON))
			goto cleanup;

	fcntl(mpd, F_SETFL, O_NONBLOCK);
	return mpd; // Success

cleanup:
	close(mpd);
	return -1;
}

STATIC char *get_title(void) {

	char *song_title, buf[SONG_INFO_LEN + 1];
	struct pollfd pfd;
	ssize_t n = 0;
	int ready;

	pfd.fd = mpdfd;
	pfd.events = POLLIN;

	ready = poll(&pfd, 1, 4000);
	if (ready == 1) {
		if (pfd.revents & POLLIN) {
			n = sock_read_non_blocking(mpdfd, buf, SONG_INFO_LEN);
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
	return strdup(song_title);
}

bool print_song(Irc server, const char *channel) {

	static char old_song[SONG_TITLE_LEN];
	char *song_title, buf[128 + 1];

	if (sock_read_non_blocking(mpdfd, buf, 128) <= 0)
		goto cleanup;

	// Preserve the connection if "noidle" was issued
	if (!starts_with(buf, "changed"))
		return true;

	// Ask for current song
	if (sock_write_non_blocking(mpdfd, "currentsong\n", 12) < 0)
		goto cleanup;

	song_title = get_title();
	if (!song_title)
		goto cleanup;

	if (!streq(old_song, song_title)) {
		send_message(server, channel, "♪ %s ♪", song_title);
		snprintf(old_song, SONG_TITLE_LEN, "%s", song_title);
	}

	// Free title and restart query
	free(song_title);
	if (mpd_announce(ON))
		return true;

cleanup:
	close(mpdfd);
	return false;
}
