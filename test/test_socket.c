#include <check.h>
#include <string.h>
#include <unistd.h>
#include "socket.h"
#include "irc.h"
#include "test_check.h"

ssize_t sock_readbyte(int sock, char *byte);

extern int mock[2];
char test_buffer[IRCLEN + 1];


START_TEST(socket_connection) {

	int sock = sock_connect("www.google.com", "80");
	ck_assert_msg(sock >= 0, "Socket creation failed");
	close(sock);

} END_TEST


START_TEST(irc_connection) {

	Irc server = irc_connect("www.google.com", "80");
	ck_assert_msg(server != NULL, "irc connection failed");

} END_TEST


START_TEST(socket_readbytes) {

	int i;
	char c;
	const char *msg = "lol\r\ntroll\r\n";

	mock_write(msg, strlen(msg));
	for (i = 0; sock_readbyte(mock[READ], &c) > 0; i++)
		test_buffer[i] = c;

	test_buffer[i] = '\0';
	ck_assert_str_eq(msg, test_buffer);

} END_TEST


START_TEST(socket_readline) {

	char buffer[IRCLEN + 1] = { 0 };
	const char *msg = "lol\r\ntroll\r\n";

	mock_write(msg, strlen(msg));
	while (sock_readline(mock[READ], test_buffer, IRCLEN) > 0)
		strcat(buffer, test_buffer);

	ck_assert_str_eq("loltroll", buffer);

} END_TEST


Suite *socket_suite(void) {

	Suite *suite = suite_create("sockets");
	TCase *core  = tcase_create("core");
	TCase *sread = tcase_create("socket read");

	suite_add_tcase(suite, core);
	tcase_add_test(core, socket_connection);
	tcase_add_test(core, irc_connection);

	suite_add_tcase(suite, sread);
	tcase_add_checked_fixture(sread, mock_start, mock_stop);
	tcase_add_test(sread, socket_readbytes);
	tcase_add_test(sread, socket_readline);

	return suite;
}
