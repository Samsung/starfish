# StarFish
## Abstract
Starfish is a lightweight Web browser engine for mobile and wearable devices.

## How to Compile

### Install required packages

```sh
sudo add-apt-repository ppa:enlightenment-git/ppa
sudo apt-get update
sudo apt-get install libefl-dev
sudo apt-get install clang-format libcurl4-openssl-dev libicu-dev libcairo2-dev libssl-dev libjpeg-turbo8-dev libgif-dev
sudo apt-get install python-pip
pip install Jinja2
```

### Download StarFish and compile third party libraries

```sh
git clone git@github.sec.samsung.net:RS7-webtf/starfish.git
cd starfish
git submodule init
git submodule update
./build_third_party.sh
```

### Compile StarFish

```sh
GYP_GENERATORS=ninja tool/gyp/gyp build.gyp --toplevel-dir=`pwd` --depth=0 -Dcomponent=executable
ninja -C out/debug starfish.x64.debug
```

#### Build options

The following build options are supported when generating ninja script using gyp.
Default values are in **bold**.

* -Dcomponent=[ executable | **static_library** | shared_library ]<br>
  Compile Starfish as a executable, static library (i.e., libStarfish.a), or shared library (i.e., libStarfish.so)
* -Ddeplib=[ **shared_library** | static_library ]<br>
  Generate third-party libraries as shared libraries or obj files
* -Dbackend=[ efl | dali | **efl_cairo** ]<br>
  Use either efl, dali, or efl_cairo as the backend graphics library
* -Dplatform=[ **linux** | tizen ]<br>
  Compile Starfish for either linux or tizen platform

The following build targets are available when running the ninja script.

```sh
ninja -C out/[ debug | release ] target
```

where target is either:

* ``starfish.x64.debug``
* ``starfish.x64.release``
* ``starfish.tizen.release``

### GBS Build

Get ``gbs-conf``
```sh
git clone https://github.sec.samsung.net/RS7-TizenPM/gbs-conf
vi gbs-conf/gbs.conf
# Uncomment the profile to use in [general] section, and
# fill out 'user' and 'passwd'
```

Build required packages
```sh
git clone git@github.sec.samsung.net:RS7-webtf/escargot.git
cd escargot
git submodule init
git submodule update
make install_header_to_include
gbs -c ../gbs-conf/gbs.conf build --define 'jobs 16' -A armv7l -P [ profile.40arm | profile.VdKantM ] --incremental --include-all
```

Build StarFish
```
cd starfish
./binding_generator/scripts/starfish_code_generator.py src/ src/binding/
gbs -c ../gbs-conf/gbs.conf build -A armv7l -P [ profile.40arm | profile.VdKantM ] --incremental --include-all
```

The following build options are supported when building RPMs.
Default values are in **bold**.

* --define 'tizen_profile_name [ **tv** | headless | mobile | wearable ]'<br>
  Genereate RPMs for either tv, iot, or mobile platform

### Directory Structure
Starfish is compiled to ``out/debug`` (or ``out/release``) directory.
The structure is as follows.

```
out
  + debug
    + Starfish.x64.debug // Starfish binary
    + lib                // contains shared libraries that Starfish needs
```

## How to run
```sh
./out/debug/Starfish.x64.debug html_file_path
```

~~Use `./run.sh [html_file_path]` to run StarFish~~


## Testing
### Summary
``` sh
# Run all test at once
make test_all
```
``` sh
# Sub tests
# A. Dom Conformance Test (4)
make dom_conformance_test
make dom_conformance_test_[webkit|blink|gecko]

# B. Web Platfrom Test (6)
make web_platform_test_[dom|dom_events|html|page_visibility|progress_events|xhr]

# C. Vendor Test (9)
make vendor_test_[webkit|blink]_fast_[dom|html|css|etc]
make vendor_test_gecko_layout

# D. Bidi Test (1)
make bidi_test

# E. CSSWG Test (8)
make csswg_test_css[1|21|3_color|3_backgrounds|3_transforms|3_selectors]
make csswg_test_[rtl|manual]

# F. Internal Test (1)
make internal_test
```
``` sh
# Specify pool size for multiprocessing
make [test_name] TEST_NPROCS=5
```

### CSSWG Test (compare with node-WebKit/previous version of StarFish)

We use the W3C's CSS conformance test suites.
(W3C CSS WG Test Suites Repository: https://hg.csswg.org/test)

You can find these in `test/reftest/csswg-test/*`

To run the pixel tests, use:

``` sh
# (1) Compare with node-webkit
make csswg_test_[name]

# (2) Compare with our previous version of Startfish
make csswg_test_manual

# Run (1) + (2) at once
make csswg_test_all
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
make web_platform_test_[name]
```

### Bidi Tests
Bidi tests perform pixel tests on a device. To run the tests,
- Connect your device
- run the following

```sh
make regression_test_bidi.tizen_wearable_arm.debug
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
git clone git@github.sec.samsung.net:RS7-webtf/starfish.git
cd starfish
git submodule init
git submodule update
./build_third_party.sh
make [x86|x64|tizen_mobile_arm|tizen_wearable_arm].[exe|lib].[debug|release] -j
```

e.g. `make x64.exe.debug -j`
