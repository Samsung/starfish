# WebKit LayoutTests

Starfish runs WebKit LayoutTests the way it runs WPT: the W3C DOM conformance
suites (`dom/html/level1`, `dom/html/level2`) and the `fast/*` tests of the
directories the legacy `webkit_*` lists drew from. They replace the copies the
legacy `reftest_all` suite kept with Starfish-specific expectations.

The tests come from the `third_party/webkit` submodule (WebKit/WebKit,
version-pinned) and are served **unmodified**: every test, `-expected.txt` and
reference page is exactly what upstream ships, so a pin bump is a submodule
update plus a re-measure. The WebKit tree is several GB, so the submodule is a
sparse checkout of the directories below, made by `tool/webkit/fetch.sh`;
`.gitmodules` sets `update = none` so `git submodule update` never checks out
the whole tree. Blink and Gecko tests run the same way
(`docs/blink_web_tests.md`, `docs/gecko_tests.md`). `LayoutTests/` is the
server root, so each line of the list, `tool/reftest/cairo/webkit.res`, is the
full URL of a test, as in the WPT and Khronos lists:

| Upstream directory (under `LayoutTests/`) | What it is |
|---|---|
| `dom/html` | W3C DOM Level 1/2 HTML conformance suites |
| `fast/*` (the directories named in `tool/webkit/fetch.sh`) | DOM, HTML, CSS, layout, text and parser tests |

The list holds every test in those directories that Starfish can judge:

| Kind | How it is recognized | Passes when |
|---|---|---|
| text | loads one of the harnesses the bridge below hooks into (`js-test-pre.js`, `js-test.js`, `selfhtml.js`) and has a text `-expected.txt` (not a render tree dump) | the dump carries the `-expected.txt` result words in the same order |
| reftest | has an `-expected` or `-expected-mismatch` page (`.html`, `.htm`, `.xhtml`, `.xht`, `.svg`) | its rendering matches each `-expected` page and differs from each `-expected-mismatch` one |

Pixel tests, render tree dumps and pages that call `testRunner` without a
harness are not run.

## Running

```sh
./tool/webkit/fetch.sh      # once, and after every pin bump
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py webkit_test
# include the tracked known-failures (`#` lines) to look for newly passing tests
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py -f webkit_test
```

The reftests need `tool/imgdiff/imgdiff` (`ninja -C <build>
install_pixel_test_dep`), as `wpt_serve_reftest` does. To run or debug one
text test, start the server by hand and give Starfish the list line as-is:

```sh
python3 tool/runner/http_server.py -d third_party/webkit/LayoutTests --bind localhost 11012 \
  --inject-script tool/webkit/inject_report.js \
  --inject-into /js-test-pre.js --inject-into /js-test.js --inject-into /selfhtml.js
xvfb-run -s '-screen 0 1920x1080x24' -a ./Starfish \
  http://localhost:11012/fast/dom/ChildNode-after.html \
  --hide-window --width=800 --height=600 --disable-console --timeout=30
```

CI runs `webkit_test` in its own job (`test_webkit_layouttests` in
`.github/workflows/x64_test.yml`), which restores the checkout through
`tool/ci/cache_vendor_tests.sh webkit` the way the Khronos job restores
`third_party/webgl`. `test_all` includes it. Each test gets 30 s
(`VENDOR_TIMEOUT_SEC` in `tool/runner/test_runner.py`); a finished test takes
a second or two.

## How results are collected

The harnesses talk to DumpRenderTree's `window.testRunner`: `dumpAsText()`
asks for a text result, `waitUntilDone()` holds the result until
`notifyDone()`, and otherwise the page is dumped once it has loaded. The
dump is the document's `innerText`, which is what each `-expected.txt` holds.

`tool/webkit/inject_report.js` provides that `testRunner`.
`tool/runner/http_server.py` prepends it to the harness scripts as it serves
them (`--inject-script` / `--inject-into`), so no upstream file or HTML is
rewritten. When the page is done it prints the dump and calls the engine's
`wptTestEnd()`. `tool/drivers/tests/webkit_test.py` then passes the test when
the dump carries the same result words (`PASS`/`FAIL`, and the DOM suites'
`Success`/`failure`/`Skipped`), in the same order, as the upstream
`-expected.txt`. A crash or timeout never reaches `wptTestEnd()` and fails.

Only the `testRunner` calls the harnesses themselves make are provided. A test
that needs another one, `eventSender` or `internals` fails, as it would in
a browser without them, and stays a tracked known failure.

A reftest is rendered at 800x600 together with each reference and compared
exactly, with the screenshot and imgdiff helpers `wpt_serve_reftest` uses
(`tool/drivers/tests/vendor_reftest.py`). The screenshot waits for a
`reftest-wait` class to go away, but not for `testRunner.waitUntilDone()`:
the bridge only reaches pages that load a harness, so a reftest that
finishes later fails and stays a tracked known failure.

The port is baked into the list (11012, in `webkit_test()` of
`test_runner.py`).

## The list

Same discipline as the WPT and Khronos lists (`docs/wpt.md`): an uncommented
line is an expected pass and gates CI; a `#`-commented line is a tracked known
failure and is never deleted or uncommented just to make a run green. After
fixing a feature, run with `-f`, then uncomment the tests that now pass.
A `# <url> # <reason>` line records why the test fails, and rewriting the list
keeps it.

Measure with the fonts `ninja install_pixel_test_dep` installs: a page that asks
for Ahem falls back to another font without them, and a reftest then compares
that font with itself. A test can still pass locally and fail on CI, which runs
a Debug build (30 s per test), has no internet access and ships an older pixman.

`tool/webkit/update_lists.py` writes the list from the checkout: the tests of
every directory `tool/webkit/fetch.sh` checks out, which is therefore the one
place the served directories are named. A line that is already listed keeps
its state; a new one is active only if it passed in
every `--pass-list` run given, otherwise it is added commented
(`tool/common/vendor_lists.py`, shared with the Blink and Gecko lists).

## Upgrading the pin

```sh
git -C third_party/webkit fetch --depth 1 --filter=blob:none origin <new-sha>
git -C third_party/webkit checkout --detach <new-sha>
git add third_party/webkit
./tool/webkit/fetch.sh
# measure every test twice; new tests become active only if both runs pass
for i in 1 2; do
  xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py -f \
    --out-pass-list --out-pass-list-filename webkit_pass$i.txt webkit_test
done
python3 tool/webkit/update_lists.py --pass-list webkit_pass1.txt --pass-list webkit_pass2.txt
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py webkit_test   # re-measure
git add tool/reftest/cairo/webkit.res
```

`update_lists.py` prints the tests the new pin dropped. An active test that
now fails is a regression or an upstream change, so look at it before
commenting it out. The weekly `cache_cleanup.yml` workflow republishes the CI
cache for the new pin; the first PR job after a bump builds it on a cache
miss.

Adding a directory means adding it to `tool/webkit/fetch.sh`, then measuring
the new tests the same way.
