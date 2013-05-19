#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bot.h"
#include "irc.h"
#include "http.h"
#include "helper.h"


char *bot(Irc server, Parsed_data pdata) {

	return send_message(server, pdata->target, "sup %s?", pdata->nick);
}

char *url(Irc server, Parsed_data pdata) {

	char *short_url, **argv, *temp = NULL;
	int argc;

	// Make sure we have at least 1 parameter before proceeding
	if (pdata->message == NULL)
		return NULL;

	argv = extract_params(pdata->message, &argc);
	if (argc == 0)
		return NULL;

	// Check first parameter for a URL that contains at least one dot.
	if (strchr(argv[0], '.') == NULL)
		return NULL;

	short_url = shorten_url(argv[0]);
	if (short_url != NULL)
		temp = send_message(server, pdata->target, "%s", short_url);

	free(argv);
	free(short_url);
	return temp;
}