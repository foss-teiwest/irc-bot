#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include "bot.h"
#include "irc.h"
#include "curl.h"
#include "helper.h"

// Quotes are seperated by commas. Multiline quote sentences must be seperated by the newline character (\n)
// Newline char is optional if the sentence is the last OR the only one from a quote
// If a color is provided to a quote starting with a number, a space must be added to avoid messing with the escape sequence
static const char *quotes[] = {
	COLOR TEAL "I mpala einai strogili\n" COLOR TEAL "to gipedo einai paralilogramo\n" COLOR TEAL " 11 autoi, 11 emeis sinolo 23\n" COLOR TEAL "kai tha boun kai 3 allages apo kathe omada sinolo 29!",
	COLOR LTCYAN "fail indeed",
	COLOR PINK "total\n" COLOR PINK "failure",
	COLOR LTGREEN "popo, ti eipes twra\n" COLOR LTGREEN "emeina me anoixto to... " COLOR RED "programma",
	COLOR ORANGE "Exeis dei file I/O me threads sto linux?"
};

void list(Irc server, Parsed_data pdata) {

	send_message(server, pdata->target, "list / help, url, mumble, fail, github, ping, traceroute, dns");
}

void bot_fail(Irc server, Parsed_data pdata) {

	int r;
	size_t len, maxlen, sum = 0;

	srand(time(NULL));
	r = rand() % SIZE(quotes);
	maxlen = strlen(quotes[r]);
	while (sum < maxlen && (len = strcspn(quotes[r] + sum, "\n")) > 0) {
		send_message(server, pdata->target, "%.*s", (int) len, quotes[r] + sum);
		sum += ++len;
		sleep(1);
	}
}

void url(Irc server, Parsed_data pdata) {

	char **argv, *temp, *url_title = NULL, short_url[ADDRLEN] = {0};
	int argc, fd[2], short_url_len = 0;

	argv = extract_params(pdata->message, &argc);
	if (argc != 1)
		goto cleanup;

	// Check first parameter for a URL that contains at least one dot.
	if (strchr(argv[0], '.') == NULL)
		goto cleanup;

	// Open pipe for inter-process communication
	if (pipe(fd) < 0) {
		perror("pipe");
		goto cleanup;
	}
	switch (fork()) {
		case -1:
			perror("fork");
			break;
		case 0:
			close(fd[0]); // Close reading end of the socket
			temp = shorten_url(argv[0]);
			if (temp == NULL)
				free(temp);
			else
				write(fd[1], temp, strlen(temp));
			close(fd[1]);
			_exit(EXIT_SUCCESS);
			break;
		default:
			url_title = get_url_title(argv[0]);
			if (url_title == NULL)
				free(url_title);

			close(fd[1]); // Close writting end
			short_url_len = read(fd[0], short_url, ADDRLEN);
			close(fd[0]);
			if (waitpid(-1, NULL, 0) < 0) // Wait for child results before continuing
				perror("waitpid");
	}
	// Only print short_url / title if they are not empty
	send_message(server, pdata->target, "%s -- %s", (short_url_len ? short_url : ""), (url_title ? url_title : ""));

cleanup:
	free(argv);
}

void mumble(Irc server, Parsed_data pdata) {

	char *user_list;

	user_list = fetch_mumble_users();
	if (user_list != NULL) {
		send_message(server, pdata->target, user_list);
		free(user_list);
	}
}

void github(Irc server, Parsed_data pdata) {

	Github *commit;
	struct mem_buffer mem = {0};
	char **argv, *short_url, repo[REPOLEN + 1] = {0};
	int argc, i, commits = 1;

	argv = extract_params(pdata->message, &argc);
	if (argc != 1 && argc != 2) {
		free(argv);
		return;
	} printf("%s-%s\n", DEFAULT_USER_REPO, argv[0]);
	// If user is not supplied, substitute with a default one
	if (strchr(argv[0], '/') == NULL)
		snprintf(repo, CMDLEN, "%s/%s", DEFAULT_USER_REPO, argv[0]);
	else
		strncat(repo, argv[0], REPOLEN);

	// Do not return more than MAXCOMMITS
	if (argc == 2) {
		commits = atoi(argv[1]);
		if (commits > MAXCOMMITS)
			commits = MAXCOMMITS;
		else if (commits <= 0) // Integer overflowed or negative input, return only 1 commit
			commits = 1;
	}
	commit = fetch_github_commits(repo, &commits, &mem);
	if (commits == 0)
		goto cleanup;

	for (i = 0; i < commits; i++) {
		short_url = shorten_url(commit[i].url);
		send_message(server, pdata->target, COLOR PURPLE "[%s]" RESETCOLOR " %s" COLOR ORANGE " --%s" COLOR BLUE " - %s",
					commit[i].sha, commit[i].msg, commit[i].author, (short_url ? short_url : ""));
		if (short_url != NULL)
			free(short_url);
	}
cleanup:
	if (commit != NULL)
		free(commit);
	free(argv);
	free(mem.buffer);
}

void ping(Irc server, Parsed_data pdata) {

	char **argv, *cmd, cmdline[CMDLEN];
	int argc, count = 3;

	argv = extract_params(pdata->message, &argc);
	if (argc != 1 && argc != 2)
		goto cleanup;

	// Find if the IP is an v4 or v6. Weak parsing
	if (strchr(argv[0], '.') != NULL)
		cmd = "ping";
	else if (strchr(argv[0], ':') != NULL)
		cmd = "ping6";
	else
		goto cleanup;

	if (argc == 2) {
		count = atoi(argv[1]);
		if (count > MAXPINGCOUNT)
			count = MAXPINGCOUNT;
		else if (count < 0)
			count = 1;
	}
	snprintf(cmdline, CMDLEN, "%s -c %d %s", cmd, count, argv[0]);
	print_cmd_output(server, pdata->target, cmdline);

cleanup:
	free(argv);
}

void traceroute(Irc server, Parsed_data pdata) {

	char **argv, *cmd, cmdline[CMDLEN];
	int argc;

	argv = extract_params(pdata->message, &argc);
	if (argc != 1)
		goto cleanup;

	if (strchr(argv[0], '.') != NULL)
		cmd = "traceroute";
	else if (strchr(argv[0], ':') != NULL)
		cmd = "traceroute6";
	else
		goto cleanup;

	snprintf(cmdline, CMDLEN, "%s -m 20 %s", cmd, argv[0]); // Limit max hops to 20
	if (strchr(pdata->target, '#') != NULL) // Don't send the following msg if the request was initiated in private
		send_message(server, pdata->target, "Printing results privately to %s", pdata->sender);
	print_cmd_output(server, pdata->sender, cmdline);

cleanup:
	free(argv);
}

void dns(Irc server, Parsed_data pdata) {

	char **argv, cmdline[CMDLEN];
	int argc;

	argv = extract_params(pdata->message, &argc);
	if (argc != 1)
		goto cleanup;

	if (strchr(argv[0], '.') == NULL)
		goto cleanup;

	snprintf(cmdline, CMDLEN, "nslookup %s", argv[0]);
	print_cmd_output(server, pdata->target, cmdline);

cleanup:
	free(argv);
}