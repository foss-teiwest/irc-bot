#!/usr/bin/env bash

RANDOM_ON=~/.mpd_random
QUEUESIZE=`mpc playlist | wc -l`

if [ ! -e $RANDOM_ON ] || [ $QUEUESIZE -eq 0 ]; then
	mpc ls | shuf | mpc add && mpc -q play
	QUEUESIZE=`mpc playlist | wc -l`
	echo "$QUEUESIZE songs queued @ http://radio.foss.teiwest.gr/"
	echo 'use "!announce on" to see on IRC which song plays'
	touch $RANDOM_ON
else
	echo "already in random mode @ http://radio.foss.teiwest.gr/"
	exit 1
fi
