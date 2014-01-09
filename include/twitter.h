#ifndef TWITTER_H
#define TWITTER_H

#define MAXLIST  10
#define DEFNONCE 32
#define TWTLEN 2560
#define TWTURL "https://api.twitter.com/1.1/statuses/update.json"

long send_tweet(char *message);

#endif
