#!/usr/bin/env bash

REALPATH=`readlink -f $0`
SCRIPTPATH=`dirname $REALPATH`

while true; do
	$SCRIPTPATH/bin/irc-bot $1
	sleep 5
done
