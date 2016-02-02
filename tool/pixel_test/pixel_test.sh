#!/bin/sh

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BOLD='\033[1m'
RESET='\033[0m'

# Variables
EXPECTED_IMAGE_PATH="test/platform/linux"
OUTDIR="out/x64/exe/debug/test"
PASS=0
FAIL=0

echo "${BOLD}###### CSS Regression Test ######${RESET}\n"
tc=$(find test -name "*.xml")
for i in $tc ; do
    dir=${i%.xml}
    file=${dir##*/}
    dir=${dir#*/}
    dir=${dir%/*}

    # Capture the screenshot
    ELM_ENGINE="shot:" ./StarFish $i > /dev/null 2>&1

    # Compare
    diff=`tool/pixel_test/bin/image_diff ${EXPECTED_IMAGE_PATH}/${dir}/${file}.html.png out.png` > /dev/null 2>&1
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
        echo "${RED}[FAIL]${RESET}" $i "(Unable to open expected image file)"
    fi
done

# Print the summary
echo "\n${BOLD}###### Summary ######${RESET}\n"
echo "${YELLOW}Run" `expr $PASS + $FAIL` "test cases:" $PASS "passed," $FAIL "failed.${RESET}\n"
