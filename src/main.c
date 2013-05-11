#include <stdio.h>
#include "irc.h"
#include "wrapper.h"

int main(void) {

	Irc freenode;
	int status;
	char line[BUFSIZE + 1]; // Space for null char

	freenode = select_server(Freenode);
	status = connect_server(freenode);
	if (status < 0)
		exit_msg("Irc connection failed");

	while (get_line(freenode, line) > 0)
		if (*line == 'P')
			ping_reply(freenode, line);

	quit_server(freenode);
	return 0;
}