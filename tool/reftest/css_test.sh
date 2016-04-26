#!/bin/bash

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BOLD='\033[1m'
RESET='\033[0m'

# Variables
TIMEOUT=3
EXPECTED_IMAGE_PATH="test/regression"
PASSFILE="tmp.res"
RESULTFILE="regression.res"
echo -n '' > ${RESULTFILE}
OUTDIR="out/x64/exe/debug/reftest/Regression"
mkdir -p $OUTDIR
mkdir -p $EXPECTED_IMAGE_PATH
PASS=0
FAIL=0
UPDATE=0
W=360
H=360

if [ "$1" = "demo" ]; then
    tc=$(find test/demo/20160* -name "*.htm*" | sort)
    PASSFILE="test/regression/demo/demo_regression.res"
    file=DEMO
elif [[ "$1" == "css"* ]]; then
    tc=$(cat tool/reftest/converter/$1_converted.res | grep -v "file:///" | grep -v "Error" | sort);
    file=${1##*/}
    file=${file%.*}
    PASSFILE="test/regression/reftest/csswg-test/"$1"_regression.res"
    screen=pc
    W=800
    H=600
else
    tc=$1
fi

echo $PASSFILE
if [[ "$tc" == *"/" ]]; then
    tc=$(find $tc -name "*.htm*" | sort);
fi

if [[ "$tc" == *".res" ]]; then
    tc=$(cat $tc)
fi

if [ "$2" = true ]; then
    echo "true"
    REGRESSION=true
fi

echo -e "${BOLD}###### CSS Regression Test - ${file} ######${RESET}\n"

for i in $tc ; do
    if [[ $i == *"network"* || $i == *"f01_interactive"* ]]
    then
        continue;
    fi
    cnt=`expr $cnt + 1`
    html=$i
    i=${i//csswg-test_converted/csswg-test}
    dir=${i%.*}
    file=${dir##*/}
    dir=${dir#*/}
    dir=${dir%/*}

    # Capture the screenshot
    ELM_ENGINE="shot:" ./run.sh $html --regression-test --width=${W} --height=${H} > /dev/null 2>&1
    mkdir -p ${EXPECTED_IMAGE_PATH}/${dir}
    GOAL_PNG="${EXPECTED_IMAGE_PATH}/${dir}/${file}_result.png"
    if [[ "$1" != "demo" ]]; then
        dir2=${dir//csswg-test_converted/csswg-test\/csswg-res}
        mkdir -p ${EXPECTED_IMAGE_PATH}/${dir2}
        GOAL_PNG=${GOAL_PNG//csswg-test/csswg-test\/csswg-res}
    fi
    updated=""
    if [ -f ${GOAL_PNG} ]
    then
        # Compare
        compare="tool/pixel_test/bin/image_diff ${GOAL_PNG} out.png"
        diff=`eval $compare` > /dev/null 2>&1
        result=${diff##* }
        ratio=${diff#* }
        mv out.png result.png
        DIFF_PNG=${GOAL_PNG}
    else
        tool/phantomjs/linux64/bin/phantomjs tool/pixel_test/capture.js -f ${i} out/ ${screen} > /dev/null 2>&1
        WEBKIT_PNG="out/"${file}_expected.png
        #echo $GOAL_PNG

        if [ ! -f out.png ]; then
            result="error"
        else
            mv out.png result.png
            ELM_ENGINE="shot:" ./run.sh $html --pixel-test --width=${W} --height=${H} > /dev/null 2>&1
            sleep 1
            diff=`tool/pixel_test/bin/image_diff ${WEBKIT_PNG} out.png`
            result=${diff##* }
            ratio=${diff#* }
            updated="${YELLOW}(updated)${RESET}"
            DIFF_PNG=${WEBKIT_PNG}
        fi
    fi

    # Print the result
    if [ "${result}" = "failed" ]; then
        mkdir -p $OUTDIR/${dir}
        diff=`tool/pixel_test/bin/image_diff --diff ${DIFF_PNG} result.png $OUTDIR/${dir}/${file}_diff.png`
        FAIL=`expr $FAIL + 1`
        STARFISH_PNG="${OUTDIR}/${dir}/${file}_result.png"
        mv result.png ${STARFISH_PNG}
        echo -e "${RED}[FAIL]${RESET}" $html "${RED}(${ratio})${RESET}"
    elif [ "${result}" = "passed" ]; then
        PASS=`expr $PASS + 1`
        echo -e "${GREEN}[PASS]${RESET}" $html $updated
        if [ ! -f ${GOAL_PNG} ]
        then
            mv result.png ${GOAL_PNG}
        fi
        echo $html >> ${RESULTFILE}
    else
        FAIL=`expr $FAIL + 1`
        echo -e "${RED}[FAIL]${RESET}" $html "${YELLOW}(Unable to open html file)${RESET}"
    fi
done

# Remove unnecessary file
#rm out.png

# Print the summary
echo -e "\n${BOLD}###### Summary ######${RESET}\n"
echo -e "${YELLOW}Run" `expr $PASS + $FAIL` "test cases:" $PASS "passed," $FAIL "failed.${RESET}\n"



# Regression test
if [ "$REGRESSION" = true ]; then
    echo -e "${BOLD}###### Regression Test ######${RESET}\n"

    if diff $PASSFILE $RESULTFILE &> /dev/null ; then
        echo -e "${GREEN}[PASSED] no change${RESET}\n"
    else
        echo -e "${RED}[CHECKED] some changes${RESET}\n"

        cp $RESULTFILE $PASSFILE
        # Copy the converted files
        file=$(cat $RESULTFILE | sort)
        for i in $file ; do
            dest=test/regression/${i#*/}
            dest=${dest//csswg-test_converted/csswg-test\/converted}
            destdir=${dest%/*}
            mkdir -p $destdir
            path=${i%/*}
            [[ -e $path/support ]] && cp -rf $path/support $destdir/
            echo $i"====>"$dest > regression_test_log
            cp $i $dest
        done

        # Copy the original files
        if [[ "$1" == "css"* ]]; then
            replace='s/_converted//g'
            perl -i -pe $replace $RESULTFILE
            file=$(cat $RESULTFILE | sort)
            for i in $file ; do
                dest=test/regression/${i#*/}
                dest=${dest//csswg-test/csswg-test\/original}
                destdir=${dest%/*}
                mkdir -p $destdir
                path=${i%/*}
                [[ -e $path/support ]] && cp -rf $path/support $destdir/
                echo $i"====>"$dest > regression_test_log
                cp $i $dest
            done
        fi
    fi

    # Remove temporary file
    #rm $RESULTFILE
fi
