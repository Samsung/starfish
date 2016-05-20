#!/bin/sh

mkdir -p out
rm -f in/dom.*.raw
path='../..';


# combine html + js
list1=`find $path/test/regression/reftest/vendor/blink/dom/html  -name '*.htm*' | sort `;
list2=`find $path/test/regression/reftest/vendor/webkit/dom/html  -name '*.htm*' | sort `;
list4=`find $path/test/regression/reftest/vendor/gecko  -name '*.htm*' | sort `;
list3=`find $path/test/regression/reftest/dom-conformance-test/html -name '*.htm*' | sort`;

for f in $list1; do
    js=`echo $f | sed -e 's/\.html/\.js/g'`
    cat $f $js > $f.js
    node genDomMethods.js $f.js >> in/dom.blink.raw
    rm -f $f.js
done


for f in $list2; do
    js=`echo $f | sed -e 's/\.html/\.js/g'`
    cat $f $js > $f.js
    node genDomMethods.js $f.js >> in/dom.webkit.raw
    rm -f $f.js
done

for f in $list4; do
    js=`echo $f | sed -e 's/\.html/\.js/g'`
    cat $f $js > $f.js
    node genDomMethods.js $f.js >> in/dom.gecko.raw
    rm -f $f.js
done

for f in $list3; do
    js=`echo $f | sed -e 's/\.html/\.js/g'`
    cat $f $js > $f.js
    node genDomMethods.js $f.js >> in/dom.dct.raw
    rm -f $f.js
done


# run .html
list1=`find $path/test/regression/reftest/vendor/blink/fast/dom -name '*.htm*' | sort `
list11=`find $path/test/regression/reftest/vendor/blink/fast/html -name '*.htm*' | sort `
list2=`find $path/test/regression/reftest/vendor/webkit/fast/dom -name '*.htm*' | sort `
list22=`find $path/test/regression/reftest/vendor/webkit/fast/html -name '*.htm*' | sort `
list3=`find $path/test/regression/reftest/bidi/International/tests/html-css -name '*.html' | sort `
list4=`find $path/test/regression/reftest/web-platform-tests -name '*.htm*' | sort `

for f in $list1; do
    node genDomMethods.js $f >> in/dom.blink.fast.raw
done

for f in $list11; do
    node genDomMethods.js $f >> in/dom.blink.fast.html.raw
done

for f in $list2; do
    node genDomMethods.js $f >> in/dom.webkit.fast.raw
done

for f in $list22; do
    node genDomMethods.js $f >> in/dom.webkit.fast.html.raw
done

for f in $list3; do
    node genDomMethods.js $f >> in/dom.bidi.raw
done

for f in $list4; do
    node genDomMethods.js $f >> in/dom.wpt.raw
done

