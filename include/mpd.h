#ifndef MPD_H
#define MPD_H

/**
 * @file mpd.h
 * Contains the functions to interact with MPD
 * External helper scripts will be used to do most of the work
 */

#include <stdbool.h>
#include "irc.h"

#define ON  true
#define OFF false
#define CMDLEN  100
#define SONG_INFO_LEN  512
#define SONG_TITLE_LEN 256

/** Remove file extension. Works with multiple dots in file as well */
#define REMOVE_EXTENSION "gawk -F. -v OFS=. '{NF--; print}'"

struct mpd_info {
	int fd;
	bool random;
	bool announce;
};

/** Download video from youtube, convert it to mp3, feed it to mpd and start streaming in icecast.
 *  If a youtube url is detected, download the video, convert it to mp3, feed it to mpd and start streaming in icecast.
 *  If there are no arguments, queue up all local songs and play them in random mode
 *  If a song name is entered a search will be performed. If it's a single result it will be added to queue, else up to 3 results will be printed */
void play(Irc server, Parsed_data pdata);

/** Auto announce songs as they play (on | off) */
void announce(Irc server, Parsed_data pdata);

/** Current playlist. First song is the one playing. */
void playlist(Irc server, Parsed_data pdata);

/** Previous played songs. First hit is the older one */
void history(Irc server, Parsed_data pdata);

/** Current song */
void current(Irc server, Parsed_data pdata);

/** Stops playback. Works in normal or random mode */
void stop(Irc server, Parsed_data pdata);

/** Skip song and print the title of the next */
void next(Irc server, Parsed_data pdata);

/** Seek to an absolute (2:53) or relative (+-) time */
void seek(Irc server, Parsed_data pdata);

/** Connect to mpd daemon and verify the reply
 *
 * @param port     MPD's default one is 6600
 * @return         a valid fd or -1 for error
 */
int mpd_connect(const char *port);

/** Announce current song playing in target. It will only print song if it's not the same as the last one.
 *  @warning  It keeps a static array for song comparison. Will restart the query for a next song automatically
 */
bool print_song(Irc server, const char *target);

#endif

