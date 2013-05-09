#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "socket.h"
#include "irc.h"
#include "wrapper.h"

int main(void) {

	Irc freenode;
	int socket;

	socket = sock_connect(SERVER, PORT);
	if (socket < 0)
		exit_msg("Socket creation failed\n");

	freenode = irc_connect(socket, NICK, USER);
	if (irc_connected(freenode) == false)
		exit_msg("Irc connection failed\n");

	irc_join(freenode, CHAN);

	sleep(15);

	return 0;
}