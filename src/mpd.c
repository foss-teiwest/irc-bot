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
extern bool *mpd_random_mode;

void play(Irc server, Parsed_data pdata) {

	char *test;

	if (!pdata.message)
		return;

	// Null terminate the the whole parameters line
	test = strrchr(pdata.message, '\r');
	if (!test)
		return;

	*test = '\0';

	if (*mpd_random_mode) {
		*mpd_random_mode = false;
		sock_write_non_blocking(mpdfd, "noidle\n", 7);
	}

	if (strstr(pdata.message, "youtu"))
		print_cmd_output(server, pdata.target, (char *[]) { SCRIPTDIR "youtube2mp3.sh", cfg.mpd_database, pdata.message, NULL });
	else
		print_cmd_output(server, pdata.target, (char *[]) { SCRIPTDIR "mpd_search.sh", cfg.mpd_database, pdata.message, NULL });
}

void playlist(Irc server, Parsed_data pdata) {

	if (*mpd_random_mode)
		send_message(server, pdata.target, "%s", "playlist disabled in random mode");
	else
		print_cmd_output_unsafe(server, pdata.target, "mpc playlist | head |" REMOVE_EXTENSION);
}

void history(Irc server, Parsed_data pdata) {

	char cmd[CMDLEN];

	if (*mpd_random_mode)
		send_message(server, pdata.target, "%s", "history disabled in random mode");
	else {
		snprintf(cmd, CMDLEN, "ls -t1 %s | head | tac |" REMOVE_EXTENSION, cfg.mpd_database);
		print_cmd_output_unsafe(server, pdata.target, cmd);
	}
}

void next(Irc server, Parsed_data pdata) {

	// TODO Only print the result to the one who send the command on channel / prive
	if (*mpd_random_mode)
		print_cmd_output_unsafe(server, pdata.target, "mpc -q next");
	else
		print_cmd_output_unsafe(server, pdata.target, "mpc next |" REMOVE_EXTENSION);
}

void random_mode(Irc server, Parsed_data pdata) {

	if (*mpd_random_mode)
		send_message(server, pdata.target, "%s", "already in random mode");
	else {
		*mpd_random_mode = true;
		print_cmd_output_unsafe(server, pdata.target, SCRIPTDIR "mpd_random.sh");
		sock_write_non_blocking(mpdfd, "idle player\n", 12);
	}
}

void current(Irc server, Parsed_data pdata) {

	print_cmd_output_unsafe(server, pdata.target, "mpc current |" REMOVE_EXTENSION);
}

void stop(Irc server, Parsed_data pdata) {

	if (*mpd_random_mode) {
		*mpd_random_mode = false;
		remove(cfg.mpd_random_file);
		sock_write_non_blocking(mpdfd, "noidle\n", 7);
		print_cmd_output_unsafe(server, pdata.target, "mpc -q random off");
	}

	print_cmd_output_unsafe(server, pdata.target, "mpc -q clear");
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

	if (*mpd_random_mode)
		if (sock_write(mpd, "idle player\n", 12) < 0)
			goto cleanup;

	fcntl(mpd, F_SETFL, O_NONBLOCK);
	return mpd; // Success

cleanup:
	close(mpd);
	return -1;
}

STATIC char *get_title(void) {

	char *test, *song_title, buf[SONGLEN + 1];
	struct pollfd pfd;
	ssize_t n = 0;
	int ready;

	pfd.fd = mpdfd;
	pfd.events = POLLIN;

	ready = poll(&pfd, 1, 4000);
	if (ready == 1) {
		if (pfd.revents & POLLIN) {
			n = sock_read_non_blocking(mpdfd, buf, SONGLEN);
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
	song_title = strstr(buf, "file");
	if (!song_title)
		return NULL;

	song_title += 6; // advance to song_title start
	test = strchr(song_title, '\n');
	if (!test)
		return NULL;

	*test = '\0'; // terminate line

	// Cut file extension (.mp3)
	test = strrchr(buf + 6, '.');
	if (!test)
		return NULL;

	*test = '\0';

	return song_title;
}

bool print_song(Irc server, const char *channel) {

	static char old_song[SONGLEN];
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
		snprintf(old_song, SONGLEN, "%s", song_title);
	}
	// Restart query
	if (sock_write_non_blocking(mpdfd, "idle player\n", 12) == 12)
		return true;

cleanup:
	close(mpdfd);
	return false;
}
