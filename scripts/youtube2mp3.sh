#!/usr/bin/env bash

DIR=$1
URL=`echo $2 | grep -o "^[^&]*"`
TIMELIMIT=720
RANDOM_ON=~/.mpd_random

IFS=$'\n'
TITLE_AND_DURATION=(`youtube-dl --get-title --get-duration "$URL"`)
TITLE=${TITLE_AND_DURATION[0]}
DURATION=`echo ${TITLE_AND_DURATION[1]} | awk -F: 'NF>1 { print $1 * 60 + $2 } NF==1'`
unset IFS

print_song() {
	touch "$DIR"/"$TITLE".mp3
	QUEUESIZE=`mpc playlist | wc -l`
	if [ $QUEUESIZE -eq 1 ]; then
		echo "♪ $TITLE ♪ playing @ https://foss.teiwest.gr/radio"
	else
		echo "♪ $TITLE ♪ queued after `expr $QUEUESIZE - 1` song(s)..."
	fi
}

disable_random_mode() {
	if [ -e $RANDOM_ON ]; then
		echo "random mode disabled"
		mpc -q crop
		rm $RANDOM_ON
	fi
}

if [ -f "$DIR"/"$TITLE".mp3 ]; then
	disable_random_mode
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
	exit 1
fi

youtube-dl -q                \
--max-filesize 150M          \
--extract-audio              \
--audio-format mp3           \
--audio-quality 192k         \
-f bestaudio/best            \
-o "$DIR"/"$TITLE"."%(ext)s" \
"$URL"

scripts/id3v2_unicode_title.py "$TITLE" "$DIR"/"$TITLE".mp3
disable_random_mode

for (( i = 0; i < 5; i++ )); do
	sleep 2
	mpc -q add "$TITLE".mp3
	if [ $? -eq 0 ]; then
		mpc play &>/dev/null
		print_song
		exit
	fi
done

echo "Could not play song"
