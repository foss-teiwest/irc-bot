#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "helper.h"
#include "mpd.h"


void play(Irc server, Parsed_data pdata) {

	char *temp;

	if (pdata.message == NULL)
		return;

	// Null terminate the the whole parameters line
	if ((temp = strrchr(pdata.message, '\r')) != NULL)
		*temp = '\0';
	else
		return;

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

	print_cmd_output_unsafe(server, pdata.target, "mpc -q next && mpc current |" REMOVE_EXTENSION);
}

void random_mode(Irc server, Parsed_data pdata) {

	print_cmd_output_unsafe(server, pdata.target, SCRIPTDIR "mpd_random.sh");
}
