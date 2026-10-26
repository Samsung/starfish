# Web Platform Tests (WPT)

WPT is the canonical measure of spec compliance: a passing WPT test means the
implementation matches the spec, not just ad-hoc behavior. Ship features with
WPT coverage.

Starfish runs curated CI lists and an uncurated nightly selection from
[web-platform-tests](https://github.com/web-platform-tests/wpt) against an
**on-demand** `wpt serve`, replacing the legacy always-on external test server.
WPT is version-pinned as the `third_party/wpt` submodule, so the
test corpus is reproducible and upgrades are explicit submodule bumps.

## Scope: testharness, reftest, crashtest

We target WPT **testharness.js** tests (JS assertions), **reftests** (`<link
rel=match|mismatch>` pixel comparison), and **crashtests** (page must load
without crashing the shell). None of these need stored expected images, so
nothing here depends on the internal `test/` submodule.

Verdicts are computed by Starfish's Python tools rather than WPT's
`wptrunner`. Testharness results come from `inject_report.js`; reftests use
`--screen-shot` and `tool/imgdiff/imgdiff`; crashtests use the injected
`WPTR CRASHOK` completion marker. This path does not use the engine's
`--ref-test`/`rtDoTest` navigation flow.

### Current runner coverage

Runner coverage is defined by `wpt_status.py`, `wpt_runner.py`,
`wpt_reftest.py`, `wpt_server.py`, `inject_report.js`, and the CI workflows.

| Area | Current behavior |
|---|---|
| Test types | The runners accept testharness, reftest, and crashtest. They do not execute wdspec, print-reftest, manual, or visual tests. |
| Browser automation | There is no Starfish testdriver vendor adapter in this path. Tests requiring automated input, permission changes, or other testdriver commands cannot be assumed to run correctly. Merely loading `testdriver.js` is not an exclusion criterion. |
| Dedicated Worker | Nightly builds with `WORKER=1`. Worker support must not be classified as generally absent. |
| SharedWorker / ServiceWorker | `worker.yml` builds the daemon executables and runs the dedicated suites through `test_runner.py`. Nightly does not build/start those peers or use that suite's shared storage setup. This is an execution-setup gap in nightly. |
| URL selection | `wpt_manifest_inventory.py` uses `wpt_urls.test_url()` to select HTTP (8000), HTTPS (8443), or HTTP/2 over TLS (9000) from the pinned WPT filename flags and `wpt_flags` query values; HTTP/2 takes priority. URL selection does not verify browser protocol support. |
| Target overlap | Nightly expands each target directory separately. Overlapping targets can schedule the same URL more than once; the reporter does not deduplicate across categories. |
| Test metadata | Nightly uses a fixed CLI timeout and concurrency. It does not apply per-test timeout metadata or the aggregate CI suite's separate Web Audio settings. |
| MANIFEST freshness | `ensure_manifest()` creates a missing MANIFEST but leaves an existing file untouched. Regenerate it after a WPT pin change before measuring. |

### Reftest limits

`resolve_references()` reads reference URLs, relations, and fuzzy metadata
from MANIFEST at run time. `run_reftest()` applies the relations but does
**not** apply fuzzy tolerances or recursively resolve reference chains.

The comparison is also not byte-exact: `imgdiff.cpp`'s `Color::operator==`
tolerates small channel differences, and its printed difference percentage
is rounded to two decimals. The runner treats a nonzero printed percentage
as a difference, ignoring imgdiff's own golden-image pass/fail marker.
Consequently both missed differences and failures on fuzzy-allowed
differences are possible. Exact comparison and WPT fuzzy support need to be
considered together because imgdiff is shared with the golden-image suites.

In test builds, `ResourceLoader.cpp` waits for either `reftest-wait` or
`test-wait` to clear before capture. It also dispatches `TestRendered` at
the root when a wait class remains at the delayed post-load test hook.
That event is already implemented; it is not a missing runner feature.

### Crashtest completion and diagnosis

`run_one_crashtest()` appends a query marker that selects the crashtest path
in `inject_report.js`. The script waits for load and for both `test-wait`
and `reftest-wait` to be absent, then prints `WPTR CRASHOK` and exits.
A missing document element at the initial check is treated as no wait class.
The runner reports `OK`, `SIGNAL_CRASH`, `TIMEOUT`, `SHELL_ERROR`, or
`NO_COMPLETION`, depending on the process result and completion marker.

A timeout alone does not identify an engine defect. Check the following:

- **Script delivery:** the pinned WPT `serve.py` injector only rewrites
  `text/html` responses and returns the document unchanged on certain
  tokenizer errors. A crashtest without the bridge cannot emit its completion
  marker. The `testharnessreport.js` alias provides another delivery path
  for testharness pages, but crashtests need not load that script.
- **Wait completion:** after load, the observer watches the initial root's
  class attribute, not document-root replacement. A class that never clears
  needs investigation into the test, engine, and required execution setup.
- **Runtime:** slow pages and genuine hangs can also exceed the configured
  timeout. Check the test's timeout metadata before drawing a conclusion.

Completion is signal-driven: a crash scheduled after the completion marker
and shell exit is not guaranteed to be observed. Historical reports of
particular crashes, hangs, and injection failures need reproduction with the
current build and WPT pin; they are not a current failure inventory.

Reproduce through `wpt_serve()` and `run_one_crashtest()`, or the
`wpt_serve_crashtest` suite. A bare
`./Starfish <file-path> --hide-window` run does not supply this completion
protocol; remaining open is not evidence of a test hang.

Out of scope (separate concerns, not run by this path):

- **Golden-image pixel tests** (`csswg`/`vendor_pixel`/`bidi` in
  `tool/runner/test_runner.py`) — Starfish's own harness, needs expected PNGs
  from the internal `test/` submodule.
- **print-reftest, wdspec, manual, visual** — need paginated rendering, a
  WebDriver session, or a human, respectively. Not run by this path.

## One-time setup

WPT requires its subdomains to resolve to loopback. Generate the entries once:

```sh
git submodule update --init third_party/wpt
python3 third_party/wpt/wpt make-hosts-file | sudo tee -a /etc/hosts
```

Verify: `getent hosts web-platform.test www.web-platform.test` should print two
`127.0.0.1` lines. On WSL2, set `[network] generateHosts=false` in
`/etc/wsl.conf` to keep the entries across restarts.

Pixel-diff fonts (Ahem etc.) and `tool/imgdiff` come from
`ninja -C <build> install_pixel_test_dep` (already needed for other suites).

## MANIFEST freshness

`ensure_manifest()` generates `third_party/wpt/MANIFEST.json` when it is
missing; it does not update an existing file. After changing the WPT pin,
refresh the manifest explicitly before enumeration or measurement:

```sh
python3 third_party/wpt/wpt manifest -p third_party/wpt/MANIFEST.json \
  --tests-root third_party/wpt --no-download
```

Manifest generation includes local test files. Check
`git -C third_party/wpt status --short` before comparing measurements with a
clean CI checkout; an unchanged submodule commit alone does not ensure an
identical local corpus.

## Running

`tool/wpt/scripts/wpt_runner.py` starts `wpt serve`, runs each URL in the
Starfish shell in parallel, and judges the result according to `--mode`
(default `testharness`):

- `testharness` (default): judges from the `WPTR` lines emitted by the
  injected report script. PASSES when the harness completes cleanly
  (`status=0`), has at least one subtest, and no subtest failed.
- `reftest`: judges from a `--screen-shot` capture of the test page and each
  MANIFEST-declared reference, diffed with `tool/imgdiff/imgdiff` (see
  `tool/wpt/scripts/wpt_reftest.py`). PASSES when every reference's relation
  (`==` must match, `!=` must not) holds.
- `crashtest`: judges from the `WPTR CRASHOK` marker the injected script emits
  once the page loads (waiting for both `test-wait` and `reftest-wait` to
  clear if present) without crashing the shell.

```sh
# whole baseline (all lists under tool/wpt/testharness_lists/)
xvfb-run -s '-screen 0 1920x1080x24' -a python3 tool/wpt/scripts/wpt_runner.py tool/wpt/testharness_lists -j8

# one list, or resume an interrupted run (results are flushed per line)
xvfb-run -s '-screen 0 1920x1080x24' -a python3 tool/wpt/scripts/wpt_runner.py tool/wpt/testharness_lists/dom_basic.res
xvfb-run -s '-screen 0 1920x1080x24' -a python3 tool/wpt/scripts/wpt_runner.py \
    tool/wpt/testharness_lists --results out.txt --resume

# reftest / crashtest lists (tool/wpt/reftest_lists/, tool/wpt/crashtest_lists/),
# capturing a --results file to baseline/re-baseline against (see wpt_annotate.py below)
xvfb-run -s '-screen 0 1920x1080x24' -a python3 tool/wpt/scripts/wpt_runner.py \
    tool/wpt/reftest_lists --mode reftest -j8 --results reftest_baseline.txt
xvfb-run -s '-screen 0 1920x1080x24' -a python3 tool/wpt/scripts/wpt_runner.py \
    tool/wpt/crashtest_lists --mode crashtest -j8 --results crashtest_baseline.txt
```

Or through `test_runner.py`, which gates on the active lists —
`wpt_serve_testharness` runs active testharness lists except the separate
Worker/ServiceWorker suites. Each `wpt_serve_testharness_<module>` runs one
group so a module can be checked in isolation; `wpt_serve_reftest` and
`wpt_serve_crashtest` run the active lists for those two kinds. These are
curated lists, not the complete WPT corpus:

```sh
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py wpt_serve_testharness
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py wpt_serve_testharness_dom
                                         # css, dom, canvas, html, xhr, fetch,
                                         # worker, idb, websocket, webrtc, svg,
                                         # webaudio, intersection_observer, others
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py wpt_serve_testharness_worker
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py wpt_serve_testharness_serviceworker
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py wpt_serve_reftest    # tool/wpt/reftest_lists/
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py wpt_serve_crashtest  # tool/wpt/crashtest_lists/
```

The Worker/ServiceWorker suites require the matching build flags and daemon
executables. See `.github/workflows/worker.yml` for the `uv_cairo_gl` build,
`Starfish-sharedworker` / `Starfish-serviceworker` executables, and dedicated
suite invocations. `test_runner.py` starts the peers and shares their storage
directory with clients; the generic status reporter does not do this.

`wpt_serve_testharness` runs `webaudio_*.res` as a second group in the same
`wpt serve` session, with `wpt_serve_testharness_webaudio`'s settings (2 jobs,
60 s per test) instead of the default 8 jobs / 20 s, because offline audio
renders are memory- and CPU-heavy in Debug builds; the run fails if either
group regresses. `STARFISH_BIN=<binary>` overrides `./Starfish` for every
`wpt_runner.py`-based run.

`wpt_serve_reftest`/`wpt_serve_crashtest` run lists generated straight from
MANIFEST.json (see below) rather than carried forward from a legacy corpus,
but they are baselined and annotated the same way as `wpt_serve_testharness`
and gate at ~100%. All three run in CI (`.github/workflows/x64_test.yml`).
Re-baseline and
re-annotate them like any other list (see `tool/wpt/scripts/wpt_annotate.py`
below) after an engine fix or WPT pin bump changes what passes.

## Running the server standalone

`tool/wpt/scripts/wpt_runner.py`/`test_runner.py` each start their own `wpt
serve` for the duration of a batch run and tear it down afterwards. To
reproduce that same environment for one test — without going through either
driver — start the server on its own and keep it up:

```sh
python3 tool/wpt/scripts/wpt_server.py            # serve until Ctrl-C or SIGTERM (third_party/wpt)
```

This is the exact same `wpt serve --inject-script inject_report.js`
process the drivers use (`tool/wpt/scripts/wpt_server.py`'s `wpt_serve()`), on
the same `web-platform.test` hosts/ports set up in "One-time setup" — so any
URL you load against it (in a browser, via `curl`, or by invoking `./Starfish`
directly) sees the exact same server-side behavior a batched run would,
including `inject_report.js`'s injection. Take the exact `http://...` or
`https://...` URL from the `.res` file/line under test.

`wpt_runner.py ... --no-serve` also accepts this same standalone server
instead of starting its own — useful for running a full batch against the
environment you're inspecting by hand.

A bare file-path invocation (`./Starfish <file-path>`, bypassing `wpt serve`)
does not supply the injected completion bridge or crashtest query marker.
A page remaining open in that mode is not evidence of an engine hang; use
the served URL and the matching runner to reproduce the test protocol.

For reftest, `python3 tool/wpt/scripts/wpt_reftest.py <url>` (run against this
same standalone server) reproduces the full capture+diff and reports which
reference failed and why.

## Tooling

Everything WPT lives under `tool/wpt/`: the drivers in `tool/wpt/scripts/`,
the curated `.res` lists and the injected page script beside them as data.
Default WPT checkout is `third_party/wpt` (override with `--wpt-root` or
`$WPT_ROOT`).

    tool/wpt/
    ├── scripts/            wpt_runner.py, wpt_reftest.py, wpt_audit.py, ...
    ├── inject_report.js    HTML injection / testharnessreport.js alias
    ├── testharness_lists/  curated .res lists (the CI gate)
    ├── reftest_lists/      generated from MANIFEST.json
    ├── crashtest_lists/    generated from MANIFEST.json
    └── wpt_status_targets.txt   spec dirs for the status board

Data flows:

    legacy lists ─wpt_audit.py─▶ testharness_lists/*.res ─wpt_runner.py─▶ results
    (tool/reftest/cairo/wpt/)         (generated)          │                 │
                                                           │                 ▼
                                                 wpt_server.py        wpt_annotate.py
                                                 (serves both)  (results ─▶ # [auto-fail])

### tool/wpt/inject_report.js
Injected into every served `text/html` page by `wpt serve --inject-script`,
and also served as `/resources/testharnessreport.js` (the vendor hook every
testharness page loads; `wpt_server.py` aliases it) because the injector
leaves non-HTML responses alone — without the alias an `.xhtml`/`.xml`
testharness page never reports and shows up as `TIMEOUT`. An HTML page
therefore receives the script twice; a window flag makes the second copy a
no-op. For a testharness page it calls `setup({output: false})` when the
harness script loads (a capture `load` listener on `document`, backed by
polling), before test scripts execute. This applies to every testharness
suite. It disables the visual results table and per-assertion stack
recording, which otherwise consume substantial memory and time for
sample-by-sample audio checks in Debug builds; assertions and verdicts still
run normally. It registers `add_completion_callback`; when the test finishes
it prints one line per subtest and a summary, then exits the shell through
the engine's `wptTestEnd()` hook (which quits when `HIDE_WINDOW` is set):

    WPTR PASS <subtest name>
    WPTR FAIL <subtest name>
    WPTR DETAIL <subtest name>: <failure message>   (diagnostic, after a FAIL)
    WPTR DONE status=<0=OK|1=ERROR|2=TIMEOUT|3=PRECONDITION_FAILED> count=<n>

For a crashtest (URL carries the `__starfish_crashtest=1` query marker that
`wpt_runner.py --mode crashtest` appends), it instead waits for `load` and for
the root element's `test-wait` and `reftest-wait` classes to be gone (if
present), then prints:

    WPTR CRASHOK

reftest has no page-side contract at all — the harness captures each page's
pixels externally via `--screen-shot` (see `tool/wpt/scripts/wpt_reftest.py`).

The query marker keeps the two page-side paths mutually exclusive, and no
other script parses page output.

### tool/wpt/scripts/wpt_server.py
On-demand `wpt serve` as a context manager, `wpt_serve(wpt_root, ...)`, plus a
CLI (`python3 tool/wpt/scripts/wpt_server.py` serves until Ctrl-C or a
plain `kill`; both tear the whole `wpt serve` group down).
Responsibilities:

- start `wpt serve --inject-script inject_report.js --alias_file ...`
  in its own session, the alias file mapping `/resources/testharnessreport.js`
  to a temp dir holding a copy of `inject_report.js` under that name (a file
  alias looks the file up by name inside `local-dir`);
- consider it healthy only after several consecutive successful HTTP and HTTPS
  probes, and fail fast if it dies during boot. The HTTPS probe accepts WPT's
  fixture certificate; it does not change browser TLS settings;
- reclaim the ports first if a stale server is lingering (a leftover holding an
  alt port like 8444 or the HTTP/2 port 9000 makes a fresh server abort
  silently);
- on exit, kill the whole process group and wait for the ports to free.

HTTP/2 is enabled by default. Disabling it with `--no-h2` leaves
`ports.h2` empty, which prevents templates such as
`websockets/constants.sub.js` from rendering before the browser receives
them.

`wpt_urls.py` follows the pinned WPT manifest/wptrunner protocol rules:
`.https.`, `.serviceworker.`, `.serviceworker-module.`, and equivalent
`wpt_flags` query values select HTTPS (8443); `.h2.` / `wpt_flags=h2` takes
priority and selects TLS on the HTTP/2 port (9000). Ordinary tests retain
HTTP (8000). Manifest inventory, list generation, and legacy URL auditing
share this selection. Reftest references retain the test origin, including
its HTTPS or HTTP/2 port; explicit absolute references retain their own URL.
The CI Debug build defines `STARFISH_ENABLE_TEST` and currently skips browser
certificate verification. HTTPS serving alone does not prove HTTP/2 negotiation.

Other scripts import `wpt_serve` / `DEFAULT_WPT_ROOT` from here.

### tool/wpt/scripts/wpt_runner.py — measure / gate
Runs the tests and judges them. Input: a `.res` file or a directory of them.
`--mode {testharness,reftest,crashtest}` (default `testharness`) selects the
verdict mechanism:

```sh
xvfb-run -s '-screen 0 1920x1080x24' -a python3 tool/wpt/scripts/wpt_runner.py tool/wpt/testharness_lists  # whole testharness baseline
xvfb-run -s '-screen 0 1920x1080x24' -a python3 tool/wpt/scripts/wpt_runner.py .../dom_basic.res -j8 --timeout 20
xvfb-run -s '-screen 0 1920x1080x24' -a python3 tool/wpt/scripts/wpt_runner.py \
    tool/wpt/reftest_lists --mode reftest      # reftest
xvfb-run -s '-screen 0 1920x1080x24' -a python3 tool/wpt/scripts/wpt_runner.py \
    tool/wpt/crashtest_lists --mode crashtest  # crashtest
```

For each active URL, `testharness` mode runs `./Starfish <url> --hide-window`,
parses the `WPTR` lines, and records a verdict:
- PASS  = `DONE status=0` and `count>0` and no FAIL subtests
- FAIL  = anything else (reason: `SUBTESTS_FAILED`, `TIMEOUT`,
  `HARNESS_STATUS_1/2`, `NO_COMPLETION`, …)

`reftest` mode delegates to `tool/wpt/scripts/wpt_reftest.py`: capture the test
page and every MANIFEST-declared reference with `--screen-shot`, diff with
`tool/imgdiff/imgdiff`, and require every reference's relation to hold (reason:
`IMG_MISMATCH`, `IMG_UNEXPECTED_MATCH`, `REF_LOAD_FAIL(...)`, `NO_REFERENCE` if
the URL isn't in MANIFEST.json's `reftest` branch, …). `crashtest` mode looks
for the `WPTR CRASHOK` marker (reason: `SIGNAL_CRASH` if the shell exited on a
signal, `NO_COMPLETION`/`TIMEOUT` otherwise).

Key flags: `-j` parallel shells; `--timeout` per-test seconds; `--results FILE`
writes `PASS|FAIL <reason> <url>` per test (line-buffered); `--resume` skips URLs
already in that file and appends (survives interruption); `-f/--force` also runs
`#`-commented lines; `--no-serve` reuses an already-running server. Prints a
per-list table and a failure-reason histogram, and exits non-zero if anything in
an active list fails (so `test_runner.py wpt_serve_testharness` is a regression
gate).

### tool/wpt/scripts/wpt_audit.py — generate / refresh the lists
Decides which legacy tests still exist in the pinned revision and (re)writes the
`tool/wpt/testharness_lists/` lists.

```sh
python3 tool/wpt/scripts/wpt_audit.py --out-dir tool/wpt/testharness_lists
```

It selects the required protocol before probing each legacy URL (status
200 = served, 404 = gone) — authoritative for WPT's *generated* tests
(`*.any.html` etc.) that have no file on disk. Missing URLs are then "remapped" by basename across three
rename patterns (directory move, `.htm`↔`.html` swap, plain→`*.any.js` /
`*.window.js`) and re-probed. With `--out-dir` it writes, per legacy list:
- `<name>.res`         — served + recovered URLs (the new list)
- `<name>.res.remap`   — `old -> new` for every recovered rename
- `<name>.res.missing` — dropped URLs with their HTTP status

Flags: `--include-commented`, `--no-remap`, `--workers`.

### tool/wpt/scripts/wpt_manifest_lists.py — generate reftest / crashtest lists
reftest and crashtest have no legacy corpus for `wpt_audit.py` to carry
forward, so this instead enumerates `third_party/wpt/MANIFEST.json` directly
per spec directory in `tool/wpt/wpt_status_targets.txt` (the same mechanism
`tool/wpt/scripts/wpt_status.py` already uses for its testharness coverage
report):

```sh
python3 tool/wpt/scripts/wpt_manifest_lists.py --mode reftest --out-dir tool/wpt/reftest_lists
python3 tool/wpt/scripts/wpt_manifest_lists.py --mode crashtest --out-dir tool/wpt/crashtest_lists
```

Caution: this rewrites **every** target's list under `--out-dir`, not just
the one you care about — any hand-applied annotations in the other lists are
silently clobbered. Diff the directory before committing.

Lists carry only test URLs, no reference/relation/fuzzy metadata — that is
read from MANIFEST.json at run time (`tool/wpt/scripts/wpt_reftest.py`).
Reference changes need no list regeneration if the test URL is unchanged,
but the MANIFEST must be refreshed after a pin bump. Fuzzy metadata is read
but is not applied by the current comparison code.

### tool/wpt/scripts/wpt_reftest.py — reftest capture + compare
`resolve_references(manifest, test_path)` reads a reftest's references
(`[[ref_path, relation], ...]` plus any `fuzzy` extras) out of
`items["reftest"]`; `run_reftest(url, ...)` captures the test page and each
reference with `--screen-shot`, diffs with `imgdiff`, and applies the relation
(`==` must match, `!=` must not). Also runnable standalone against one URL for
debugging: `xvfb-run -s '-screen 0 1920x1080x24' -a python3
tool/wpt/scripts/wpt_reftest.py <url>` (run inside a `wpt_serve` context, e.g.
via `python3 tool/wpt/scripts/wpt_server.py` in another shell).

### tool/wpt/scripts/wpt_annotate.py — mark failures
Turns a measurement into the green gate. Reads a `--results` file and, in each
`.res`, prefixes every FAIL URL with `# [auto-fail:REASON] ` (e.g.
`# [auto-fail:NO_REFERENCE]`) — the reason from the results file is kept in the
marker so a known tooling gap stays distinguishable from a real engine bug
(leaving passes active and existing comments untouched). Re-run after an
engine fix or pin bump to refresh which tests gate.

The marker is write-once: an already-commented line is never re-verified or
reactivated by a later run, so a test you just fixed stays commented — and
invisible to the gate — until someone manually uncomments it and re-runs.

```sh
# testharness
xvfb-run -s '-screen 0 1920x1080x24' -a python3 tool/wpt/scripts/wpt_runner.py \
    tool/wpt/testharness_lists --results testharness_baseline.txt
python3 tool/wpt/scripts/wpt_annotate.py testharness_baseline.txt tool/wpt/testharness_lists

# reftest
xvfb-run -s '-screen 0 1920x1080x24' -a python3 tool/wpt/scripts/wpt_runner.py \
    tool/wpt/reftest_lists --mode reftest --results reftest_baseline.txt
python3 tool/wpt/scripts/wpt_annotate.py reftest_baseline.txt tool/wpt/reftest_lists/

# crashtest
xvfb-run -s '-screen 0 1920x1080x24' -a python3 tool/wpt/scripts/wpt_runner.py \
    tool/wpt/crashtest_lists --mode crashtest --results crashtest_baseline.txt
python3 tool/wpt/scripts/wpt_annotate.py crashtest_baseline.txt tool/wpt/crashtest_lists/
```

`wpt_annotate.py` itself never launches Starfish (it only rewrites `.res` files
from a results file), so it does not need `xvfb-run` — only the `wpt_runner.py`
measurement step does.

## Verdict reasons

`tool/wpt/scripts/wpt_runner.py`/`tool/wpt/scripts/wpt_reftest.py` record one
of these reason strings per test, in `--results` output and the failure-reason
histogram. A reason with a `:` or `(...)` suffix (`IMGDIFF_ERROR: ...`,
`REF_LOAD_FAIL(TIMEOUT)`) is bucketed by the text before it — that's also what
`wpt_annotate.py` writes into the `# [auto-fail:<category>]` marker
(`reason_category()` in `tool/wpt/scripts/wpt_runner.py`).

testharness (`run_one`):
- `OK` — `DONE status=0`, at least one subtest, no `FAIL`.
- `TIMEOUT` — the shell didn't exit within `--timeout` (default 15s; 20s in
  `test_runner.py` suites) — a hang, an infinite loop, or a page whose
  completion signal never fires.
- `SHELL_ERROR` — failed to exec Starfish (`OSError`).
- `NO_COMPLETION` — the shell exited but no `WPTR DONE` line was printed
  (injection didn't run, the page exited early, testharness.js never loaded).
- `HARNESS_STATUS_<n>` — `DONE status=n` with n≠0: `1`=ERROR, `2`=TIMEOUT
  (testharness.js's own internal timeout, distinct from the runner's
  `--timeout`), `3`=PRECONDITION_FAILED (see the status legend above).
- `NO_SUBTESTS` — completed with `count=0`.
- `SUBTESTS_FAILED` — `status=0` but at least one `WPTR FAIL`.

crashtest (`run_one_crashtest`):
- `OK` — `WPTR CRASHOK` seen (page loaded, any `test-wait` class cleared, no
  crash).
- `SIGNAL_CRASH` — the shell was killed by a signal — the actual crash a
  crashtest exists to catch.
- `TIMEOUT` / `SHELL_ERROR` — same meaning as testharness.
- `NO_COMPLETION` — exited cleanly but no `WPTR CRASHOK` (e.g. injection
  skipped on malformed markup, or the completion signal never fired — see the
  crashtest TIMEOUT-taxonomy discussion above).

reftest (`run_reftest`/`_screenshot`):
- `OK` — every MANIFEST-declared reference's relation held (`==` matched,
  `!=` didn't).
- `NO_REFERENCE` — the URL has no reftest reference in `MANIFEST.json`.
- `TC_CRASH` — the render didn't come out right (a broad "something's wrong
  with the screenshot" bucket inherited from the golden-image driver) —
  distinct from crashtest's `SIGNAL_CRASH`, not a synonym for it.
- `REF_LOAD_FAIL(<reason>)` — capturing the *reference* page failed; the
  parenthesized reason is one of `TIMEOUT`/`SHELL_ERROR`/`TC_CRASH` above.
- `IMGDIFF_TIMEOUT` / `IMGDIFF_ERROR: <msg>` — `tool/imgdiff/imgdiff` itself
  timed out or errored.
- `IMG_MISMATCH` — `==` reference but pixels differed.
- `IMG_UNEXPECTED_MATCH` — `!=` reference but pixels matched.
- `TIMEOUT` / `SHELL_ERROR` — same meaning as testharness, for the test-page
  capture itself.

Tooling-only, not a test verdict: `INTERNAL_ERROR: <exc>` from `run_all`'s
per-item exception backstop — a bug in the runner/tooling, not the page under
test.

## Test lists

Active lines are expected-pass and gate CI; `#`-commented lines are tracked
known failures, not dead entries — deleting or uncommenting one to make a run
green erases the failure's tracking, not the failure (`wpt_runner.py` skips
commented lines unless `--force` / `test_runner.py -f` is passed).

Besides the `# [auto-fail:REASON]` markers `wpt_annotate.py` writes, a line
can be commented by hand with `# [vacuous-pass:REASON]`: the test *passes*,
but only because the feature it exercises is unimplemented on both sides of
the comparison (e.g. a reftest whose test and reference pages both degrade to
the same rendering), so the pass asserts nothing. Such a line must stay
commented until the named feature is implemented and the test re-measured —
uncommenting it earlier just re-adds a meaningless green.

`tool/wpt/testharness_lists/*.res` (testharness) — generated by `wpt_audit.py` from the
legacy `tool/reftest/cairo/wpt/*.res` against the pinned revision. Only the
`.res` files are tracked; the sibling `*.remap` (renamed tests, old→new) and
`*.missing` (dropped, with HTTP status) provenance files are regenerated by
`wpt_audit.py` and git-ignored.

`tool/wpt/reftest_lists/*.res` and `tool/wpt/crashtest_lists/*.res` —
generated by `wpt_manifest_lists.py` straight from MANIFEST.json (no legacy
corpus to carry forward), then baselined and annotated the same way as the
testharness lists (`tool/wpt/scripts/wpt_runner.py --mode ... --results` +
`wpt_annotate.py`, see above). Re-run this
after an engine fix or WPT pin bump changes what passes.

## Upgrading the pin

```sh
git -C third_party/wpt fetch --depth 1 origin <new-sha>
git -C third_party/wpt checkout <new-sha>
python3 tool/wpt/scripts/wpt_audit.py --out-dir tool/wpt/testharness_lists   # regen lists (HTTP probe only, no Starfish)
xvfb-run -s '-screen 0 1920x1080x24' -a python3 tool/wpt/scripts/wpt_runner.py tool/wpt/testharness_lists  # re-measure
git add third_party/wpt tool/wpt/testharness_lists
```

## Historical baseline

These are initial measurements, not a current pass-rate report. Measured at
the initial pin (WPT `1c4810772`, 2026-06-08): of 1919 testharness
tests, **78.1% pass**. Coverage of the legacy curated set after rename recovery
is 95.4% (1930/2024). Weak areas (engine-gap signal): webrtc, websockets, svg,
html/syntax, html/rendering, cors. Strong: css, dom, html/canvas, workers, xhr.

## Status board

The nightly status board at <https://pages.github.sec.samsung.net/lws/starfish/>
runs uncurated tests from the directories in
`tool/wpt/wpt_status_targets.txt`. It is not yet an all-MANIFEST run.
Unlike the curated CI gate, individual test failures contribute to the report
rather than fail the job: `wpt_status.py` returns success after writing its
outputs, even if tests failed. Tool or infrastructure errors can still fail
the job.

`.github/workflows/wpt_status_nightly.yml` explicitly requests
**testharness, reftest, and crashtest**, with 16 parallel shells and a
15-second per-test timeout. The standalone `wpt_status.py` CLI still defaults
to testharness only. SharedWorker/ServiceWorker daemon setup and CI's separate
Web Audio execution settings are not currently part of the nightly reporter.

### Inventory helpers

`wpt_manifest_inventory.manifest_inventory` enumerates unique
`(test type, URL)` candidates across complete MANIFEST branches.
`wpt_scope.compare_inventory` compares candidates with supplied active CI
entries, retaining list provenance and diagnosing origin/type/URL mismatches.
These are library helpers; the master nightly reporter does not call them
to select its execution scope. They neither prove execution completion nor
add missing CI URLs automatically. Callers must include separately scheduled
CI suites when constructing an inventory.

The contract tests use synthetic data and start no browser or WPT server:

```sh
xvfb-run -s '-screen 0 1920x1080x24' -a python3 -B -m unittest discover -s tool/wpt/scripts -p test_wpt_inventory.py
```

### Counting and comparison

`wpt_status.py:score` counts testharness results at the subtest level: a
test with subtests contributes its passing and total subtest counts. A result
without subtests contributes one total, passing only when its harness status
is OK/PASS. Reftests and crashtests have no subtests, so each contributes one
pass or fail. The HTML report separates requested types; JSON metrics also
include a per-type `types` breakdown when multiple types are requested.

The combined metric mixes testharness subtests with whole-file
reftest/crashtest verdicts. A similar counting rule does not make the overall
percentage directly comparable to another browser's wpt.fyi result:
test selection, WPT revision, build flags, URL protocols, execution setup,
and runner limitations also affect the result. Compare matching scopes and
types, and retain the WPT revision recorded in each report.

Pipeline (`.github/workflows/wpt_status_nightly.yml`, nightly):

    wpt_status.py ──▶ report-YYYYMMDD.html  (per-type/category results)
                 └──▶ metrics.json          (combined and per-type counts;
                                             file and category breakdowns)
    metrics.json ──wpt_update_data.py──▶ data.json ──wpt_generate_dashboard.py──▶ index.html

Run a small selection locally with the same three types:

```sh
xvfb-run -s '-screen 0 1920x1080x24' -a python3 tool/wpt/scripts/wpt_status.py \
  --only css/selectors --limit 30 --test-types testharness,reftest,crashtest \
  -o report.html --output-json metrics.json
```

The workflow's `reset_history: true` input clears dashboard history when a
deliberate metric/scope migration requires it; ordinary runs retain history.
