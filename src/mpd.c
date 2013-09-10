#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include "socket.h"
#include "irc.h"
#include "mpd.h"
#include "helper.h"

extern int mpdfd;
extern bool *mpd_random_mode;

void play(Irc server, Parsed_data pdata) {

	char *temp;

	if (pdata.message == NULL)
		return;

	// Null terminate the the whole parameters line
	if ((temp = strrchr(pdata.message, '\r')) == NULL)
		return;
	*temp = '\0';

	if (*mpd_random_mode) {
		*mpd_random_mode = false;
		if (write(mpdfd, "noidle\n", 7) <= 0)
			perror("mpd play");
	}

	if (strstr(pdata.message, "youtu") == NULL)
		print_cmd_output(server, pdata.target, (char *[]) { SCRIPTDIR "mpd_search.sh", cfg.mpd_database, pdata.message, NULL });
	else
		print_cmd_output(server, pdata.target, (char *[]) { SCRIPTDIR "youtube2mp3.sh", cfg.mpd_database, pdata.message, NULL });
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
		if (write(mpdfd, "idle player\n", 12) <= 0)
			perror("random_mode write");
	}
}

void current(Irc server, Parsed_data pdata) {

	print_cmd_output_unsafe(server, pdata.target, "mpc current |" REMOVE_EXTENSION);
}

int mpd_connect(const char *port) {

	int mpd;
	char buf[64];

	if ((mpd = sock_connect(LOCALHOST, port)) < 0)
		return -1;

	if (read(mpd, buf, sizeof(buf) - 1) < 0)
		goto cleanup;

	if (strncmp(buf, "OK", 2) != 0)
		goto cleanup;

	if (*mpd_random_mode)
		if (write(mpd, "idle player\n", 12) <= 0)
			goto cleanup;

	return mpd; // Success

cleanup:
	close(mpd);
	return -1;
}

bool print_song(Irc server, const char *channel) {

	static char old_song[SONGLEN];
	char *test, *song_title, buf[SONGLEN + 1];
	ssize_t n;


	errno = 0;
	if (read(mpdfd, buf, SONGLEN) <= 0) {
		perror("print_song");
		goto cleanup;
	}
	if (strncmp(buf, "changed", 7) != 0)
		goto cleanup;

	if (write(mpdfd, "currentsong\n", 12) != 12) {
		perror("print_song");
		goto cleanup;
	}
	if ((n = read(mpdfd, buf, SONGLEN)) <= 0) {
		perror("print_song");
		goto cleanup;
	}
	buf[n] = '\0'; // terminate reply
	if ((song_title = strstr(buf, "file")) == NULL)
		goto cleanup;

	song_title += 6; // advance to song_title start
	if ((test = strchr(song_title, '\n')) == NULL)
		goto cleanup;

	*test = '\0'; // terminate line

	// Cut file extension (.mp3)
	if ((test = strrchr(buf + 6, '.')) == NULL)
		goto cleanup;
	*test = '\0';

	if (strcmp(old_song, song_title) != 0) {
		send_message(server, channel, "♪ %s ♪", song_title);
		snprintf(old_song, SONGLEN, "%s", song_title);
	}
	// Restart query
	if (write(mpdfd, "idle player\n", 12) == 12)
		return true;

cleanup:
	close(mpdfd);
	return false;
}
