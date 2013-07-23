#include <stdio.h>
#include "irc.h"
#include "helper.h"

int main(void) {

	Irc freenode;

	freenode = connect_server("chat.freenode.net", "6667");
	if (freenode == NULL)
		exit_msg("Irc connection failed");

	set_nick(freenode, "fossbot");
	set_user(freenode, "bot");
	set_channel(freenode, "#foss-teimes");

	// Keep reading & parsing lines as long the connection is active and act on any registered actions found
	while (parse_line(freenode) > 0);

	// If we reach here, it means we got disconnected from server. Exit with error (1)
	quit_server(freenode, "poulos");
	return 1;
}
