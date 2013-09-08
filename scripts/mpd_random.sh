#!/usr/bin/env bash

RANDOM_ON=~/.mpd_random
BEFORE=`mpc playlist | wc -l`

mpc add "" && mpc -q random on && mpc -q play
AFTER=`mpc playlist | wc -l`
echo "`expr $AFTER - $BEFORE` songs queued @ http://foss.tesyd.teimes.gr:8000/"
touch $RANDOM_ON
