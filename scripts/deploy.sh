#!/usr/bin/env bash

OPERATION=$1
NEW=versions/irc-bot-new
OLD=versions/irc-bot-old
CURRENT=bin/irc-bot

if [ "$OPERATION" == "-u" ]; then
	git fetch > /dev/null 2>&1
	reslog=$(git log HEAD..origin/master --oneline)
	if [[ "${reslog}" != "" ]]; then
		git rebase origin/master > /dev/null 2>&1
		cp -f $CURRENT $OLD
		make -s -j3 release
		cp -f $CURRENT $NEW
		exit
	fi

	cmp -s $CURRENT $NEW && echo "Already at the latest version" && exit 1
	cp -f $NEW $CURRENT
elif [ "$OPERATION" == "-d" ]; then
	cmp -s $OLD $CURRENT && echo "Already at the oldest version" && exit 1
	cp -f $OLD $CURRENT
fi
