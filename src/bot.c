#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bot.h"
#include "irc.h"
#include "curl.h"
#include "helper.h"


#ifdef TEST
	void print_cmd_output(Irc server, const char *dest, const char *cmd)
#else
	static void print_cmd_output(Irc server, const char *dest, const char *cmd)
#endif
{
	char line[LINELEN];
	FILE *prog;
	int len;

	// Open the program with arguments specified
	prog = popen(cmd, "r");
	if (prog == NULL)
		return;

	// Print line by line the output of the program
	while (fgets(line, LINELEN, prog) != NULL) {
		len = strlen(line) - 1;
		if (len > 1) { // Only print if line is not empty
			line[len] = '\0'; // Remove last newline char (\n) since we add it inside send_message()
			send_message(server, dest, "%s", line);
		}
	}
	pclose(prog);
}

void list(Irc server, Parsed_data pdata) {

	send_message(server, pdata->target, "list / help, bot, url, mumble, fail, github, ping, traceroute, dns");
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
	sleep(2);
	send_message(server, pdata->target, "to gipedo einai paralilogramo");
	sleep(2);
	send_message(server, pdata->target, "11 autoi, 11 emeis sinolo 23");
	sleep(2);
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

	// Argument must have the user/repo format
	if (strchr(argv[0], '/') == NULL)
		return;

	// Do not return more than MAXCOMMITS
	if (argc == 2) {
		commits = atoi(argv[1]);
		if (commits > MAXCOMMITS)
			commits = MAXCOMMITS;
		else if (commits < 0) // Integer overflowed / negative input, return only 1 commit
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
	free(argv);
	free(commit);
	free(mem.buffer);
}

void ping(Irc server, Parsed_data pdata) {

	char **argv, *cmd, cmdline[CMDLEN];
	int argc, count = 3;

	argv = extract_params(pdata->message, &argc);
	if (argc != 1 && argc != 2)
		return;

	// Find if the IP is an v4 or v6. Weak parsing
	if (strchr(argv[0], '.') != NULL)
		cmd = "ping";
	else if (strchr(argv[0], ':') != NULL)
		cmd = "ping6";
	else
		return;

	if (argc == 2) {
		count = atoi(argv[1]);
		if (count > MAXPINGCOUNT)
			count = MAXPINGCOUNT;
		else if (count < 0)
			count = 1;
	}
	snprintf(cmdline, CMDLEN, "%s -c %d %s", cmd, count, argv[0]);
	print_cmd_output(server, pdata->target, cmdline);

	free(argv);
}

void traceroute(Irc server, Parsed_data pdata) {

	char **argv, *cmd, cmdline[CMDLEN];
	int argc;

	argv = extract_params(pdata->message, &argc);
	if (argc != 1)
		return;

	if (strchr(argv[0], '.') != NULL)
		cmd = "traceroute";
	else if (strchr(argv[0], ':') != NULL)
		cmd = "traceroute6";
	else
		return;

	snprintf(cmdline, CMDLEN, "%s %s", cmd, argv[0]);
	send_message(server, pdata->target, "Printing result privately to %s", pdata->nick);
	print_cmd_output(server, pdata->nick, cmdline);

	free(argv);
}

void dns(Irc server, Parsed_data pdata) {

	char **argv, cmdline[CMDLEN];
	int argc;

	argv = extract_params(pdata->message, &argc);
	if (argc != 1)
		return;

	if (strchr(argv[0], '.') == NULL)
		return;

	snprintf(cmdline, CMDLEN, "nslookup %s", argv[0]);
	print_cmd_output(server, pdata->target, cmdline);

	free(argv);
}