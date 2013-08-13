#ifndef BOT_H
#define BOT_H

#include "irc.h"

#define MAXCOMMITS 10
#define MAXPINGS   10
#define REPOLEN    80
#define QUOTELEN   150

// Irc color codes
#define COLOR   RESET
#define RESET   "\x03"
#define WHITE   "\x03""0"
#define BLACK   "\x03""1"
#define BLUE    "\x03""2"
#define GREEN   "\x03""3"
#define RED     "\x03""4"
#define BROWN   "\x03""5"
#define PURPLE  "\x03""6"
#define ORANGE  "\x03""7"
#define YELLOW  "\x03""8"
#define LTGREEN "\x03""9"
#define TEAL    "\x03""10"
#define LTCYAN  "\x03""11"
#define LTBLUE  "\x03""12"
#define PINK    "\x03""13"
#define GREY    "\x03""14"
#define LTGREY  "\x03""15"
#define COLORCOUNT 16

// *** All BOT functions in this file are running in a new process. They will not crash main program in case of failure ***

// Data available to all bot functions through pdata structure:
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
void help(Irc server, Parsed_data pdata);

// Print latest repo commits
void github(Irc server, Parsed_data pdata);

// Print the result from the command specified. Traceroute will be printed in private to avoid spam
void traceroute(Irc server, Parsed_data pdata);
void ping(Irc server, Parsed_data pdata);
void dns(Irc server, Parsed_data pdata);
void uptime(Irc server, Parsed_data pdata);

#endif
