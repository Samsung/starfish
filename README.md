# StarFish

## Building

``` sh
git clone git@10.113.64.203:StarFish/starfish.git
cd starfish
./build_third_party.sh
make [x86|x64|tizen_arm|tizen_wearable_arm].[exe|lib].[debug|release] -j
```

e.g. `make x64.exe.debug -j`

## Running

Use `./run.sh [html_file_path]` to run StarFish

## Testing

### Regression Test (compare with pre-version)

To run the regression tests, use:

``` sh
make regression_test                         // all html file within test/demo/20160115
make regression_test [tc=unittest.html]      // only one file
```

### Pixel Test (compare with WebKit)

We use the W3C's CSS conformance test suites.
(W3C CSS WG Test Suites Repository: https://hg.csswg.org/test)

You can find these in `test/reftest/csswg-test/*`

To run the pixel tests, use:

``` sh
make pixel_test                         // all html file within test/
make pixel_test_css*                    // specific test suite
make pixel_test [tc=unittest.html]      // only one file
```

After the pixel test, the result image files are saved in `out/x64/exe/debug/reftest/`

- `test_expected.png` - captured file of WebKit
- `test_result.png` - captured file of StarFish
- `test_diff.png` - diff file

If you want to capture the screenshot on the command line, use:

``` sh
// StarFish
ELM_ENGINE="shot:file=[capture.png]" ./run.sh [filepath=*.html] --pixel-test

// WebKit
phantomjs capture.js [category=css|dom|xhr]
phantomjs capture.js -f [filepath=*.html]
```

### Web Platform Tests

We use the [Web Platform Tests](https://github.com/w3c/web-platform-tests). The Web Platform Tests Project is a W3C-coordinated attempt to build a cross-browser testsuite for the Web-platform stack.

You can find these in `test/reftest/web-platform-tests/*`

To run the Web Platform Tests, use:

``` sh
make wpt_test_*[dom | etc.]             // specific test suite
make wpt_test [tc=xxx]                  // specific directory or file
```

[Wiki](http://10.113.64.203/StarFish/starfish/wikis/home)

