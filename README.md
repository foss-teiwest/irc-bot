# Installation

Build main program with optimizations

`make release`

Build main program with debug symbols

`make`

Run unit tests if check framework is installed

`make test`

Clean output directory

`make clean`

Generate documentation and test coverage

`make doc`

# Usage

Run bot with the included irc-bot.sh script

`./irc-bot path_to_config_file`

If config argument is omitted, it will try to find one in the current working directory

# Dependencies

Library    | Version   | Reason
---        | ---       | ---
curl       | >= 7.0    | Interact with the http protocol
yajl       | >= 2.0.4  | json support for config file and API's like Github's
gperf      | >= 3.0.0  | [optional] Update hash table when adding new bot commands
check      | >= 9.10   | [optional] Run unit tests
lcov       | >= 1.10   | [optional] Generate test coverage html report
doxygen    | >= 1.80   | [optional] Generate documentation
Murmur ice | >= 3.4    | [optional] Murmur integration
youtube-dl | LATEST    | [optional] MPD integration
MPD        | >= 0.16   | [optional] MPD integration
MPC        | >= 0.22   | [optional] MPD integration
mutagen    | >= 1.20   | [optional] MPD integration

# Documentation

[**Doxygen documentation**](https://foss.tesyd.teimes.gr/~freestyler/irc-bot/doc)  
[**Test coverage**](https://foss.tesyd.teimes.gr/~freestyler/irc-bot/coverage/src/index.html)
