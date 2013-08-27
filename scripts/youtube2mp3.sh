#!/usr/bin/env bash

TIMELIMIT=550
DIR=$1
URL=`echo $2 | grep -o "^[^&]*"`

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
	QUEUESIZE=`mpc playlist | wc -l`
	echo "♪ $TITLE ♪ queued after `expr $QUEUESIZE - 1` song(s)..."
}

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
	echo "Song longer than `expr $TIMELIMIT / 60`:`expr $TIMELIMIT % 60` mins, skipping..."
	exit
fi

~/bin/youtube-dl        \
-q                      \
--max-filesize 120M     \
--extract-audio         \
--audio-format mp3      \
--audio-quality 192k    \
--max-quality 22 "$URL" \
-o "$DIR"/"$TITLE"."%(ext)s"

mpc -q update --wait && mpc add "$TITLE".mp3 && mpc -q play
if [ $? -eq 0 ]; then
	print_song
else
	echo "Could not play song"
fi
