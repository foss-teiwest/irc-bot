#include <check.h>
#include <unistd.h>
#include "test_check.h"
#include "socket.h"
#include "irc.h"
#include "common.h"

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
extern int mock[2];
char test_buffer[IRCLEN + 1];

void connect_irc(void) {

	server = irc_connect("www.google.com", "80");
	ck_assert_ptr_ne(server, NULL);
}

void disconnect_irc(void) {

	quit_server(server, "bye");
}

void mock_irc_write(void) {

	mock_start();
	server->sock = mock[WRITE];
}

void mock_irc_read(void) {

	mock_start();
	server->sock = mock[READ];
}

START_TEST(irc_connection) {

	Irc test = irc_connect("www.google.com", "80000");
	ck_assert_ptr_eq(test, NULL);

} END_TEST


START_TEST(irc_get_socket) {

	ck_assert_int_eq(get_socket(server), server->sock);

} END_TEST


START_TEST(irc_set_nick) {

	set_nick(server, "trololol");
	ck_assert_str_eq(server->nick, "trololol");

	read(mock[READ], test_buffer, IRCLEN);
	ck_assert_str_eq(test_buffer, "NICK trololol\r\n");

} END_TEST


START_TEST(irc_numeric_reply) {

	set_nick(server, "trololol");
	numeric_reply(server, NICKNAMEINUSE);
	if (server->nick[strlen(server->nick) - 1] != '_')
		ck_abort_msg("nick rename failed");

} END_TEST


START_TEST(irc_set_user) {

	set_user(server, "trololol");
	ck_assert_str_eq(server->user, "trololol");

	read(mock[READ], test_buffer, IRCLEN);
	ck_assert_str_eq(test_buffer, "USER trololol 0 * :trololol\r\n");

} END_TEST


START_TEST(irc_join_channel) {

	server->isConnected = true;
	join_channel(server, "#trololol");
	ck_assert_str_eq(server->channels[0], "#trololol");

	read(mock[READ], test_buffer, IRCLEN);
	ck_assert_str_eq(test_buffer, "JOIN #trololol\r\n");

} END_TEST


START_TEST(irc_join_channels) {

	join_channel(server, "#foss-teimes");
	join_channel(server, "#trolltown");

	server->isConnected = true;
	ck_assert_int_eq(join_channel(server, NULL), 2);
	ck_assert_str_eq(server->channels[0], "#foss-teimes");
	ck_assert_str_eq(server->channels[1], "#trolltown");

	read(mock[READ], test_buffer, IRCLEN);
	ck_assert_str_eq(test_buffer, "JOIN #foss-teimes\r\nJOIN #trolltown\r\n");

} END_TEST


START_TEST(irc_default_channel) {

	join_channel(server, "#foss-teimes");
	ck_assert_str_eq(default_channel(server), "#foss-teimes");

} END_TEST


START_TEST(irc_parse_line_ping) {

	const char *msg = "hm\r\n";

	write(mock[WRITE], msg, strlen(msg));
	ck_assert_int_eq(parse_irc_line(server), strlen(msg) - 2);

	msg = "PING :wolfe.freenode.net\r\n";
	write(mock[WRITE], msg, strlen(msg));
	parse_irc_line(server);
	read(mock[WRITE], test_buffer, IRCLEN);
	ck_assert_str_eq(test_buffer, "PONG :wolfe.freenode.net\r\n");

} END_TEST


START_TEST(irc_parse_line_tokens) {

	const char *msg = ":laxanofido!~laxanofid@snf-23545.vm.okeanos.grnet.gr PRIVMSG1 #foss-teimes :How YA doing fossbot\r\n";

	int len = strlen(msg);
	write(mock[WRITE], msg, len);
	ck_assert_int_eq(parse_irc_line(server), len - 2);

	msg = ":laxanofido!~laxanofid@snf-23545.vm.okeanos.grnet.gr\0PRIVMSG1\0#foss-teimes :How YA doing fossbot";
	if (memcmp(server->line, msg, len - 2))
		ck_abort();

} END_TEST


START_TEST(irc_privemsg) {

	Parsed_data pdata;
	char msg[] = "freestyl3r!~laxanofid@snf-23545.vm.okeanos.grnet.gr\0PRIVMSG\0freestylerbot :!bot lol re";

	pdata.sender  = &msg[0];
	pdata.command = &msg[52];
	pdata.message = &msg[60];

	irc_privmsg(server, pdata);
	ck_assert_str_eq(pdata.sender, "freestyl3r");
	ck_assert_str_eq(pdata.message, "freestylerbot");

} END_TEST

Suite *irc_suite(void) {

	Suite *suite = suite_create("irc");
	TCase *core  = tcase_create("core");
	TCase *parse = tcase_create("parse");

	suite_add_tcase(suite, core);
	tcase_add_unchecked_fixture(core, connect_irc, disconnect_irc);
	tcase_add_checked_fixture(core, mock_irc_write, mock_stop);
	tcase_add_test(core, irc_connection);
	tcase_add_test(core, irc_get_socket);
	tcase_add_test(core, irc_numeric_reply);
	tcase_add_test(core, irc_set_nick);
	tcase_add_test(core, irc_set_user);
	tcase_add_test(core, irc_join_channel);
	tcase_add_test(core, irc_join_channels);
	tcase_add_test(core, irc_default_channel);

	suite_add_tcase(suite, parse);
	tcase_add_unchecked_fixture(parse, connect_irc, disconnect_irc);
	tcase_add_checked_fixture(parse, mock_irc_read, mock_stop);
	tcase_add_test(parse, irc_parse_line_ping);
	tcase_add_test(parse, irc_parse_line_tokens);
	tcase_add_test(parse, irc_privemsg);

	return suite;
}

