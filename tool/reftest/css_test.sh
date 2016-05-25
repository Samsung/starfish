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
DESTFILE_PATH="test/regression/reftest/csswg-test/"
PASSFILE="tmp.res"
RESULTFILE="regression.res"
echo -n '' > ${RESULTFILE}
XMLDIR="out/x64/exe/debug/"
OUTDIR="out/x64/exe/debug/reftest/Regression"
IMGDIFF="tool/imgdiff/imgdiff"
mkdir -p $XMLDIR
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
    tc=$(cat tool/reftest/tclist/wpt_$1.res | grep -v "file:///" | grep -v "Error" | sort);
    file=${1##*/}
    file=${file%.*}
    PASSFILE="test/regression/tool/csswg-test/test_"$1
    screen=pc
    W=800
    H=600
    if [[ "$1" == "css1" || "$1" == "css21" ]]; then
        DESTFILE_PATH=${DESTFILE_PATH}$1
    elif [[ "$1" == "css3_backgrounds" ]]; then
        DESTFILE_PATH=${DESTFILE_PATH}"css-backgrounds-3"
    elif [[ "$1" == "css3_color" ]]; then
        DESTFILE_PATH=${DESTFILE_PATH}"css-color-3"
    elif [[ "$1" == "css3_transforms" ]]; then
        DESTFILE_PATH=${DESTFILE_PATH}"css-transforms-1"
    fi
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
    xmlfile=${i#*/}
    xmlfile=${xmlfile%.*}
    i=${i//_converted\//\/}
    dir=${i%.*}
    file=${dir##*/}
    dir=${dir#*/}
    dir=${dir%/*}

    # Capture the screenshot
    xmlfile=${XMLDIR}${xmlfile}"/result.xml"
    if [[ -f ${xmlfile} ]]
    then
        ./StarFish $xmlfile --regression-test --width=${W} --height=${H} --working-directory=${html%/*} --screen-shot=out.png / > /dev/null 2>&1
    else
        ./run.sh $html --regression-test --width=${W} --height=${H} --screen-shot=out.png > /dev/null 2>&1
    fi
    mkdir -p ${EXPECTED_IMAGE_PATH}/${dir}
    GOAL_PNG="${EXPECTED_IMAGE_PATH}/${dir}/${file}-expected.png"
    if [[ "$1" != "demo" ]]; then
        dir2=${dir//csswg-test/csswg-test\/csswg-res}
        mkdir -p ${EXPECTED_IMAGE_PATH}/${dir2}
        GOAL_PNG=${GOAL_PNG//csswg-test/csswg-test\/csswg-res}
    fi
    updated=""
    if [ -f ${GOAL_PNG} ]
    then
        # Compare
        compare="${IMGDIFF} ${GOAL_PNG} out.png"
        #compare="tool/pixel_test/bin/image_diff ${GOAL_PNG} out.png"
        diff=`eval $compare` > /dev/null 2>&1
        result=${diff##* }
        ratio=${diff#* }
        mv out.png result.png
        DIFF_PNG=${GOAL_PNG}
    else
        tool/phantomjs/linux64/bin/phantomjs tool/pixel_test/capture.js -f ${html} ${OUTDIR} ${screen} > /dev/null 2>&1
        WEBKIT_PNG=${OUTDIR}"/"${file}_expected.png
        #echo $WEBKIT_PNG ## out/x64/exe/debug/reftest/Regression/c15-ids-000_expected.png
        #echo $GOAL_PNG

        if [ ! -f out.png ]; then
            result="error"
        else
            mv out.png result.png
            ./run.sh $html --pixel-test --width=${W} --height=${H} --screen-shot=out.png > /dev/null 2>&1
            sleep 1
            diff=`${IMGDIFF} ${WEBKIT_PNG} out.png`
            if [[ $diff == *passed* ]]
            then
                result="passed"
            else
                result=${diff##* }
            fi
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
            #echo $html
            passedhtml=${html//_converted\//\/}
            #echo ${passedhtml%/*}
            mkdir -p ${passedhtml%/*}
            htmlpath=${html%/*}
            if [[ -d ${htmlpath}/support && ! -d ${passedhtml%/*}/support ]]
            then
                echo "--- Copied folder: "${htmlpath}"/support"
                cp -rf ${htmlpath}/support ${passedhtml%/*}/support
            fi
            cp $html ${passedhtml}
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

    [[ -e regression_test_log ]] && rm regression_test_log
    if diff $PASSFILE $RESULTFILE &> /dev/null ; then
        echo -e "${GREEN}[PASSED] no change${RESET}\n"
    else
        echo -e "${RED}[CHECKED] some changes${RESET}\n"

        [[ -d $DESTFILE_PATH ]] && rm -rf $DESTFILE_PATH && mkdir $DESTFILE_PATH
        cp $RESULTFILE $PASSFILE
        # Copy the converted files
        file=$(cat $RESULTFILE | sort)
        for i in $file ; do
            dest=test/regression/${i#*/}
            dest=${dest//_converted\//\/}
            destdir=${dest%/*}
            mkdir -p $destdir
            path=${i%/*}
            [[ -e $path/support ]] && cp -rf $path/support $destdir/
            echo $i"====>"$dest >> regression_test_log
            cp $i $dest
        done

        # Copy the original files
  #       if [[ "$1" == "css"* ]]; then
  #           replace='s/_converted//g'
  #           perl -i -pe $replace $RESULTFILE
  #           file=$(cat $RESULTFILE | sort)
  #           for i in $file ; do
  #               dest=test/regression/${i#*/}
  #               dest=${dest//csswg-test/csswg-test\/original}
  #               destdir=${dest%/*}
  #               mkdir -p $destdir
  #               path=${i%/*}
  #               [[ -e $path/support ]] && cp -rf $path/support $destdir/
  #               echo $i"====>"$dest > regression_test_log
  #               cp $i $dest
  #           done
  #       fi
    fi

    # Remove temporary file
    #rm $RESULTFILE
fi
