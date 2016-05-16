#!/bin/sh

# This is a bidi test to be run on a device

export LD_LIBRARY_PATH=.

for f in `ls *h5.html`; do
    # ./StarFish $f --screen-shot=$f.tmp.png --width=900 --height=900 > /dev/null 2>&1
    ELM_ENGINE="shot:file=$f.tmp.png" ./StarFish $f --width=900 --height=900 > /dev/null 2>&1
    r=`./imgdiffEvas.exe $f-expected.png $f.tmp.png`
    echo $r: $f
    rm -f $f.tmp.png
done

