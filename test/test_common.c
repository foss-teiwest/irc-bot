#include <check.h>
#include <string.h>
#include <unistd.h>
#include "test_main.h"
#include "common.h"
#include "irc.h"
#include "socket.h"

ssize_t n;

START_TEST(getint) {

	ck_assert_int_eq(get_int("234", 50), 50);
	ck_assert_int_eq(get_int("-234", 50), 1);
	ck_assert_int_eq(get_int("0", 50), 1);
	ck_assert_int_eq(get_int("0", 0), 0);
	ck_assert_int_eq(get_int("543463464463634634436346436346", 2), 2);
	ck_assert_int_eq(get_int("-543463464463634634436346436346", 2), 1);
	ck_assert_int_eq(get_int("432efgdger", 21324124), 432);
	ck_assert_int_eq(get_int("sdfsdf462", 2), 1);

} END_TEST

START_TEST(parameter_extraction) {

	char msg[] = " 	trolol  re noob  	";
	char **argv;
	int argc;

	argc = extract_params(msg, &argv);
	ck_assert_int_eq(argc, 3);
	ck_assert_str_eq(argv[0], "trolol");
	ck_assert_str_eq(argv[1], "re");
	ck_assert_str_eq(argv[2], "noob");

} END_TEST

START_TEST(strings_compare) {

	if (!streq("lol", "lol"))
		ck_abort_msg("streq");

	if (!starts_with("heya", "hey"))
		ck_abort_msg("starts_with");

	if (!starts_case_with("fR333", "fr33"))
		ck_abort_msg("starts_case_with");

} END_TEST

START_TEST(nullterminate) {

	strcpy(test_buffer, "|free|");
	null_terminate(test_buffer, '|');
	ck_assert_str_eq(test_buffer, "");

	strcpy(test_buffer, "||free|");
	null_terminate(test_buffer, '|');
	ck_assert_str_eq(test_buffer, "");

	strcpy(test_buffer, "free");
	null_terminate(test_buffer, '|');
	ck_assert_str_eq(test_buffer, "free");

} END_TEST

START_TEST(trim_trailing) {

	char *test;

	ck_assert(!trim_whitespace(NULL));

	strcpy(test_buffer, "");
	test = trim_whitespace(test_buffer);
	ck_assert_ptr_eq(test, NULL);

	strcpy(test_buffer, "                    ");
	test = trim_whitespace(test_buffer);
	ck_assert_ptr_eq(test, NULL);

	strcpy(test_buffer, "  a  ");
	test = trim_whitespace(test_buffer);
	ck_assert_str_eq(test, "a");

	strcpy(test_buffer, "   a");
	test = trim_whitespace(test_buffer);
	ck_assert_str_eq(test, "a");

	strcpy(test_buffer, "   a   ");
	test = trim_whitespace(test_buffer);
	ck_assert_str_eq(test, "a");

} END_TEST

START_TEST(iso8859_7_to_utf8_test) {

	char *conv = iso8859_7_to_utf8("\xc5\xc8\xcd\xc9\xca\xcf\x20\xcc\xc5\xd4\xd3\xcf\xc2\xc9\xcf\x20\xd0\xcf\xcb\xd5\xd4\xc5\xd7\xcd\xc5\xc9\xcf");
	ck_assert_str_eq(conv, "ΕΘΝΙΚΟ ΜΕΤΣΟΒΙΟ ΠΟΛΥΤΕΧΝΕΙΟ");

} END_TEST

START_TEST(cmd_output_unsafe) {

	print_cmd_output_unsafe(server, "#test", "echo rofl");
	n = read(mock[RD], test_buffer, IRCLEN);
	test_buffer[n - 2] = '\0';
	ck_assert_str_eq(test_buffer, "PRIVMSG #test :rofl");

} END_TEST

START_TEST(cmd_output) {

	print_cmd_output(server, "#test", CMD("echo", "rofl"));
	n = read(mock[RD], test_buffer, IRCLEN);
	test_buffer[n - 2] = '\0';
	ck_assert_str_eq(test_buffer, "PRIVMSG #test :rofl");

} END_TEST

Suite *common_suite(void) {

	Suite *suite  = suite_create("common");
	TCase *core   = tcase_create("core");
	TCase *output = tcase_create("output");

	suite_add_tcase(suite, core);
	tcase_add_test(core, getint);
	tcase_add_test(core, parameter_extraction);
	tcase_add_test(core, strings_compare);
	tcase_add_test(core, nullterminate);
	tcase_add_test(core, trim_trailing);
	tcase_add_test(core, iso8859_7_to_utf8_test);

	suite_add_tcase(suite, output);
	tcase_add_unchecked_fixture(output, connect_irc, disconnect_irc);
	tcase_add_unchecked_fixture(output, mock_irc_write, mock_stop);
	tcase_add_test(output, cmd_output_unsafe);
	tcase_add_test(output, cmd_output);

	return suite;
}

