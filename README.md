# Starfish
## Abstract
Starfish is a lightweight Web browser engine for TV, mobile, headless and wearable devices.

## Supported Platforms
The following platforms are supported.

* Ubuntu 18.04, 16.04, 14.04
* Tizen
* Windows
* Android

## How to Compile: Ubuntu

### Install required packages

```sh
# Verified on Ubuntu 20.04.
sudo apt-get install clang-format libcurl4-openssl-dev libicu-dev libcairo2-dev libssl-dev libturbojpeg libturbojpeg0-dev libgif-dev cmake autoconf automake libtool ninja libwebp-dev libefl-all-dev

sudo apt-get install python-pip
pip install Jinja2

# optional for zeromq.
sudo apt-get install asciidoc xmlto
```

### Download Starfish and compile third party libraries

```sh
git clone git@github.sec.samsung.net:lws/starfish.git
cd starfish
git submodule init
git submodule update
```

### Compile Starfish

```sh
cmake CMakeLists.txt -DMODE=release -DHOST=linux -DARCH=x64 -DBACKEND=efl_cairo_gl -DTARGETNAME=Starfish -G Ninja
ninja starfish.executable
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
* -DBACKEND=[ efl_cairo | **efl_cairo_gl**  | efl_skia | dali | glfw_cairo_gl | ecore_wayland2_cairo_gl ]<br>
  Use either cairo, cairo_gl, skia, or dali as the backend graphics library
* -DARCH=[ **x64** | arm ]
  Compile Starfish for either x64 or arm target
* -DTOUCH_UI=[ 0 | **1** ]<br>
  Enable a touch UI.
* -DLTO=[ **0** | 1 ]<br>
  Enable complier link time optimization
* -DENABLE_DEBUGGER=[ **0** | 1 ]<br>
  Enable debugger
* -DTARGETNAME=[ Starfish | **lightweight-web-engine** ]<br>
  Define target output name


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
./tool/test_runner.py wpt_[css_css21|css_backgrounds|css_color|css_flexbox|css_transforms|css_variables|cssom_view|mediaqueries|selectors|others|canvas|pwa]

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

### CI Infrastructure

http://10.113.138.181/overview/444

## Outdated
All instructions in this section are outdated. They are listed here only for historical reasons.

### GYP-based Build System

#### Compile Starfish
```sh
./build_third_party.sh

GYP_GENERATORS=ninja tool/gyp/gyp build.gyp --toplevel-dir=`pwd` --depth=0 -Dcomponent=executable
ninja -C out/release starfish.x64.release
```

#### Build options
The following build options are supported when generating ninja script using gyp.
Default values are in **bold**.

* -Dcomponent=[ executable | **static_library** | shared_library ]<br>
  Compile Starfish as a executable, static library (i.e., libStarfish.a), or shared library (i.e., libStarfish.so)
* -Ddeplib=[ **shared_library** | static_library ]<br>
  Generate third-party libraries as shared libraries or obj files
* -Dbackend=[ efl_cairo | **efl_cairo_gl**  | efl_skia | dali ]<br>
  Use either cairo, cairo_gl, skia, or dali as the backend graphics library
* -Dplatform=[ **linux** | tizen ]<br>
  Compile Starfish for either Linux or Tizen platform
* -DtouchUi=[ 0 | **1** ]<br>
  Enable a touch UI.


### Makefile-based Build System

``` sh
git clone git@github.sec.samsung.net:lws/starfish.git
cd starfish
git submodule init
git submodule update
./build_third_party.sh
make [x86|x64|tizen_mobile_arm|tizen_wearable_arm].[exe|lib].[debug|release] -j
```

e.g. `make x64.exe.debug -j`

## Governance
All decisions in this project are made by consensus, respecting the principles and rules of the community.

Please refer to the [Samsung Inner Source Governance](https://github.sec.samsung.net/InnerSource/SamsungInnerSourceProgram/blob/master/GettingStarted/Governance.md) in more detail.

