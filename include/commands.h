#ifndef COMMANDS_H
#define COMMANDS_H

/**
 * @file commands.h
 * Data available to all bot functions through pdata structure:
 * sender: the one initiated the request
 * target: the channel or the query it was requested on
 * command: the actual command that called the function without '!'
 * message: all the text that comes after the command (parameters)

 * @warning All functions that are prepended with bot_ MUST be thread-safe and only call re-entrant functions.
 *          If a bot command crashes, it will bring the whole program down.
 */

#include "irc.h"

#define MAXCOMMITS   10
#define MAXPINGS     10
#define REPOLEN      80
#define QUOTELEN     250
#define DEFAULT_ROLL 100
#define MAXROLL      1000000

// Irc color codes
#define COLOR   "\x03"
#define RESET   COLOR
#define BLUE    COLOR"02"
#define GREEN   COLOR"03"
#define RED     COLOR"04"
#define BROWN   COLOR"05"
#define PURPLE  COLOR"06"
#define ORANGE  COLOR"07"
#define YELLOW  COLOR"08"
#define LTGREEN COLOR"09"
#define TEAL    COLOR"10"
#define LTCYAN  COLOR"11"
#define LTBLUE  COLOR"12"
#define PINK    COLOR"13"
#define GREY    COLOR"14"
#define LTGREY  COLOR"15"
#define COLORCOUNT 14

/** Print available commands. It's a static list. Means you have to update it yourself if you add a command... */
void bot_help(Irc server, struct parsed_data pdata);

/** Takes a URL as a first argument and replies back with a shortened version along with it's title
 *  Current url extraction is weak. It only checks for at least one '.' */
void bot_url(Irc server, struct parsed_data pdata);

/** Get murmur user list by communicating with the ICE protocol */
void bot_mumble(Irc server, struct parsed_data pdata);

/** Add people in the access list */
void bot_access_add(Irc server, struct parsed_data pdata);

/** Print random messages (with random colors) from the quotes list in the config */
void bot_fail(Irc server, struct parsed_data pdata);

/** Add a quote to the database
 *  @warning  Multiline quote sentences must be seperated by the pipe character (|)
 *            Pipe char is optional if the sentence is the last OR the only one in a quote */
void bot_fail_add(Irc server, struct parsed_data pdata);

/** Modify the latest added quote. Only a short time window is available for changes */
void bot_fail_modify(Irc server, struct parsed_data pdata);

/**
 * Print last number_of_commits details
 * Usage: [author/]repo number_of_comits
 * If author is omitted, a default one will be used from config (github_repo). Default number is 1
 */
void bot_github(Irc server, struct parsed_data pdata);

/** Traceroute IPv4 / IPv6 host / IP and print the result in private to avoid spam */
void bot_traceroute(Irc server, struct parsed_data pdata);

/** Ping IPv4 / IPv6 host / IP and print the result. Takes an optional ping count argument. Default is 3 */
void bot_ping(Irc server, struct parsed_data pdata);

/** Print the result of nslookup running on hostname argument */
void bot_dns(Irc server, struct parsed_data pdata);

/** Print uptime command */
void bot_uptime(Irc server, struct parsed_data pdata);

/** Rolls a random number (optional maximum roll number) */
void bot_roll(Irc server, struct parsed_data pdata);

/** Marker to help measuring tweet's max length (140 chars) */
void bot_marker(Irc server, struct parsed_data pdata);

/** Send tweet */
void bot_tweet(Irc server, struct parsed_data pdata);

#endif

