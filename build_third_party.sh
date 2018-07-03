#!/bin/bash

arch="x64"
if [ "$1" == "x64" ]; then
    arch="x64"
elif [ "$1" == "arm" ]; then
    arch="arm"
fi

echo "=================================="
echo "Building third party for: " $arch
echo "=================================="

STARFISH_ROOT=`pwd`


if [ "$arch" == "x64" ]; then
    echo "arch: x64"
    cd $STARFISH_ROOT
    ./build/build_zeromq.sh

    cd $STARFISH_ROOT
    ./build/build_escargot.sh

    cd $STARFISH_ROOT
    ./build/build_gc.sh

    if [ "$2" == "skia" ]; then
        echo "build skia"
        cd $STARFISH_ROOT
        cd ./third_party/android/skia/
        rm -rf out/
        bin/gn gen out/Release/Shared --args="is_component_build=true is_debug=false target_cpu=\"x64\""
        ninja -C out/Release/Shared
        bin/gn gen out/Debug/Shared --args="is_component_build=true target_cpu=\"x64\""
        ninja -C out/Debug/Shared
    fi
#TODO libtuv for x64

elif [ "$arch" == "arm" ]; then
    #cd $STARFISH_ROOT
    #./build/build_zeromq.sh tizen_obs_arm

    cd $STARFISH_ROOT
    ./build/build_escargot.sh tizen_obs_arm

    cd $STARFISH_ROOT
    if [ "$2" == "gear" ]; then
        ./build/build_gc.sh tizen_obs_arm gear
    else
        ./build/build_gc.sh tizen_obs_arm
    fi

    cd $STARFISH_ROOT
    cd third_party/libtuv
    make clean
    TUV_BUILD_TYPE=release TUV_BUILDTESTER=no TUV_CREATE_SHARED_LIB=yes TUV_BOARD=None TUV_PLATFORM=noarch-tizen make -j$NUMPROC
    cd -
else
    echo "Unknown option"
fi


cd $STARFISH_ROOT
#python ./binding_generator/scripts/starfish_code_generator.py src/ src/binding/
