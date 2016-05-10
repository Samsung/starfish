#!/bin/bash
git submodule init
git submodule update
./build_third_party.sh
make clean
make tizen_wearable_arm.lib.release TIZEN_DEVICE_API=true -j
cp libWebWidgetEngine.so public/source/lib/tizen-wearable-2.3-target-arm/
cp libWebWidgetEngine.so public/starting_with_ide/tizen-wearable-2.3-target-arm/lib/
make tizen_wearable_emulator.lib.release TIZEN_DEVICE_API=true -j
cp libWebWidgetEngine.so public/source/lib/tizen-wearable-2.3-emulator-x86/
cp libWebWidgetEngine.so public/starting_with_ide/tizen-wearable-2.3-emulator-x86/lib/
cp -r inc/* public/source/include/
