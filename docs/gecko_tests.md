# Gecko tests

Starfish runs Firefox's DOM mochitests and layout reftests the way it runs the
WebKit LayoutTests (`docs/webkit_layouttests.md`): unmodified, out of a
version-pinned submodule, with a bridge standing in for the vendor's test
shell. They replace the copies the legacy `gecko_*` lists kept with
Starfish-specific expectations.

The tests come from the `third_party/gecko` submodule (mozilla-firefox/firefox)
and every test and reference page is exactly what upstream ships, so a pin
bump is a submodule update plus a re-measure. The Firefox tree is several GB,
so the submodule is a sparse checkout of the directories the legacy lists drew
from, made by `tool/gecko/fetch.sh`; `.gitmodules` sets `update = none` so
`git submodule update` never checks out the whole tree. The checkout root is
the server root, so each line of the list, `tool/reftest/cairo/gecko.res`, is
the full URL of a test.

| Upstream directory | What it is |
|---|---|
| `dom/tests/mochitest/dom-level1-core`, `dom-level2-core`, `dom-level2-html` | W3C DOM Level 1/2 conformance suites, as mochitests |
| `dom/tests/mochitest/general` | DOM API mochitests |
| `layout/reftests/box-sizing`, `floats`, `text-indent`, `text-transform` | Layout reftests |
| `testing/mochitest/tests/SimpleTest` | The mochitest harness, served at `/tests/SimpleTest/` |
| `layout/reftests/fonts` | Fonts the reftests load |

The list holds the tests of a directory's `mochitest*.toml` manifests (not
`chrome.toml`, whose tests need a privileged window) and the `==` / `!=` tests
of its `reftest.list`.

## Running

```sh
./tool/gecko/fetch.sh      # once, and after every pin bump
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py gecko_test
# include the tracked known-failures (`#` lines) to look for newly passing tests
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py -f gecko_test
```

The reftests need `tool/imgdiff/imgdiff` (`ninja -C <build>
install_pixel_test_dep`), as `wpt_serve_reftest` does. To run or debug one
mochitest, start the server by hand and give Starfish the list line as-is:

```sh
python3 tool/runner/http_server.py -d third_party/gecko --bind localhost 11014 \
  --inject-script tool/gecko/inject_report.js \
  --inject-into /tests/SimpleTest/SimpleTest.js \
  --alias /tests/SimpleTest/=testing/mochitest/tests/SimpleTest
xvfb-run -s '-screen 0 1920x1080x24' -a ./Starfish \
  http://localhost:11014/dom/tests/mochitest/dom-level1-core/test_attrcreatedocumentfragment.html \
  --hide-window --width=800 --height=600 --disable-console --timeout=30
```

CI runs `gecko_test` in its own job (`test_gecko_tests` in
`.github/workflows/x64_test.yml`), which restores the checkout through
`tool/ci/cache_vendor_tests.sh gecko`. `test_all` includes it. Each test gets
30 s (`VENDOR_TIMEOUT_SEC` in `tool/runner/test_runner.py`). The port, 11014,
is baked into the list (`gecko_test()` in `test_runner.py`).

## How results are collected

A mochitest loads `/tests/SimpleTest/SimpleTest.js`, which the server maps to
the harness in `testing/mochitest/tests/SimpleTest` (`--alias`). When no
mochitest runner frame hosts it, SimpleTest prints one line per check through
Firefox's `dump()` (`TEST-PASS`, `TEST-UNEXPECTED-FAIL`, `TEST-KNOWN-FAIL`,
...) and relies on `SpecialPowers`, Firefox's privileged test API, while it
loads and finishes. `tool/gecko/inject_report.js`, which
`tool/runner/http_server.py` prepends to `SimpleTest.js`, provides `dump()`,
the `SpecialPowers` members SimpleTest.js itself calls, and `window.opener`
(`null`; Starfish leaves it unimplemented), and calls `wptTestEnd()` once
SimpleTest has shown its report. `tool/drivers/tests/gecko_test.py` passes a
mochitest that finished, passed a check and printed no `TEST-UNEXPECTED-*`
line; `todo()` checks may fail. A test that needs another `SpecialPowers`
member fails, as it would in a browser without it.

A reftest is rendered at 800x1000, the size of Gecko's reftest window,
together with each reference its `reftest.list` names, and compared exactly
with the screenshot and imgdiff helpers `wpt_serve_reftest` uses
(`tool/drivers/tests/vendor_reftest.py`). The screenshot waits for the
`reftest-wait` class to go away. Gecko's annotations on a `reftest.list` line
(`fails`, `fuzzy()`, `skip-if()`, `pref()`, ...) are Firefox's expectations,
not Starfish's, and are ignored; Starfish's are the list's `#` lines.

## The list

Same discipline as the WPT, Khronos and WebKit lists (`docs/wpt.md`): an
uncommented line is an expected pass and gates CI; a `#`-commented line is a
tracked known failure and is never deleted or uncommented just to make a run
green. After fixing a feature, run with `-f`, then uncomment the tests that
now pass.

`tool/gecko/update_lists.py` writes the list from the checkout: the tests of
every directory `tool/gecko/fetch.sh` checks out, which is therefore the one
place the served directories are named. A line that is already listed keeps
its state; a new one is active only if it passed in every `--pass-list` run
given, otherwise it is added commented.

## Upgrading the pin

```sh
git -C third_party/gecko fetch --depth 1 --filter=blob:none origin <new-sha>
git -C third_party/gecko checkout --detach <new-sha>
git add third_party/gecko
./tool/gecko/fetch.sh
# measure every test twice; new tests become active only if both runs pass
for i in 1 2; do
  xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py -f \
    --out-pass-list --out-pass-list-filename gecko_pass$i.txt gecko_test
done
python3 tool/gecko/update_lists.py --pass-list gecko_pass1.txt --pass-list gecko_pass2.txt
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py gecko_test   # re-measure
git add tool/reftest/cairo/gecko.res
```

`update_lists.py` prints the tests the new pin dropped. An active test that
now fails is a regression or an upstream change, so look at it before
commenting it out. Adding a directory means adding it to `tool/gecko/fetch.sh`
and measuring its tests the same way.
