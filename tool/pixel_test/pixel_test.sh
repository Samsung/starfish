#!/bin/sh

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BOLD='\033[1m'
RESET='\033[0m'

# Variables
EXPECTED_IMAGE_PATH="test/platform/linux"
# FIXME
OUTDIR="out/x64/exe/debug/reftest"
mkdir -p $OUTDIR
PASS=0
FAIL=0
CURDIR=`pwd`

if [ "$1" = "" ]; then
    tc=$(find test -name "*.xml" | sort)
else
    tc=$1
fi

echo "${BOLD}###### CSS Regression Test ######${RESET}\n"
for i in $tc ; do
    dir=${i%.xml}
    html=$dir".html"
    file=${dir##*/}
    dir=${dir#*/}
    dir=${dir%/*}

    # Capture the screenshot
    ELM_ENGINE="shot:" ./StarFish $i --pixel-test > /dev/null 2>&1

    # Compare
    compare="tool/pixel_test/bin/image_diff ${EXPECTED_IMAGE_PATH}/${dir}/${file}.html.png out.png"
    diff=`eval $compare` > /dev/null 2>&1
    if [ "$diff" = "" ]; then
        cd tool/pixel_test
        phantomjs capture.js -f $CURDIR/$html > /dev/null 2>&1
        cd - > /dev/null 2>&1
        diff=`eval $compare` > /dev/null 2>&1
    fi
    result=${diff##* }

    # Print the result
    if [ "${result}" = "failed" ]; then
        diff=`tool/pixel_test/bin/image_diff --diff ${EXPECTED_IMAGE_PATH}/${dir}/${file}.html.png out.png $OUTDIR/${file}_diff.png`
        FAIL=`expr $FAIL + 1`
        echo "${RED}[FAIL]${RESET}" $i
    elif [ "${result}" = "passed" ]; then
        PASS=`expr $PASS + 1`
        echo "${GREEN}[PASS]${RESET}" $i
    else
        FAIL=`expr $FAIL + 1`
        echo "${RED}[FAIL]${RESET}" $i "${YELLOW}(Unable to open html file)${RESET}"
    fi
done

# Remove unnecessary file
rm out.png

# Print the summary
echo "\n${BOLD}###### Summary ######${RESET}\n"
echo "${YELLOW}Run" `expr $PASS + $FAIL` "test cases:" $PASS "passed," $FAIL "failed.${RESET}\n"
