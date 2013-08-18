#ifndef MPD_H
#define MPD_H

#include "irc.h"

#define CMDLEN 100
#define SCRIPTDIR "scripts/"
#define REMOVE_EXTENSION "gawk -F. -v OFS=. '{NF--; print}'"

void play(Irc server, Parsed_data pdata);
void playlist(Irc server, Parsed_data pdata);
void history(Irc server, Parsed_data pdata);
void current(Irc server, Parsed_data pdata);
void next(Irc server, Parsed_data pdata);

#endif
