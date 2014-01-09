#ifndef BOT_H
#define BOT_H

#include "irc.h"
#include "murmur.h"

/**
 * @file bot.h
 * Data available to all bot functions through pdata structure:
 * sender: the one initiated the request
 * target: the channel or the query it was requested on
 * command: the actual command that called the function without '!'
 * message: all the text that comes after the command (parameters)

 * @warning All BOT functions in this file are running in a new process.
 *  They will not crash main program in case of failure
 */

#define MAXCOMMITS   10
#define MAXPINGS     10
#define REPOLEN      80
#define QUOTELEN     150
#define DEFAULT_ROLL 100
#define MAXROLL      1000000

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


/** Print available commands. It's a static list. Means you have to update it yourself if you add a command... */
void help(Irc server, Parsed_data pdata);

/** Takes a URL as a first argument and replies back with a shortened version along with it's title
 *  Current url extraction is weak. It only checks for at least one '.' */
void url(Irc server, Parsed_data pdata);

/** Get foss-tesyd mumble user list by reading a webpage made specific for this. No parsing involved */
void mumble(Irc server, Parsed_data pdata);

/** Print random messages (with random colors) from the quotes list in the config */
void bot_fail(Irc server, Parsed_data pdata);

/**
 * Print latest commits info in the format "[sha] commit_message --author - short_url"
 *
 * Usage: [author/]repo number_of_comits
 * If author is omitted, a default one will be used from config (github_repo). Default number is 1
 */
void github(Irc server, Parsed_data pdata);

/** Traceroute IPv4 / IPv6 host / IP and print the result in private to avoid spam */
void traceroute(Irc server, Parsed_data pdata);

/** Ping IPv4 / IPv6 host / IP and print the result. Takes an optional ping count argument. Default is 3 */
void ping(Irc server, Parsed_data pdata);

/** Print the result of nslookup running on hostname argument */
void dns(Irc server, Parsed_data pdata);

/** Print uptime command */
void uptime(Irc server, Parsed_data pdata);

/** Rolls a random number (optional maximum roll number) */
void roll(Irc server, Parsed_data pdata);

/** Marker to help measuring tweet's max length (140 chars) */
void marker(Irc server, Parsed_data pdata);

/** Send tweet */
void tweet(Irc server, Parsed_data pdata);

#endif
