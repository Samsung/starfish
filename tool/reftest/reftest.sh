#!/bin/bash

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BOLD='\033[1m'
RESET='\033[0m'

# Variables
PASSFILENEW="tmp.res"
RESULTCSV="result.csv"
PASSTC=0
FAILTC=0
SKIPTC=0
PASSTCFILE=0
CHECKTCFILE=0
TCFILE=0
OUTDIR="out/x64/exe/debug/reftest/regression/"

if [ "$1" = "" ]; then
    echo "Please specify the input file"
    exit
elif [[ "$1" = *".htm" || "$1" = *".html" ]]; then
    tc=$1
elif [[ "$1" = *".res" ]]; then
    tc=$(cat $1)
else
    echo "Input file is not supported"
    exit
fi

# Test Suites
# 0: W3C DOM Conformace Test Suites
# 1: Web Platform Tests
# 2: Vendor Tests - Pixel Tests
# 3: Vendor Tests
# 4: Bidi Tests - Pixel Tests
# 5: CSSWG - Pixel Tests (font-dependent)
if [[ "$1" = *"dom_conformance_test.res" ||
      "$1" = *"dom-conformance-test"*".htm"* || "$1" = *"blink/dom"*".htm"* ||
      "$1" = *"gecko/dom_tests_mochitest"*".htm"* || "$1" = *"webkit/dom"*".htm"* ]]; then
    TESTSUITE=0
    if [[ "$1" = *"blink"* ]]; then
        TESTSUITENAME="Blink DOM Conformance Test"
        PASSFILE="test/regression/tool/vendor/blink/test_blink_dom_conformance"
    elif [[ "$1" = *"webkit"* ]]; then
        TESTSUITENAME="WebKit DOM Conformance Test"
        PASSFILE="test/regression/tool/vendor/webkit/test_webkit_dom_conformance"
    elif [[ "$1" = *"gecko"* ]]; then
        TESTSUITENAME="Gecko DOM Conformance Test"
        PASSFILE="test/regression/tool/vendor/gecko/test_gecko_dom_conformance"
    else
        TESTSUITENAME="DOM Conformance Test"
        PASSFILE="test/regression/tool/dom-conformance-test/test_dom_conformance"
    fi
elif [[ "$1" = *"wpt"*".res" || "$1" = *"web-platform-tests"*".htm"* ]]; then
    TESTSUITE=1
    if [[ "$1" = *"wpt_dom.res" ]]; then
        TESTSUITENAME="WPT DOM"
        PASSFILE="test/regression/tool/web-platform-tests/test_wpt_dom"
    elif [[ "$1" = *"wpt_dom_events.res" ]]; then
        TESTSUITENAME="WPT DOM Events"
        PASSFILE="test/regression/tool/web-platform-tests/test_wpt_dom_events"
    elif [[ "$1" = *"wpt_html.res" ]]; then
        TESTSUITENAME="WPT HTML"
        PASSFILE="test/regression/tool/web-platform-tests/test_wpt_html"
    elif [[ "$1" = *"wpt_page_visibility.res" ]]; then
        TESTSUITENAME="WPT Page Visibility"
        PASSFILE="test/regression/tool/web-platform-tests/test_wpt_page_visibility"
    elif [[ "$1" = *"wpt_progress_events.res" ]]; then
        TESTSUITENAME="WPT Progress Events"
        PASSFILE="test/regression/tool/web-platform-tests/test_wpt_progress_events"
    elif [[ "$1" = *"wpt_xhr.res" ]]; then
        TESTSUITENAME="WPT XMLHttpRequest"
        PASSFILE="test/regression/tool/web-platform-tests/test_wpt_xhr"
    fi
elif [[ "$1" = *"webkit_fast_css.res" || "$1" = *"webkit/fast/css"*".htm"* ]]; then
    TESTSUITE=2
    TESTSUITENAME="WebKit Fast CSS"
    #PASSFILE="test/regression/tool/vendor/webkit/test_webkit_fast_css"
elif [[ "$1" = *"blink_fast"*".res" || "$1" = *"webkit_fast"*".res" ||
        "$1" = *"blink/fast"*".htm"* || "$1" = *"webkit/fast"*".htm"* ]]; then
    TESTSUITE=3
    if [[ "$1" = *"blink_fast_dom.res" ]]; then
        TESTSUITENAME="Blink Fast DOM"
        PASSFILE="test/regression/tool/vendor/blink/test_blink_fast_dom"
    elif [[ "$1" = *"blink_fast_html.res" ]]; then
        TESTSUITENAME="Blink Fast HTML"
        PASSFILE="test/regression/tool/vendor/blink/test_blink_fast_html"
    elif [[ "$1" = *"webkit_fast_dom.res" ]]; then
        TESTSUITENAME="WebKit Fast DOM"
        PASSFILE="test/regression/tool/vendor/webkit/test_webkit_fast_dom"
    elif [[ "$1" = *"webkit_fast_html.res" ]]; then
        TESTSUITENAME="WebKit Fast HTML"
        PASSFILE="test/regression/tool/vendor/webkit/test_webkit_fast_html"
    fi
elif [[ "$1" = *"bidi.res" || "$1" = *"bidi/International/tests"*".htm"* ]]; then
    TESTSUITE=4
    TESTSUITENAME="International Tests"
    PASSFILE="test/regression/tool/bidi/test_bidi"
elif [[ "$1" = *"csswg"*".res" || "$1" = *"csswg-test/"*".htm"* ]]; then
    TESTSUITE=5
    name=${1##*/}
    name=${name%.*}
    TESTSUITENAME="CSSWG Tests - $name"
    PASSFILE="test/regression/tool/csswg-test/test_$name"
else
    echo "Unsupported tests"
    exit
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
    if [ $TESTSUITE -eq 4 ]; then
        RESIMG="out"$cnt".png"
        ./StarFish $i --regression-test --screen-shot=$RESIMG --screen-shot-width=900 --screen-shot-height=900 --width=900 --height=900 &> /dev/null 2>&1 &
    elif [ $TESTSUITE -eq 2 ] || [ $TESTSUITE -eq 5 ]; then
        RESIMG="out"$cnt".png"
        ./StarFish $i --regression-test --screen-shot=$RESIMG --screen-shot-width=800 --screen-shot-height=600 --width=800 --height=600 &> /dev/null 2>&1 &
    else
        ./StarFish $i --hide-window &> $RESFILE &
    fi
    if [[ $cnt != $((ROTATE-1)) ]] && [[ $i != $last ]]; then
        continue;
    fi
    wait;

    for c in $(seq 0 $cnt); do
        TMPFILE="tmp"$c
        RESIMG="out"$c".png"

        # Collect the result
        if [ `expr $TESTSUITE \< 2` -eq 1 ] || [ $TESTSUITE -eq 3 ] ; then
            replace1='s/\\n"//g'
            replace2='s/"//g'
            replace3='s/\n//g'
            replace4='s/spawnSTDOUT:\s//g'
            perl -i -pe $replace1 $TMPFILE
            perl -i -pe $replace2 $TMPFILE
            perl -i -pe $replace3 $TMPFILE
            perl -i -pe $replace4 $TMPFILE
        fi

        if [ $TESTSUITE -eq 0 ]; then
            EXPECTED_FILE=${filenames[$c]%.*}"-expected.txt"
            EXPECTED=$(cat $EXPECTED_FILE)
            EXPECTED=${EXPECTED##*Status:}
            EXPECTED=${EXPECTED%Detail*}
            EXPECTED="Status: "$(sed 's/[[:space:]]//' <<< "$EXPECTED")
            RESULT=`grep -o "$EXPECTED" $TMPFILE | wc -l`
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
            PASS=`grep -o "PASS " $TMPFILE | wc -l`
            FAIL=`grep -o "FAIL " $TMPFILE | wc -l`
            SUM=`expr $PASS + $FAIL`
            PASSTC=`expr $PASSTC + $PASS`
            FAILTC=`expr $FAILTC + $FAIL`

            if [ $SUM -eq 0 ]; then
                CHECKTCFILE=`expr $CHECKTCFILE + 1`
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
            EXPIMG=${filenames[$c]}
            EXPIMG=`echo $EXPIMG | sed 's/fast\/css/fast\/css_result/g'`
            EXPIMG=`echo $EXPIMG | sed 's/\.html/-expected\.png/g'`
            IMGDIFF="./tool/pixel_test/bin/image_diff"
            DIFF=`$IMGDIFF $RESIMG $EXPIMG`
            if [[ "$DIFF" = *"0.00% passed" ]]; then
                PASSTC=`expr $PASSTC + 1`
                echo -e "${GREEN}[PASS]${RESET}" ${filenames[$c]}
                if [ "$REGRESSION" = true ]; then
                    echo -e ${filenames[$c]} >> $PASSFILENEW
                fi
            else
                FAILTC=`expr $FAILTC + 1`
                echo -e "${RED}[FAIL]${RESET}" ${filenames[$c]}
                if [ "$2" = "capture" ]; then
                    EXPDIR=${EXPIMG%/*}
                    mkdir $EXPDIR
                    cp $RESIMG $EXPIMG
                fi
            fi
            rm $RESIMG
        elif [ $TESTSUITE -eq 3 ]; then
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
        elif [ $TESTSUITE -eq 4 ]; then
            EXPIMG=${filenames[$c]}
            EXPIMG=`echo $EXPIMG | sed 's/reftest\//regression\/reftest\//'`
            EXPIMG=`echo $EXPIMG | sed 's/html-css/html-css_result\/x64/'`
            EXPIMG=`echo $EXPIMG | sed 's/\.html/-expected\.png/'`
            DIFFIMG=${filenames[$c]}
            DIFFIMG=`echo $DIFFIMG | sed 's/reftest\//regression\/reftest\//'`
            DIFFIMG=`echo $DIFFIMG | sed 's/html-css/html-css\/x64/'`
            DIFFIMG=`echo $DIFFIMG | sed 's/\.html/-diff\.png/'`
            IMGDIFF="./tool/pixel_test/bin/image_diff"
            DIFF=`$IMGDIFF $RESIMG $EXPIMG`
            if [[ "$DIFF" = *"0.00% passed" ]]; then
                PASSTC=`expr $PASSTC + 1`
                echo -e "${GREEN}[PASS]${RESET}" ${filenames[$c]}
                if [ "$REGRESSION" = true ]; then
                    echo -e ${filenames[$c]} >> $PASSFILENEW
                fi
            else
                FAILTC=`expr $FAILTC + 1`
                echo -e "${RED}[FAIL]${RESET}" ${filenames[$c]}
                echo $DIFFIMG
                DIFF=`$IMGDIFF --diff $RESIMG $EXPIMG $DIFFIMG`
            fi
            rm $RESIMG
        elif [ $TESTSUITE -eq 5 ]; then
            RESIMGPATH=${filenames[$c]%/*}
            RESIMGPATH=`echo $RESIMGPATH | sed 's/test\/reftest\///'`
            RESIMGPATH=$OUTDIR$RESIMGPATH
            mkdir -p $RESIMGPATH
            NEWRESIMG=${filenames[$c]##*/}
            NEWRESIMG=`echo $NEWRESIMG | sed 's/\.html/-expected\.png/'`
            NEWRESIMG=`echo $NEWRESIMG | sed 's/\.htm/-expected\.png/'`
            NEWRESIMG=$RESIMGPATH"/"$NEWRESIMG
            #echo $NEWRESIMG
            mv $RESIMG $NEWRESIMG

            EXPIMG=${filenames[$c]}
            EXPIMG=`echo $EXPIMG | sed 's/_converted/_result\/font_dependent\/x64/'`
            EXPIMG=`echo $EXPIMG | sed 's/\.html/-expected\.png/'`
            EXPIMG=`echo $EXPIMG | sed 's/\.htm/-expected\.png/'`
            IMGDIFF="./tool/pixel_test/bin/image_diff"
            if [[ -f ${EXPIMG} ]]; then
                DIFF=`$IMGDIFF $NEWRESIMG $EXPIMG`
            else
                DIFF="failed"
            fi
            if [[ "$DIFF" = *"0.00% passed" ]]; then
                PASSTC=`expr $PASSTC + 1`
                echo -e "${GREEN}[PASS]${RESET}" ${filenames[$c]}
                passfile=${filenames[$c]//_converted\//\/}
                if [ ! -f ${passfile} ]; then
                    filepath=${filenames[$c]%/*}
                    passpath=${passfile%/*}
                    mkdir -p ${passpath}
                    if [[ -d ${filepath}/support && ! -d ${passpath}/support ]]; then
                        echo "--- Copied folder: "${filepath}"/support"
                        cp -rf ${filepath}/support ${passpath}/support
                    fi
                    cp ${filenames[$c]} $passpath
                fi
                if [[ "$2" = "update" ]]; then
                    passpng=${EXPIMG//test\/reftest/test\/regression\/reftest}
                    mkdir -p ${passpng%/*}
                    cp $EXPIMG ${passpng}
                fi
            else
                FAILTC=`expr $FAILTC + 1`
                echo -e "${RED}[FAIL]${RESET}" ${filenames[$c]}
                DIFFIMG=${NEWRESIMG//-expected/-diff}
                if [[ -f ${EXPIMG} ]]; then
                    $IMGDIFF --diff $NEWRESIMG $EXPIMG $DIFFIMG
                    if [[ "$2" = "update" ]]; then
                        passpng=${EXPIMG//test\/reftest/test\/regression\/reftest}
                        if [[ -f ${passpng} ]]; then
                            rm ${passpng}
                        fi
                    fi
                fi
            fi
            rm -f $RESIMG
        fi

        TCFILE=`expr $TCFILE + 1`
        if [ $TESTSUITE -ne 3 ]; then
            rm -f $TMPFILE
        fi
    done
done

wait;

# Print the summary
echo -e "\n${BOLD}###### Summary ######${RESET}\n"
if [ ! -f "$RESULTCSV" ]; then
    echo -e "Test Suite\tPass\tFail\tSkip\tCheck\tTotal" >> $RESULTCSV
fi

if [ $TESTSUITE -eq 0 ]; then
    echo -e "${YELLOW}Run" `expr $TCFILE` "test cases:" $PASSTC "passed," $FAILTC "failed," $SKIPTC "skipped.${RESET}\n"
    echo -e $TESTSUITENAME"\t"$PASSTC"\t"$FAILTC"\t"$SKIPTC"\t"$CHECKTCFILE"\t"$TCFILE >> $RESULTCSV
elif [ $TESTSUITE -eq 1 ]; then
    echo -e "${YELLOW}Run" `expr $PASSTC + $FAILTC` "test cases ("$TCFILE" files):" $PASSTC "passed ("$PASSTCFILE "files)," $FAILTC "failed," $CHECKTCFILE "files are needed to check.${RESET}\n"
    echo -e $TESTSUITENAME"\t"$PASSTC"\t"$FAILTC"\t0\t"$CHECKTCFILE"\t"`expr $PASSTC + $FAILTC` >> $RESULTCSV
else
    echo -e "${YELLOW}Run" `expr $TCFILE` "test cases:" $PASSTC "passed," $FAILTC "failed.${RESET}\n"
    echo -e $TESTSUITENAME"\t"$PASSTC"\t"$FAILTC"\t0\t"$CHECKTCFILE"\t"$TCFILE >> $RESULTCSV
fi

# Regression test
if [ "$REGRESSION" = true ]; then
    echo -e "${BOLD}###### Regression Test ######${RESET}\n"

    DIFF=`diff -u $PASSFILE $PASSFILENEW`
    if [ "$DIFF" = "" ] ; then
        echo -e "${GREEN}[PASSED] no change${RESET}\n"
    else
        echo -e "${RED}[CHECKED] some changes${RESET}\n"
        diff -u $PASSFILE $PASSFILENEW
        echo

        # Update the regression lists
        #cp $PASSFILENEW test/regression/reftest/dom
    fi

    # Remove temporary file
    rm $PASSFILENEW
fi
