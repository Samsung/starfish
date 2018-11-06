# StarFish
## Abstract
Starfish is a lightweight Web browser engine for TV, mobile and wearable devices.

## Supported Platforms
The following platforms are supported.

* Ubuntu 18.04, 16.04, 14.04
* Tizen
* Windows
* Android

## How to Compile: Ubuntu

### Install required packages

```sh
sudo apt-get install clang-format libcurl4-openssl-dev libicu-dev libcairo2-dev libssl-dev libjpeg-turbo8-dev libgif-dev cmake autoconf automake libtool
sudo apt-get install python-pip
pip install Jinja2
sudo add-apt-repository ppa:enlightenment-git/ppa
sudo apt-get update
sudo apt-get install libefl-dev

# optional for zeromq
sudo apt-get install asciidoc xmlto
```

### Download StarFish and compile third party libraries

```sh
git clone git@github.sec.samsung.net:lws/starfish.git
cd starfish
git submodule init
git submodule update
```

### Compile StarFish

```sh
cmake CMakeLists.txt -DMODE=release -DCOMPONENT=executable -DHOST=linux -DARCH=x64 -DBACKEND=efl_cairo_gl -G Ninja
ninja
```

#### Build options

The following build options are supported when generating ninja script using cmake.
Default values are in **bold**.

* -DHOST=[ **linux** | tizen ]<br>
  Compile Starfish for either Linux or Tizen platform
* -DCOMPONENT=[ **executable** | static_library | shared_library ]<br>
  Compile Starfish as a executable, static library (i.e., libStarfish.a), or shared library (i.e., libStarfish.so)
* -DMODE=[ debug | **release** ]<br>
  Compile Starfish for either release or debug mode
* -DBACKEND=[ efl_cairo | **efl_cairo_gl**  | efl_skia | dali ]<br>
  Use either cairo, cairo_gl, skia, or dali as the backend graphics library
* -DARCH=[ **x64** | arm ]
  Compile Starfish for either x64 or arm target
* -DTOUCH_UI=[ 0 | **1** ]<br>
  Enable a touch UI.


### Directory Structure
Starfish is compiled to ``out/release`` (or ``out/debug``) directory.
The structure is as follows.

```
out
  + release
    + bin/Starfish          // Starfish binary
    + lib                   // contains shared libraries that Starfish needs
```

### How to run
```sh
./out/release/Starfish 'html/file/path'
```

## How to Compile: Tizen
### GBS Build

Get ``gbs-conf``
```sh
git clone https://github.sec.samsung.net/TizenPM/gbs-conf.git
vi gbs-conf/gbs.conf
# fill out 'user' and 'passwd'
```

Build StarFish
```
cd starfish
gbs -c ../gbs-conf/gbs.conf build -A armv7l -P profile.50std  --incremental --include-all
```

The following build options are supported when building RPMs.
Default values are in **bold**.

* --define 'build_profile [ tv | mobile | wearable | **all** ]'<br>
  Genereate RPMs for TV, mobile, and wearable platforms.


## Testing
### Summary
``` sh
# Run all test at once
ninja test_all
```
``` sh
# Sub tests
# A. Dom Conformance Test (4)
ninja dom_conformance_test
ninja dom_conformance_test_[webkit|blink|gecko]

# B. Web Platfrom Test (6)
ninja web_platform_test_[dom|dom_events|html|page_visibility|progress_events|xhr]

# C. Vendor Test (9)
ninja vendor_test_[webkit|blink]_fast_[dom|html|css|etc]
ninja vendor_test_gecko_layout

# D. Bidi Test (1)
ninja bidi_test

# E. CSSWG Test (8)
ninja csswg_test_css[1|21|3_color|3_backgrounds|3_transforms|3_selectors]
ninja csswg_test_[rtl|manual]

# F. Internal Test (1)
ninja internal_test
```
``` sh
# Specify pool size for multiprocessing
ninja [test_name] TEST_NPROCS=5
```

### CSSWG Test (compare with node-WebKit/previous version of StarFish)

We use the W3C's CSS conformance test suites.
(W3C CSS WG Test Suites Repository: https://hg.csswg.org/test)

You can find these in `test/reftest/csswg-test/*`

To run the pixel tests, use:

``` sh
# (1) Compare with node-webkit
ninja csswg_test_[name]

# (2) Compare with our previous version of Startfish
ninja csswg_test_manual

# Run (1) + (2) at once
ninja csswg_test_all
```

If you want to capture the screenshot on the command line, use:

``` sh
# StarFish
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
ninja web_platform_test_[name]
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

## Misc.

### CI Infrastructure

http://10.113.138.181/overview/444

## Outdated
All instructions in this section are outdated. They are listed here only for historical reasons.

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
