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
}

void mock_write(const void *buffer, size_t len) {

	switch(fork()) {
	case -1:
		ck_abort_msg("fork failed");
	case 0:
		close(mock[READ]);
		if (write(mock[WRITE], buffer, len) != (int) len)
			ck_abort_msg("write failed");

		close(mock[WRITE]);
		_exit(0);
	}
	close(mock[WRITE]);
}

