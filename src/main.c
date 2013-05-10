#include <stdio.h>
#include "irc.h"
#include "wrapper.h"

int main(void) {

	Irc freenode;
	int ret_value;
	char *buffer;

	freenode = irc_select_server(Freenode);
	ret_value = irc_connect_server(freenode);
	if (ret_value < 0)
		exit_msg("Irc connection failed\n");

	// Incomplete
	while (irc_parse_line(freenode, buffer) > 0) ;

	irc_quit_server(freenode);
	return 0;
}