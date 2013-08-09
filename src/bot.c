#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <time.h>
#include <yajl/yajl_tree.h>
#include "bot.h"
#include "irc.h"
#include "curl.h"
#include "helper.h"

// Quotes are seperated by commas. Multiline quote sentences must be seperated by the newline character (\n)
// Newline char is optional if the sentence is the last OR the only one from a quote
// If a color is provided to a quote starting with a number, a space must be added to avoid messing with the escape sequence
static const char *quotes[] = {
	TEAL "I mpala einai strogili\n" TEAL "to gipedo einai paralilogramo\n" TEAL "11 autoi, 11 emeis sinolo 23\n" TEAL "kai tha boun kai 3 allages apo kathe omada sinolo 29!",
	LTCYAN "fail indeed",
	PINK "fail den les tipota",
	LTGREEN "popo, ti eipes twra\n" LTGREEN "emeina me anoixto to... " RED "programma",
	ORANGE "Exeis dei file I/O me threads sto linux?",
	ORANGE "Kai ta Ubuntu kala einai",
	LTCYAN "kapote hmoun fanatikos tis micro$oft"
};

void list(Irc server, Parsed_data pdata) {

	send_message(server, pdata.target, "list / help, url, mumble, fail, github, ping, traceroute, dns, uptime");
}

void bot_fail(Irc server, Parsed_data pdata) {

	int r;
	size_t len, maxlen, sum = 0;

	// Make sure the seed is different even if we call the command twice in a second
	srand(time(NULL) + getpid());
	r = rand() % SIZE(quotes);
	maxlen = strlen(quotes[r]);

	// Pick a random entry from the read-only quotes array and print it.
	// We use indexes and lengths since we can't make changes to the array
	while (sum < maxlen && (len = strcspn(quotes[r] + sum, "\n")) > 0) {
		send_message(server, pdata.target, "%.*s", (int) len, quotes[r] + sum);
		sum += ++len;
		sleep(1);
	}
}

void url(Irc server, Parsed_data pdata) {

	char **argv, *temp, *short_url, *url_title = NULL;
	int argc;

	argv = extract_params(pdata.message, &argc);
	if (argc != 1)
		goto cleanup;

	// Check first parameter for a URL that contains at least one dot.
	if (strchr(argv[0], '.') == NULL)
		goto cleanup;

	// Map shared memory for inter-process communication
	short_url = mmap(NULL, ADDRLEN + 1, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (short_url == MAP_FAILED) {
		perror("mmap");
		goto cleanup;
	}

	switch (fork()) {
		case -1:
			perror("fork");
			break;
		case 0:
			temp = shorten_url(argv[0]);
			if (temp != NULL) {
				strncpy(short_url, temp, ADDRLEN);
				free(temp);
			} else // Put a null char in the first byte if shorten_url fails so we can test for it in send_message
				*short_url = '\0';

			munmap(short_url, ADDRLEN + 1); // Unmap pages from child process
			_exit(EXIT_SUCCESS);
		default:
			url_title = get_url_title(argv[0]);
			wait(NULL); // Wait for child results before continuing

			// Only print short_url / title if they are not empty
			send_message(server, pdata.target, "%s -- %s", (*short_url ? short_url : ""), (url_title ? url_title : ""));
	}
	munmap(short_url, ADDRLEN + 1); // Unmap pages from parent as well

cleanup:
	free(argv);
	free(url_title);
}

void mumble(Irc server, Parsed_data pdata) {

	char *user_list;

	user_list = fetch_mumble_users();
	if (user_list != NULL) {
		send_message(server, pdata.target, user_list);
		free(user_list);
	}
}

void github(Irc server, Parsed_data pdata) {

	Github *commits;
	yajl_val root = NULL;
	char **argv, *short_url, repo[REPOLEN + 1] = {0};
	int argc, i, commit_count = 1;

	argv = extract_params(pdata.message, &argc);
	if (argc != 1 && argc != 2) {
		free(argv);
		return;
	}
	// If user is not supplied, substitute with a default one
	if (strchr(argv[0], '/') == NULL)
		snprintf(repo, CMDLEN, "%s/%s", DEFAULT_USER_REPO, argv[0]);
	else
		strncat(repo, argv[0], REPOLEN);

	// Do not return more than MAXCOMMITS
	if (argc == 2)
		commit_count = get_int(argv[1], MAXCOMMITS);

	commits = fetch_github_commits(repo, &commit_count, root);
	if (commit_count == 0)
		goto cleanup;

	// Print each commit info with it's short url in a seperate colorized line
	for (i = 0; i < commit_count; i++) {
		short_url = shorten_url(commits[i].url);
		send_message(server, pdata.target,
					PURPLE "[%.7s]"
					RESET " %.120s"
					ORANGE " --%s"
					BLUE " - %s",
					commits[i].sha, commits[i].msg, commits[i].name, (short_url ? short_url : ""));
		free(short_url);
	}
cleanup:
	yajl_tree_free(root);
	free(commits);
	free(argv);
}

void ping(Irc server, Parsed_data pdata) {

	char **argv, *cmd, cmdline[CMDLEN];
	int argc, count = 3;

	argv = extract_params(pdata.message, &argc);
	if (argc != 1 && argc != 2)
		goto cleanup;

	// Find if the IP is an v4 or v6. Weak parsing
	if (strchr(argv[0], '.') != NULL)
		cmd = "ping";
	else if (strchr(argv[0], ':') != NULL)
		cmd = "ping6";
	else
		goto cleanup;

	if (argc == 2)
		count = get_int(argv[1], MAXPINGCOUNT);

	snprintf(cmdline, CMDLEN, "%s -c %d %s", cmd, count, argv[0]);
	print_cmd_output(server, pdata.target, cmdline);

cleanup:
	free(argv);
}

void traceroute(Irc server, Parsed_data pdata) {

	char **argv, *cmd, cmdline[CMDLEN];
	int argc;

	argv = extract_params(pdata.message, &argc);
	if (argc != 1)
		goto cleanup;

	if (strchr(argv[0], '.') != NULL)
		cmd = "traceroute";
	else if (strchr(argv[0], ':') != NULL)
		cmd = "traceroute6";
	else
		goto cleanup;

	snprintf(cmdline, CMDLEN, "%s -m 20 %s", cmd, argv[0]); // Limit max hops to 20
	if (strchr(pdata.target, '#') != NULL) // Don't send the following msg if the request was initiated in private
		send_message(server, pdata.target, "Printing results privately to %s", pdata.sender);
	print_cmd_output(server, pdata.sender, cmdline);

cleanup:
	free(argv);
}

void dns(Irc server, Parsed_data pdata) {

	char **argv, cmdline[CMDLEN];
	int argc;

	argv = extract_params(pdata.message, &argc);
	if (argc != 1)
		goto cleanup;

	if (strchr(argv[0], '.') == NULL)
		goto cleanup;

	snprintf(cmdline, CMDLEN, "nslookup %s", argv[0]);
	print_cmd_output(server, pdata.target, cmdline);

cleanup:
	free(argv);
}

void uptime(Irc server, Parsed_data pdata) {

	print_cmd_output(server, pdata.target, "uptime");
}
