# Blink web tests

Starfish runs Chromium's Blink web tests the way it runs the WebKit
LayoutTests (`docs/webkit_layouttests.md`): unmodified, out of a
version-pinned submodule, with a bridge standing in for the vendor's test
shell. They replace the copies the legacy `blink_*` lists kept with
Starfish-specific expectations.

The tests come from the `third_party/blink` submodule (chromium/chromium) and
every test, `-expected.txt` and reference page is exactly what upstream ships,
so a pin bump is a submodule update plus a re-measure. The Chromium tree is
far too large to check out, so the submodule is a sparse checkout of the web
test directories the legacy lists drew from, made by `tool/blink/fetch.sh`;
`.gitmodules` sets `update = none` so `git submodule update` never checks out
the whole tree. `third_party/blink/web_tests/` is the server root, so each
line of the list, `tool/reftest/cairo/blink.res`, is the full URL of a test.

| Upstream directory (under `web_tests/`) | What it is |
|---|---|
| `fast/*` (the directories named in `tool/blink/fetch.sh`) | DOM, CSS, layout, canvas and parser tests |
| `css3/flexbox` | Flexbox tests |
| `svg` | SVG tests |
| `http/tests/inspector-protocol` | Chromium's CDP tests. The CDP suite runs them (`tool/cdp_test`), not `blink_test` |

The list holds every test in those directories, apart from the CDP tests, that
Starfish can judge:

| Kind | How it is recognized | Passes when |
|---|---|---|
| js-test | loads `resources/js-test.js` and has a text `-expected.txt` (not a render tree dump) | the dump carries the `-expected.txt` result words in the same order |
| testharness | loads `testharness.js` | the same, where a test without `-expected.txt` must pass every subtest |
| reftest | has an `-expected` or `-expected-mismatch` page (`.html`, `.htm`, `.xhtml`, `.xht`, `.svg`) | its rendering matches each `-expected` page and differs from each `-expected-mismatch` one |

Pixel tests (an `-expected.png` of Chrome's rendering only), render tree
dumps, `-manual` tests and pages that call `testRunner` without a harness are
not run.

## Running

```sh
./tool/blink/fetch.sh      # once, and after every pin bump
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py blink_test
# include the tracked known-failures (`#` lines) to look for newly passing tests
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py -f blink_test
```

The reftests need `tool/imgdiff/imgdiff` (`ninja -C <build>
install_pixel_test_dep`), as `wpt_serve_reftest` does. To run or debug one
text test, start the server by hand and give Starfish the list line as-is:

```sh
python3 tool/runner/http_server.py -d third_party/blink/third_party/blink/web_tests \
  --bind localhost 11013 --inject-script tool/webkit/inject_report.js \
  --inject-into /resources/js-test.js --inject-into /resources/testharnessreport.js
xvfb-run -s '-screen 0 1920x1080x24' -a ./Starfish \
  http://localhost:11013/fast/dom/Element/class-list-update.html \
  --hide-window --width=800 --height=600 --disable-console --timeout=30
```

CI runs `blink_test` in its own job (`test_blink_web_tests` in
`.github/workflows/x64_test.yml`), which restores the checkout through
`tool/ci/cache_vendor_tests.sh blink`. `test_all` includes it. Each test gets
30 s (`VENDOR_TIMEOUT_SEC` in `tool/runner/test_runner.py`). The port, 11013,
is baked into the list (`blink_test()` in `test_runner.py`).

## How results are collected

Blink kept WebKit's `window.testRunner` API, so the WebKit bridge,
`tool/webkit/inject_report.js`, serves both suites: `tool/runner/http_server.py`
prepends it to `resources/js-test.js` and `resources/testharnessreport.js` as
it serves them, and it prints the page's `innerText` when the harness is done.
Blink's `testharnessreport.js` writes one line per subtest that did not pass,
so a passing test's dump holds no result words, and its upstream
`-expected.txt` exists only to record the subtests Chrome fails.
`tool/drivers/tests/blink_test.py` compares the result words (`PASS`, `FAIL`,
`TIMEOUT`, `NOTRUN`, `PRECONDITION_FAILED`, `Harness Error`) of the dump with
those of the `-expected.txt`, or with none when there is no `-expected.txt`.
A crash or timeout never reaches `wptTestEnd()` and fails.

A reftest is rendered at 800x600 together with each reference and compared
exactly, with the screenshot and imgdiff helpers `wpt_serve_reftest` uses
(`tool/drivers/tests/vendor_reftest.py`). The screenshot waits for a
`reftest-wait` class to go away, but not for `testRunner.waitUntilDone()`:
the bridge only reaches pages that load a harness, so a reftest that
finishes later fails and stays a tracked known failure. So does a test that
needs `eventSender`, `internals` or a `testRunner` call the harnesses do not
make.

## The list

Same discipline as the WPT, Khronos and WebKit lists (`docs/wpt.md`): an
uncommented line is an expected pass and gates CI; a `#`-commented line is a
tracked known failure and is never deleted or uncommented just to make a run
green. After fixing a feature, run with `-f`, then uncomment the tests that
now pass.

`tool/blink/update_lists.py` writes the list from the checkout: the tests of
every directory `tool/blink/fetch.sh` checks out, which is therefore the one
place the served directories are named. The exception is
`http/tests/inspector-protocol`, which is checked out for the CDP suite
(`tool/cdp_test`) and which `blink_test` does not run. A line that is already
listed keeps its state; a new one is active only if it passed in every
`--pass-list` run given, otherwise it is added commented.

## Upgrading the pin

```sh
git -C third_party/blink fetch --depth 1 --filter=blob:none origin <new-sha>
git -C third_party/blink checkout --detach <new-sha>
git add third_party/blink
./tool/blink/fetch.sh
# measure every test twice; new tests become active only if both runs pass
for i in 1 2; do
  xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py -f \
    --out-pass-list --out-pass-list-filename blink_pass$i.txt blink_test
done
python3 tool/blink/update_lists.py --pass-list blink_pass1.txt --pass-list blink_pass2.txt
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py blink_test   # re-measure
git add tool/reftest/cairo/blink.res
# the pin also moves Chromium's CDP tests
python3 tool/cdp_test/run.py behavior --include-fails
git add tool/cdp_test/testlist-behavior.json
```

`update_lists.py` prints the tests the new pin dropped. An active test that
now fails is a regression or an upstream change, so look at it before
commenting it out. Adding a directory means adding it to `tool/blink/fetch.sh`
and measuring its tests the same way.
