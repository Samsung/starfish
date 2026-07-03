# Starfish
## Abstract
Starfish is a lightweight Web browser engine for TV, mobile, headless and wearable devices.

## Supported Platforms
The following platforms are supported.

* Ubuntu 24.04 / 22.04 (x64 native, and aarch64 / armhf / x86 cross builds)
* Tizen
* Windows
* Android

## How to Compile: Ubuntu

### Install required packages

```sh
# Verified on Ubuntu 24.04 (noble).
sudo apt-get update
sudo apt-get install -y \
    build-essential cmake ninja-build pkg-config git \
    autoconf automake libtool patchelf clang-format \
    python3 python3-jinja2 \
    libglib2.0-dev libcairo2-dev libfreetype-dev libfontconfig-dev libharfbuzz-dev \
    libx11-dev libxext-dev libxrender-dev libxi-dev \
    libegl-dev libgles-dev libgl1-mesa-dev \
    libpng-dev libturbojpeg0-dev libgif-dev libwebp-dev \
    libcurl4-openssl-dev libssl-dev libicu-dev libcap-dev libasound2-dev zlib1g-dev

# optional for zeromq.
sudo apt-get install -y asciidoc xmlto
```

> Notes for newer Ubuntu (22.04+):
> * `Jinja2` is installed via the distro package `python3-jinja2` (the old `python-pip` / `pip install Jinja2` no longer applies).
> * `libfreetype-dev` / `libfontconfig-dev` are the current names (the `*6-dev` / `*1-dev` variants are transitional).
> * `libegl-dev` / `libgles-dev` replace the old `libegl1-mesa-dev` / `libgles2-mesa-dev`.

### Download Starfish and compile third party libraries

```sh
git clone git@github.sec.samsung.net:lws/starfish.git
cd starfish
git submodule init
git submodule update
```

### Compile Starfish

```sh
cmake -Bout/release -DMODE=release -DHOST=linux -DARCH=x64 -DBACKEND=glib_cairo_gl -DSHELL=x11 -DTARGETNAME=Starfish -G Ninja
ninja -C out/release starfish.executable
```

#### Build targets

* starfish.executable
  Build Starfish as an executable
```sh
ninja starfish.executable
```
* starfish.shared_library
  Build Starfish as a shared library (i.e., liblightweight-web-engine.so)
```sh
ninja starfish.shared_library
```
* starfish.static_library
  Build Starfish as a static library (i.e., liblightweight-web-engine.a)
```sh
ninja starfish.static_library
```

#### Build options

The following build options are supported when generating ninja script using cmake.
Default values are in **bold**.

* -DHOST=[ **linux** | tizen ]<br>
  Compile Starfish for either Linux or Tizen platform
* -DMODE=[ debug | **release** ]<br>
  Compile Starfish for either release or debug mode
* -DBACKEND=[ **glib_cairo_gl** | uv_cairo_gl ]<br>
  Use either cairo or cairo_gl as the backend graphics library
* -DARCH=[ **x64** | aarch64 | arm | x86 ]<br>
  Target architecture. `x64` is native; `aarch64` / `arm` (armhf) / `x86` (i386) are
  cross targets (see "How to Cross-Compile: Linux").
* -DLTO=[ **0** | 1 ]<br>
  Enable complier link time optimization
* -DENABLE_DEBUGGER=[ **0** | 1 ]<br>
  Enable debugger
* -DTARGETNAME=[ Starfish | **lightweight-web-engine** ]<br>
  Define target output name
* -DCOVERAGE=[ **0** | 1 ]<br>
  Enable coverage measurements with gcov
* -DSHELL=[ **x11** | glib_headless ]<br>
  Create an executable build target.

### Directory Structure
Starfish is compiled to ``out/release`` (or ``out/debug``) directory.
The structure is as follows.

```
out
  + release
    + bin/lightweight-web-engine    // Starfish binary
    + lib                           // contains shared libraries that Starfish needs
```

### How to run
```sh
./out/release/lightweight-web-engine 'html/file/path'
```

## How to Cross-Compile: Linux (aarch64 / armhf / x86)

Cross builds target other Linux architectures (e.g. Raspberry Pi 5 = aarch64) from
an x64 host. The key requirement is that **the cross toolchain's glibc must be the
same or newer than the target's glibc** — otherwise linking against a target sysroot
fails with errors like `undefined reference to '...@GLIBC_2.3x'`. Ubuntu 24.04 (noble)
ships cross gcc-13 with glibc 2.39, which matches a noble (RPi5) sysroot.

### Option A: Docker (recommended)

A ready-to-use image bakes in all the cross toolchains and target sysroots under
`/opt/sysroot/{aarch64,arm,x86}`. See [`Dockerfile.starfish`](Dockerfile.starfish),
[`mk-sysroot.sh`](mk-sysroot.sh) and [`build_starfish_docker.sh`](build_starfish_docker.sh).

```sh
# Build the image once (or pull the pre-built one)
DOCKER_BUILDKIT=1 docker build -f Dockerfile.starfish -t starfish-cross-build:24.04 .

# Build for a target (writes to build/out_rpi5, build/out_linux_arm, build/out_linux_x86)
./build_starfish_docker.sh aarch64        # RPi5
./build_starfish_docker.sh arm32
./build_starfish_docker.sh x86
./build_starfish_docker.sh all            # native + all three
```

### Option B: Manual (host toolchain + sysroot)

1. Install the cross toolchain and create a target sysroot (multiarch dev libs):

```sh
# aarch64 example
sudo apt-get install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
# A sysroot can be built with mk-sysroot.sh, debootstrap, or copied from the device.
# It must contain the target dev libraries listed in the "Install required packages"
# section above (cairo, glib, egl/glesv2, x11, turbojpeg, curl, cap, ...).
```

2. Point the build at the sysroot via env vars + CMake (`SYSROOT` = path to the sysroot):

```sh
export SYSROOT=/opt/sysroot/aarch64
export T=aarch64-linux-gnu                       # arm-linux-gnueabihf | i386-linux-gnu
export CC=$T-gcc CXX=$T-g++ AR=$T-ar RANLIB=$T-ranlib STRIP=$T-strip
export CFLAGS="--sysroot=$SYSROOT -I$SYSROOT/usr/lib/$T/glib-2.0/include -I$SYSROOT/usr/include/$T"
export CXXFLAGS="$CFLAGS"
export LDFLAGS="--sysroot=$SYSROOT"
export PKG_CONFIG_LIBDIR="$SYSROOT/usr/lib/$T/pkgconfig:$SYSROOT/usr/share/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR="$SYSROOT"

cmake CMakeLists.txt -G Ninja -Bout/rpi5 -DTARGETNAME=Starfish \
  -DMODE=release -DHOST=linux -DARCH=aarch64 -DBACKEND=glib_cairo_gl -DSHELL=x11 -DWEBGL=0 \
  -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=$T-gcc -DCMAKE_CXX_COMPILER=$T-g++ \
  -DCMAKE_SYSROOT=$SYSROOT -DCMAKE_FIND_ROOT_PATH=$SYSROOT \
  -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
  -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
  -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY
ninja -C out/rpi5 starfish.executable
```

`ARCH`/`CMAKE_SYSTEM_PROCESSOR` values per target: `aarch64`, `arm` (armhf, also add `-msse2`-free
default flags), `x86` (i386, compiler `i686-linux-gnu-gcc`, add `-msse2`). `cmake.sh` contains the
canonical per-target env blocks.

## How to Compile: Tizen
### GBS Build

Get ``gbs-conf``
```sh
git clone https://github.sec.samsung.net/TizenPM/gbs-conf.git
vi gbs-conf/gbs.conf
# fill out 'user' and 'passwd'
```

Build Starfish
```
cd starfish
gbs -c ../gbs-conf/gbs.conf build -A armv7l -P profile.50std  --incremental --include-all
```

The following build options are supported when building RPMs.
Default values are in **bold**.

* --define 'build_profile [ tv | mobile | headless | wearable | **all** ]'<br>
  Genereate RPMs for TV, mobile, headless and wearable platforms.

### How to Compile: Windows x86
Open Visual Studio x86 Command tools prompt
```sh
cmake -G "Visual Studio 16 2019" -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_SYSTEM_VERSION:STRING="10.0" -DCMAKE_SYSTEM_PROCESSOR=x86 -DCMAKE_GENERATOR_PLATFORM=Win32,version=10.0.18362.0 -DARCH=x86 -DMODE=release -Bout_windows/ -DHOST=windows
cmake --build out_windows --config Release -j
msbuild build/windows/winform_shell/StarfishWinformShell/StarfishWinformShell.sln /p:Platform="Any CPU"
```

## How to Compile: Android
### Prerequisite
```
export ANDROID_HOME=$HOME/Your/Android/Sdk
```
android-ndk-r16b

### Compile LWE
```
cd build/android/apk
gradle build
```

## Testing
### Prerequisite
```sh
# install imgdiff tool
ninja install_pixel_test_dep
```
### Summary
``` sh
# Run all test at once
./tool/test_runner.py
```
``` sh
# Sub tests
# A. Dom Conformance Test
./tool/test_runner.py dom_conformance

# B. Web Platfrom Test
./tool/test_runner.py wpt_all or
./tool/test_runner.py wpt_[css_css21|css_backgrounds|css_color|css_flexbox|css_transforms|css_variables|mediaqueries|selectors]

# C. Vendor Test
./tool/test_runner.py vendor_test or vendor_test_[blink|webkit|gecko]

# D. Bidi Test
./tool/test_runner.py bidi_test

# E. Internal Test
./tool/test_runner.py internal_test
```

If you want to capture the screenshot on the command line, use:

``` sh
# Starfish
ELM_ENGINE="shot:file=[capture.png]" ./run.sh [filepath=*.html] --pixel-test --width=800 --height=600

# node-WebKit
test/tool/nwjs-no-AA/nw tool/pixel_test/nw_capture/ -l [filepath=**.res] pc
test/tool/nwjs-no-AA/nw tool/pixel_test/nw_capture/ -f [filepath=**.html] pc
```

### Web Platform Tests

We use the [Web Platform Tests](https://github.com/w3c/web-platform-tests). The Web Platform Tests Project is a W3C-coordinated attempt to build a cross-browser testsuite for the Web-platform stack.

You can find these in `test/reftest/web-platform-tests/*`

To run the Web Platform Tests, use:

``` sh
./tool/test_runner.py wpt_[name]
```

### Bidi Tests
Bidi tests perform pixel tests on a device. To run the tests,
- Connect your device
- run the following

```sh
ninja regression_test_bidi.tizen_wearable_arm.debug
sdb shell
cd /home/developer
./bidi_test_run.sh
./bidi_test_clean.sh
```

### JS Debugging
If you enable debugger feature when build,
You can debug JS with escargot vscode extension.
See: [escargot-vscode-extension](https://github.com/Samsung/escargot-vscode-extension)

## Misc.
