#include <check.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "test_check.h"

int mock[2];

void mock_start(void) {

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, mock))
		ck_abort_msg("socketpair failed");
}

void mock_stop(void) {

	close(mock[READ]);
	close(mock[WRITE]);
}

