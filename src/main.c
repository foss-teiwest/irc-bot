#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "irc.h"
#include "wrappers.h"

int main(void) {

	Irc freenode;
	char *line;
	Parsed_data pdata;
	curl_global_init(CURL_GLOBAL_ALL);

	line = malloc_w(BUFSIZE * sizeof(char) + 1); // Space for null char
	pdata = malloc_w(sizeof(*pdata));
	freenode = connect_server(Freenode);
	if (freenode == NULL)
		exit_msg("Irc connection failed");

	// Use server's predefined values
	set_nick(freenode, NULL);
	set_user(freenode, NULL);
	join_channel(freenode, NULL);

	// Keep running as long the connection is active and act on any registered actions found
	while (get_line(freenode, line) > 0)
		parse_line(freenode, line, pdata);

	free(line);
	free(pdata);
	quit_server(freenode, NULL); // No quit message
	curl_global_cleanup();
	return 0;
}