#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bot.h"
#include "irc.h"
#include "http.h"
#include "helper.h"


char *list(Irc server, Parsed_data pdata) {

        return send_message(server, pdata->target, "list, help, bot, url, mumble, fail");
}

char *bot(Irc server, Parsed_data pdata) {

	return send_message(server, pdata->target, "sup %s?", pdata->nick);
}

char *url(Irc server, Parsed_data pdata) {

	char *short_url, **argv, *temp = NULL;
	int argc;

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

char *mumble(Irc server, Parsed_data pdata) {

	char *user_list, *temp;

	user_list = fetch_mumble_users();
	temp = send_message(server, pdata->target, "%s", user_list);

	free(user_list);
	return temp;
}

char *fail(Irc server, Parsed_data pdata){

	send_message(server, pdata->target, "I mpala einai strogili");
	sleep(5);
	send_message(server, pdata->target, "to gipedo einai paralilogramo");
        sleep(5);
        send_message(server, pdata->target, "11 autoi, 11 emeis sinolo 23");
	sleep(5);
        return send_message(server, pdata->target, "kai tha boun kai 3 allages apo kathe omada sinolo 29!");
}
