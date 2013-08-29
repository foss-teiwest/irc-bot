#ifndef MPD_H
#define MPD_H

#include "irc.h"

/**
 * @file mpd.h
 * Contains the functions to interact with MPD
 * A helper script will be used (youtube2mp3.sh) to do most of the work
 */

#define CMDLEN 100
#define SCRIPTDIR "scripts/" //!< default folder to look for scripts like the youtube one
#define REMOVE_EXTENSION "gawk -F. -v OFS=. '{NF--; print}'" //!< Remove file extension. Works with multiple dots in file as well

/** Download video from youtube, convert it to mp3, feed it to mpd and start streaming in icecast.
 *  If there is no dot '.' on the argument, then a search will be performed.
 *  If there is a single result it will be added to queue, else up to 3 results will be printed */
void play(Irc server, Parsed_data pdata);

/** Current playlist. First song is the one playing */
void playlist(Irc server, Parsed_data pdata);

/** Previous played songs. First hit is the older one */
void history(Irc server, Parsed_data pdata);

/** Current song */
void current(Irc server, Parsed_data pdata);

/** Skip song and print the title of the next */
void next(Irc server, Parsed_data pdata);

#endif
