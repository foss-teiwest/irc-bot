#!/usr/bin/env bash

mpc seek "$1" | awk 'NR==2 {print $3,$4}'
