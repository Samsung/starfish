#!/bin/bash
BOLD='\033[1m'
RESET='\033[0m'

TESTSUITE=("test/reftest/vendor/blink/fast/html/"
           "test/reftest/vendor/webkit/fast/html/"
           "test/reftest/csswg-test/css1/"
           "test/reftest/csswg-test/css21/"
           "test/reftest/csswg-test/css-backgrounds-3/"
           "test/reftest/csswg-test/css-color-3"
           "test/reftest/csswg-test/css-transforms-1"
           "test/reftest/dom-conformance-test/html/"
           "test/reftest/web-platform-tests/dom/"
           "test/reftest/web-platform-tests/DOMEvents/"
           "test/reftest/web-platform-tests/html/"
           "test/reftest/web-platform-tests/page-visibility/"
           "test/reftest/web-platform-tests/progress-events/"
           "test/reftest/web-platform-tests/XMLHttpRequest/"
           "test/reftest/bidi/"
           "test/reftest/vendor/blink/dom/"
           "test/reftest/vendor/blink/fast/dom/"
           "test/reftest/vendor/blink/fast/html/"
           "test/reftest/vendor/gecko/dom_tests_mochitest/"
           "test/reftest/vendor/webkit/dom/"
           "test/reftest/vendor/webkit/fast/dom/"
           "test/reftest/vendor/webkit/fast/html/")

rm tmp tmp.res > /dev/null 2>&1

cnt=0
RAWFILE="tmp.raw"
RESFILE="tmp.res"

for i in ${TESTSUITE[@]}; do
    cnt=$((cnt+1))
    TMPFILE="tmp"$cnt

    echo -e "${BOLD}## Syntax checking for [${i}]..${RESET}\n"
    phantomjs tool/pixel_test/syntaxRelease.js ${i} > $TMPFILE 

    cat $RAWFILE $TMPFILE > tmp
    cat tmp > $RAWFILE
    rm tmp

    cat $TMPFILE | grep "+++" > $TMPFILE".res" 
    sed -i 's/+++//g' $TMPFILE".res"
    cat $RESFILE $TMPFILE".res" > tmp
    cat tmp > $RESFILE
    rm tmp
done

sed -i 's/+++//g' tmp.res

