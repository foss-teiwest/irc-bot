#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <pthread.h>
#include <sqlite3.h>
#include <yajl/yajl_tree.h>
#include "init.h"
#include "commands.h"
#include "irc.h"
#include "curl.h"
#include "murmur.h"
#include "twitter.h"
#include "database.h"
#include "common.h"

extern char *program_name_arg;
extern char *config_file_arg;
extern struct pollfd pfd[TOTAL];

void bot_help(Irc server, struct parsed_data pdata) {

	send_message(server, pdata.target, "%s", "url, mumble, fail, github, ping, traceroute, dns, uptime, roll, marker, fit, weather, population");
	send_message(server, pdata.target, "%s", "ACCESS REQUIRED: access_add, fail_add, fail_modify, tweet, upgrade, downgrade");
	send_message(server, pdata.target, "%s", "MPD: play, playlist, history, current, next, shuffle, stop, seek, announce");
}

static void deploy(char *operation) {

	char fd_args[] = {pfd[IRC].fd, pfd[MURM_LISTEN].fd, pfd[MURM_ACCEPT].fd};
	execv(program_name_arg, CMD(program_name_arg, operation, "-f", fd_args, config_file_arg));
	perror(__func__);
}

void bot_upgrade(Irc server, struct parsed_data pdata) {

	if (user_has_access(server, pdata.sender))
		if (print_cmd_output(server, pdata.target, CMD(SCRIPTDIR "deploy.sh", "-u")) == EXIT_SUCCESS)
			deploy("-u");
}

void bot_downgrade(Irc server, struct parsed_data pdata) {

	if (user_has_access(server, pdata.sender))
		if (print_cmd_output(server, pdata.target, CMD(SCRIPTDIR "deploy.sh", "-d")) == EXIT_SUCCESS)
			deploy("-d");
}

void bot_access_add(Irc server, struct parsed_data pdata) {

	int argc;
	char **argv;

	argc = extract_params(pdata.message, &argv);
	if (!argc)
		return;

	if (!user_has_access(server, pdata.sender)) {
		free(argv);
		return;
	}
	if (add_user(argv[0]))
		send_message(server, pdata.target, "%s, with great power...", argv[0]);

	free(argv);
}

STATIC void print_quote(Irc server, struct parsed_data pdata, const char *quote) {

	int clr, len, maxlen, sum = 0;
	char clr_quote[QUOTELEN];

	clr = (random() % COLORCOUNT) + 2;
	maxlen = strlen(quote);

	while (sum < maxlen) {
		len = strcspn(quote + sum, "|");
		if (!len)
			break;

		snprintf(clr_quote, QUOTELEN, COLOR "%.2d%.*s", clr, len, quote + sum);
		send_message(server, pdata.target, "%s", clr_quote);
		sum += ++len; // Skip newline
	}
}

void bot_fail(Irc server, struct parsed_data pdata) {

	int argc;
	char **argv, *quote;

	argc = extract_params(pdata.message, &argv);
	if (!argc)
		quote = random_quote();
	else if (strcase_eq(argv[0], "last"))
		quote = get_quote(-1);
	else
		quote = get_quote(get_int(argv[0], 100000));

	if (quote)
		print_quote(server, pdata, quote);

	free(quote);
	free(argv);
}

void bot_fail_add(Irc server, struct parsed_data pdata) {

	int status;

	pdata.message = trim_whitespace(pdata.message);
	if (!pdata.message)
		return;

	if (!user_has_access(server, pdata.sender))
		return;

	status = add_quote(pdata.message, pdata.sender);
	switch (status) {
	case SQLITE_DONE:
		send_message(server, pdata.target, "%s", "quote added!");         break;
	case SQLITE_CONSTRAINT_UNIQUE:
		send_message(server, pdata.target, "%s", "quote already exists"); break;
	default:
		send_message(server, pdata.target, "%s", sqlite3_errstr(status));
	}
}

void bot_fail_modify(Irc server, struct parsed_data pdata) {

	int status;

	pdata.message = trim_whitespace(pdata.message);
	if (!pdata.message)
		return;

	if (!user_has_access(server, pdata.sender))
		return;

	status = modify_last_quote(pdata.message);
	switch (status) {
	case SQLITE_DONE:
		send_message(server, pdata.target, "%s", "quote modified!");       break;
	case SQLITE_READONLY:
		send_message(server, pdata.target, "%s", "modify period elapsed"); break;
	case SQLITE_CONSTRAINT_UNIQUE:
		send_message(server, pdata.target, "%s", "quote already exists");  break;
	default:
		send_message(server, pdata.target, "%s", sqlite3_errstr(status));
	}
}

void bot_url(Irc server, struct parsed_data pdata) {

	int argc;
	char **argv;
	pthread_t id;
	bool thread_active = false;
	char *short_url = NULL;
	char *url_title = NULL;

	argc = extract_params(pdata.message, &argv);
	if (!argc)
		return;

	// Check first parameter for a URL that contains at least one dot.
	if (!strchr(argv[0], '.'))
		goto cleanup;

	if (!pthread_create(&id, NULL, shorten_url, argv[0]))
		thread_active = true;

	url_title = get_url_title(argv[0]);
	if (thread_active)
		pthread_join(id, (void *) &short_url);

	// Only print short_url / title if they are not empty (some stdlibs like glibc will print (NULL) but we can't depend on that)
	send_message(server, pdata.target, "%s -- %s", (short_url ? short_url : ""), (url_title ? url_title : ""));

cleanup:
	free(url_title);
	free(short_url);
	free(argv);
}

void bot_mumble(Irc server, struct parsed_data pdata) {

	char *user_list;

	user_list = fetch_murmur_users();
	if (user_list) {
		send_message(server, pdata.target, "%s", user_list);
		free(user_list);
	}
}

void bot_github(Irc server, struct parsed_data pdata) {

	struct github *commits;
	yajl_val root = NULL;
	int argc, commit_count = 1;
	char **argv, *short_url, repo[REPOLEN + 1];

	argc = extract_params(pdata.message, &argv);
	if (!argc)
		return;

	// If user is not supplied, substitute with a default one
	if (!strchr(argv[0], '/'))
		snprintf(repo, REPOLEN, "%s/%s", cfg.github_repo, argv[0]);
	else {
		strncpy(repo, argv[0], REPOLEN);
		repo[REPOLEN] = '\0';
	}
	// Do not return more than MAXCOMMITS
	if (argc >= 2)
		commit_count = get_int(argv[1], MAXCOMMITS);

	commits = fetch_github_commits(&root, repo, &commit_count);
	if (!commit_count)
		goto cleanup;

	// Print each commit info with it's short url in a separate colorized line
	for (int i = 0; i < commit_count; i++) {
		short_url = shorten_url(commits[i].url);
		send_message(server, pdata.target, PURPLE "[%.7s]" RESET " %.120s" ORANGE " --%s" BLUE " - %s",
				commits[i].sha, commits[i].msg, commits[i].name, (short_url ? short_url : ""));

		free(short_url);
	}
cleanup:
	yajl_tree_free(root);
	free(commits);
	free(argv);
}

void bot_ping(Irc server, struct parsed_data pdata) {

	int argc, count = 3;
	char **argv, *cmd, count_str[5];

	argc = extract_params(pdata.message, &argv);
	if (!argc)
		return;

	// Find if the IP is an v4 or v6. Weak parsing
	if (strchr(argv[0], '.'))
		cmd = "ping";
	else if (strchr(argv[0], ':'))
		cmd = "ping6";
	else {
		free(argv);
		return;
	}
	if (argc >= 2)
		count = get_int(argv[1], MAXPINGS);

	snprintf(count_str, sizeof(count_str), "%d", count);
	print_cmd_output(server, pdata.target, CMD(cmd, "-c", count_str, argv[0]));
	free(argv);
}

void bot_traceroute(Irc server, struct parsed_data pdata) {

	int argc;
	char **argv, *cmd;

	argc = extract_params(pdata.message, &argv);
	if (!argc)
		return;

	if (strchr(argv[0], '.'))
		cmd = "traceroute";
	else if (strchr(argv[0], ':'))
		cmd = "traceroute6";
	else {
		free(argv);
		return;
	}
	// Don't send the following msg if the request was initiated in private
	if (*pdata.target == '#')
		send_message(server, pdata.target, "Printing results privately to %s", pdata.sender);

	// Limit max hops to 18
	print_cmd_output(server, pdata.sender, CMD(cmd, "-m 18", argv[0]));
	free(argv);
}

void bot_dns(Irc server, struct parsed_data pdata) {

	int argc;
	char **argv;

	argc = extract_params(pdata.message, &argv);
	if (!argc)
		return;

	// Strip leading dashes
	while (*argv[0] == '-')
		argv[0]++;

	if (strchr(argv[0], '.'))
		print_cmd_output(server, pdata.target, CMD("nslookup", argv[0]));

	free(argv);
}

void bot_uptime(Irc server, struct parsed_data pdata) {

	print_cmd_output_unsafe(server, pdata.target, "uptime");
}

void bot_roll(Irc server, struct parsed_data pdata) {

	char **argv;
	int argc, r, def_roll = DEFAULT_ROLL;

	argc = extract_params(pdata.message, &argv);
	if (argc >= 1) {
		def_roll = get_int(argv[0], MAXROLL);
		if (def_roll == 1)
			def_roll = DEFAULT_ROLL;

		free(argv);
	}
	r = random() % def_roll + 1;
	send_message(server, pdata.target, "%s rolls %d", pdata.sender, r);
}

void bot_marker(Irc server, struct parsed_data pdata) {

	send_message(server, pdata.target, "%s", "tweet max length. URL's not accounted for:  -  -  -  -  -  60"
			"  -  -  -  -  -  80  -  -  -  -  -  -  100  -  -  -  -  -  120  -  -  -  -  -  140");
}

void bot_tweet(Irc server, struct parsed_data pdata) {

	long http_status;
	char tweet_url[TWEET_URLLEN];

	pdata.message = trim_whitespace(pdata.message);
	if (!pdata.message)
		return;

	if (!cfg.twitter_details_set) {
		send_message(server, pdata.target, "%s", "twitter account details not set");
		return;
	}
	if (!user_has_access(server, pdata.sender)) {
		send_message(server, pdata.target, "%s is not found in the access list or identified to the NickServ", pdata.sender);
		return;
	}
	http_status = send_tweet(pdata.message, tweet_url);
	switch (http_status) {
	case OK:
		send_message(server, pdata.target, "%s", *tweet_url ? tweet_url : "message posted (failed to get tweet id)"); break;
	case FORBIDDEN:
		send_message(server, pdata.target, "%s", "message too long or duplicate");                                    break;
	case UNAUTHORIZED:
		send_message(server, pdata.target, "%s", "authentication error");                                             break;
	default:
		send_message(server, pdata.target, "%s", "unknown error");
	}
}

void bot_fit(Irc server, struct parsed_data pdata) {

	if (fit_status())
		send_message(server, pdata.target, "%s", "freestyl3r is fit!");
	else
		send_message(server, pdata.target, "%s", "freestyl3r is not fit yet :|");
}

STATIC void weather_and_population(Irc server, struct parsed_data pdata, char *type) {

	pdata.message = trim_whitespace(pdata.message);
	if (!pdata.message)
		return;

	print_cmd_output(server, pdata.target, CMD(SCRIPTDIR "weather_and_population.py", type, pdata.message));
}

void bot_weather(Irc server, struct parsed_data pdata) {

	weather_and_population(server, pdata, "-w");
}

void bot_population(Irc server, struct parsed_data pdata) {

	weather_and_population(server, pdata, "-p");
}
