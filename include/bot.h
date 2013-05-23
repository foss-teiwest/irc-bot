#ifndef BOT_H
#define BOT_H

#include "irc.h"

// Data available to all bot functions through pdata:
// nick: the one initiated the request
// target: the channel or the query it was requested on
// command: the actual command that called the function without '@'
// message: all the text that comes after the command (parameters)
// All functions return the actual message send to server (raw form)

// Reply with "sup" back to sender
char *bot(Irc server, Parsed_data pdata);

// Extracts long url from message and replies back with a shortened version
// Current url extraction is very weak. The url must come as the first parameter and it must have at least one '.'
char *url(Irc server, Parsed_data pdata);

// Get foss-tesyd mumble user list
char *mumble(Irc server, Parsed_data pdata);

// Send funny messages
char *fail(Irc server, Parsed_data pdata);

// Print available commands
char *list(Irc server, Parsed_data pdata);

#endif