#!/bin/bash
BOLD='\033[1m'
RESET='\033[0m'

TESTSUITE=("test/regression/reftest/web-platform-tests/"
           "test/regression/reftest/dom-conformance-test/html/"
           "test/regression/reftest/vendor/blink/dom/html/"
           "test/regression/reftest/vendor/blink/fast/dom/"
           "test/regression/reftest/vendor/blink/fast/html/"
           "test/regression/reftest/vendor/webkit/dom/html/"
           "test/regression/reftest/vendor/webkit/fast/dom/"
           "test/regression/reftest/vendor/webkit/fast/html/"
           "test/regression/reftest/vendor/gecko/dom_tests_mochitest/"
           "test/regression/reftest/bidi/"
           "test/regression/reftest/csswg-test/css1/"
           "test/regression/reftest/csswg-test/css21/"
           "test/regression/reftest/csswg-test/css-backgrounds-3/"
           "test/regression/reftest/csswg-test/css-color-3/"
           "test/regression/reftest/csswg-test/css-transforms-1/")

DESTDIR="tool/coverage/in"
RESFILE="tmp.res"
mkdir out > /dev/null 2>&1
rm $RAWFILE $RESFILE > /dev/null 2>&1

for i in ${TESTSUITE[@]}; do
    if [[ "$i" = *"web-platform-tests"* ]]; then
        TESTNAME="wpt"
    elif [[ "$i" = *"dom-conformance-test"* ]]; then
        TESTNAME="dct"
    elif [[ "$i" = *"blink/dom"* ]]; then
        TESTNAME="blink.dom"
    elif [[ "$i" = *"blink/fast/dom"* ]]; then
        TESTNAME="blink.fast.dom"
    elif [[ "$i" = *"blink/fast/html"* ]]; then
        TESTNAME="blink.fast.html"
    elif [[ "$i" = *"webkit/dom"* ]]; then
        TESTNAME="webkit.dom"
    elif [[ "$i" = *"webkit/fast/dom"* ]]; then
        TESTNAME="webkit.fast.dom"
    elif [[ "$i" = *"webkit/fast/html"* ]]; then
        TESTNAME="webkit.fast.html"
    elif [[ "$i" = *"gecko/dom_tests_mochitest"* ]]; then
        TESTNAME="gecko.dom"
    elif [[ "$i" = *"bidi"* ]]; then
        TESTNAME="bidi"
    elif [[ "$i" = *"css1"* ]]; then
        TESTNAME="css1"
    elif [[ "$i" = *"css21"* ]]; then
        TESTNAME="css21"
    elif [[ "$i" = *"css-backgrounds-3"* ]]; then
        TESTNAME="css.backgrounds.3"
    elif [[ "$i" = *"css-color-3"* ]]; then
        TESTNAME="css.color.3"
    elif [[ "$i" = *"css-transforms-1"* ]]; then
        TESTNAME="css.transforms.1"
    fi

    echo -e "${BOLD}## [CSS & HTML] Syntax checking for [${i}]..${RESET}\n"

    TC=$(find $i -regex ".*\.\(htm\|html\)$" | sort)
    CNT=-1
    if [[ "$j" = *"/csswg-test/"* || "$j" = *"/bidi/"* ]]; then
        ROTATE=1
    else
        ROTATE="$(nproc)"
    fi
    ARRAY=($TC)
    POS=$((${#ARRAY[*]}-1))
    LAST=${ARRAY[$POS]}

    for j in $TC; do
        CNT=$((CNT+1))
        if [[ $CNT == $ROTATE ]]; then
            CNT=0
        fi
        RAWFILE="tmp"$CNT

        echo -e "+++TC:"$j >> $RAWFILE
        if [[ "$j" = *"/csswg-test/"* || "$j" = *"/bidi/"* ]]; then
            ./StarFish $j --screen-shot=./out.png &>> $RAWFILE &
        else
            ./StarFish $j --hide-window &>> $RAWFILE &
        fi

        if [[ $CNT != $((ROTATE-1)) && $j != $LAST ]]; then
            continue;
        fi
        wait;

        for k in $(seq 0 $CNT); do
            TMPFILE="tmp"$k
            cat $TMPFILE | grep "+++" >> $RESFILE
            rm $TMPFILE
        done
        sed -i 's/+++//g' $RESFILE
    done

    # Save the results
    CSSRES=$DESTDIR"/css."$TESTNAME".raw"
    HTMLRES=$DESTDIR"/html."$TESTNAME".raw"
    rm $CSSRES $HTMLRES > /dev/null 2>&1

    LINE=$(cat $RESFILE)
    for i in $LINE; do
        if [[ "$i" == "TC:"* ]]; then
            echo $i >> $CSSRES
            echo $i >> $HTMLRES
        elif [[ "$i" == "tag:"* || "$i" == "attr:"* || "$i" == "doctype:"* ]]; then
            echo $i >> $HTMLRES
        elif [[ "$i" == "style:"* || "$i" == "selector:"* ]]; then
            echo $i >> $CSSRES
        fi
    done

    perl -i -pe 's/tag://g' $HTMLRES
    perl -i -pe 's/attr://g' $HTMLRES
    perl -i -pe 's/doctype://g' $HTMLRES
    perl -i -pe 's/style://g' $CSSRES
    perl -i -pe 's/selector://g' $CSSRES
    perl -i -pe 's/\n/\t/g' $CSSRES $HTMLRES
    perl -i -pe 's/TC:/\n/g' $CSSRES $HTMLRES

    # Remove unnecessary files 
    rm $RESFILE
done

echo -e "${BOLD}Finished! You can find the results in \`tool/coverage/in/\`${RESET}\n"

