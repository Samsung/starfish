# StarFish

## Building

``` sh
git clone git@10.113.64.203:StarFish/starfish.git
cd starfish
./build_third_party.sh
make [x86|x64|tizen_mobile_arm|tizen_wearable_arm].[exe|lib].[debug|release] -j
```

e.g. `make x64.exe.debug -j`

## Running

Use `./run.sh [html_file_path]` to run StarFish

## Testing

### CSSWG Test (compare with node-WebKit/previous version of StarFish)

We use the W3C's CSS conformance test suites.
(W3C CSS WG Test Suites Repository: https://hg.csswg.org/test)

You can find these in `test/reftest/csswg-test/*`

To run the pixel tests, use:

``` sh
// compare node-webkit
make pixel_test_css_all                        // all csswg test suite
make pixel_test_css*                           // specific test suite
make pixel_test [tc=unittest.html] [screen=pc] // only one file

// compare the prev ver. of StarFish
make font_dependent_test_css                   // files in tool/reftest/tclist/csswg_manual.res
```

After the pixel test, the result image files are saved in `out/x64/exe/debug/reftest/`

- `test_expected.png` - captured file of node-WebKit
- `test_result.png` - captured file of StarFish
- `test_diff.png` - diff file

If you want to capture the screenshot on the command line, use:

``` sh
// StarFish
ELM_ENGINE="shot:file=[capture.png]" ./run.sh [filepath=*.html] --pixel-test --width=800 --height=600

// node-WebKit
test/tool/nwjs-no-AA/nw tool/pixel_test/nw_capture/ -l [filepath=**.res] pc
test/tool/nwjs-no-AA/nw tool/pixel_test/nw_capture/ -f [filepath=**.html] pc
```

### Web Platform Tests

We use the [Web Platform Tests](https://github.com/w3c/web-platform-tests). The Web Platform Tests Project is a W3C-coordinated attempt to build a cross-browser testsuite for the Web-platform stack.

You can find these in `test/reftest/web-platform-tests/*`

To run the Web Platform Tests, use:

``` sh
make regression_test_wpt_*[dom | etc.]             // specific test suite
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

[Wiki](http://10.113.64.203/StarFish/starfish/wikis/home)

