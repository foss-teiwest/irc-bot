#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bot.h"
#include "irc.h"
#include "curl.h"
#include "helper.h"


void list(Irc server, Parsed_data pdata) {

	send_message(server, pdata->target, "list, help, bot, url, mumble, fail, github");
}

void bot(Irc server, Parsed_data pdata) {

	send_message(server, pdata->target, "sup %s?", pdata->nick);
}

void url(Irc server, Parsed_data pdata) {

	char *short_url, **argv;
	int argc;

	argv = extract_params(pdata->message, &argc);
	if (argc != 1)
		return;

	// Check first parameter for a URL that contains at least one dot.
	if (strchr(argv[0], '.') == NULL)
		return;

	short_url = shorten_url(argv[0]);
	if (short_url != NULL) {
		send_message(server, pdata->target, "%s", short_url);
		free(short_url);
	}
	free(argv);
}

void mumble(Irc server, Parsed_data pdata) {

	char *user_list;

	user_list = fetch_mumble_users();
	send_message(server, pdata->target, "%s", user_list);

	free(user_list);
}

void bot_fail(Irc server, Parsed_data pdata) {

	send_message(server, pdata->target, "I mpala einai strogili");
	sleep(3);
	send_message(server, pdata->target, "to gipedo einai paralilogramo");
	sleep(3);
	send_message(server, pdata->target, "11 autoi, 11 emeis sinolo 23");
	sleep(3);
	send_message(server, pdata->target, "kai tha boun kai 3 allages apo kathe omada sinolo 29!");
}

void github(Irc server, Parsed_data pdata) {

	Github *commit;
	struct mem_buffer mem;
	char **argv, *short_url;
	int argc, i, commits = 1;

	argv = extract_params(pdata->message, &argc);
	if (argc != 1 && argc != 2)
		return;

	if (strchr(argv[0], '/') == NULL)
		return;

	if (argc == 2) {
		commits = atoi(argv[1]);
		if (commits > MAXCOMMITS)
			commits = MAXCOMMITS;
		else if (commits < 0)
			commits = 1;
	}
	commit = fetch_github_commits(argv[0], &commits, &mem);
	for (i = 0; i < commits; i++) {
		short_url = shorten_url(commit[i].url);
		if (short_url == NULL)
			short_url = "";
		send_message(server, pdata->target, "[%s] \"%s\" --%s @ %s", commit[i].sha, commit[i].msg, commit[i].author, short_url);
		free(short_url);
	}
	free(commit);
	free(mem.buffer);
}

void traceroute(Irc server, Parsed_data pdata) {

	struct mem_buffer mem;
	char **argv, **line, *command;
	int i, argc, lines;

	argv = extract_params(pdata->message, &argc);
	if (argc != 1)
		return;

	if (strchr(argv[0], '.') != NULL)
		command = "traceroute";
	else if (strchr(argv[0], ':') != NULL)
		command = "traceroute6";
	else
		return;

	line = network_tools(command, argv[0], &lines, &mem);
	for (i = 0; i < lines; i++) {
		send_message(server, pdata->nick, "%s", line[i]);
		sleep(1);
	}
	free(line);
	free(mem.buffer);
}

void ping(Irc server, Parsed_data pdata) {

	struct mem_buffer mem;
	char **argv, **line, *command;
	int i, argc, lines;

	argv = extract_params(pdata->message, &argc);
	if (argc != 1)
		return;

	if (strchr(argv[0], '.') != NULL)
		command = "ping";
	else if (strchr(argv[0], ':') != NULL)
		command = "ping6";
	else
		return;

	line = network_tools(command, argv[0], &lines, &mem);
	for (i = 0; i < lines; i++) {
		send_message(server, pdata->nick, "%s", line[i]);
		sleep(1);
	}
	free(line);
	free(mem.buffer);
}

void dns(Irc server, Parsed_data pdata) {

	struct mem_buffer mem;
	char **argv, **line;
	int i, argc, lines;

	argv = extract_params(pdata->message, &argc);
	if (argc != 1)
		return;

	if (strchr(argv[0], '.') == NULL)
		return;

	line = network_tools("nslookup", argv[0], &lines, &mem);
	for (i = 0; i < lines; i++) {
		send_message(server, pdata->nick, "%s", line[i]);
		sleep(1);
	}
	free(line);
	free(mem.buffer);
}