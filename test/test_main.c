#include <check.h>
#include <unistd.h>
#include <sys/socket.h>
#include "test_main.h"

int mock[2];

void mock_start(void) {

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, mock))
		ck_abort_msg("socketpair failed");
}

void mock_stop(void) {

	close(mock[READ]);
	close(mock[WRITE]);
}

int main(void) {

	int tests_failed;

	SRunner *sr = srunner_create(socket_suite());
	srunner_add_suite(sr, irc_suite());
	srunner_add_suite(sr, curl_suite());

	srunner_run_all(sr, CK_ENV);
	tests_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	// 0 or 1 only
	return !!tests_failed;
}

