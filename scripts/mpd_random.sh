#!/usr/bin/env bash

RANDOM_ON=~/.mpd_random
BEFORE=`mpc playlist | wc -l`

if [ ! -e $RANDOM_ON ] || [ $BEFORE -eq 0 ]; then
	mpc add "" && mpc -q random on && mpc -q play
	AFTER=`mpc playlist | wc -l`
	echo "`expr $AFTER - $BEFORE` songs queued @ http://foss.tesyd.teimes.gr:8000/"
	echo "playlist & history commands will not give meaningful results while in random mode"
	touch $RANDOM_ON
else
	echo "already in random mode"
fi
