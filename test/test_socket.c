#include <unistd.h>
#include <check.h>
#include "socket.h"
#include "irc.h"
#include "test_check.h"

static Irc server;
static int sock;

START_TEST(socket_connection) {

	sock = sock_connect("www.google.com", "80");
	ck_assert_msg(sock >= 0, "Socket creation failed");
	close(sock);

} END_TEST

START_TEST(irc_connection) {

	server = irc_connect("www.google.com", "80");
	ck_assert_msg(server != NULL, "irc connection failed");
	quit_server(server, "bye");

} END_TEST


Suite *socket_suite(void) {

	Suite *suite = suite_create("sockets");
	TCase *core  = tcase_create("core");

	suite_add_tcase(suite, core);
	tcase_add_test(core, socket_connection);
	tcase_add_test(core, irc_connection);

	return suite;
}
