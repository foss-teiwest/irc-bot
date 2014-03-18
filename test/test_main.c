#include <check.h>
#include <unistd.h>
#include <sys/socket.h>
#include "test_main.h"
#include "irc.h"

struct irc_type {
	int sock;
	int pipe[2];
	char line[IRCLEN + 1];
	size_t line_offset;
	char address[ADDRLEN];
	char port[PORTLEN];
	char nick[NICKLEN];
	char user[USERLEN];
	char channels[MAXCHANS][CHANLEN];
	int channels_set;
	bool isConnected;
};

Irc server;
int mock[2];
Parsed_data pdata;
char test_buffer[IRCLEN + 1];

void connect_irc(void) {

	server = irc_connect("www.google.com", "80");
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
	server->sock = mock[READ];
}

void mock_irc_write(void) {

	mock_start();
	server->sock = mock[WRITE];
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
	srunner_add_suite(sr, common_suite());

	srunner_run_all(sr, CK_ENV);
	tests_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	// 0 or 1 only
	return !!tests_failed;
}

