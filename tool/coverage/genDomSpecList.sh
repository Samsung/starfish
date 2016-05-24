#!/bin/sh

mkdir -p out
rm -f in/dom.*.raw
path='../..';


# combine html + js
list1=`find $path/test/regression/reftest/vendor/blink/dom/html  -name '*.htm*' | sort `;
list2=`find $path/test/regression/reftest/vendor/webkit/dom/html  -name '*.htm*' | sort `;
list4=`find $path/test/regression/reftest/vendor/gecko  -name '*.htm*' | sort `;
list3=`find $path/test/regression/reftest/dom-conformance-test/html -name '*.htm*' | sort`;

run() {
    res=$1
    shift
    list="$@"
    for f in $list; do
        echo $f
        echo $res
        $path/StarFish $f --screen-shot="out/tmp.png" > out/t.txt
        grep '&&&' out/t.txt | sort | uniq | tr '\n' '\t' > out/tt.txt
        echo $f > out/t
        paste out/t out/tt.txt >> $res
    done
}

run "in/dom.blink.raw" $list1
run "in/dom.webkit.raw" $list2
run "in/dom.gecko.raw" $list4
run "in/dom.dct.raw" $list3

# run .html
list1=`find $path/test/regression/reftest/vendor/blink/fast/dom -name '*.htm*' | sort `
list11=`find $path/test/regression/reftest/vendor/blink/fast/html -name '*.htm*' | sort `
list2=`find $path/test/regression/reftest/vendor/webkit/fast/dom -name '*.htm*' | sort `
list22=`find $path/test/regression/reftest/vendor/webkit/fast/html -name '*.htm*' | sort `
list3=`find $path/test/regression/reftest/bidi/International/tests/html-css -name '*.html' | sort `
list4=`find $path/test/regression/reftest/web-platform-tests -name '*.htm*' | sort `


run "in/dom.blink.fast.raw" $list1
run "in/dom.blink.fast.html.raw" $list11
run "in/dom.webkit.fast.raw" $list2
run "in/dom.webkit.fast.html.raw" $list22
run "in/dom.bidi.raw" $list3
run "in/dom.wpt.raw" $list4
