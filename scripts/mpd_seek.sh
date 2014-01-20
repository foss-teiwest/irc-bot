#!/usr/bin/env bash

mpc seek "$1" | gawk 'NR==2 {print $3,$4}'
