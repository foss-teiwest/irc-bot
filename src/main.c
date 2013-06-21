#include <stdio.h>
#include "irc.h"
#include "helper.h"

int main(void) {

	Irc freenode;

	freenode = connect_server("wolfe.freenode.net", "6667");
	if (freenode == NULL)
		exit_msg("Irc connection failed");

	set_nick(freenode, "fossbot");
	set_user(freenode, "bot");
	set_channel(freenode, "#foss-teimes");

	// Keep reading & parsing lines as long the connection is active and act on any registered actions found
	while (parse_line(freenode) > 0);

	quit_server(freenode, "poulos");
	return 0;
}