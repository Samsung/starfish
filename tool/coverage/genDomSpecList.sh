#!/bin/sh

mkdir -p out
rm -f in/dom.raw.csv
path='../..';

# combine html + js
list1=`find $path/test/regression/reftest/vendor/blink/dom/html  -name '*.html'`;
list2=`find $path/test/regression/reftest/vendor/webkit/dom/html  -name '*.html'`;
list3=`find $path/test/regression/reftest/dom-conformance-test/html -name '*.html'`;

for f in $list1 $list2 $list3; do
    js=`echo $f | sed -e 's/\.html/\.js/g'`
    cat $f $js > $f.js
    node genDomMethods.js $f.js >> in/dom.raw.csv
    rm -f $f.js
done

# run .html
list1=`find $path/test/regression/reftest/vendor/blink/fast/dom -name '*.html'`
list2=`find $path/test/regression/reftest/vendor/webkit/fast/dom -name '*.html'`
list3=`find $path/test/regression/reftest/bidi/International/tests/html-css -name '*.html'`
list4=`find $path/test/reftest/web-platform-tests/dom -name '*.html'`

for f in $list1 $list2 $list3 $list4; do
    node genDomMethods.js $f >> in/dom.raw.csv
done
