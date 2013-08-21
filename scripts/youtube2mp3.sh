#!/usr/bin/env bash

URL=`echo $1 | grep -o "^[^&]*"`
TITLE=`youtube-dl --get-title --playlist-end 1 $URL`
DIR=$2

print_song() {

	QUEUESIZE=`mpc playlist | wc -l`
	echo "♪ $TITLE ♪ queued after `expr $QUEUESIZE - 1` song(s)..."
}

if [ -f "$DIR/$TITLE.mp3" ]; then
	mpc add "$TITLE".mp3 && mpc -q play
	if [ $? -eq 0 ]; then
		print_song
	else
		echo "Could not play song"
	fi
else
	youtube-dl            \
	-q                    \
	--skip-download       \
	--playlist-end 1      \
	--write-info-json     \
	--max-quality 22 $URL \
	-o "song"

	DURATION=`cat song.info.json | json_reformat | grep duration | grep -o '[0-9]*'`
	rm "song".info.json

	if [ "$DURATION" -gt 480 ]; then
		echo "Song longer than 8 mins, skipping..."
	else
		youtube-dl            \
		-q                    \
		--playlist-end 1      \
		--max-filesize 100M   \
		--extract-audio       \
		--audio-format mp3    \
		--audio-quality 192k  \
		--max-quality 22 $URL \
		-o "$DIR/$TITLE"."%(ext)s"

		mpc -q update --wait && mpc add "$TITLE".mp3 && mpc -q play
		if [ $? -eq 0 ]; then
			print_song
		else
			echo "Could not play song"
		fi
	fi
fi
