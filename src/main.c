#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>
#include "irc.h"
#include "helper.h"


int main(void) {

	Irc freenode;
	char nick_pwd[NICKLEN];

	// Initialize curl library and store our process id to help exit_msg function exit appropriately,
	// if the process that called it was a child
	curl_global_init(CURL_GLOBAL_ALL);
	main_pid = getpid();

	freenode = connect_server("wolfe.freenode.net", "6667");
	if (freenode == NULL)
		exit_msg("Irc connection failed");

	set_nick(freenode, "fossbot");
	set_user(freenode, "bot");
	set_channel(freenode, "#foss-teimes");

	printf("Enter nick identify password: ");
	if (scanf("%19s", nick_pwd) != EOF) // Don't identify if password is not given
		identify_nick(freenode, nick_pwd);

	// Keep reading & parsing lines as long the connection is active and act on any registered actions found
	while (parse_line(freenode) > 0) ;

	// Close server connection
	quit_server(freenode, "poulos");
	curl_global_cleanup();
	return 0;
}