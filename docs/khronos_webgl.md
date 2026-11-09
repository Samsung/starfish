# Khronos WebGL Conformance Tests

The [Khronos WebGL conformance suites](https://github.com/KhronosGroup/WebGL)
are the proof of WebGL spec compliance, the way WPT is for the rest of the
platform. Starfish runs a curated subset of them against a `-DWEBGL=1` build.

The suites are version-pinned as the `third_party/webgl` submodule (shallow,
like `third_party/wpt`) and served **unmodified**: nothing in the checkout is
edited, so a pin bump is a plain submodule update and every test is exactly
what upstream ships. The submodule root is the server root, so each list
line is the full URL of a test, as in the WPT lists, and the three list files
map onto three directories of that one repository:

| List | URL prefix | What it is |
|---|---|---|
| `tool/reftest/cairo/khronos_webgl.res` | `http://localhost:11010/conformance-suites/1.0.3/` | Frozen WebGL 1.0.3 release |
| `tool/reftest/cairo/khronos_webgl2.res` | `http://localhost:11010/conformance-suites/2.0.0/` | Frozen WebGL 2.0.0 release |
| `tool/reftest/cairo/khronos_webglsdk.res` | `http://localhost:11010/sdk/tests/` | Development tip (moves with every upstream commit) |

The frozen releases still receive occasional backports upstream, so a pin
bump can change them too — re-measure all three lists, not just `sdk`.

## Running

```sh
# all three suites
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py khronos_test
# one suite
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py vendor_test_khronos2
# include the tracked known-failures (`#` lines) to look for newly passing tests
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py -f vendor_test_khronos
```

To run or debug one test, start the server by hand and give Starfish the
list line as-is:

```sh
python3 tool/runner/http_server.py -d third_party/webgl --bind localhost 11010 \
  --inject-script tool/khronos/inject_report.js \
  --inject-into /js-test-pre.js --inject-into /more/unit.js
xvfb-run -s '-screen 0 1920x1080x24' -a ./Starfish \
  http://localhost:11010/conformance-suites/2.0.0/conformance2/buffers/buffer-bind-test.html \
  --hide-window --width=800 --height=600 --disable-console --timeout=300
```

`khronos_test` is deliberately not part of `reftest_all`; `test_all` still
includes it. The GCC workflow (`.github/workflows/x64_test.yml`) separates
its Release build from two test jobs. Each test job restores the WebGL
submodule through `tool/ci/cache_webgl.sh`, like the WPT cache restore.

`build_khronos_webgl` runs on the company runner (`code-linux`,
`code-2xlarge`) and builds GCC (`glib_cairo_gl`) with
`-DCMAKE_BUILD_TYPE=Release -DENABLE_WASM=1 -DWALRUS_JIT=ON` in `out/webgl-ci`.
It sets `-DENABLE_TEST=1` to retain the harness's `testEnd()` hook and has a
30-minute budget. It publishes `starfish-x64_gcc_webgl_release`, a tarball
of the executable and shared libraries, preserving permissions and library
symlinks. Both test jobs depend on this build and use
`STARFISH_BIN=./out/webgl-ci/bin/lightweight-web-engine`:

| Job | Runner | Suite | Parallel tests | llvmpipe threads per process |
|---|---|---|---|---|
| `test_khronos_webgl` | Company (`code-linux`, `code-2xlarge`) | `khronos_conformance_test` | 8 | 4 |
| `test_khronos_deqp` | Self-hosted Linux (`self-hosted`, `linux`) | `khronos_deqp_test` | 26 | 2 |

Both test jobs have a 60-minute budget. The dEQP job forces software Mesa
with `LIBGL_ALWAYS_SOFTWARE=1` and `GALLIUM_DRIVER=llvmpipe`.
The two suites partition all three curated lists by `/deqp/` in the URL,
using temporary lists that preserve known-failure comment markers. Their
combined coverage matches `khronos_test`; this split enables no new tests.
Use `-f` to include known failures in either partition. Run locally with:

```sh
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py khronos_conformance_test
LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe LP_NUM_THREADS=2 \
  xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py khronos_deqp_test --khronos-jobs 26
```

Clang (`uv_cairo_gl`) reuses the `starfish-x64_clang_debug` artifact from
`build_x64_clang`, without a separate build or Wasm JIT configuration.
`STARFISH_BIN=./out/debug/bin/lightweight-web-engine` selects that Debug
binary. Its WebGL job runs `khronos_smoke_test` with a 15-minute test budget
and a 30-minute job budget. The dedicated `tool/reftest/cairo/khronos_webgl_smoke.res`
list selects 20 active passing entries from the three full lists: WebGL 1
and 2 context creation, buffers, shaders, vertex attributes, uniforms,
textures, drawing and readback, plus WebGL 2 vertex arrays and pixel pack
buffers and two SDK regression tests. Keep smoke entries in the full lists
as well, so GCC continues to cover them. Run the same subset locally with:

```sh
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py khronos_smoke_test
```

The runner defaults to 8 parallel tests (`KHRONOS_WEBGL_JOBS`), adjustable
with `--khronos-jobs`. Every test drives its own GL context, so tune test
parallelism together with `LP_NUM_THREADS` to avoid oversubscribing software
Mesa. Each test has a 1200 s budget (`KHRONOS_WEBGL_TIMEOUT_SEC` in
`tool/runner/test_runner.py`); a few legitimately take several minutes
(`uninitialized-test-2.html`, `multisample-corruption.html`). The three
source lists run sequentially within each suite.

## How results are collected

The suite's two harnesses, `js-test-pre.js` (every `conformance*/` test) and
`more/unit.js` (`conformance/more/`), report each assertion through
`window.parent.webglTestHarness`. That hook exists for Khronos's own
iframe-based runner (`webgl-conformance-tests.html`), but a page loaded at top
level has `window.parent === window`, so defining the object on the page
itself receives the same callbacks with no iframe involved.

`tool/khronos/inject_report.js` does exactly that. `tool/runner/http_server.py`
prepends it to the two harness scripts as it serves them (`--inject-script` /
`--inject-into`), so the bridge runs before the harness in the same script
and no HTML is rewritten. On the harness's `notifyFinished()` it prints one
`FAIL <message>` line per failed assertion plus a `KHRONOS DONE pass=<n>
fail=<n> skipped=<n>` summary, then ends the shell through the engine's
`testEnd()`, whose `[PASS]` / `[FAIL]` is what
`tool/drivers/basics/starfish_basic_test.py` counts. A page whose harness
never reports anything cannot pass. A skipped assertion (a case the platform
legitimately doesn't support) counts as a pass, as in Khronos's own runner.

The port is baked into the lists (`KHRONOS_PORT` in `test_runner.py`); if it
ever changes, re-prefix the three lists with it.

## Lists

Same discipline as the WPT lists (`docs/wpt.md`): an uncommented line is an
expected pass and gates CI; a `#`-commented line is a tracked known failure
and is never deleted or uncommented just to make a run green. After fixing a
feature, run the list with `-f`, then uncomment the tests that now pass.

Every `khronos_webgl2.res` line ends in `?webglVersion=2`, the query Khronos's
own runner (`webgl-conformance-tests.html`) appends to each page of the 2.0.0
suite. The harness creates a WebGL 1 context unless that query says
otherwise, so without it the `conformance/` half of the suite — and the
`conformance2/glsl3` shader pages — would re-run as WebGL 1 tests. Keep the
query on new lines. The 1.0.3 list needs none: that runner asks for WebGL 1,
the default. The sdk list is bare for the same reason, but its harness also
runs most pages as WebGL 2 (2.0.1), and each such page gets a second line
with the query.

## Upgrading the pin

```sh
git -C third_party/webgl fetch --depth 1 origin <new-sha>
git -C third_party/webgl checkout <new-sha>
# every listed test must still exist (upstream renames are rare but happen)
python3 - <<'EOF'
import glob, os
for f in glob.glob("tool/reftest/cairo/khronos_webgl*.res"):
    for line in open(f):
        p = line.strip().lstrip("# ").split()
        if not p:
            continue
        local = p[0].replace("http://localhost:11010/", "third_party/webgl/")
        if not os.path.exists(local.split("?")[0]):
            print(f, p[0])
EOF
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py khronos_test   # re-measure
git add third_party/webgl tool/reftest/cairo/khronos_webgl*.res
```

Only `third_party/webgl` and the three lists change in a pin bump. The weekly
`cache_cleanup.yml` workflow republishes the CI cache for the new pin; the
first PR job after a bump builds it on a cache miss.
