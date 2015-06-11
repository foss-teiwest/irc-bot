#!/usr/bin/env bash

OPERATION=$1
CURRENT=bin/irc-bot
OLD=irc-bot-old

if [ $OPERATION == "-u" ]; then
	cp -f $CURRENT $OLD
	git pull > /dev/null 2>&1 && make -s -j3 release
	if [ $? -ne 0 ]; then
		echo "Error while updating git or compile error"
		exit 1
	fi
	cmp -s $CURRENT $OLD
	if [ $? -eq 0 ]; then
		echo "Already at the latest version"
		exit 1
	fi
elif [ $OPERATION == "-d" ]; then
	cmp -s $CURRENT $OLD
	if [ $? -eq 0 ]; then
		echo "Already at the oldest version"
		exit 1
	fi
	cp -f $OLD $CURRENT
fi
