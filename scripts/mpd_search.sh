#!/usr/bin/env bash

DIR=$1
QUERY=$2
RESULT=`mpc search filename "$QUERY"`
RESULT_COUNT=`echo "$RESULT" | wc -l`

if [ $RESULT_COUNT -eq 1 ]; then
	mpc add "$RESULT" && mpc -q play
	if [ $? -eq 0 ]; then
		touch "$DIR"/"$RESULT".mp3
		QUEUESIZE=`mpc playlist | wc -l`
		echo "♪ $RESULT ♪ queued after `expr $QUEUESIZE - 1` song(s)..."
	else
		echo "Could not play song"
	fi
else
	echo "$RESULT_COUNT results found. Printing first 3..."
	echo "$RESULT" | head -n3 | gawk -F. -v OFS=. '{NF--; print}'
fi
