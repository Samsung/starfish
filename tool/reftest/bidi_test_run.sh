#!/bin/sh

# This is a bidi test to be run on desktop and device
export LD_LIBRARY_PATH=.
BIDI_PATH=test/regression/reftest/bidi/International/tests/html-css
OUT=out/bidi

mkdir -p $OUT

for f in `ls $BIDI_PATH/*.html`; do
    # ./StarFish $f --screen-shot=$f.tmp.png --width=900 --height=900 > /dev/null 2>&1
    out=$f
    out=`echo $out | sed 's/html-css/html-css\/x64/'`
    out=`echo $out | sed 's/\.html/-expected\.png/'`
    out=`basename $out`
    echo "$f -> $OUT/$out"
    ELM_ENGINE="shot:file=$OUT/$out" ./StarFish $f --width=900 --height=900 > /dev/null 2>&1
    #r=`./imgdiffEvas.exe $f-expected.png $f.tmp.png`
    #rm -f $f.tmp.png
done

