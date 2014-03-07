#include <check.h>
#include "test_check.h"

int main(void) {

	int tests_failed;

	SRunner *sr = srunner_create(socket_suite());
	// srunner_add_suite(sr, );

	srunner_run_all(sr, CK_ENV);
	tests_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	// 0 or 1 only
	return !!tests_failed;
}

