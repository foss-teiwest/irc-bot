#!/usr/bin/env bash

TIMELIMIT=600
DIR=$1
URL=`echo $2 | grep -o "^[^&]*"`
RANDOM_ON=~/.mpd_random

~/bin/youtube-dl        \
-q                      \
--skip-download         \
--write-info-json       \
--max-quality 22 "$URL" \
-o "$DIR"/song

TITLE=`cat "$DIR"/song.info.json | bin/json_value fulltitle`
DURATION=`cat "$DIR"/song.info.json | bin/json_value duration`
rm "$DIR"/song.info.json

print_song() {
	touch "$DIR"/"$TITLE".mp3
	if [ $QUEUESIZE -eq 0 ]; then
		echo "♪ $TITLE ♪ playing @ https://foss.tesyd.teimes.gr/~zed/flashplayer"
	else
		echo "♪ $TITLE ♪ queued after $QUEUESIZE song(s)..."
	fi
}

if [ -e $RANDOM_ON ]; then
	echo "random mode disabled"
	mpc -q crop
	rm $RANDOM_ON
fi
QUEUESIZE=`mpc playlist | wc -l`


if [ -f "$DIR"/"$TITLE".mp3 ]; then
	mpc add "$TITLE".mp3 && mpc -q play
	if [ $? -eq 0 ]; then
		print_song
	else
		echo "Could not play song"
	fi
	exit
fi

if [ "$DURATION" -gt $TIMELIMIT ]; then
	echo "Song longer than `expr $TIMELIMIT / 60` mins, skipping..."
	exit
fi

~/bin/youtube-dl        \
-q                      \
--max-filesize 150M     \
--extract-audio         \
--audio-format mp3      \
--audio-quality 192k    \
--max-quality 22 "$URL" \
-o "$DIR"/"$TITLE"."%(ext)s"

id3v2 -t "$TITLE" "$DIR"/"$TITLE".mp3
mpc -q update --wait && mpc add "$TITLE".mp3 && mpc -q play
if [ $? -eq 0 ]; then
	print_song
else
	echo "Could not play song"
fi
