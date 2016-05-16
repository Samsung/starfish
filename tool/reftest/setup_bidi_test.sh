#!/bin/sh

DIR=/home/developer/

make tizen_wearable_arm.exe.debug -j
sdb push StarFish $DIR
sdb push $TIZEN_SDK_HOME/tools/arm-linux-gnueabi-gcc-4.9/arm-linux-gnueabi/lib/debug/libstdc++.so.6 $DIR
sdb push tool/imgdiff/imgdiffEvas.exe $DIR
sdb push tool/reftest/bidi_test_run.sh $DIR
sdb push tool/reftest/bidi_test_clean.sh $DIR
sdb push test/regression/reftest/bidi/International/tests/html-css $DIR
