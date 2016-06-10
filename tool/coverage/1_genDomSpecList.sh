#!/bin/bash
BOLD='\033[1m'
RESET='\033[0m'

mkdir -p out
rm -f in/dom.*.raw
path='../..';

TESTSUITE=("test/demo/coverage"
           "test/regression/reftest/vendor/blink/dom/html"
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
    if [[ "$tc" = *"web-platform-tests"* ]]; then
        res="wpt"
    elif [[ "$tc" = *"dom-conformance-test"* ]]; then
        res="dct"
    elif [[ "$tc" = *"blink/dom"* ]]; then
        res="blink.dom"
    elif [[ "$tc" = *"blink/fast/dom"* ]]; then
        res="blink.fast.dom"
    elif [[ "$tc" = *"blink/fast/html"* ]]; then
        res="blink.fast.html"
    elif [[ "$tc" = *"webkit/dom"* ]]; then
        res="webkit.dom"
    elif [[ "$tc" = *"webkit/fast/dom"* ]]; then
        res="webkit.fast.dom"
    elif [[ "$tc" = *"webkit/fast/html"* ]]; then
        res="webkit.fast.html"
    elif [[ "$tc" = *"gecko"* ]]; then
        res="gecko.dom"
    elif [[ "$tc" = *"bidi"* ]]; then
        res="bidi"
    elif [[ "$tc" = *"css1"* ]]; then
        res="css1"
    elif [[ "$tc" = *"css21"* ]]; then
        res="css21"
    elif [[ "$tc" = *"css-backgrounds-3"* ]]; then
        res="css.backgrounds.3"
    elif [[ "$tc" = *"css-color-3"* ]]; then
        res="css.color.3"
    elif [[ "$tc" = *"css-transforms-1"* ]]; then
        res="css.transforms.1"
    elif [[ "$tc" = *"demo"* ]]; then
        res="demo"
    fi

    echo -e "${BOLD}## [DOM] Syntax checking for [${tc}]..${RESET}\n"

    res="in/dom.$res.raw"
    for f in `find $path/$tc -name '*.htm*' | sort`; do
        echo $f | sed 's/\.\.\/\.\.\/test/test/' > out/t.txt
        $path/StarFish $f --screen-shot="out/tmp.png" &> out/tt.txt
        grep '&&&' out/tt.txt | sort | uniq >> out/t.txt
        tr '\n' '\t' < out/t.txt | sed 's/\t$/\n/' >> $res
    done
done

echo -e "${BOLD}Finished! You can find the results in \`tool/coverage/in/\`${RESET}\n"

