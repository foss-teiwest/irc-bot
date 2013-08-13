#include <stdio.h>
#include "irc.h"
#include "helper.h"

int main(void) {

	Irc freenode;

	parse_config();

	freenode = connect_server(cfg.server, cfg.port);
	if (freenode == NULL)
		exit_msg("Irc connection failed");

	set_nick(freenode, cfg.nick);
	set_user(freenode, cfg.user);
	set_channels(freenode, cfg.ch.channels, cfg.ch.channels_set);

	// Keep reading & parsing lines as long the connection is active and act on any registered actions found
	while (parse_line(freenode) > 0);

	// If we reach here, it means we got disconnected from server. Exit with error (1)
	quit_server(freenode, cfg.quit_msg);

	return 0;
}
