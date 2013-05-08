#ifndef WRAPPER_H
#define WRAPPER_H

#include <stdio.h>
#include <stdlib.h>

void exit_msg(const char *msg) {

	perror(msg);
	exit(EXIT_FAILURE);
}

void exit_msg_error(const char *msg, const char *error) {

	fprintf(stderr, "%s: %s\n", msg, error);
	exit(EXIT_FAILURE);
}

#endif