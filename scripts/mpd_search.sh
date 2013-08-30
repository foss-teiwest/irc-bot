#!/usr/bin/env bash

DIR=$1
QUERY=$2
RESULT=`mpc search filename "$QUERY"`
RESULT_BYTES=`echo "$RESULT" | wc -c`
RESULT_LINES=`echo "$RESULT" | wc -l`

if [ $RESULT_BYTES -lt 5 ]; then
	exit
fi

if [ $RESULT_LINES -eq 1 ]; then
	mpc add "$RESULT" && mpc -q play
	if [ $? -eq 0 ]; then
		touch "$DIR"/"$RESULT".mp3
		RESULT=`echo "$RESULT" | gawk -F. -v OFS=. '{NF--; print}'`
		QUEUESIZE=`mpc playlist | wc -l`
		if [ $QUEUESIZE -eq 1 ]; then
			echo "♪ $RESULT ♪ playing @ http://foss.tesyd.teimes.gr:8000/"
		else
			echo "♪ $RESULT ♪ queued after `expr $QUEUESIZE - 1` song(s)..."
		fi
	fi
else
	echo "$RESULT_LINES results found. Printing first 3..."
	echo "$RESULT" | head -n3 | gawk -F. -v OFS=. '{NF--; print}'
fi
