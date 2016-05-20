#!/bin/sh

# coverage
list="
dom.wpt.raw
dom.dct.raw
dom.blink.raw
dom.blink.fast.raw
dom.blink.fast.html.raw
dom.webkit.raw
dom.webkit.fast.raw
dom.webkit.fast.html.raw
dom.gecko.raw
dom.bidi.raw
"

for f in $list; do
    node genTable.js in/specDOM.txt in/$f > out/$f.csv
done

rm -f out/dom.raw
for f in $list; do
    cat in/$f >> out/dom.raw
done

rm -f out/dom.csv.tmp
for f in $list; do
    cat out/$f.csv >> out/dom.csv.tmp
done

node genTable.js in/specDOM.txt out/dom.raw | tail -1 > t.txt
cat out/dom.csv.tmp t.txt > out/dom.csv
rm out/dom.csv.tmp t.txt

node genTable.js in/specHTML.txt in/html.res > out/html.csv
node genTable.js in/specCSS.txt in/css.res > out/css.csv
