#!/bin/bash
#set -x

function build_escargot_for_tizen() {
    ROOT=`pwd`
    export ESCARGOT_ARCH=arm

    cd third_party/escargot/
    mkdir -p include
    make install_header_to_include

    #./build_third_party.sh tizen_obs_${ESCARGOT_ARCH} only_release
    mkdir -p out/tizen_obs/${ESCARGOT_ARCH}/interpreter/release
    touch out/tizen_obs/${ESCARGOT_ARCH}/interpreter/release/escargot
    touch out/tizen_obs/${ESCARGOT_ARCH}/interpreter/debug/escargot

    make tizen_obs_${ESCARGOT_ARCH}.interpreter.release.static -j$NUMPROC

    cp out/tizen_obs/${ESCARGOT_ARCH}/interpreter/release/libescargot.a ./
    cd $ROOT
}

function build_escargot_for_linux() {
    ROOT=`pwd`
    cd third_party/escargot/
    git submodule init
    git submodule update third_party/GCutil third_party/checked_arithmetic third_party/double_conversion third_party/icu third_party/rapidjson third_party/yarr
    make clean
    make x64.interpreter.release.static -j$NUMPROC
    make x64.interpreter.debug.static -j$NUMPROC
    make install_header_to_include
    cd $ROOT
}

if [[ $1 == tizen_obs_arm ]]; then
    build_escargot_for_tizen
elif [[ $1 == tizen_obs_i386 ]]; then
    echo "TODO"
else # full build
    build_escargot_for_linux
fi
