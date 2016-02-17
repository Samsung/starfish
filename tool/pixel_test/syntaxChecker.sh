#!/bin/bash
BOLD='\033[1m'
RESET='\033[0m'

echo -e "${BOLD}## Syntax checking for html files in [test/reftest/csswg-test/${1}]..${RESET}\n"
phantomjs tool/pixel_test/syntaxChecker.js ${1} > tmp.res
cat tmp.res | grep "###"
mv tmp.res tool/pixel_test/${1}.log
cat tool/pixel_test/${1}.log | grep -v "#" > tool/pixel_test/${1}.res
