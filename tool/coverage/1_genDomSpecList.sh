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
        echo $f | sed 's/\.\.\/\.\.\/test/test/' > out/t
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

# css
css1=`find $path/test/regression/reftest/csswg-test/css1 -name '*.htm*' | sort `
css21=`find $path/test/regression/reftest/csswg-test/css21 -name '*.htm*'  | sort`
cssBackgrounds=`find $path/test/regression/reftest/csswg-test/css-backgrounds-3 -name '*.htm*' | sort`
cssColor=`find $path/test/regression/reftest/csswg-test/css-color-3 -name '*.htm*' | sort`
cssTransforms=`find $path/test/regression/reftest/csswg-test/css-transforms-1 -name '*.htm*' | sort`

run "in/dom.blink.fast.raw" $list1
run "in/dom.blink.fast.html.raw" $list11
run "in/dom.webkit.fast.raw" $list2
run "in/dom.webkit.fast.html.raw" $list22
run "in/dom.bidi.raw" $list3
run "in/dom.wpt.raw" $list4

run "in/dom.css1.raw" $css1
run "in/dom.css21.raw" $css21
run "in/dom.css.backgrounds.3.raw" $cssBackgrounds
run "in/dom.css.color.3.raw" $cssColor
run "in/dom.css.transforms.1.raw" $cssTransforms
