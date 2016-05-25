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
           "test/regression/reftest/bidi/")

DESTDIR="tool/coverage/in"
RAWFILE="tmp.raw"
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
    fi

    echo -e "${BOLD}## Syntax checking for [${i}]..${RESET}\n"
    TC=$(find $i -regex ".*\.\(htm\|html\)$" | sort)
    for j in $TC; do
        echo -e "+++TC:"$j >> $RAWFILE
        ./StarFish $j --screen-shot=./out.png &>> $RAWFILE

        cat $RAWFILE | grep "+++" > $RESFILE
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
    perl -i -pe 's/dom://g' $HTMLRES
    perl -i -pe 's/\n/\t/g' $CSSRES $HTMLRES
    perl -i -pe 's/TC:/\n/g' $CSSRES $HTMLRES
    perl -i -pe 's/window\tdocument\tinnerWidth\tinnerHeight//g' $HTMLRES

    # Remove unnecessary files 
    rm $RAWFILE $RESFILE
done

echo -e "${BOLD}Finished! You can find the results in \`tool/coverage/in/\`${RESET}\n"

