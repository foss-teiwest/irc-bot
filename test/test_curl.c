#include <check.h>
#include <stdlib.h>
#include "test_main.h"
#include "curl.h"

size_t curl_write_memory(char *data, size_t size, size_t elements, void *membuf);

START_TEST(curl_writeback) {

	Mem_buffer mem = {0};
	char *data = "random stuff ftw!";
	size_t n = curl_write_memory(data, strlen(data), 1, &mem);
	ck_assert_str_eq(data, mem.buffer);
	ck_assert_uint_eq(n, strlen(data));

} END_TEST


START_TEST(url_shortener_test) {

	char *short_url = shorten_url("rofl.com");
	ck_assert_str_eq(short_url, "http://goo.gl/LJbW");

} END_TEST


START_TEST(titleurl) {

	char *url_title = get_url_title("https://www.ntua.gr/");
	ck_assert_str_eq(url_title, "ΕΘΝΙΚΟ ΜΕΤΣΟΒΙΟ ΠΟΛΥΤΕΧΝΕΙΟ");

} END_TEST


START_TEST(github_commits) {

	Github *commits;
	yajl_val root = NULL;
	int n = 10;

	commits = fetch_github_commits(&root, "foss-teimes/irc-bot", &n);

	ck_assert_str_eq(commits[0].sha,  "de7579c08e35f232af4938dc7dc325b9809d63bf");
	ck_assert_str_eq(commits[0].name, "Bill Kolokithas");
	ck_assert_str_eq(commits[0].msg,  "small changes to the shell helper script");
	ck_assert_str_eq(commits[0].url,  "https://github.com/foss-teimes/irc-bot/commit/de7579c08e35f232af4938dc7dc325b9809d63bf");
	ck_assert_str_eq(commits[2].sha,  "363e91d5e12701be9001bcf62062fd0a93561082");
	ck_assert_str_eq(commits[2].name, "Bill Kolokithas");
	ck_assert_str_eq(commits[2].msg,  "restart bot on failed exit");
	ck_assert_str_eq(commits[2].url,  "https://github.com/foss-teimes/irc-bot/commit/363e91d5e12701be9001bcf62062fd0a93561082");

} END_TEST


Suite *curl_suite(void) {

	Suite *suite = suite_create("curl");
	TCase *core  = tcase_create("core");

	suite_add_tcase(suite, core);
	tcase_add_test(core, curl_writeback);
	tcase_add_test(core, url_shortener_test);
	tcase_add_test(core, titleurl);
	tcase_add_test(core, github_commits);

	return suite;
}
