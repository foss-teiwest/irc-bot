#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <curl/curl.h>
#include "irc.h"
#include "helper.h"

int main(void) {

	Irc freenode;

	curl_global_init(CURL_GLOBAL_ALL); // Initialize curl library
	signal(SIGCHLD, SIG_IGN); // Instruct child processes to not leave zombies behind when killed
	main_pid = getpid(); // store our process id to help exit_msg function exit appropriately

	freenode = connect_server("wolfe.freenode.net", "6667");
	if (freenode == NULL)
		exit_msg("Irc connection failed");

	set_nick(freenode, "fossbot");
	set_user(freenode, "bot");
	set_channel(freenode, "#foss-teimes");

	// Keep reading & parsing lines as long the connection is active and act on any registered actions found
	while (parse_line(freenode) > 0)
		continue;

	// Close server connection & cleanup curl library
	quit_server(freenode, "poulo");
	curl_global_cleanup();
	return 0;
}