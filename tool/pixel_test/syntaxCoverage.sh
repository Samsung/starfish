#!/bin/bash
BOLD='\033[1m'
RESET='\033[0m'

rm test/reftest/dom-conformance-test/html/level1/core/hc_noderemovechild.html
rm test/reftest/vendor/blink/dom/html/level1/core/hc_noderemovechild.html
rm test/reftest/vendor/gecko/dom_tests_mochitest/dom-level1-core/hc_noderemovechild.html
rm test/reftest/vendor/webkit/dom/html/level1/core/hc_noderemovechild.html

TESTSUITE=("test/reftest/csswg-test/css1/"
           "test/reftest/csswg-test/css21/"
           "test/reftest/csswg-test/css-backgrounds-3/"
           "test/reftest/csswg-test/css-color-3/"
           "test/reftest/csswg-test/css-transforms-1/"
           "test/reftest/dom-conformance-test/html/"
           "test/reftest/web-platform-tests/dom/"
           "test/reftest/web-platform-tests/DOMEvents/"
           "test/reftest/web-platform-tests/html/"
           "test/reftest/web-platform-tests/page-visibility/"
           "test/reftest/web-platform-tests/progress-events/"
           "test/reftest/web-platform-tests/XMLHttpRequest/"
           "test/reftest/bidi/"
           "test/reftest/vendor/blink/dom/html/"
           "test/reftest/vendor/blink/fast/dom/"
           "test/reftest/vendor/blink/fast/html/"
           "test/reftest/vendor/gecko/dom_tests_mochitest/"
           "test/reftest/vendor/webkit/dom/html/"
           "test/reftest/vendor/webkit/fast/dom/"
           "test/reftest/vendor/webkit/fast/html/")

CSS="css.res"
HTML="html.res"
RAWFILE="tmp.raw"
RESFILE="tmp.res"
TMPFILE1="tmp1"
TMPFILE2="tmp2"
rm $CSS $HTML $RAWFILE $RESFILE $TMPFILE1 $TMPFILE2 > /dev/null 2>&1

for i in ${TESTSUITE[@]}; do
    echo -e "${BOLD}## Syntax checking for [${i}]..${RESET}\n"
    phantomjs tool/pixel_test/syntaxCoverage.js ${i} > $TMPFILE1 

    cat $TMPFILE1 >> $RAWFILE
    cat $TMPFILE1 | grep "+++" > $TMPFILE2
    sed -i 's/+++//g' $TMPFILE2
    cat $TMPFILE2 >> $RESFILE
    rm $TMPFILE1 $TMPFILE2
done

sed -i 's/+++//g' $RESFILE
LINE=$(cat $RESFILE)

for i in $LINE; do
    if [[ "$i" == "TC:"* ]]; then
        echo $i >> $CSS
        echo $i >> $HTML
    elif [[ "$i" == "tag:"* || "$i" == "attr:"* || "$i" == "doctype:"* ]]; then
        echo $i >> $HTML
    elif [[ "$i" == "style:"* || "$i" == "inlinestyle:"* ]]; then
        echo $i >> $CSS
    fi
done

perl -i -pe 's/tag://g' $CSS $HTML
perl -i -pe 's/attr://g' $CSS $HTML
perl -i -pe 's/doctype://g' $CSS $HTML
perl -i -pe 's/inlinestyle://g' $CSS $HTML
perl -i -pe 's/style://g' $CSS $HTML
perl -i -pe 's/\n/\t/g' $CSS $HTML
perl -i -pe 's/TC:/\n/g' $CSS $HTML

git checkout test/reftest/dom-conformance-test/html/level1/core/hc_noderemovechild.html
git checkout test/reftest/vendor/blink/dom/html/level1/core/hc_noderemovechild.html
git checkout test/reftest/vendor/gecko/dom_tests_mochitest/dom-level1-core/hc_noderemovechild.html
git checkout test/reftest/vendor/webkit/dom/html/level1/core/hc_noderemovechild.html

echo -e "${BOLD}Finished! You can find the results in css.res and html.res${RESET}\n"

