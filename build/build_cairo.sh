#!/bin/bash
cd ../third_party/windows/cairo
#export CFLAGS+=" -ffat-lto-objects"
#export CXXFLAGS+=" -ffat-lto-objects"
NOCONFIGURE=1 ./autogen.sh
./configure --with-pic --enable-fc --enable-ft --enable-tee --disable-xlib --disable-xcb --disable-gtk-doc --enable-static
make -j$NUMPROC V=1
