#!/bin/bash
#tool/phantomjs/linux64/bin/phantomjs --web-security=false --local-to-remote-url-access=true runner.js $1 $2 $3 $4 $5 $6 $7
#set -x #echo on
./StarFish  $1 $2 $3 $4 $5 $6 $7 2> /dev/null
