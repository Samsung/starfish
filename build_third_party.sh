#!/bin/bash
#set -x
git submodule init
git submodule update

./build/build_zeromq.sh
./build/build_gc.sh

cd third_party/escargot/
git submodule init
git submodule update third_party/GCutil
make clean
make x64.interpreter.release.static -j$NUMPROC
make x64.interpreter.debug.static -j$NUMPROC
make install_header_to_include

cd -

cd third_party/libtuv
make clean
make -j$NUMPROC

cd -
