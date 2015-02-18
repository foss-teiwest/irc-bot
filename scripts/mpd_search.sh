#!/usr/bin/env bash

DIR=$1
COUNT=`echo $2 | cut -c -2 | grep -o "[0-9]*"`
QUERY=$3
RANDOM_ON=~/.mpd_random
RESULT=`mpc search filename "$QUERY"`
RESULT_BYTES=`echo "$RESULT" | wc -c`
RESULT_LINES=`echo "$RESULT" | wc -l`

if [ $RESULT_BYTES -lt 5 ]; then
	exit 1
fi

if [ $COUNT -gt 10 ]; then
	COUNT=10
fi

if [ $RESULT_LINES -eq 1 ]; then
	if [ -e $RANDOM_ON ]; then
		echo "random mode disabled"
		mpc -q crop
		rm $RANDOM_ON
	fi
	QUEUESIZE=`mpc playlist | wc -l`
	mpc add "$RESULT" && mpc -q play
	if [ $? -eq 0 ]; then
		touch "$DIR"/"$RESULT"
		RESULT=`echo "$RESULT" | awk -F. -v OFS=. '{NF--; print}'`

		if [ $QUEUESIZE -eq 0 ]; then
			echo "♪ $RESULT ♪ playing @ https://foss.teiwest.gr/radio"
		else
			echo "♪ $RESULT ♪ queued after $QUEUESIZE song(s)..."
		fi
	fi
else
	if [ $COUNT -gt $RESULT_LINES ]; then
		COUNT=$RESULT_LINES
	fi
	echo "$RESULT_LINES results found. Printing first $COUNT..."
	echo "$RESULT" | head -n$COUNT | awk -F. -v OFS=. '{NF--; print}'
	exit 1
fi
