#!/bin/bash
BOLD='\033[1m'
RESET='\033[0m'

echo -e "${BOLD}## Syntax checking for html files in [test/reftest/csswg-test/${1}]..${RESET}\n"
./tool/phantomjs/phantomjs-2.1.1 tool/pixel_test/syntaxChecker.js ${1} > tmp.res
cat tmp.res | grep "###"
cp tmp.res tool/pixel_test/${1}.log
cat tool/pixel_test/${1}.log | grep -v "#" | grep "htm" > tool/pixel_test/${1}.res
