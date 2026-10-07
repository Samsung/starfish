# Starfish
## Abstract
Starfish is a lightweight Web browser engine for TV, common and headless devices.

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
    libpng-dev libturbojpeg0-dev libjpeg-dev libgif-dev libwebp-dev \
    libcurl4-openssl-dev libssl-dev libicu-dev libcap-dev libasound2-dev zlib1g-dev

# optional for zeromq.
sudo apt-get install -y asciidoc xmlto
```

> Notes for newer Ubuntu (22.04+):
> * `Jinja2` is installed via the distro package `python3-jinja2` (the old `python-pip` / `pip install Jinja2` no longer applies).
> * `libfreetype-dev` / `libfontconfig-dev` are the current names (the `*6-dev` / `*1-dev` variants are transitional).
> * `libegl-dev` / `libgles-dev` replace the old `libegl1-mesa-dev` / `libgles2-mesa-dev`.
> * For `-DUSE_FFMPEG_MEDIA_PLAYER=1`, also install `libavcodec-dev`,
>   `libavformat-dev`, `libavutil-dev`, and `libswresample-dev`.

### Download Starfish and compile third party libraries

```sh
git clone git@github.sec.samsung.net:lws/starfish.git
cd starfish
git submodule init
git submodule update
```

### Compile Starfish

```sh
cmake -Bout/release -DCMAKE_BUILD_TYPE=Release -DBACKEND=glib_cairo_gl -DSHELL=x11 -DTARGETNAME=Starfish -G Ninja
ninja -C out/release starfish.executable
```

> Note: JS bindings for spec-defined interfaces are generated from `src/**/*.idl`
> at cmake configure time (see `build/binding.cmake`). After adding, editing or
> deleting any `.idl`, re-run the cmake command above — an incremental `ninja`
> alone will not regenerate the bindings.

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

* -DCMAKE_SYSTEM_NAME=[ **(native)** | Tizen | Windows ]<br>
  Compile Starfish for either Linux (leave unset, CMake auto-detects it natively),
  Tizen, or Windows platform
* -DCMAKE_BUILD_TYPE=[ Debug | **Release** ]<br>
  Compile Starfish for either release or debug mode
* -DBACKEND=[ **glib_cairo_gl** | uv_cairo_gl ]<br>
  Use either cairo or cairo_gl as the backend graphics library
* -DCMAKE_SYSTEM_PROCESSOR=[ **x86_64** | aarch64 | arm | x86 ]<br>
  Target architecture. Native `x86_64` needs no flag; `aarch64` / `arm` (armhf) / `x86` (i386) are
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
* -DCLI=[ **0** | 1 ]<br>
  Build the CLI on Linux. Requires a shell executable and
  -DSTARFISH_ENABLE_CDP=1.
* -DWEBAUDIO=[ **0** | 1 ]<br>
  Enable Web Audio on processors other than x86_64, where it is always on.
  On Tizen it needs capi-media-audio-io, capi-media-sound-manager and the
  platform FFmpeg (libavcodec, libavformat, libavutil, libswresample) for
  compressed `decodeAudioData()`.
* -DUSE_FFMPEG_MEDIA_PLAYER=[ **0** | 1 ]<br>
  Use the common FFmpeg software media player. The default is `1` on Windows
  and `0` on Linux. Linux also enables Web Audio compressed `decodeAudioData()`
  and needs the FFmpeg development packages listed above. Windows installs
  the pinned LGPL shared prebuilt package through vcpkg.

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
  -DCMAKE_BUILD_TYPE=Release -DBACKEND=glib_cairo_gl -DSHELL=x11 -DWEBGL=0 \
  -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=$T-gcc -DCMAKE_CXX_COMPILER=$T-g++ \
  -DCMAKE_SYSROOT=$SYSROOT -DCMAKE_FIND_ROOT_PATH=$SYSROOT \
  -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
  -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
  -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY
ninja -C out/rpi5 starfish.executable
```

`CMAKE_SYSTEM_PROCESSOR` values per target: `aarch64`, `arm` (armhf, also add `-msse2`-free
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

* --define 'build_profile [ tv | common | headless | **all** ]'<br>
  Genereate RPMs for TV, common and headless platforms.
* --define 'enable_webaudio [ **0** | 1 ]'<br>
  Build with Web Audio (`-DWEBAUDIO=1`).

### How to Compile: Windows x86/x64

Windows supports Intel x86 and x64 only. ARM/ARM64 is intentionally rejected.

On a Windows 10/11 host, install the matching MSVC Build Tools and Windows SDK,
CMake 3.18 or newer, Ninja, Python 3 with Jinja2 and ply, and vcpkg. Run the
appropriate Visual Studio Native Tools Command Prompt first: CMake coordinates
the build, while MSVC supplies the compiler and Windows SDK.

vcpkg works like a native package manager in manifest mode. The first CMake
configure installs the dependencies in `vcpkg.json` at the registry baseline
pinned by `vcpkg-configuration.json`. That same file registers
`vcpkg/ports-public` as an overlay, so the repository's patched ports (cairo,
libwebsockets) are picked up by every build -- native or Docker -- without a
command line flag. FFmpeg is the prebuilt exception: initialize its pinned
submodule before configuring; the overlay installs the selected x86/x64
binaries and generates x64 MSVC import libraries with `lib.exe`. It does not
compile FFmpeg or run GNU build tools.

```bat
git submodule update --init third_party/windows/ffmpeg
```

```bat
git clone https://github.com/microsoft/vcpkg C:\src\vcpkg
C:\src\vcpkg\bootstrap-vcpkg.bat
set VCPKG_ROOT=C:\src\vcpkg
```

For x86, use an x86 Native Tools prompt:

```bat
cmake -S . -B build\windows-x86 -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_PROCESSOR=x86 ^
  -DSTARFISH_WINDOWS_ENABLE_MULTIMEDIA=ON ^
  -DSTARFISH_WINDOWS_BUILD_SHELL=ON ^
  -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
  -DVCPKG_TARGET_TRIPLET=x86-windows
cmake --build build\windows-x86 --target starfish.windows_shell --parallel
```

For x64, use an x64 Native Tools prompt:

```bat
cmake -S . -B build\windows-x64 -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_PROCESSOR=AMD64 ^
  -DSTARFISH_WINDOWS_ENABLE_MULTIMEDIA=ON ^
  -DSTARFISH_WINDOWS_BUILD_SHELL=ON ^
  -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
  -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build\windows-x64 --target starfish.windows_shell --parallel
```

Building `starfish.windows_shell` also builds `Starfish.dll`. Run
`Release\StarfishShell.exe [URL-or-HTML-file]`; with no argument it opens a
built-in smoke page. The pure Win32 shell uses no .NET, WinForms, or MSBuild,
and no Windows-specific bridge inside the engine: like the other ports it drives
`LWE::WebContainer` through the public `inc/LWEWebView.h` API only. Its UI
thread owns the native window, WGL presentation, input, and IME; the engine runs
on the LWE thread that `LWE::LWE::Initialize` starts inside the DLL
(`InitializeOption::PreferSeparateThread`), and the embedding API marshals every
call there. Set
`STARFISH_WINDOWS_BUILD_SHELL=OFF` (the default) when only the DLL is needed.

Multimedia is on by default for both x86 and x64. MP4Parser and WebM are emitted
as `mp4parse.dll` and `webm.dll`; set
`STARFISH_WINDOWS_ENABLE_MULTIMEDIA=OFF` for an engine-only build. Escargot,
gc-lib, Clipper, skia_matrix, and libtuv are also separate DLLs. vcpkg uses
dynamic triplets and CMake copies their runtime DLLs and Fontconfig
configuration beside `Starfish.dll`. libtuv (`tuv.dll`) supplies the engine
idler/timer loop and is built from `third_party/libtuv` by this repository's own
CMake target, the same way skia_matrix is. The official
PThreads4W port is built from source with the
active MSVC toolchain and deployed as `pthreadVC3.dll`; the vcpkg path does
not use the checked-in VC2010-era `pthreadVC2.dll`. A target Windows system
must provide the Visual C++ runtime and the Windows system `icu.dll` API.

The x86/x64 LGPL shared FFmpeg prebuilt packages are pinned separately in
`third_party/windows/ffmpeg`, on the external submodule branch
`modules/third_party/windows/ffmpeg`. It retains the five media libraries,
development files, license, source archives, and archive checksums, so its
availability does not depend on upstream nightly release retention.
`USE_FFMPEG_MEDIA_PLAYER=1` and `STARFISH_WINDOWS_ENABLE_MULTIMEDIA=ON` select
`MediaPlayerFFmpeg` by default. The five runtime DLLs are deployed next to
`Starfish.dll`, with license and package provenance in `licenses/ffmpeg`;
Windows audio uses shared-mode WASAPI with interleaved S16 PCM,
while Linux uses PulseAudio. An unavailable audio endpoint leaves decoding and
video playback usable. Windows Web Audio remains gated separately.
`-DUSE_FFMPEG_MEDIA_PLAYER=0` selects the mock Windows player;
`-DSTARFISH_WINDOWS_ENABLE_MULTIMEDIA=OFF` removes the multimedia surface.
The prebuilt dependency remains part of the vcpkg manifest in both cases.

With the Windows shell enabled, verify the libraries and audio cancellation:

```bat
cmake --build build\windows-x64 --target starfish.windows_ffmpeg_smoke starfish.windows_mp4_fragment_smoke --parallel
build\windows-x64\Release\MP4FragmentSmoke.exe
build\windows-x64\Release\FFmpegSmoke.exe --audio
python tool\windows\test_progressive_media.py --browser build\windows-x64\Release\StarfishShell.exe
python tool\windows\test_media_rendering.py --browser build\windows-x64\Release\StarfishShell.exe
```

Use `build\windows-x86` for the x86 build. The library probe tests PCM decoding,
resampling and RGBA conversion; `--audio` also checks cancellation and reset
with the default endpoint, reporting explicitly when no endpoint exists.
The browser regression checks audio-only WAV, software H.264/AAC video,
playback timing, pause, paused seek and ended. It uses local fixtures and a
local HTTP server, and also runs on Linux under the usual `xvfb-run` wrapper.
The rendering regression checks progressive H.264 and MSE H.264/AV1, including
actual Windows framebuffer colors. Screenshot mode uses an offscreen ANGLE
buffer, so these checks also run from SSH or CI without an interactive desktop.

To check a YouTube iframe with sound in an interactive Windows desktop:

```bat
python tool\windows\run_youtube_iframe.py --browser build\windows-x64\Release\StarfishShell.exe
```

Use `--video VIDEO_ID` to select another video. The helper serves the iframe
over local HTTP and bypasses the corporate proxy only for loopback addresses;
YouTube requests continue to use the configured proxy. Its status reports
playing time, mute and volume. `STARFISH_FFMPEG_TRACE=1` logs decoder, audio
endpoint and texture initialization for diagnosis.

For repeated builds, enable a vcpkg binary cache, for example:

```bat
set "VCPKG_BINARY_SOURCES=clear;files,C:\vcpkg-cache,readwrite"
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

Wrap every test run in `xvfb-run -s '-screen 0 1920x1080x24' -a`, even on a
machine with a live desktop session: the suites launch Starfish instances
8-way in parallel, which spikes load on a real X server, and the fixed
virtual screen keeps pixel/reftest comparisons reproducible.

### Summary
``` sh
# Run all test at once
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py
```
``` sh
# Sub tests
# A. Dom Conformance Test
./tool/runner/test_runner.py dom_conformance

# B. Web Platfrom Test
./tool/runner/test_runner.py wpt_all or
./tool/runner/test_runner.py wpt_[css_css21|css_backgrounds|css_color|css_flexbox|css_transforms|css_variables|mediaqueries|selectors]

# C. Vendor Test
./tool/runner/test_runner.py vendor_test or vendor_test_[blink|webkit|gecko]

# C-1. Khronos WebGL conformance (needs a -DWEBGL=1 build; see docs/khronos_webgl.md)
./tool/runner/test_runner.py khronos_test or vendor_test_[khronos|khronos2|khronossdk]

# D. Bidi Test
./tool/runner/test_runner.py bidi_test

# E. Internal Test
./tool/runner/test_runner.py internal_test
# The direct internal-test driver accepts STARFISH_BIN to choose a build
# without changing the repository's ./Starfish symlink.
STARFISH_BIN="$PWD/out/release/bin/Starfish" xvfb-run -s '-screen 0 1920x1080x24' -a \
  python3 tool/drivers/run_test.py basic tool/reftest/cairo/internal.res common -p8

# Web Audio (x86_64 builds enable STARFISH_ENABLE_WEBAUDIO by default).
# CI (.github/workflows/x64_test.yml, x64_test_clang.yml) runs:
#  - webaudio_*.res WPT lists, inside wpt_serve_testharness (2 jobs, 60 s
#    per test, same wpt serve session as the other lists);
#  - tool/webaudio/test_media_output.py (skips without pulseaudio/parec);
#  - tool/webaudio/test_graph_thread.py in the clang build job (skipped with
#    a warning when Clang's TSan runtime is missing).
# Everything needing FFmpeg (compressed decoding, MSE, decoder Valgrind)
# is manual: CI builds without USE_FFMPEG_MEDIA_PLAYER.
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py wpt_serve_testharness_webaudio

# The FFmpeg-only checks below use a second build directory:
cmake -Bout/ffmpeg -DCMAKE_BUILD_TYPE=Release -DBACKEND=glib_cairo_gl -DSHELL=x11 \
  -DTARGETNAME=Starfish -DUSE_FFMPEG_MEDIA_PLAYER=1 -G Ninja
ninja -C out/ffmpeg starfish.executable

# Compressed-audio decodeAudioData WPT (manual).
STARFISH_BIN="$PWD/out/ffmpeg/bin/Starfish" xvfb-run -s '-screen 0 1920x1080x24' -a \
  ./tool/runner/test_runner.py wpt_serve_testharness_webaudio_ffmpeg

# On Linux, AudioContext uses libpulse-simple.so.0 at runtime when available;
# otherwise it renders silently. No PulseAudio development package is needed.
# Each real-time AudioContext or playing <audio> element opens its own
# PulseAudio client; at most 8 are open at once (further ones render
# silently against the wall clock).
# To check audible output manually, open
# test/cairo/internal-test/webaudio/manual-pulse-output.html and press Play.
# OfflineAudioContext returns rendered PCM without an output device.

# Linux media PCM regression (CI): needs pulseaudio, pactl, and parec.
# Starts an isolated null sink; no sound is sent to physical speakers.
# Checks loop playback rates and real PCM/automation while JavaScript blocks.
# Also repeats context creation, graph mutation, suspend/resume, and close.
STARFISH_BIN="$PWD/out/release/bin/Starfish" xvfb-run -s '-screen 0 1920x1080x24' -a \
  python3 tool/webaudio/test_media_output.py

# Linux MSE -> Web Audio PCM regression (manual): requires the FFmpeg build
# above and the ffmpeg CLI (generates an AAC fixture). No audio device is needed.
STARFISH_BIN="$PWD/out/ffmpeg/bin/Starfish" xvfb-run -s '-screen 0 1920x1080x24' -a \
  python3 tool/webaudio/test_mse_source.py

# Valid compressed WAVE fallback (manual): requires the FFmpeg build and
# ffmpeg CLI.
STARFISH_BIN="$PWD/out/ffmpeg/bin/Starfish" xvfb-run -s '-screen 0 1920x1080x24' -a \
  python3 tool/webaudio/test_compressed_wave.py

# Native decoder cleanup and local-file rejection (manual): requires the
# FFmpeg build directory above, FFmpeg development libraries/CLI, Valgrind
# and a C++ compiler. Compiles only decoder sources into a temporary directory.
xvfb-run -s '-screen 0 1920x1080x24' -a \
  python3 tool/webaudio/test_decoder_memory.py --build-dir out/ffmpeg

# Native graph/connection-queue ThreadSanitizer stress test (CI, clang job):
# requires Clang 18 with its TSan runtime and an existing configured Ninja
# build. Compiles into a temporary directory; does not replace the browser
# build's objects. Covers AudioGraph, AudioBus and AudioParamTimeline with a
# silent device, not DOM/GC, PulseAudio or all DSP handlers. Override
# --compiler if needed.
xvfb-run -s '-screen 0 1920x1080x24' -a \
  python3 tool/webaudio/test_graph_thread.py --build-dir out/release

# F. CDP Test
./tool/cdp_test/run.py all --worker

# G. LWE CLI Test
python3 tool/cli_test/run_cli_test.py
```

If you want to capture the screenshot on the command line, use:

``` sh
# Starfish
ELM_ENGINE="shot:file=[capture.png]" ./run.sh [filepath=*.html] --pixel-test --width=800 --height=600

# node-WebKit
test/tool/nwjs-no-AA/nw tool/pixel_test/nw_capture/ -l [filepath=**.res] pc
test/tool/nwjs-no-AA/nw tool/pixel_test/nw_capture/ -f [filepath=**.html] pc
```

### Khronos WebGL Conformance Tests

The [Khronos WebGL conformance suites](https://github.com/KhronosGroup/WebGL)
(1.0.3, 2.0.0 and the development `sdk/tests`) are pinned as the
`third_party/webgl` submodule and served unmodified; `./tool/runner/test_runner.py
khronos_test` runs the curated lists in `tool/reftest/cairo/khronos_webgl*.res`
against a `-DWEBGL=1` build. See `docs/khronos_webgl.md` for how results are
collected and how to move the pin.

### Web Platform Tests

We use the [Web Platform Tests](https://github.com/w3c/web-platform-tests). The Web Platform Tests Project is a W3C-coordinated attempt to build a cross-browser testsuite for the Web-platform stack.

You can find these in `test/reftest/web-platform-tests/*`

To run the Web Platform Tests, use:

``` sh
./tool/runner/test_runner.py wpt_[name]
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
