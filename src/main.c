#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "socket.h"
#include "irc.h"
#include "wrapper.h"

int main(void) {

	Irc freenode;
	int ret_value;

	freenode = irc_select_server(Freenode);
	ret_value = irc_connect_server(freenode);
	if (ret_value < 0)
		exit_msg("Irc connection failed\n");

	sleep(20);

	irc_quit_server(freenode);
	return 0;
}