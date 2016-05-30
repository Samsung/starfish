#!/bin/bash

if [ ! -n "$LWE_REPO" ]; then
    echo "Define the \$LWE_REPO."
    exit
fi

# Update `test/regression`
./tool/reftest/update_regression.sh

make tizen_wearable_arm.exe.debug -j
if [[ `file StarFish` != *"ARM"* ]]; then
    echo "Build failed! check the StarFish binary."
    exit
fi
cp StarFish test/regression/bin/tizen_arm

# Update lwe repository
rm $LWE_REPO/test/bin $LWE_REPO/test/reftest $LWE_REPO/test/tool -rf
cp test/regression/bin test/regression/reftest test/regression/tool $LWE_REPO/test -rf
rm $LWE_REPO/test/reftest/csswg-test $LWE_REPO/test/tool/csswg-test -rf
rm $LWE_REPO/test/reftest/bidi/International/tests/html-css/x64 -rf

