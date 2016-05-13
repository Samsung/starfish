#!/bin/bash

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BOLD='\033[1m'
RESET='\033[0m'

# Variables
PASSFILE="test/regression/reftest/dom/dom_regression.res"
PASSFILENEW="dom_regression.res"
PASSTC=0
SKIPTC=0
FAILTC=0
TCFILE=0

if [ "$1" = "" ]; then
    echo "Please specify the input file"
    exit
fi

# Test Suites
# 0: W3C DOM Conformace Test Suites
# 1: Web Platform Tests
# 2: Vendor Tests
if [[ "$1" = *"dom_conformance_test_regression.res" ]]; then
    TESTSUITE=0
    tc=$(cat $1)
elif [[ "$1" = *"dom-conformance-test"*".htm"* ]]; then
    TESTSUITE=0
    tc=$1
elif [[ "$1" = *"vendor/blink/dom"*".htm"* || "$1" = *"vendor/webkit/dom"*".htm"* ]]; then
    TESTSUITE=0
    tc=$1
elif [[ "$1" = *"wpt"*".res" ]]; then
    TESTSUITE=1
    tc=$(cat $1)
elif [[ "$1" = *"web-platform-tests"*".htm"* ]]; then
    TESTSUITE=1
    tc=$1
elif [[ "$1" = *"blink_fast"*".res" || "$1" = *"webkit_fast"*".res" ]]; then
    TESTSUITE=2
    tc=$(cat $1)
elif [[ "$1" = *"vendor/blink/fast"*".htm"* || "$1" = *"vendor/webkit/fast"*".htm"* ]]; then
    TESTSUITE=2
    tc=$1
else
    echo "Unsupported tests"
    exit
fi

if [ "$2" = true ]; then
    REGRESSION=true
fi

echo -e "${BOLD}###### Ref Tests ######${RESET}\n"

cnt=-1
ROTATE="$(nproc)"
filenames=()
array=( $tc )
pos=$(( ${#array[*]} - 1 ))
last=${array[$pos]}
for i in $tc ; do
    cnt=$((cnt+1))
    if [[ $cnt == $ROTATE ]]; then
        cnt=0
    fi
    RESFILE="tmp"$cnt

    # Running the tests
    filenames[$cnt]=$i
    ./StarFish $i --result-folder="out/$i" --hide-window &> $RESFILE &
    if [[ $cnt != $((ROTATE-1)) ]] && [[ $i != $last ]]; then
        continue;
    fi
    wait;

    for c in $(seq 0 $cnt); do
        TMPFILE="tmp"$c

        # Collect the result
        replace1='s/\\n"//g'
        replace2='s/"//g'
        replace3='s/\n//g'
        replace4='s/spawnSTDOUT:\s//g'
        perl -i -pe $replace1 $TMPFILE
        perl -i -pe $replace2 $TMPFILE
        perl -i -pe $replace3 $TMPFILE
        perl -i -pe $replace4 $TMPFILE

        if [ $TESTSUITE -eq 0 ]; then
            EXPECTED_FILE=${filenames[$c]%.*}"-expected.txt"
            EXPECTED=$(cat $EXPECTED_FILE)
            EXPECTED=${EXPECTED##*Status:}
            EXPECTED=${EXPECTED%Detail*}
            RESULT=`grep -o $EXPECTED $TMPFILE | wc -l`
            SKIP=`grep -o Skipped $TMPFILE | wc -l`

            if [ $SKIP -eq 1 ]; then
                SKIPTC=`expr $SKIPTC + 1`
                echo -e "${YELLOW}[SKIP]${RESET}" ${filenames[$c]}
            elif [ $RESULT -eq 1 ]; then
                PASSTC=`expr $PASSTC + 1`
                echo -e "${GREEN}[PASS]${RESET}" ${filenames[$c]}
                if [ "$REGRESSION" = true ]; then
                    echo -e ${filenames[$c]} >> $PASSFILENEW
                fi
            else
                FAILTC=`expr $FAILTC + 1`
                echo -e "${RED}[FAIL]${RESET}" ${filenames[$c]}
            fi
        elif [ $TESTSUITE -eq 1 ]; then
            PASS=`grep -o "Pass " $TMPFILE | wc -l`
            FAIL=`grep -o "Fail " $TMPFILE | wc -l`
            SUM=`expr $PASS + $FAIL`
            PASSTC=`expr $PASSTC + $PASS`
            FAILTC=`expr $FAILTC + $FAIL`

            if [ $SUM -eq 0 ]; then
                echo -e "${YELLOW}[CHECK]${RESET}" ${filenames[$c]} "(${BOLD}No results${RESET})"
            elif [ $FAIL -eq 0 ]; then
                PASSTCFILE=`expr $PASSTCFILE + 1`
                echo -e "${GREEN}[PASS]${RESET}" ${filenames[$c]} "(${GREEN}PASS:" $PASS"${RESET})"
                if [ "$REGRESSION" = true ]; then
                    echo -e ${filenames[$c]} >> $PASSFILENEW
                fi
            elif [ $PASS -eq 0 ]; then
                echo -e "${RED}[FAIL]${RESET}" ${filenames[$c]} "(${RED}FAIL:" $FAIL"${RESET})"
            else
                echo -e "${RED}[FAIL]${RESET}" ${filenames[$c]} "(${GREEN}PASS:" $PASS"${RESET}," "${RED}FAIL:" $FAIL"${RESET})"
            fi
        elif [ $TESTSUITE -eq 2 ]; then
            EXPECTED_FILE=${filenames[$c]%.*}"-expected.txt"
            EXPECTED=`grep -Eo "PASS|FAIL" $EXPECTED_FILE`
            RESULT=`grep -Eo "PASS|FAIL" $TMPFILE`

            if [ "$EXPECTED" = "$RESULT" ]; then
                PASSTC=`expr $PASSTC + 1`
                echo -e "${GREEN}[PASS]${RESET}" ${filenames[$c]}
                if [ "$REGRESSION" = true ]; then
                    echo -e ${filenames[$c]} >> $PASSFILENEW
                fi
            else
                FAILTC=`expr $FAILTC + 1`
                echo -e "${RED}[FAIL]${RESET}" ${filenames[$c]}
            fi
        fi

        TCFILE=`expr $TCFILE + 1`
        rm $TMPFILE
    done
done

wait;

# Print the summary
echo -e "\n${BOLD}###### Summary ######${RESET}\n"

if [ $TESTSUITE -eq 0 ]; then
    echo -e "${YELLOW}Run" `expr $TCFILE` "test cases:" $PASSTC "passed," $FAILTC "failed," $SKIPTC "skipped.${RESET}\n"
elif [ $TESTSUITE -eq 1 ]; then
    echo -e "${YELLOW}Run" `expr $PASSTC + $FAILTC` "test cases ("$TCFILE" files):" $PASSTC "passed ("$PASSTCFILE "files)," $FAILTC "failed.${RESET}\n"
elif [ $TESTSUITE -eq 2 ]; then
    echo -e "${YELLOW}Run" `expr $TCFILE` "test cases:" $PASSTC "passed," $FAILTC "failed.${RESET}\n"
fi

# Regression test
if [ "$REGRESSION" = true ]; then
    echo -e "${BOLD}###### Regression Test ######${RESET}\n"

    diff=`diff -u $PASSFILE $PASSFILENEW`
    if [ "$diff" = "" ] ; then
        echo -e "${GREEN}[PASSED] no change${RESET}\n"
    else
        echo -e "${RED}[CHECKED] some changes${RESET}\n"
        diff -u $PASSFILE $PASSFILENEW
        echo

        # Update the regression lists
        cp $PASSFILENEW test/regression/reftest/dom

        # Copy the converted files
        file=$(cat $PASSFILENEW)
        for i in $file ; do
            dest=test/regression/${i#*/}
            destdir=${dest%/*}
            mkdir -p $destdir
            tc=${i%.*}
            dest=${dest%.*}
            cp $tc".html" $dest".html"
            cp $tc".js" $dest".js"
            cp $tc"-expected.txt" $dest"-expected.txt"
        done

        # Copy the original files
        replace='s/html_converted/html/g'
        perl -i -pe $replace $PASSFILENEW
        file=$(cat $PASSFILENEW)
        for i in $file ; do
            dest=test/regression/${i#*/}
            destdir=${dest%/*}
            mkdir -p $destdir
            tc=${i%.*}
            dest=${dest%.*}
            cp $tc".html" $dest".html"
            cp $tc".js" $dest".js"
            cp $tc"-expected.txt" $dest"-expected.txt"
        done

        # Copy the resource files
        #cp -rf test/reftest/web-platform-tests/resources test/regression/reftest/web-platform-tests/
    fi

    # Remove temporary file
    rm $PASSFILENEW
fi
