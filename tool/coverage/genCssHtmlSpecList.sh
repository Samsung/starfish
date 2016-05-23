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

CSS="out/css.res"
HTML="out/html.res"
#XHR="out/xhr.res"
RAWFILE="tmp.raw"
RESFILE="tmp.res"
TMPFILE1="tmp1"
TMPFILE2="tmp2"
mkdir out > /dev/null 2>&1
rm $CSS $HTML $RAWFILE $RESFILE $TMPFILE1 $TMPFILE2 > /dev/null 2>&1

for i in ${TESTSUITE[@]}; do
    echo -e "${BOLD}## Syntax checking for [${i}]..${RESET}\n"
    TC=$(find $i -regex ".*\.\(htm\|html\)$" | sort)
    for j in $TC; do
        echo -e "TC:"$j
        echo -e "+++TC:"$j >> $RAWFILE
        echo -e "+++TC:"$j >> $RESFILE
        ./StarFish $j --screen-shot=./out.png &> $TMPFILE1

        cat $TMPFILE1 >> $RAWFILE
        cat $TMPFILE1 | grep "+++" > $TMPFILE2
        sed -i 's/+++//g' $TMPFILE2
        cat $TMPFILE2 >> $RESFILE
    done
    rm $TMPFILE1 $TMPFILE2
done

sed -i 's/+++//g' $RESFILE
LINE=$(cat $RESFILE)

for i in $LINE; do
    if [[ "$i" == "TC:"* ]]; then
        echo $i >> $CSS
        echo $i >> $HTML
        #echo $i >> $XHR
    elif [[ "$i" == "tag:"* || "$i" == "attr:"* || "$i" == "doctype:"* ]]; then
        echo $i >> $HTML
    elif [[ "$i" == "style:"* || "$i" == "selector:"* ]]; then
        echo $i >> $CSS
    #elif [[ "$i" == "dom:"* ]]; then
    #    echo $i >> $HTML
    #elif [[ "$i" == "xhr:"* ]]; then
    #    echo $i >> $XHR
    fi
done

perl -i -pe 's/tag://g' $HTML
perl -i -pe 's/attr://g' $HTML
perl -i -pe 's/doctype://g' $HTML
perl -i -pe 's/style://g' $CSS
perl -i -pe 's/selector://g' $CSS
perl -i -pe 's/dom://g' $HTML
#perl -i -pe 's/xhr://g' $XHR
#perl -i -pe 's/\n/\t/g' $CSS $HTML $XHR
perl -i -pe 's/\n/\t/g' $CSS $HTML
perl -i -pe 's/TC:/\n/g' $CSS $HTML
perl -i -pe 's/window\tdocument\tinnerWidth\tinnerHeight//g' $HTML

echo -e "${BOLD}Finished! You can find the results in css.res and html.res${RESET}\n"

