#include <check.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "test_main.h"
#include "socket.h"
#include "irc.h"
#include "init.h"

void ctcp_handle(Irc server, struct parsed_data pdata);

ssize_t n;

START_TEST(irc_get_socket) {

	ck_assert_int_eq(get_socket(server), server->conn);

} END_TEST

START_TEST(irc_set_nick) {

	set_nick(server, "trololol");
	ck_assert_str_eq(server->nick, "trololol");

	read(mock[RD], test_buffer, IRCLEN);
	ck_assert_str_eq(test_buffer, "NICK trololol\r\n");

} END_TEST

START_TEST(irc_numeric_NICKNAMEINUSE) {

	set_nick(server, "trololol");
	numeric_reply(server, pdata, NICKNAMEINUSE);
	if (server->nick[strlen(server->nick) - 1] != '_')
		ck_abort_msg("nick rename failed");

} END_TEST

START_TEST(irc_numeric_ENDOFMOTD) {

	join_channel(server, "#eleos");
	join_channel(server, "#eleos2");
	numeric_reply(server, pdata, ENDOFMOTD);
	read(mock[RD], test_buffer, IRCLEN);
	ck_assert_str_eq(test_buffer, "JOIN #eleos\r\nJOIN #eleos2\r\n");

} END_TEST

START_TEST(irc_numeric_BANNEDFROMCHAN) {

	char msg[] = "#trololol re";
	pdata.message = msg;

	server->connected = true;
	join_channel(server, "#trololol");
	join_channel(server, "#trololol2");
	ck_assert_str_eq(server->channels[0], "#trololol");
	numeric_reply(server, pdata, BANNEDFROMCHAN);
	ck_assert_int_eq(server->channels_set, 1);
	ck_assert_str_eq(server->channels[0], "#trololol2");

	numeric_reply(server, pdata, BANNEDFROMCHAN);
	ck_assert_int_eq(server->channels_set, 0);

} END_TEST

START_TEST(irc_set_user) {

	set_user(server, "trololol");
	ck_assert_str_eq(server->user, "trololol");

	read(mock[RD], test_buffer, IRCLEN);
	ck_assert_str_eq(test_buffer, "USER trololol 0 * :trololol\r\n");

} END_TEST

START_TEST(irc_join_channel) {

	server->connected = true;
	join_channel(server, "#trololol");
	ck_assert_str_eq(server->channels[0], "#trololol");

	read(mock[RD], test_buffer, IRCLEN);
	ck_assert_str_eq(test_buffer, "JOIN #trololol\r\n");

} END_TEST

START_TEST(irc_join_channels) {

	join_channel(server, "#foss-teimes");
	join_channel(server, "#trolltown");

	server->connected = true;
	ck_assert_int_eq(join_channel(server, NULL), 2);
	ck_assert_str_eq(server->channels[0], "#foss-teimes");
	ck_assert_str_eq(server->channels[1], "#trolltown");

	read(mock[RD], test_buffer, IRCLEN);
	ck_assert_str_eq(test_buffer, "JOIN #foss-teimes\r\nJOIN #trolltown\r\n");

} END_TEST

START_TEST(irc_default_channel) {

	join_channel(server, "#foss-teimes");
	ck_assert_str_eq(default_channel(server), "#foss-teimes");

} END_TEST

START_TEST(irc_command_test) {

	_irc_command(server, "PRIVMSG", "#yolo", "%s1!", "why so bad");
	n = read(mock[RD], test_buffer, IRCLEN);
	test_buffer[n - 2] = '\0';
	ck_assert_str_eq(test_buffer, "PRIVMSG #yolo :why so bad1!");

	_irc_command(server, "NICK", "yolo", NULL, NULL);
	n = read(mock[RD], test_buffer, IRCLEN);
	test_buffer[n - 2] = '\0';
	ck_assert_str_eq(test_buffer, "NICK yolo");

} END_TEST

START_TEST(irc_quit_server) {

	quit_server(server, "trolol");
	n = read(mock[RD], test_buffer, IRCLEN);
	test_buffer[n - 2] = '\0';
	ck_assert_str_eq(test_buffer, "QUIT :trolol");

} END_TEST

START_TEST(irc_ctcp_time) {

	pdata.sender = "mitsos";
	pdata.command = "\x01TIME";

	ctcp_handle(server, pdata);
	n = read(mock[RD], test_buffer, IRCLEN);
	ck_assert(test_buffer[n - 4] != '\n');

} END_TEST

START_TEST(irc_ctcp_ping) {

	pdata.sender = "mitsos";
	pdata.command = "\x01ping";
	pdata.message = NULL;

	ctcp_handle(server, pdata);
	read(mock[RD], test_buffer, IRCLEN);
	char *temp = strpbrk(test_buffer, "1234567890");
	ck_assert_ptr_ne(temp, NULL);
	ck_assert_int_gt(strlen(temp), 18);

	pdata.message = "123456789 1234";
	ctcp_handle(server, pdata);
	n = read(mock[RD], test_buffer, IRCLEN);
	test_buffer[n] = '\0';
	ck_assert_str_eq(test_buffer, "NOTICE mitsos :\x01PING 123456789 1234\x01\r\n");

} END_TEST

START_TEST(irc_parse_line_ping) {

	const char *msg = "hm\r\n";

	write(mock[WR], msg, strlen(msg));
	ck_assert_int_eq(parse_irc_line(server), strlen(msg) - 2);

	msg = "PING :wolfe.freenode.net\r\n";
	write(mock[WR], msg, strlen(msg));
	parse_irc_line(server);
	read(mock[WR], test_buffer, IRCLEN);
	ck_assert_str_eq(test_buffer, "PONG :wolfe.freenode.net\r\n");

} END_TEST

START_TEST(irc_parse_line_tokens) {

	const char *msg = ":laxanofido!~laxanofid@snf-23545.vm.okeanos.grnet.gr PRIVMSG1 #foss-teimes :How YA doing fossbot\r\n";

	int len = strlen(msg);
	write(mock[WR], msg, len);
	ck_assert_int_eq(parse_irc_line(server), len - 2);

	msg = ":laxanofido!~laxanofid@snf-23545.vm.okeanos.grnet.gr\0PRIVMSG1\0#foss-teimes :How YA doing fossbot";
	if (memcmp(server->line, msg, len - 2))
		ck_abort();

} END_TEST

START_TEST(irc_parse_line_offset) {

	const char *msg = "half";

	fcntl(mock[RD], F_SETFL, O_NONBLOCK);
	write(mock[WR], msg, 4);
	n = parse_irc_line(server);
	ck_assert_int_eq(n, -EAGAIN);

	msg = "life\r\n";
	write(mock[WR], msg, 6);
	n = parse_irc_line(server);
	ck_assert_int_eq(n, 8);
	ck_assert_str_eq(server->line, "halflife");

} END_TEST

START_TEST(irc_parse_line_length) {

	char buf1[300] = {'a'};
	char buf2[250] = {'b'};

	memset(buf1, 'a', sizeof(buf1));
	memset(buf2, 'b', sizeof(buf2));
	buf2[248] = '\r';
	buf2[249] = '\n';

	fcntl(mock[RD], F_SETFL, O_NONBLOCK);
	write(mock[WR], buf1, strlen(buf1));
	n = parse_irc_line(server);
	ck_assert_int_eq(n, -EAGAIN);

	write(mock[WR], buf2, strlen(buf2));
	n = parse_irc_line(server);
	ck_assert_int_eq(n, 512);

} END_TEST

START_TEST(irc_privemsg) {

	char msg[] = "freestyl3r!~laxanofid@snf-23545.vm.okeanos.grnet.gr\0PRIVMSG\0freestylerbot :!bot lol re";

	pdata.sender  = &msg[0];
	pdata.command = &msg[52];
	pdata.message = &msg[60];

	irc_privmsg(server, pdata);
	ck_assert_str_eq(pdata.sender, "freestyl3r");
	ck_assert_str_eq(pdata.message, "freestylerbot");

} END_TEST

START_TEST(irc_ctcp_version) {

	char sender[] = "bot!~a@b.c";
	char message[] = "bot :\x01VERSION\x01";
	cfg.bot_version = "irC bot";
	pdata.sender = sender;
	pdata.message = message;

	irc_privmsg(server, pdata);
	n = read(mock[WR], test_buffer, IRCLEN);
	test_buffer[n - 2] = '\0';

	char buf[256];
	sprintf(buf, "NOTICE bot :\x01VERSION irC bot %s\x01", VERSION);
	ck_assert_str_eq(test_buffer, buf);

} END_TEST

START_TEST(irc_privemsg_command) {

	memcpy(server->line, "bot!~a@b.c\0bot :!marker", IRCLEN);
	char *reply = "PRIVMSG bot :tweet max length. URL's not accounted for:  -  -  -  -  -  60"
			"  -  -  -  -  -  80  -  -  -  -  -  -  100  -  -  -  -  -  120  -  -  -  -  -  140";

	pdata.sender  = &server->line[0];
	pdata.message = &server->line[11];
	pdata.target  = pdata.sender;

	irc_privmsg(server, pdata);
	n = read(mock[WR], test_buffer, IRCLEN);
	test_buffer[n - 2] = '\0';
	ck_assert_str_eq(test_buffer, reply);

} END_TEST

START_TEST(irc_notice_identify) {

	char sender[] = "NickServ!~a@b.c";
	char message[] = "bot :This nickname is registeredTRAILING";
	char password[] = "lololol";
	cfg.nick_password = password;
	pdata.sender = sender;
	pdata.message = message;

	irc_notice(server, pdata);
	n = read(mock[WR], test_buffer, IRCLEN);
	test_buffer[n - 2] = '\0';
	ck_assert_str_eq(test_buffer, "PRIVMSG NickServ :identify lololol");
	ck_assert_str_eq(cfg.nick_password, "");

} END_TEST

START_TEST(irc_kick_test) {

	char sender[] = "noob!~a@b.c";
	char message[] = "#hey bot";
	strcpy(server->nick, "bot");
	pdata.sender = sender;
	pdata.message = message;

	irc_kick(server, pdata);
	n = read(mock[WR], test_buffer, IRCLEN);
	test_buffer[n - 2] = '\0';
	ck_assert_str_eq(test_buffer, "PRIVMSG #hey :magkas noob...");

} END_TEST

Suite *irc_suite(void) {

	Suite *suite = suite_create("irc");
	TCase *core  = tcase_create("core");
	TCase *parse = tcase_create("parse");

	suite_add_tcase(suite, core);
	tcase_add_unchecked_fixture(core, connect_irc, disconnect_irc);
	tcase_add_checked_fixture(core, mock_irc_write, mock_stop);
	tcase_add_test(core, irc_get_socket);
	tcase_add_test(core, irc_numeric_NICKNAMEINUSE);
	tcase_add_test(core, irc_numeric_ENDOFMOTD);
	tcase_add_test(core, irc_numeric_BANNEDFROMCHAN);
	tcase_add_test(core, irc_set_nick);
	tcase_add_test(core, irc_set_user);
	tcase_add_test(core, irc_join_channel);
	tcase_add_test(core, irc_join_channels);
	tcase_add_test(core, irc_default_channel);
	tcase_add_test(core, irc_command_test);
	tcase_add_test(core, irc_quit_server);
	tcase_add_test(core, irc_ctcp_ping);
	tcase_add_test(core, irc_ctcp_time);

	suite_add_tcase(suite, parse);
	tcase_add_unchecked_fixture(parse, connect_irc, disconnect_irc);
	tcase_add_unchecked_fixture(parse, mock_irc_read, mock_stop);
	tcase_add_test(parse, irc_parse_line_ping);
	tcase_add_test(parse, irc_parse_line_tokens);
	tcase_add_test(parse, irc_parse_line_offset);
	tcase_add_test(parse, irc_parse_line_length);
	tcase_add_test(parse, irc_privemsg);
	tcase_add_test(parse, irc_ctcp_version);
	tcase_add_test(parse, irc_privemsg_command);
	tcase_add_test(parse, irc_notice_identify);
	tcase_add_test(parse, irc_kick_test);

	return suite;
}

