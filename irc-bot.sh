#!/usr/bin/env bash

nickpwd=xxxxxxxx

echo $nickpwd | bin/irc-bot
retval=$?

while [ $retval -gt 0 ]; do
	sleep 5
	echo $nickpwd | bin/irc-bot
	retval=$?
done
