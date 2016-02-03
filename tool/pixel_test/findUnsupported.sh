#!/bin/sh
echo "Processing.."
phantomjs testsetChecker.js $1 $2 $3 > result.sh
echo "COMPLETE checking files within [test/unittest/"$1"]..."
echo "If you want to remove unsupported files, execute result.sh"
chmod +x result.sh
