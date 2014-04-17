#ifndef INIT_H
#define INIT_H

/**
 * @file init.h
 * Initialization & setup functions that are called only once
 */

#include <stdio.h>
#include "irc.h"

#define DEFAULT_CONFIG_NAME "config.json"
#define CONFSIZE     4096
#define PATHLEN      120
#define MAXACCLIST   10
#define MAXQUOTES    20
#define POLL_TIMEOUT 300 * MILLISECS

enum poll_array {IRC, MURM_LISTEN, MURM_ACCEPT, MPD, FIFO, TOTAL};

/** Parse arguments, load config, install signal handlers etc
 *  @param argc, argv main's parameters unaltered */
void initialize(int argc, char *argv[]);

//@{
/** The return file descriptors are always valid. exit() is called on failure */
Irc setup_irc(void);
int setup_mumble(void);
int setup_mpd(void);
int setup_fifo(FILE **stream);
//@}

/** Cleanup init's mess */
void cleanup(void);

#endif

