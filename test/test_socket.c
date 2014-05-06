#include <check.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include "test_main.h"
#include "socket.h"
#include "irc.h"

ssize_t sock_readbyte(int sock, char *byte);

START_TEST(socket_connect) {

	int sock = sock_connect("www.google.com", "80");
	ck_assert_int_gt(sock, 0);

} END_TEST

START_TEST(socket_listen) {

	ck_assert_int_gt(sock_listen(LOCALHOST, "12345"), 0);
	ck_assert_int_eq(sock_listen("121.1.1.1", "12345"), -1);

} END_TEST

START_TEST(socket_accept) {

	int listenfd = sock_listen(LOCALHOST, "12345");
	int fd, acceptfd;

	if (fork() == 0) {
		fd = sock_connect(LOCALHOST, "12345");
		ck_assert_int_gt(fd, 0);
		fd = sock_connect(LOCALHOST, "12345");
		ck_assert_int_gt(fd, 0);
		_exit(0);
	}
	acceptfd = sock_accept(listenfd, 0);
	ck_assert_int_gt(acceptfd, 0);
	ck_assert_int_eq(fcntl(acceptfd, F_GETFL, 0) & O_NONBLOCK, 0);

	acceptfd = sock_accept(listenfd, NONBLOCK);
	ck_assert_int_gt(acceptfd, 0);
	ck_assert_int_ne(fcntl(acceptfd, F_GETFL, 0) & O_NONBLOCK, 0);

} END_TEST

START_TEST(socket_write) {

	const char *msg = "rofl";
	ssize_t sent, received;

	sent = sock_write(mock[WR], msg, strlen(msg));
	received = read(mock[RD], test_buffer, IRCLEN);
	ck_assert_int_eq(sent, received);
	ck_assert_str_eq(msg, test_buffer);

	sent = sock_write(mock[WR], msg, 0);
	ck_assert_int_eq(sent, 0);
	sent = sock_write(mock[WR], msg, -2);
	ck_assert_int_eq(sent, -1);

} END_TEST

START_TEST(socket_write_non_blocking) {

	const char *msg = "rofl";
	ssize_t sent, received;

	fcntl(mock[WR], F_SETFL, O_NONBLOCK);
	sent = sock_write(mock[WR], msg, strlen(msg));
	received = read(mock[RD], test_buffer, 64);
	ck_assert_int_eq(sent, received);
	ck_assert_str_eq(test_buffer, msg);

	sent = sock_write(mock[WR], msg, 0);
	ck_assert_int_eq(sent, 0);
	sent = sock_write(mock[WR], msg, -2);
	ck_assert_int_eq(sent, -1);

} END_TEST

START_TEST(socket_read) {

	const char *msg = "whysobad";
	ssize_t sent, received;

	sent = write(mock[WR], msg, strlen(msg));
	received = sock_read(mock[RD], test_buffer, IRCLEN);
	ck_assert_int_eq(sent, received);
	ck_assert_str_eq(test_buffer, msg);

	write(mock[WR], msg, strlen(msg));
	received = sock_read(mock[RD], test_buffer, 4);
	ck_assert_int_eq(received, 4);

} END_TEST

START_TEST(socket_read_non_blocking) {

	const char *msg = "whysobad";
	ssize_t received;

	fcntl(mock[RD], F_SETFL, O_NONBLOCK);
	write(mock[WR], msg, 0);
	received = sock_read(mock[RD], test_buffer, IRCLEN);
	ck_assert_int_lt(received, 0);

	write(mock[WR], msg, strlen(msg));
	received = sock_read(mock[RD], test_buffer, 4);
	ck_assert_int_eq(received, 4);

} END_TEST

START_TEST(socket_readbytes) {

	int i;
	char c;
	const char *msg = "lol\r\ntroll\r\n";

	sock_write(mock[WR], msg, strlen(msg));
	close(mock[WR]);

	for (i = 0; sock_readbyte(mock[RD], &c) > 0; i++)
		test_buffer[i] = c;

	test_buffer[i] = '\0';
	ck_assert_str_eq(msg, test_buffer);

} END_TEST

START_TEST(socket_readline_blocking) {

	const char *msg = "lol";

	sock_write(mock[WR], msg, strlen(msg));
	alarm(1);
	sock_readline(mock[RD], test_buffer, IRCLEN);
	alarm(0);
	ck_abort();

} END_TEST

START_TEST(socket_readline_blocking2) {

	const char *msg = "lol\r\n";

	sock_write(mock[WR], msg, strlen(msg));
	ssize_t n = sock_readline(mock[RD], test_buffer, IRCLEN);
	ck_assert_int_eq(n, 5);

} END_TEST

START_TEST(socket_readline_non_blocking) {

	const char *msg = "yo\r\n";

	fcntl(mock[WR], F_SETFL, O_NONBLOCK);
	sock_write(mock[WR], msg, strlen(msg));
	ssize_t n = sock_readline(mock[RD], test_buffer, IRCLEN);
	ck_assert_int_eq(n, 4);

} END_TEST

START_TEST(socket_readline_non_blocking2) {

	const char *msg = "yo";
	ssize_t n;
	size_t offset;

	fcntl(mock[RD], F_SETFL, O_NONBLOCK);
	sock_write(mock[WR], msg, strlen(msg));
	n = sock_readline(mock[RD], test_buffer, IRCLEN);
	ck_assert_int_eq(n, -EAGAIN);

	msg = "\r\nhey\r\n";
	sock_write(mock[WR], msg, strlen(msg));
	offset = strlen(test_buffer);
	n = sock_readline(mock[RD], test_buffer + offset, IRCLEN);
	ck_assert_int_eq(n, 2);
	ck_assert_str_eq(test_buffer, "yo\r\n");

	n = sock_readline(mock[RD], test_buffer, IRCLEN);
	ck_assert_int_eq(n, 5);
	ck_assert_str_eq(test_buffer, "hey\r\n");

} END_TEST

Suite *socket_suite(void) {

	Suite *suite  = suite_create("socket");
	TCase *core   = tcase_create("core");
	TCase *sockIO = tcase_create("socket IO");

	suite_add_tcase(suite, core);
	tcase_add_test(core, socket_connect);
	tcase_add_test(core, socket_listen);
	tcase_add_test(core, socket_accept);

	suite_add_tcase(suite, sockIO);
	tcase_add_checked_fixture(sockIO, mock_start, mock_stop);
	tcase_add_test(sockIO, socket_write);
	tcase_add_test(sockIO, socket_write_non_blocking);
	tcase_add_test(sockIO, socket_read);
	tcase_add_test(sockIO, socket_read_non_blocking);
	tcase_add_test(sockIO, socket_readbytes);
	tcase_add_test_raise_signal(sockIO, socket_readline_blocking, SIGKILL);
	tcase_add_test(sockIO, socket_readline_blocking2);
	tcase_add_test(sockIO, socket_readline_non_blocking);
	tcase_add_test(sockIO, socket_readline_non_blocking2);

	return suite;
}
