#!/usr/bin/env bash

RANDOM_ON=~/.mpd_random
QUEUESIZE=`mpc playlist | wc -l`

if [ ! -e $RANDOM_ON ] || [ $QUEUESIZE -eq 0 ]; then
	mpc ls | sort -R | mpc add && mpc -q play
	QUEUESIZE=`mpc playlist | wc -l`
	echo "$QUEUESIZE songs queued @ https://foss.tesyd.teimes.gr/radio"
	echo 'use "!announce on" to begin the spam'
	touch $RANDOM_ON
else
	echo "already in random mode"
	exit 1
fi
