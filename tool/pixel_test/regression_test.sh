#!/bin/bash

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BOLD='\033[1m'
RESET='\033[0m'

# Variables
EXPECTED_IMAGE_PATH="test/reftest/Regression"
RESULT_PATH="Demo.regression"
echo "" > ${RESULT_PATH}
# FIXME
OUTDIR="out/x64/exe/debug/reftest/Regression"
mkdir -p $OUTDIR
PASS=0
FAIL=0
UPDATE=0

if [ "$1" = "" ]; then
    tc=$(find test/demo/20160115/ -name "*.html" | sort)
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
    ELM_ENGINE="shot:" nodejs runner.js $i --regression-test > /dev/null 2>&1
    WEBKIT_PNG="${EXPECTED_IMAGE_PATH}/${dir}/${file}.html.png"
    mv out.png result.png
    if [ -f ${WEBKIT_PNG} ]
    then
        # Compare
        compare="tool/pixel_test/bin/image_diff ${WEBKIT_PNG} result.png"
        diff=`eval $compare` > /dev/null 2>&1
        result=${diff##* }
        ratio=${diff#* }
        # Print the result
        if [ "${result}" = "failed" ]; then
            diff=`tool/pixel_test/bin/image_diff --diff ${EXPECTED_IMAGE_PATH}/${dir}/${file}.html.png result.png $OUTDIR/${file}_diff.png`
            FAIL=`expr $FAIL + 1`
            mv result.png ${WEBKIT_PNG}_
            echo -e "${RED}[FAIL]${RESET}" $i "${RED}(${ratio})${RESET}"
        elif [ "${result}" = "passed" ]; then
            PASS=`expr $PASS + 1`
            echo -e "${GREEN}[PASS]${RESET}" $i
            echo $i >> ${RESULT_PATH}
        else
            FAIL=`expr $FAIL + 1`
            echo -e "${RED}[FAIL]${RESET}" $i "${YELLOW}(Unable to open html file)${RESET}"
        fi
    else
        ELM_ENGINE="shot:" nodejs runner.js $i --regression-test > /dev/null 2>&1
        mv out.png ${WEBKIT_PNG}_
        echo -e "${YELLOW}[UPDATE]${RESET}" $i "${YELLOW}(Screenshot is captured)${RESET}"
        UPDATE=`expr $UPDATE + 1`
    fi
done

# Remove unnecessary file
#rm out.png

# Print the summary
echo -e "\n${BOLD}###### Summary ######${RESET}\n"
echo -e "${YELLOW}Run" `expr $PASS + $UPDATE + $FAIL` "test cases:" $PASS "passed," $FAIL "failed," $UPDATE "need to check.${RESET}\n"
