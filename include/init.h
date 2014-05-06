#ifndef INIT_H
#define INIT_H

/**
 * @file init.h
 * Initialization & setup functions that are called only once
 */

#include <stdio.h>
#include <stdbool.h>
#include "irc.h"

#define DEFAULT_CONFIG_NAME "config.json"
#define FIFO_PERMISSIONS (S_IRUSR | S_IWUSR | S_IWGRP | S_IWOTH)

#define CONFSIZE      4096
#define PATHLEN       120
#define MAXACCLIST    10
#define MAXQUOTES     20
#define POLL_TIMEOUT (300 * MILLISECS)

struct config_options {
	char *server;
	char *port;
	char *nick;
	char *nick_password;
	char *user;
	char *channels[MAXCHANS];
	int channels_set;
	char *bot_version;
	char *github_repo;
	char *quit_message;
	char *murmur_port;
	char *mpd_port;
	char *mpd_database;
	char *mpd_random_file;
	char *fifo_path;
	char *oauth_consumer_key;
	char *oauth_consumer_secret;
	char *oauth_token;
	char *oauth_token_secret;
	bool twitter_details_set;
	char *access_list[MAXACCLIST];
	int access_list_count;
	char *quotes[MAXQUOTES];
	int quote_count;
	bool verbose;
};

extern struct config_options cfg; //!< global struct with config's values

/** Parse arguments, load config, install signal handlers etc
 *  @param argc, argv main's parameters unaltered */
void initialize(int argc, char *argv[]);

//@{
/** The return file descriptors are always valid. exit() is called on failure */
int setup_irc(Irc *server);
int setup_mumble(void);
int setup_mpd(void);
int setup_fifo(FILE **stream);
//@}

/** Cleanup init's mess */
void cleanup(void);

#endif

