#ifndef IRC_H
#define IRC_H

/**
 * @file irc.h
 * Contains the functions to interact with the IRC network
 */

#include <stddef.h>
#include <sys/types.h>
#include <stdbool.h>

#define IRCLEN   512
#define QUITLEN  160
#define NICKLEN  20
#define USERLEN  15
#define CHANLEN  40
#define ADDRLEN  40
#define PORTLEN  5
#define MAXCHANS 8

/** Pointer to the internal irc struct, making it an incomplete type
 *  Use the available functions in this file to change it's attributes */
typedef struct irc_type *Irc;

/** Holds the info extracted from an IRC message */
typedef struct {
	char *sender;  //!< The person / server who send the message
	char *command; //!< The command type. Examples: "PRIVMSG", "MODE", "433"
	char *target;  //!< The channel (or private) the message is directed at
	char *message; //!< The actual message
} Parsed_data;

/** IRC server numeric replies. See http://www.ietf.org/rfc/rfc1459.txt for a detailed list */
enum irc_reply {
	ENDOFMOTD      = 376, //!< Registration successful, join the channels already set
	NICKNAMEINUSE  = 433, //!< Add an extra '_' to the end of our nickname each time
	BANNEDFROMCHAN = 474  //!< Remove the channel from the joined list
};

/**
 * Connect to an IRC server and set the underlying socket to non-blocking mode
 * Use quit_server() to disconnect from server and destroy the object
 *
 * @param address  hostname / IP
 * @param port     Port in string form
 * @returns        On success: A handler than can be used on every function that requires one or NULL on failure
 */
Irc irc_connect(const char *address, const char *port);

/* Extract the socket descriptor from the opaque Irc object
 * @returns  Always a valid descriptor */
int get_socket(Irc server);

/** Set queue file descriptor */
void set_queue(Irc server, int fd);

/** Returns the first channel set or NULL if there is not one */
char *default_channel(Irc server);

/** Set nickname */
void set_nick(Irc server, const char *nick);

/** Set user. Will use user as a real name as well and set default flags 0.
 *  Can only be set during server connection so it should only be called once */
void set_user(Irc server, const char *user);

/** Find out if the user is in config access list and has identified to the NickServ
 *  @warning  Calling this function from the main proccess will abort (if debug is on) in order to avoid deadlocking */
bool user_has_access(Irc server, const char *nick);

/**
 * Set channel but do not try to join if we are not connected yet
 *
 * @param channel  Join single channel specified. If NULL then join all channels previous set
 * @returns        Number of channels joined or -1 if limit is reached
 */
int join_channel(Irc server, const char *channel);


/* Read line from server, split it into Parsed_data structure elements and launch the function associated with the IRC command
 * @returns  On success: line length, -1 on error, -2 if the operation would block or 0 if connection is closed */
ssize_t parse_irc_line(Irc server);

/** Parse channel / private messages and launch the function that matches the BOT command (must begin with '!') or CTCP request.
 *  Info available in pdata: nick, command, message (the rest message after command, including target)*/
void irc_privmsg(Irc server, Parsed_data pdata);

/** Handle notices. If nick requires identify, the password will be sent and then immediately destroyed */
void irc_notice(Irc server, Parsed_data pdata);

/** Rejoin few secs after being kicked and send message to offender */
void irc_kick(Irc server, Parsed_data pdata);

/* Handle server numeric replies
 * @returns the numeric reply received */
int numeric_reply(Irc Server, Parsed_data pdata, int reply);

//@{
/**
 * Wrappers for sending messages, notices and commands
 * Example: "send_message(server, pdata.target, "hi %s", pdata->sender);"
 * @warning  Do not call _irc_command() directly
 *
 * @param target  channel / person to send message
 * @param format  Standard printf format accepted
 * @param type    command type e.x. PONG, NICK, JOIN etc
 * @param arg     the arguments after type
 */
#define  irc_command(server, type, arg)           _irc_command(server, type, arg, NULL, (char *) NULL)
#define  send_notice(server, target, format, ...) _irc_command(server, "NOTICE",  target, format, __VA_ARGS__)
#define send_message(server, target, format, ...) _irc_command(server, "PRIVMSG", target, format, __VA_ARGS__)
//@}

void _irc_command(Irc server, const char *type, const char *target, const char *format, ...);

/** Close socket and free resources */
void quit_server(Irc server, const char *msg);

#endif

