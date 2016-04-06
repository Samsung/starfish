#!/bin/bash
BOLD='\033[1m'
RESET='\033[0m'

echo -e "${BOLD}## Converting test files from [test/reftest/csswg-test/${1}] to [test/reftest/csswg-test_converted/${1}]..${RESET}\n"
cat tool/pixel_test/${1}.res | sed "s/csswg-test/csswg-test_converted/g" > tool/reftest/converter/${1}_converted.res
phantomjs tool/reftest/converter/converter.js ${1} | tee tool/reftest/converter/${1}.log
./tool/reftest/converter/convert_diff.sh ${1} > tool/reftest/converter/${1}_converted.diff
