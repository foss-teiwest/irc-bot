#ifndef MPD_H
#define MPD_H

#include "irc.h"

#define CMDLEN 100
#define SCRIPTDIR "scripts/"
#define REMOVE_EXTENSION "gawk -F. -v OFS=. '{NF--; print}'"

// Queue up song and start streaming
void play(Irc server, Parsed_data pdata);

// Currnet playlist. First song is the one playing
void playlist(Irc server, Parsed_data pdata);

// Last played songs
void history(Irc server, Parsed_data pdata);

// Current song
void current(Irc server, Parsed_data pdata);

// Skip song and print the title of the next
void next(Irc server, Parsed_data pdata);

#endif
