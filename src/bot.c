#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bot.h"
#include "irc.h"
#include "url.h"

char *bot(Irc server, Parsed_data pdata) {

	return send_message(server, pdata->target, "sup %s?", pdata->nick);
}

char *url(Irc server, Parsed_data pdata) {

	struct mem_buffer mem = {0};
	char *short_url, *long_url, *temp;

	long_url = strtok(pdata->message, " "); // Get to the first non-space char
	if (long_url == NULL)
		return NULL;

	// Make sure the first parameter is null terminated
	temp = strtok(NULL, " ");
	if (temp == NULL) {
		temp = strchr(long_url, '\r');
		*temp = '\0';
	}
	// Must contain at least one dot. NOTE: need more checks to identify a valid url
	if (strchr(long_url, '.') == NULL)
		return NULL;

	short_url = shorten_url(long_url, &mem);
	if (short_url != NULL)
		temp = send_message(server, pdata->target, "%s", short_url);

	free(mem.buffer);
	return temp;
}