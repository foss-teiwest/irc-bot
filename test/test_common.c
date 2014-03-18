#include <check.h>
#include <unistd.h>
#include "test_main.h"
#include "common.h"

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


START_TEST(cmd_output_unsafe) {

	print_cmd_output_unsafe(server, "#test", "echo rofl");
	n = read(mock[READ], test_buffer, IRCLEN);
	test_buffer[n - 2] = '\0';
	ck_assert_str_eq(test_buffer, "PRIVMSG #test :rofl");

} END_TEST


Suite *common_suite(void) {

	Suite *suite = suite_create("common");
	TCase *core  = tcase_create("core");
	TCase *output  = tcase_create("output");

	suite_add_tcase(suite, core);
	tcase_add_test(core, getint);
	tcase_add_test(core, parameter_extraction);

	tcase_add_unchecked_fixture(output, connect_irc, disconnect_irc);
	tcase_add_checked_fixture(output, mock_irc_write, mock_stop);
	suite_add_tcase(suite, output);
	tcase_add_test(output, cmd_output_unsafe);

	return suite;
}

