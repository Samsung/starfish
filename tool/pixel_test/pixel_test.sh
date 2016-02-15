#!/bin/bash

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BOLD='\033[1m'
RESET='\033[0m'

# Variables
EXPECTED_IMAGE_PATH="test/reftest"
# FIXME
OUTDIR="out/x64/exe/debug/reftest"
mkdir -p $OUTDIR
PASS=0
FAIL=0
CURDIR=`pwd`

if [ "$1" = "" ]; then
    tc=$(find test -name "*.html" | sort)
else
    tc=$1
fi

if [[ "$tc" == *"/" ]]; then
    tc=$(find $tc -name "*.html" | sort);
fi

echo -e "${BOLD}###### CSS Regression Test ######${RESET}\n"
for i in $tc ; do
    dir=${i%.html}
    html=$dir".html"
    file=${dir##*/}
    dir=${dir#*/}
    dir=${dir%/*}

    # Capture the screenshot
#    ELM_ENGINE="shot:" ./StarFish $i --pixel-test > /dev/null 2>&1
    ELM_ENGINE="shot:" nodejs runner.js $i --pixel-test > /dev/null 2>&1

    # Compare
    WEBKIT_PNG="${EXPECTED_IMAGE_PATH}/${dir}/${file}.html.png"
    compare="tool/pixel_test/bin/image_diff ${WEBKIT_PNG} out.png"
    diff=`eval $compare` > /dev/null 2>&1
    if [ "$diff" = "" ]; then
        #echo $CURDIR/$html
        phantomjs tool/pixel_test/capture.js -f $CURDIR/$html ${WEBKIT_PNG%/*} > /dev/null 2>&1
        diff=`eval $compare` > /dev/null 2>&1
    fi
    result=${diff##* }
    ratio=${diff#* }

    # Print the result
    if [ "${result}" = "failed" ]; then
        diff=`tool/pixel_test/bin/image_diff --diff ${EXPECTED_IMAGE_PATH}/${dir}/${file}.html.png out.png $OUTDIR/${file}_diff.png`
        if [[ "${ratio}" == "0."* ]]; then
            CHECK=`expr $CHECK + 1`
            echo -e "${YELLOW}[CHECK]${RESET}" $i "("$ratio")"
        else
            FAIL=`expr $FAIL + 1`
            echo -e "${RED}[FAIL]${RESET}" $i
        fi
    elif [ "${result}" = "passed" ]; then
        PASS=`expr $PASS + 1`
        echo -e "${GREEN}[PASS]${RESET}" $i
    else
        FAIL=`expr $FAIL + 1`
        echo -e "${RED}[FAIL]${RESET}" $i "${YELLOW}(Unable to open html file)${RESET}"
    fi
done

# Remove unnecessary file
#rm out.png

# Print the summary
echo -e "\n${BOLD}###### Summary ######${RESET}\n"
echo -e "${YELLOW}Run" `expr $PASS + $CHECK + $FAIL` "test cases:" $PASS "passed," $FAIL "failed," $CHECK "need to check.${RESET}\n"
