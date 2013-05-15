#include "bot.h"
#include "irc.h"

char *bot(Irc server, Parsed_data pdata) {

	return send_message(server, pdata->target, "sup %s?", pdata->nick);
}