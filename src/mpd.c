#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "socket.h"
#include "irc.h"
#include "mpd.h"
#include "helper.h"

extern int mpdfd;

void play(Irc server, Parsed_data pdata) {

	char *temp;

	if (pdata.message == NULL)
		return;

	// Null terminate the the whole parameters line
	if ((temp = strrchr(pdata.message, '\r')) != NULL)
		*temp = '\0';
	else
		return;

	if (write(mpdfd, "noidle\n", 7) <= 0)
		perror("play write");

	if (strstr(pdata.message, "youtu") == NULL)
		print_cmd_output(server, pdata.target, (char *[]) { SCRIPTDIR "mpd_search.sh", cfg.mpd_database, pdata.message, NULL });
	else
		print_cmd_output(server, pdata.target, (char *[]) { SCRIPTDIR "youtube2mp3.sh", cfg.mpd_database, pdata.message, NULL });
}

void playlist(Irc server, Parsed_data pdata) {

	print_cmd_output_unsafe(server, pdata.target, "mpc playlist | head |" REMOVE_EXTENSION);
}

void history(Irc server, Parsed_data pdata) {

	char cmd[CMDLEN];

	snprintf(cmd, CMDLEN, "ls -t1 %s | head | tac |" REMOVE_EXTENSION, cfg.mpd_database);
	print_cmd_output_unsafe(server, pdata.target, cmd);
}

void current(Irc server, Parsed_data pdata) {

	print_cmd_output_unsafe(server, pdata.target, "mpc current |" REMOVE_EXTENSION);
}

void next(Irc server, Parsed_data pdata) {

	if (access(cfg.mpd_random_mode_file, F_OK) == 0)
		print_cmd_output_unsafe(server, pdata.target, "mpc -q next");
	else
		print_cmd_output_unsafe(server, pdata.target, "mpc next |" REMOVE_EXTENSION_1LN);
}

void random_mode(Irc server, Parsed_data pdata) {

	if (write(mpdfd, "idle player\n", 12) <= 0)
		perror("random_mode write");

	print_cmd_output_unsafe(server, pdata.target, SCRIPTDIR "mpd_random.sh");
}

int mpd_connect(const char *port) {

	int mpd;
	char buf[64];

	if ((mpd = sock_connect("127.0.0.1", port)) < 0)
		return -1;

	if (read(mpd, buf, sizeof(buf) - 1) < 0) {
		close(mpd);
		return -1;
	}
	if (strncmp(buf, "OK", 2) != 0) {
		close(mpd);
		return -1;
	}
	return mpd;
}

ssize_t print_song(Irc server, const char *channel) {

	static char old_song[SONGLEN];
	char *test, *song_title, buf[SONGLEN + 1];
	ssize_t n;

	if (read(mpdfd, buf, SONGLEN) < 0)
		return -1;

	if (strncmp(buf, "changed", 7) != 0)
		return 1;

	if (write(mpdfd, "currentsong\n", 12) != 12)
		return -1;

	if ((n = read(mpdfd, buf, SONGLEN)) < 0)
		return -1;

	buf[n] = '\0'; // terminate reply
	if ((song_title = strstr(buf, "file")) == NULL)
		return -1;

	song_title += 6; // advance to song_title start
	if ((test = strchr(song_title, '\n')) == NULL)
		return -1;

	*test = '\0'; // terminate line

	// Cut file extension (.mp3)
	if ((test = strrchr(buf + 6, '.')) == NULL)
		return -1;
	*test = '\0';

	if (strcmp(old_song, song_title) != 0) {
		send_message(server, channel, "♪ %s ♪", song_title);
		snprintf(old_song, SONGLEN, "%s", song_title);
	}
	// Restart query
	return write(mpdfd, "idle player\n", 12);
}
