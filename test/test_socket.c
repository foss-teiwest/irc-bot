#include <check.h>
#include <string.h>
#include <unistd.h>
#include "socket.h"
#include "irc.h"
#include "test_check.h"

ssize_t sock_readbyte(int sock, char *byte);

extern int mock[2];
char test_buffer[IRCLEN + 1];


START_TEST(socket_connect) {

	int sock = sock_connect("www.google.com", "80");
	ck_assert_int_gt(sock, 0);

} END_TEST


START_TEST(socket_listen) {

	ck_assert_int_gt(sock_listen(LOCALHOST, "12345"), 0);
	ck_assert_int_eq(sock_listen(LOCALHOST, "65536"), -1);
	ck_assert_int_eq(sock_listen("121.1.1.1", "12345"), -1);

} END_TEST


START_TEST(socket_accept) {

	int listenfd = sock_listen(LOCALHOST, "12345");
	int fd, acceptfd;

	switch(fork()) {
	case -1:
		ck_abort_msg("fork failed");
	case 0:
		fd = sock_connect(LOCALHOST, "12345");
		ck_assert_int_gt(fd, 0);
		_exit(0);
	}
	acceptfd = sock_accept(listenfd, BLOCK);
	ck_assert_int_gt(acceptfd, 0);

} END_TEST


START_TEST(socket_write) {

	char buf[65] = {0};
	const char *msg = "rofl";
	ssize_t sent, received;

	sent = sock_write(mock[WRITE], msg, strlen(msg));
	received = read(mock[READ], buf, 64);
	ck_assert_int_eq(sent, received);
	ck_assert_str_eq(msg, buf);

	sent = sock_write(mock[WRITE], msg, 0);
	ck_assert_int_eq(sent, 0);
	sent = sock_write(mock[WRITE], msg, -2);
	ck_assert_int_eq(sent, -1);

} END_TEST


START_TEST(socket_write_non_blocking) {

	const char *msg = "rofl";
	ssize_t sent, received;

	sent = sock_write_non_blocking(mock[WRITE], msg, strlen(msg));
	received = read(mock[READ], test_buffer, 64);
	ck_assert_int_eq(sent, received);
	ck_assert_str_eq(test_buffer, msg);

	sent = sock_write_non_blocking(mock[WRITE], msg, 0);
	ck_assert_int_eq(sent, 0);
	sent = sock_write_non_blocking(mock[WRITE], msg, -2);
	ck_assert_int_eq(sent, -1);

} END_TEST


START_TEST(socket_read) {

	const char *msg = "whysobad";
	ssize_t sent, received;

	sent = sock_write(mock[WRITE], msg, strlen(msg));
	received = sock_read(mock[READ], test_buffer, IRCLEN);
	ck_assert_int_eq(sent, received);
	ck_assert_str_eq(test_buffer, msg);

	sock_write(mock[WRITE], msg, strlen(msg));
	received = sock_read(mock[READ], test_buffer, 4);
	ck_assert_int_eq(received, 4);

} END_TEST


START_TEST(socket_read_non_blocking) {

	const char *msg = "whysobad";
	ssize_t sent, received;

	sent = sock_write(mock[WRITE], msg, strlen(msg));
	received = sock_read_non_blocking(mock[READ], test_buffer, IRCLEN);
	ck_assert_int_eq(sent, received);
	ck_assert_str_eq(test_buffer, msg);

	sock_write(mock[WRITE], msg, strlen(msg));
	received = sock_read_non_blocking(mock[READ], test_buffer, 4);
	ck_assert_int_eq(received, 4);

} END_TEST


START_TEST(socket_readbytes) {

	int i;
	char c;
	const char *msg = "lol\r\ntroll\r\n";

	sock_write(mock[WRITE], msg, strlen(msg));
	close(mock[WRITE]);

	for (i = 0; sock_readbyte(mock[READ], &c) > 0; i++)
		test_buffer[i] = c;

	test_buffer[i] = '\0';
	ck_assert_str_eq(msg, test_buffer);

} END_TEST


START_TEST(socket_readline) {

	char buffer[IRCLEN + 1] = {0};
	const char *msg = "lol\r\ntroll\r\n";

	sock_write(mock[WRITE], msg, strlen(msg));
	close(mock[WRITE]);

	while (sock_readline(mock[READ], test_buffer, IRCLEN) > 0)
		strcat(buffer, test_buffer);

	ck_assert_str_eq("loltroll", buffer);

} END_TEST


Suite *socket_suite(void) {

	Suite *suite = suite_create("sockets");
	TCase *core  = tcase_create("core");
	TCase *sread = tcase_create("socket IO");

	suite_add_tcase(suite, core);
	tcase_add_test(core, socket_connect);
	tcase_add_test(core, socket_listen);
	tcase_add_test(core, socket_accept);

	suite_add_tcase(suite, sread);
	tcase_add_checked_fixture(sread, mock_start, mock_stop);
	tcase_add_test(sread, socket_write);
	tcase_add_test(sread, socket_write_non_blocking);
	tcase_add_test(sread, socket_read);
	tcase_add_test(sread, socket_read_non_blocking);
	tcase_add_test(sread, socket_readbytes);
	tcase_add_test(sread, socket_readline);

	return suite;
}
