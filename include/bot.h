#ifndef BOT_H
#define BOT_H

#include "irc.h"

#define CMDLEN 200
#define MAXCOMMITS 10
#define MAXPINGCOUNT 10
#define REPOLEN 60
#define DEFAULT_USER_REPO "foss-teimes"

// Data available to all bot functions through pdata:
// sender: the one initiated the request
// target: the channel or the query it was requested on
// command: the actual command that called the function without '!'
// message: all the text that comes after the command (parameters)

// Extracts url from message and replies back with a shortened version along with it's title
// Current url extraction is weak. The url must come as the first parameter and it must have at least one '.'
void url(Irc server, Parsed_data pdata);

// Get foss-tesyd mumble user list
void mumble(Irc server, Parsed_data pdata);

// Print random funny messages
void bot_fail(Irc server, Parsed_data pdata);

// Print available commands
void list(Irc server, Parsed_data pdata);

// Print latest repo commits
void github(Irc server, Parsed_data pdata);

// Print the result from the command specified. Traceroute will be printed in private to avoid spam
void traceroute(Irc server, Parsed_data pdata);
void ping(Irc server, Parsed_data pdata);
void dns(Irc server, Parsed_data pdata);

#endif