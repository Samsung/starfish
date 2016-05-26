#!/bin/bash

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BOLD='\033[1m'
RESET='\033[0m'

# Variables
RESULTFILE="result"
PASSTC=0
SKIPTC=0
FAILTC=0
TCFILE=0
PASSTCFILE=0

PLATFORM=`uname -m`
if [[ "$PLATFORM" != "i686" && "$PLATFORM" != "armv7l" ]]; then
    echo "Not supported platform"
    exit
fi

export LD_LIBRARY_PATH=./test/lib

# Test Suites
# 0: W3C DOM Conformance Test Suites
# 1: Web Platform Tests
# 2: Vendor Tests(Blink, Gechko, WebKit)
# 3: Pixel Test
tc=$1
if [[ "$tc" == "dom_conformance" ]]; then
    tc=$(cat test/tool/dom-conformance-test/test_$tc)
    TESTSUITE=0
elif [[ "$tc" == "blink_dom_conformance" ]]; then
    tc=$(cat test/tool/vendor/blink/test_$tc)
    TESTSUITE=0
elif [[ "$tc" == "gecko_dom_conformance" ]]; then
    tc=$(cat test/tool/vendor/gecko/test_$tc)
    TESTSUITE=0
elif [[ "$tc" == "webkit_dom_conformance" ]]; then
    tc=$(cat test/tool/vendor/webkit/test_$tc)
    TESTSUITE=0
elif [[ "$tc" == "wpt_dom" ]]; then
    tc=$(cat test/tool/web-platform-tests/test_$tc)
    TESTSUITE=1
elif [[ "$tc" == "wpt_dom_events" ]]; then
    tc=$(cat test/tool/web-platform-tests/test_$tc)
    TESTSUITE=1
elif [[ "$tc" == "wpt_html" ]]; then
    tc=$(cat test/tool/web-platform-tests/test_$tc)
    TESTSUITE=1
elif [[ "$tc" == "wpt_page_visibility" ]]; then
    tc=$(cat test/tool/web-platform-tests/test_$tc)
    TESTSUITE=1
elif [[ "$tc" == "wpt_progress_events" ]]; then
    tc=$(cat test/tool/web-platform-tests/test_$tc)
    TESTSUITE=1
elif [[ "$tc" == "wpt_xhr" ]]; then
    tc=$(cat test/tool/web-platform-tests/test_$tc)
    TESTSUITE=1
elif [[ "$tc" == "blink_fast_dom" ]]; then
    tc=$(cat test/tool/vendor/blink/test_$tc)
    TESTSUITE=2
elif [[ "$tc" == "blink_fast_html" ]]; then
    tc=$(cat test/tool/vendor/blink/test_$tc)
    TESTSUITE=2
elif [[ "$tc" == "webkit_fast_dom" ]]; then
    tc=$(cat test/tool/vendor/webkit/test_$tc)
    TESTSUITE=2
elif [[ "$tc" == "webkit_fast_html" ]]; then
    tc=$(cat test/tool/vendor/webkit/test_$tc)
    TESTSUITE=2
elif [[ "$tc" == "bidi" ]]; then
    tc=$(cat test/tool/bidi/test_$tc)
    TESTSUITE=3
else
    echo "Usage: ./runner.js [test_input]"
    echo "test_input: [ dom_conformance | wpt_dom | wpt_dom_events | wpt_html | wpt_page_visibility | wpt_progress_event | wpt_xhr | blink_dom_conformance | blink_fast_dom | blink_fast_html | gecko_dom_conformance | webkit_dom_conformance | webkit_fast_dom | webkit_fast_html | bidi ]"
    exit
fi

rm $RESULTFILE > /dev/null 2>&1

echo -e "${BOLD}###### Tests ######${RESET}\n"

cnt=-1
filenames=()
array=( $tc )
pos=$(( ${#array[*]} - 1 ))
last=${array[$pos]}
ROTATE=1

if [[ "$PLATFORM" == "i686" ]]; then
   	STARFISH="./test/bin/tizen_emulator/StarFish"
elif [[ "$PLATFORM" == "armv7l" ]]; then
    STARFISH="./test/bin/tizen_arm/StarFish"
fi

for i in $tc ; do
    cnt=$((cnt+1))
    if [[ $cnt == $ROTATE ]]; then
        cnt=0
    fi

	if [[ "$PLATFORM" == "armv7l" || "$PLATFORM" == "i686" ]]; then
		dlogutil -c
	fi

    # Running the tests
    filenames[$cnt]=$i
    if [ $TESTSUITE -eq 3 ]; then
        RESIMG="./out.png"
        ELM_ENGINE="shot:file="$RESIMG $StarFish $i --width=900 --height=900 > /dev/null 2&>1 &
    else
        $STARFISH $i --hide-window &
    fi

    if [[ $cnt != $((ROTATE-1)) ]] && [[ $i != $last ]]; then
        continue;
    fi
    wait;

    if [[ "$PLATFORM" == "armv7l" || "$PLATFORM" == "i686" ]]; then
        if [ $TESTSUITE -eq 0 ]; then
            EXPECTED_FILE=${filenames[$c]%.*}"-expected.txt"
            EXPECTED=$(cat $EXPECTED_FILE)
            EXPECTED=${EXPECTED##*Status:}
            EXPECTED=${EXPECTED%*Detail*}
            EXPECTED=$(sed 's/[[:space:]]//g' <<< "$EXPECTED")
            EXPECTED="console.*"$EXPECTED
            RESULT=`dlogutil -d | grep -Eo $EXPECTED | wc -l`
            SKIP=`dlogutil -d | grep -Eo "console.*Skipped" | wc -l`

            if [ $SKIP -eq 1 ]; then
                SKIPTC=`expr $SKIPTC + 1`
                echo -e "${YELLOW}[SKIP]${RESET}" ${filenames[$c]}
                echo -e "[SKIP]" ${filenames[$c]} >> $RESULTFILE
            elif [ $RESULT -eq 1 ]; then
                PASSTC=`expr $PASSTC + 1`
                echo -e "${GREEN}[PASS]${RESET}" ${filenames[$c]}
                echo -e "[PASS]" ${filenames[$c]} >> $RESULTFILE
            else
                FAILTC=`expr $FAILTC + 1`
                echo -e "${RED}[FAIL]${RESET}" ${filenames[$c]}
                echo -e "[FAIL]" ${filenames[$c]} >> $RESULTFILE
            fi
        elif [ $TESTSUITE -eq 1 ]; then
            PASS=`dlogutil -d | grep -Eo "console.*PASS" | wc -l`
            FAIL=`dlogutil -d | grep -Eo "console.*FAIL" | wc -l`
            SUM=`expr $PASS + $FAIL`
            PASSTC=`expr $PASSTC + $PASS`
            FAILTC=`expr $FAILTC + $FAIL`

            if [ $SUM -eq 0 ]; then
                echo -e "${YELLOW}[CHECK]${RESET}" ${filenames[$c]} "(${BOLD}No results${RESET})"
                echo -e "[CHECK]" ${filenames[$c]} "(No results)" >> $RESULTFILE
            elif [ $FAIL -eq 0 ]; then
                echo -e "${GREEN}[PASS]${RESET}" ${filenames[$c]} "(${GREEN}PASS:" $PASS"${RESET})"
                echo -e "[PASS]" ${filenames[$c]} "(PASS:" $PASS")" >> $RESULTFILE
                PASSTCFILE=`expr $PASSTCFILE + 1`
            elif [ $PASS -eq 0 ]; then
                echo -e "${RED}[FAIL]${RESET}" ${filenames[$c]} "(${RED}FAIL:" $FAIL"${RESET})"
                echo -e "[FAIL]" ${filenames[$c]} "(FAIL:" $FAIL")" >> $RESULTFILE
            else
                echo -e "${RED}[FAIL]${RESET}" ${filenames[$c]} "(${GREEN}PASS:" $PASS"${RESET}," "${RED}FAIL:" $FAIL"${RESET})"
                echo -e "[FAIL]" ${filenames[$c]} "(PASS:" $PASS"," "FAIL:" $FAIL")" >> $RESULTFILE
            fi
        elif [ $TESTSUITE -eq 2 ]; then
            EXPECTED_FILE=${filenames[$c]%.*}"-expected.txt"
            EXPECTED=`grep -Eo "PASS|FAIL" $EXPECTED_FILE`
            RESULT=`dlogutil -d | grep -Eo "PASS|FAIL"`

            if [ "$EXPECTED" = "$RESULT" ]; then
                PASSTC=`expr $PASSTC + 1`
                echo -e "${GREEN}[PASS]${RESET}" ${filenames[$c]}
                echo -e "[PASS]" ${filenames[$c]} >> $RESULTFILE
            else
                FAILTC=`expr $FAILTC + 1`
                echo -e "${RED}[FAIL]${RESET}" ${filenames[$c]}
                echo -e "[FAIL]" ${filenames[$c]} >> $RESULTFILE
            fi
        elif [ $TESTSUITE -eq 3 ]; then
            EXPIMG=${filenames[$c]}
            EXPIMG=`echo $EXPIMG | sed 's/html-css/html-css\/tizen_wearable_arm/'
            EXPIMG=`echo $EXPIMG | sed 's/\.html/-expected\.png/'
            IMGDIFF="./test/tool/imgdiffEvas"
            DIFF=`$IMGDIFF $RESIMG $EXPIMG`
            if [[ "$DIFF" = *"passed"* ]]; then
                PASSTC=`expr $PASSTC + 1`
                echo -e "${GREEN}[PASS]${RESET}" ${filenames[$c]}
                echo -e "[PASS]" ${filenames[$c]} >> $RESULTFILE
            else
                FAILTC=`expr $FAILTC + 1`
                echo -e "${RED}[FAIL]${RESET}" ${filenames[$c]}
                echo -e "[FAIL]" ${filenames[$c]} >> $RESULTFILE
            fi
        fi

        TCFILE=`expr $TCFILE + 1`
    fi
    done

wait;

# Print the summary
echo -e "\n${BOLD}###### Summary ######${RESET}\n"
echo -e "\n###### Summary ######\n" >> $RESULTFILE

if [ $TESTSUITE -eq 0 ]; then
    echo -e "${YELLOW}Run" `expr $TCFILE` "test cases:" $PASSTC "passed," $FAILTC "failed," $SKIPTC "skipped.${RESET}\n"
    echo -e "Run" `expr $TCFILE` "test cases:" $PASSTC "passed," $FAILTC "failed," $SKIPTC "skipped.\n" >> $RESULTFILE
elif [ $TESTSUITE -eq 1 ]; then
    echo -e "${YELLOW}Run" `expr $PASSTC + $FAILTC` "test cases ("$TCFILE" files):" $PASSTC "passed ("$PASSTCFILE "files)," $FAILTC "failed.${RESET}\n"
    echo -e "Run" `expr $PASSTC + $FAILTC` "test cases ("$TCFILE" files):" $PASSTC "passed ("$PASSTCFILE "files)," $FAILTC "failed.\n" >> $RESULTFILE
else
    echo -e "${YELLOW}Run" `expr $TCFILE` "test cases:" $PASSTC "passed," $FAILTC "failed," $SKIPTC "skipped.${RESET}\n"
    echo -e "Run" `expr $TCFILE` "test cases:" $PASSTC "passed," $FAILTC "failed," $SKIPTC "skipped.\n" >> $RESULTFILE
fi

