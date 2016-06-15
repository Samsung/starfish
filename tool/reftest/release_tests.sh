#!/bin/bash

if [ ! -n "$LWE_REPO" ]; then
    echo "Define the \$LWE_REPO."
    exit
fi

# Update `test/regression`
./tool/reftest/update_regression.sh

# Build binaries
./build_third_party.sh
make clean && make tizen_wearable_arm.exe.debug -j
if [[ `file StarFish` != *"ARM"* ]]; then
    echo "Build failed! check the StarFish binary."
    exit
fi
cp StarFish test/regression/bin/tizen-wearable-2.3-target-arm

make clean && make tizen_wearable_emulator.exe.debug -j
if [[ `file StarFish` != *"ELF 32-bit"* ]]; then
    echo "Build failed! check the StarFish binary."
    exit
fi
cp StarFish test/regression/bin/tizen-wearable-2.3-emulator

# Update lwe repository
rm $LWE_REPO/test/bin $LWE_REPO/test/reftest $LWE_REPO/test/tool -rf
cp test/regression/bin test/regression/reftest test/regression/tool $LWE_REPO/test -rf
rm $LWE_REPO/test/reftest/bidi/International/tests/html-css/x64 -rf

