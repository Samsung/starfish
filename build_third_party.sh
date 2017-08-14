#!/bin/bash
#set -x
git submodule init
git submodule update

if [ ! -f /proc/cpuinfo ]; then
	echo "Is this Linux? Cannot find or read /proc/cpuinfo"
	exit 1
fi
NUMPROC=$(grep 'processor' /proc/cpuinfo | wc -l)
CU=$PWD

#COMPILER_VERSION_MAJOR=4.9
#COMPILER_VERSION_MINOR=4.9.2
COMPILER_VERSION_MAJOR=4.6
COMPILER_VERSION_MINOR=4.6.4

###########################################################
# LIBAV(FFmpeg) build
###########################################################

AVCONFFLAGS_COMMON=" --enable-shared --enable-pic --disable-asm --disable-encoders --disable-muxers --disable-avresample --disable-swscale --disable-avfilter --disable-avdevice"
AVCONFFLAGS_release=""
AVCONFFLAGS_debug="--enable-debug --disable-optimizations"

AVCONFFLAGS_arm="--target-os=linux --arch=arm32"
AVCONFFLAGS_i386="--target-os=linux --arch=x86"
AVCONFFLAGS_mobile="--enable-cross-compile"

function build_av_for_linux() {
    AVCFLAGS_COMMON=
    AVLDFLAGS_COMMON=
    for host in linux; do
    for arch in x64; do
    for mode in debug release; do
        echo =========================================================================
        echo Building libav for $host $arch $mode

        BUILDDIR=out/$host/$arch/$mode
        rm -rf $BUILDDIR
        mkdir -p $BUILDDIR
        cd $BUILDDIR

        AVCONFFLAGS_HOST=AVCONFFLAGS_$host CFLAGS_HOST=CFLAGS_$host LDFLAGS_HOST=LDFLAGS_$host
        AVCONFFLAGS_ARCH=AVCONFFLAGS_$arch CFLAGS_ARCH=CFLAGS_$arch LDFLAGS_ARCH=LDFLAGS_$arch
        AVCONFFLAGS_MODE=AVCONFFLAGS_$mode CFLAGS_MODE=CFLAGS_$mode LDFLAGS_MODE=LDFLAGS_$mode

        AVCONFFLAGS="$AVCONFFLAGS_COMMON ${!AVCONFFLAGS_HOST} ${!AVCONFFLAGS_ARCH} ${!AVCONFFLAGS_MODE} ${!AVCONFFLAGS_LIBTYPE}"
        CFLAGS="$AVCFLAGS_COMMON ${!CFLAGS_HOST} ${!CFLAGS_ARCH} ${!CFLAGS_MODE} ${!CFLAGS_LIBTYPE}"
        LDFLAGS="$AVLDFLAGS_COMMON ${!LDFLAGS_HOST} ${!LDFLAGS_ARCH} ${!LDFLAGS_MODE} ${!LDFLAGS_LIBTYPE}"

        ../../../../configure $AVCONFFLAGS --extra-cflags="$CFLAGS" --extra-ldflags="$LDFLAGS" > /dev/null
        make -j$NUMPROC > /dev/null

        echo Building libav for $host $arch $mode done
        cd -
    done
    done
    done
}

function build_av_for_tizen() {

    for version in 2.3.1 2.4 3.0; do
    for host in mobile; do
    for arch in arm i386; do
    for mode in debug release; do

        if [[ $arch == arm ]]; then
            device=device
        elif [[ $arch == i386 ]]; then
            device=emulator
        fi

        TIZEN_SYSROOT=$TIZEN_SDK_HOME/platforms/tizen-$version/$host/rootstraps/$host-$version-$device.core
        COMPILER_PREFIX=$arch-linux-gnueabi
        TIZEN_TOOLCHAIN=$TIZEN_SDK_HOME/tools/$COMPILER_PREFIX-gcc-$COMPILER_VERSION_MAJOR

        echo =========================================================================
        if [[ -a $TIZEN_SYSROOT ]]; then
            echo Building libav for tizen $version $host $arch $mode
        else
            echo Skipping libav build for tizen $version $host $arch $mode
            continue
        fi

        BUILDDIR=out/tizen_${version}_${host}/$arch/$mode
        rm -rf $BUILDDIR
        mkdir -p $BUILDDIR
        cd $BUILDDIR

        AVCONFFLAGS_HOST=AVCONFFLAGS_$host CFLAGS_HOST=CFLAGS_$host LDFLAGS_HOST=LDFLAGS_$host
        AVCONFFLAGS_ARCH=AVCONFFLAGS_$arch CFLAGS_ARCH=CFLAGS_$arch LDFLAGS_ARCH=LDFLAGS_$arch
        AVCONFFLAGS_MODE=AVCONFFLAGS_$mode CFLAGS_MODE=CFLAGS_$mode LDFLAGS_MODE=LDFLAGS_$mode

        AVCONFFLAGS="$AVCONFFLAGS_COMMON ${!AVCONFFLAGS_HOST} ${!AVCONFFLAGS_ARCH} ${!AVCONFFLAGS_MODE} --sysroot=${TIZEN_SYSROOT}"
        CFLAGS="$AVCFLAGS_COMMON ${!CFLAGS_HOST} ${!CFLAGS_ARCH} ${!CFLAGS_MODE} ${!CFLAGS_LIBTYPE} --sysroot=${TIZEN_SYSROOT}"
        LDFLAGS="$LDFLAGS_COMMON ${!LDFLAGS_HOST} ${!LDFLAGS_ARCH} ${!LDFLAGS_MODE} ${!LDFLAGS_LIBTYPE}"
        if [[ $LTO == true ]]; then
            CFLAGS+= "-flto -ffat-lto-objects"
            LDFLAGS+= "-flto -ffat-lto-objects"
#            PLUGINFLAGS="--plugin=$TIZEN_TOOLCHAIN/libexec/gcc/$COMPILER_PREFIX/$COMPILER_VERSION_MINOR/liblto_plugin.so"
            TOOLCHAIN_GCC_WRAPPER=-gcc
        fi

        ../../../../configure $AVCONFFLAGS --extra-cflags="$CFLAGS" --extra-ldflags="$LDFLAGS" \
            --cross-prefix=$TIZEN_TOOLCHAIN/bin/$COMPILER_PREFIX- \
            --cc=$TIZEN_TOOLCHAIN/bin/$COMPILER_PREFIX-gcc > /dev/null
        make -j$NUMPROC > /dev/null

        echo Building libav for tizen $version $host $arch $mode done
        cd -
    done
    done
    done
    done
}

###########################################################
# GC build
###########################################################
cd third_party/zeromq/
rm -rf ./out

# Common flags --------------------------------------------

GCCONFFLAGS_COMMON=" --enable-static "
CFLAGS_COMMON=" -g3 -I"$PWD"/tweetnacl/contrib/randombytes -I"$PWD"/tweetnacl/src "
LDFLAGS_COMMON=

# HOST flags : linux / wearable / mobile / tv -------------

GCCONFFLAGS_wearable=
CFLAGS_wearable=" -Os "
LDFLAGS_wearable=

# ARCH flags : x86 / arm ----------------------------------
# CAUTION: these flags are NOT used when building for tizen_obs

GCCONFFLAGS_x86=
CFLAGS_x86=" -m32 "
LDFLAGS_x86=" -m32 "

GCCONFFLAGS_arm=
CFLAGS_arm=" -march=armv7-a -mthumb -finline-limit=64 "
LDFLAGS_arm=

# MODE flags : debug / release ----------------------------

GCCONFFLAGS_release=""
GCCONFFLAGS_debug=""
CFLAGS_release=' -O2 '
CFLAGS_debug=' -O0 '

# LIBTYPE flags : shared / static -------------------------

GCCONFFLAGS_static=
GCCONFFLAGS_shared=
CFLAGS_static=
CFLAGS_shared=' -fPIC'


function build_gc_for_linux() {
    for host in linux; do
    for arch in x64; do
    for mode in debug release; do
    for libtype in shared; do
        echo =========================================================================
        echo Building zeromq for $host $arch $mode $libtype

        BUILDDIR=out/$host/$arch/$mode.$libtype
        rm -rf $BUILDDIR
        mkdir -p $BUILDDIR
        cd $BUILDDIR

        GCCONFFLAGS_HOST=GCCONFFLAGS_$host CFLAGS_HOST=CFLAGS_$host LDFLAGS_HOST=LDFLAGS_$host
        GCCONFFLAGS_ARCH=GCCONFFLAGS_$arch CFLAGS_ARCH=CFLAGS_$arch LDFLAGS_ARCH=LDFLAGS_$arch
        GCCONFFLAGS_MODE=GCCONFFLAGS_$mode CFLAGS_MODE=CFLAGS_$mode LDFLAGS_MODE=LDFLAGS_$mode
        GCCONFFLAGS_LIBTYPE=GCCONFFLAGS_$libtype CFLAGS_LIBTYPE=CFLAGS_$libtype LDFLAGS_LIBTYPE=LDFLAGS_$libtype

        GCCONFFLAGS="$GCCONFFLAGS_COMMON ${!GCCONFFLAGS_HOST} ${!GCCONFFLAGS_ARCH} ${!GCCONFFLAGS_MODE} ${!GCCONFFLAGS_LIBTYPE}"
        CFLAGS="$CFLAGS_COMMON ${!CFLAGS_HOST} ${!CFLAGS_ARCH} ${!CFLAGS_MODE} ${!CFLAGS_LIBTYPE}"
        LDFLAGS="$LDFLAGS_COMMON ${!LDFLAGS_HOST} ${!LDFLAGS_ARCH} ${!LDFLAGS_MODE} ${!LDFLAGS_LIBTYPE}"

        ../../../../configure $GCCONFFLAGS CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS $CFLAGS" CXXFLAGS="$CFLAGS" > /dev/null
        make -j$NUMPROC > /dev/null

        echo Building zeromq for $host $arch $mode $libtype done
        cd -
    done
    done
    done
    done
}


function build_gc_for_tizen() {

    for version in 2.3.1 2.4 3.0; do
    for host in wearable mobile; do
    for arch in arm i386; do
    for mode in debug release; do
    for libtype in shared; do

        if [[ $arch == arm ]]; then
            device=device
        elif [[ $arch == i386 ]]; then
            device=emulator
        fi

        TIZEN_SYSROOT=$TIZEN_SDK_HOME/platforms/tizen-$version/$host/rootstraps/$host-$version-$device.core
        COMPILER_PREFIX=$arch-linux-gnueabi
        TIZEN_TOOLCHAIN=$TIZEN_SDK_HOME/tools/$COMPILER_PREFIX-gcc-$COMPILER_VERSION_MAJOR

        echo =========================================================================
        if [[ -a $TIZEN_SYSROOT ]]; then
            echo Building zeromq for tizen $version $host $arch $mode $libtype
        else
            echo Skipping zeromq build for tizen $version $host $arch $mode $libtype
            continue
        fi

        BUILDDIR=out/tizen_${version}_${host}/$arch/$mode.$libtype
        rm -rf $BUILDDIR
        mkdir -p $BUILDDIR
        cd $BUILDDIR

        GCCONFFLAGS_HOST=GCCONFFLAGS_$host CFLAGS_HOST=CFLAGS_$host LDFLAGS_HOST=LDFLAGS_$host
        GCCONFFLAGS_ARCH=GCCONFFLAGS_$arch CFLAGS_ARCH=CFLAGS_$arch LDFLAGS_ARCH=LDFLAGS_$arch
        GCCONFFLAGS_MODE=GCCONFFLAGS_$mode CFLAGS_MODE=CFLAGS_$mode LDFLAGS_MODE=LDFLAGS_$mode
        GCCONFFLAGS_LIBTYPE=GCCONFFLAGS_$libtype CFLAGS_LIBTYPE=CFLAGS_$libtype LDFLAGS_LIBTYPE=LDFLAGS_$libtype

        GCCONFFLAGS="$GCCONFFLAGS_COMMON ${!GCCONFFLAGS_HOST} ${!GCCONFFLAGS_ARCH} ${!GCCONFFLAGS_MODE} ${!GCCONFFLAGS_LIBTYPE}"
        CFLAGS="$CFLAGS_COMMON ${!CFLAGS_HOST} ${!CFLAGS_ARCH} ${!CFLAGS_MODE} ${!CFLAGS_LIBTYPE} --sysroot=$TIZEN_SYSROOT "
        LDFLAGS="$LDFLAGS_COMMON ${!LDFLAGS_HOST} ${!LDFLAGS_ARCH} ${!LDFLAGS_MODE} ${!LDFLAGS_LIBTYPE}"
        if [[ $LTO == true ]]; then
            CFLAGS+= "-flto -ffat-lto-objects"
            LDFLAGS+= "-flto -ffat-lto-objects"
            PLUGINFLAGS="--plugin=$TIZEN_TOOLCHAIN/libexec/gcc/$COMPILER_PREFIX/$COMPILER_VERSION_MINOR/liblto_plugin.so"
            TOOLCHAIN_GCC_WRAPPER=-gcc
        fi

        ../../../../configure --host=$COMPILER_PREFIX $GCCONFFLAGS CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" \
            CXXFLAGS="$CFLAGS" \
            ARFLAGS="$PLUGINFLAGS" NMFLAGS="$PLUGINFLAGS" RANLIBFLAGS="$PLUGINFLAGS" \
            CC=$TIZEN_TOOLCHAIN/bin/$COMPILER_PREFIX-gcc \
            CXX=$TIZEN_TOOLCHAIN/bin/$COMPILER_PREFIX-g++ \
            AR=$TIZEN_TOOLCHAIN/bin/${COMPILER_PREFIX}${TOOLCHAIN_GCC_WRAPPER}-ar \
            NM=$TIZEN_TOOLCHAIN/bin/${COMPILER_PREFIX}${TOOLCHAIN_GCC_WRAPPER}-nm \
            RANLIB=$TIZEN_TOOLCHAIN/bin/${COMPILER_PREFIX}${TOOLCHAIN_GCC_WRAPPER}-ranlib \
            LD=$TIZEN_TOOLCHAIN/bin/$COMPILER_PREFIX-ld > /dev/null
        make -j$NUMPROC > /dev/null

        echo Building zeromq for tizen $version $host $arch $mode $libtype done
        cd -
    done
    done
    done
    done
    done
}

function build_gc_for_tizen_obs() {

    for host in wearable; do
    for arch in $1; do
    for mode in release debug; do
    for libtype in shared; do
        echo =========================================================================
        echo Building zeromq for $host $arch $mode $libtype

        BUILDDIR=out/tizen_obs/$arch/$mode.$libtype
        rm -rf $BUILDDIR
        mkdir -p $BUILDDIR
        cd $BUILDDIR

        if [[ $2 == only_release ]]; then
            if [ $mode == debug ] || [ $libtype == static ]; then
                mkdir .libs/
                touch .libs/libgc.a
                cd -
                continue;
            fi
        fi

        GCCONFFLAGS_HOST=GCCONFFLAGS_$host CFLAGS_HOST=CFLAGS_$host LDFLAGS_HOST=LDFLAGS_$host
        GCCONFFLAGS_MODE=GCCONFFLAGS_$mode CFLAGS_MODE=CFLAGS_$mode LDFLAGS_MODE=LDFLAGS_$mode
        GCCONFFLAGS_LIBTYPE=GCCONFFLAGS_$libtype CFLAGS_LIBTYPE=CFLAGS_$libtype LDFLAGS_LIBTYPE=LDFLAGS_$libtype

        GCCONFFLAGS="$GCCONFFLAGS_COMMON ${!GCCONFFLAGS_HOST} ${!GCCONFFLAGS_MODE} ${!GCCONFFLAGS_LIBTYPE}"
        CFLAGS="$CFLAGS_COMMON ${!CFLAGS_HOST} ${!CFLAGS_MODE} ${!CFLAGS_LIBTYPE}"
        LDFLAGS="$LDFLAGS_COMMON ${!LDFLAGS_HOST} ${!LDFLAGS_MODE} ${!LDFLAGS_LIBTYPE}"
        if [[ $LTO == true ]]; then
            CFLAGS+= "-flto -ffat-lto-objects"
            LDFLAGS+= "-flto -ffat-lto-objects"
            PLUGINFLAGS="--plugin=/usr/lib/bfd-plugins/liblto_plugin.so"
        fi

        ../../../../configure $GCCONFFLAGS CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS $CFLAGS" \
            CXXFLAGS="$CFLAGS" \
            ARFLAGS="$PLUGINFLAGS" NMFLAGS="$PLUGINFLAGS" RANLIBFLAGS="$PLUGINFLAGS"
        make -j

        echo Building zeromq for $host $arch $mode $libtype done
        cd -
    done
    done
    done
    done
}

#build zeromq + tizen
if [[ $1 == tizen_obs_arm ]]; then
    build_gc_for_tizen_obs arm $2
elif [[ $1 == tizen_obs_i386 ]]; then
    build_gc_for_tizen_obs i386 $2
else # full build

#build zeromq
build_gc_for_linux

if [ -z "$TIZEN_SDK_HOME" ]; then
    echo "Do not build for Tizen"
else
    echo "TIZEN_SDK_HOME env is ...""$TIZEN_SDK_HOME"
    build_gc_for_tizen
    #cd $CU
    #mkdir -p third_party/zeromq/out/tizen_2.4_tv/i386/release.shared/.libs/
    #cp third_party/zeromq/out/tizen_2.4_mobile/i386/release.shared/.libs/*  third_party/zeromq/out/tizen_2.4_tv/i386/release.shared/.libs/
    #mkdir -p third_party/zeromq/out/tizen_2.4_tv/i386/debug.shared/.libs/
    #cp third_party/zeromq/out/tizen_2.4_mobile/i386/debug.shared/.libs/*  third_party/zeromq/out/tizen_2.4_tv/i386/debug.shared/.libs/
fi
fi


cd $CU
cd third_party/libav/
rm -rf ./out
#build_av_for_linux

if [ -z "$TIZEN_SDK_HOME" ]; then
    echo "Do not build for Tizen"
else
    echo "TIZEN_SDK_HOME env is ...""$TIZEN_SDK_HOME"
#    build_av_for_tizen
fi

cd $CU


./build_gc.sh

cd third_party/escargot/
git submodule init
git submodule update third_party/GCutil
make clean
make x64.interpreter.release.static -j$NUMPROC
make x64.interpreter.debug.static -j$NUMPROC
make install_header_to_include

cd -
