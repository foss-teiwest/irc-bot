#!/usr/bin/env bash

RANDOM_ON=~/.mpd_random

mpc add "" && mpc -q random on && mpc -q play
QUEUESIZE=`mpc playlist | wc -l`
echo "$QUEUESIZE songs queued @ http://foss.tesyd.teimes.gr:8000/"
touch $RANDOM_ON
