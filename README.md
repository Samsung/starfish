# StarFish

## Building

``` sh
git clone git@10.113.64.74:StarFish/starfish2.git
cd starfish
./build_third_party.sh
make [x86|x64|tizen_mobile_arm|tizen_wearable_arm].[exe|lib].[debug|release] -j
```

e.g. `make x64.exe.debug -j`

## Running

Use `./run.sh [html_file_path]` to run StarFish

## Testing
#### Summary
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

#### CSSWG Test (compare with node-WebKit/previous version of StarFish)

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

#### Web Platform Tests

We use the [Web Platform Tests](https://github.com/w3c/web-platform-tests). The Web Platform Tests Project is a W3C-coordinated attempt to build a cross-browser testsuite for the Web-platform stack.

You can find these in `test/reftest/web-platform-tests/*`

To run the Web Platform Tests, use:

``` sh
make web_platform_test_[name]
```

#### Bidi Tests
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

[Wiki](http://10.113.64.203/StarFish/starfish/wikis/home)

