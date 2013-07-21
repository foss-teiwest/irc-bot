#!/usr/bin/env bash

nickpwd=xxxxxxxx

echo $nickpwd | bin/irc-bot
retval=$?

while [ $retval -eq 142 ]; do
	sleep 2
	echo $nickpwd | bin/irc-bot
	retval=$?
done
