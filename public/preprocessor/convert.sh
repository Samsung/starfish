#!/bin/bash

rm -rf out
mkdir out
node runner.js $1
mv result.xml ./out/
rm result.png

echo "----------------------------------------------------------------"
echo "convert complete"
echo "result.xml saved in out/result.xml"
echo "copy your external resources(image, script...) into out/"
echo "ex) bar/test.html, bar/foo.png -> out/result.xml, out/foo.png"
