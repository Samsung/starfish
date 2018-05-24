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
elif [ "$arch" == "arm" ]; then
    #cd $STARFISH_ROOT
    #./build/build_zeromq.sh tizen_obs_arm

    cd $STARFISH_ROOT
    ./build/build_escargot.sh tizen_obs_arm

    cd $STARFISH_ROOT
    ./build/build_gc.sh tizen_obs_arm
else
    echo "Unknown option"
fi

cd $STARFISH_ROOT
cd third_party/libtuv
make clean
make -j$NUMPROC
cd -

cd $STARFISH_ROOT
#python ./binding_generator/scripts/starfish_code_generator.py src/ src/binding/
