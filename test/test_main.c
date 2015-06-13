#include <check.h>
#include <unistd.h>
#include <sys/socket.h>
#include "test_main.h"
#include "irc.h"

Irc server;
int mock[RDWR];
struct parsed_data pdata;
char test_buffer[IRCLEN + 1];
struct pollfd pfd[TOTAL];

void connect_irc(void) {

	server = irc_connect("www.google.com", "80", 0);
	ck_assert_ptr_ne(server, NULL);
}

void disconnect_irc(void) {

	quit_server(server, "bye");
}

void mock_start(void) {

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, mock))
		ck_abort_msg("socketpair failed");
}

void mock_irc_read(void) {

	mock_start();
	server->conn = mock[RD];
}

void mock_irc_write(void) {

	mock_start();
	server->conn = mock[WR];
}

void mock_stop(void) {

	close(mock[RD]);
	close(mock[WR]);
}

int main(void) {

	int tests_failed;

	SRunner *sr = srunner_create(socket_suite());
	srunner_add_suite(sr, irc_suite());
	srunner_add_suite(sr, curl_suite());
	srunner_add_suite(sr, common_suite());

	srunner_run_all(sr, CK_ENV);
	tests_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	// 0 or 1 only
	return !!tests_failed;
}

