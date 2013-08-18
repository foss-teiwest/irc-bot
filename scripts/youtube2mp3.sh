#!/usr/bin/env bash

TITLE=`youtube-dl --get-title $1`
DIR=$2

if [ -f "$DIR/$TITLE.mp3" ];
then
	mpc add "$TITLE".mp3 && mpc -q play
	echo "♪ $TITLE ♪ added in queue...";
else
	youtube-dl \
	-q \
	--skip-download \
	--write-info-json \
	-f 22/35/34 $1 \
	-o "$DIR/$TITLE".mp3

	DURATION=`cat "$DIR/$TITLE".mp3.info.json | json_reformat | grep duration | grep -o '[0-9]*'`

	if [ "$DURATION" -gt "500" ];
	then
		echo "Error: The duration looks like my dick... too long!";
		rm "$DIR/$TITLE".mp3;
	else
		youtube-dl \
		-q \
		--extract-audio \
		--audio-format mp3 \
		--audio-quality 192k \
		-f 22/35/34 $1 \
		-o "$DIR/$TITLE".mp3

		mpc -q update --wait && mpc add "$TITLE".mp3 && mpc -q play
		echo "♪ $TITLE ♪ added in queue...";
	fi
	rm "$DIR/$TITLE".mp3.info.json
fi
