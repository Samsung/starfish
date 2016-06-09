#!/bin/bash

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BOLD='\033[1m'
RESET='\033[0m'

# Variables
EXPECTED_IMAGE_PATH="out/x64/exe/debug/reftest/pixel_test"
# FIXME
OUTDIR="out/x64/exe/debug/reftest"
mkdir -p $OUTDIR
mkdir -p $EXPECTED_IMAGE_PATH
PASS=0
FAIL=0
W=360
H=360

if [ "$1" = "" ]; then
    tc=$(find test -name "*.html" | grep -v "reftest" | grep -v "test/regression" | sort)
else
    tc=$1
fi

if [ "$2" = "pc" ]; then
    W=800
    H=600
fi

if [[ "$tc" == *"reftest"* ]]; then
    echo "[ERROR] Use make pixel_test_css*"
#    exit 1;
fi

if [[ "$tc" == *"/" ]]; then
    tc=$(find $tc -name "*.htm*" | grep -v "reftest" | sort);
fi

if [[ "$tc" == *".res" ]]; then
    tc=$(cat $tc | grep -v "file:///" | grep -v "Error" | sort);
fi

echo -e "${BOLD}###### CSS Pixel Test ######${RESET}\n"
for i in $tc ; do
    if [[ $i == *"network"* || $i == *"interactive" || $i == *"demo/poc"* || $i == "."*".swp" ]]
    then
        continue;
    fi
    dir=${i%.htm*}
    file=${dir##*/}
    dir=${dir#*/}
    dir=${dir%/*}

    # Capture the screenshot
#    ELM_ENGINE="shot:" ./StarFish $i --pixel-test > /dev/null 2>&1
#ELM_ENGINE="shot:" ./run.sh ${i} --pixel-test --width=${W} --height=${H} > /dev/null 2>&1
./run.sh ${i} --screen-shot=./out.png --pixel-test --width=${W} --height=${H} > /dev/null 2>&1

    # Compare
    mkdir -p ${EXPECTED_IMAGE_PATH}/${dir}
    WEBKIT_PNG="${EXPECTED_IMAGE_PATH}/${dir}/${file}_expected.png"
    STARFISH_PNG="${EXPECTED_IMAGE_PATH}/${dir}/${file}_result.png"
    DIFF_PNG="${EXPECTED_IMAGE_PATH}/${dir}/${file}_diff.png"
    #echo $WEBKIT_PNG
    #echo $STARFISH_PNG
    #echo $DIFF_PNG
    if [ ! -f ${WEBKIT_PNG} ]; then
#        tool/phantomjs/linux64/bin/phantomjs tool/pixel_test/capture.js -f ${i} out/ $2 > /dev/null 2>&1
        nw tool/pixel_test/nw_capture/ -f ${i} ${EXPECTED_IMAGE_PATH}/${dir} ${2}
#        mv out/${file}_expected.png ${WEBKIT_PNG}
    fi

    if [ ! -f out.png ]; then
        result="error"
    else
        mv out.png "${STARFISH_PNG}"
        diff=`tool/imgdiff/imgdiff ${WEBKIT_PNG} ${STARFISH_PNG}`
        result=${diff##* }
        ratio=${diff#* }
        if [[ "${diff}" == *"passed"* ]]; then
            if [[ "${result}" != "passed" ]]; then
                msg="**${ratio}"
            fi
            result="passed"
        fi
    fi

    # Print the result
    if [ "${result}" = "failed" ]; then
        diff=`tool/pixel_test/bin/image_diff --diff ${WEBKIT_PNG} ${STARFISH_PNG} ${DIFF_PNG}`
        if [[ "${ratio}" == "0."* ]]; then
            CHECK=`expr $CHECK + 1`
            echo -e "${YELLOW}[CHECK]${RESET}" $i "("$ratio")"
        else
            FAIL=`expr $FAIL + 1`
            echo -e "${RED}[FAIL]${RESET}" $i "${RED}("$ratio")${RESET}"
        fi
    elif [ "${result}" = "passed" ]; then
        PASS=`expr $PASS + 1`
        echo -e "${GREEN}[PASS]${RESET}" $i "${YELLOW}${msg}${RESET}"
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
