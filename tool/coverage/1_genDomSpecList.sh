#!/bin/bash

mkdir -p out
rm -f in/dom.*.raw
path='../..';

TESTSUITE=("test/regression/reftest/vendor/blink/dom/html"
           "test/regression/reftest/vendor/webkit/dom/html"
           "test/regression/reftest/vendor/gecko"
           "test/regression/reftest/dom-conformance-test/html"
           "test/regression/reftest/vendor/blink/fast/dom"
           "test/regression/reftest/vendor/blink/fast/html"
           "test/regression/reftest/vendor/webkit/fast/dom"
           "test/regression/reftest/vendor/webkit/fast/html"
           "test/regression/reftest/bidi/International/tests/html-css"
           "test/regression/reftest/web-platform-tests"
           "test/regression/reftest/csswg-test/css1"
           "test/regression/reftest/csswg-test/css21"
           "test/regression/reftest/csswg-test/css-backgrounds-3"
           "test/regression/reftest/csswg-test/css-color-3"
           "test/regression/reftest/csswg-test/css-transforms-1")

for tc in ${TESTSUITE[@]}; do
    echo $tc
    res=$tc
    res=`echo $res | sed 's/test\/regression\/reftest\///'`
    res=`echo $res | sed 's/\//\./g'`
    res="in/dom.$res.raw"
    for f in `find $path/$tc -name '*.htm*' | sort`; do
        echo $f | sed 's/\.\.\/\.\.\/test/test/' > out/t.txt
        $path/StarFish $f --screen-shot="out/tmp.png" &> out/tt.txt
        grep '&&&' out/tt.txt | sort | uniq >> out/t.txt
        tr '\n' '\t' < out/t.txt | sed 's/\t$/\n/' >> $res
    done
done
