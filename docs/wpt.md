# Web Platform Tests (WPT)

Starfish runs a curated subset of [web-platform-tests](https://github.com/web-platform-tests/wpt)
against an **on-demand** `wpt serve`, replacing the legacy always-on external
test server. WPT is version-pinned as the `third_party/wpt` submodule, so the
test corpus is reproducible and upgrades are explicit submodule bumps.

## Scope: testharness, reftest, crashtest

We target WPT **testharness.js** tests (JS assertions), **reftests** (`<link
rel=match|mismatch>` pixel comparison), and **crashtests** (page must load
without crashing the shell). None of these need stored expected images, so
nothing here depends on the internal `test/` submodule.

Verdicts are computed harness-side rather than by wptrunner (WPT's own runner,
in `third_party/wpt/tools/wptrunner/`): wptrunner drives a browser over
WebDriver/BiDi, which Starfish doesn't implement yet. Reftest verdicts instead
compose two already-CI-proven primitives —
`./Starfish <url> --screen-shot=<file>` (used as-is by the golden-image pixel
suites) and `tool/imgdiff/imgdiff` — with the reference URL, comparison
relation, and any fuzzy tolerance read from `third_party/wpt/MANIFEST.json` at
run time, the same way `wptrunner`'s reftest executor resolves them (see
`tool/wpt_reftest.py`). If Starfish ever gains WebDriver-BiDi support (for its
own sake, not just for this), migrating this tooling to wptrunner would pick
up `testdriver.js`-dependent tests, wdspec, and print-reftest for free — worth
revisiting then.

MVP limits (tracked as follow-ups, not blocking): fuzzy tolerance is not
applied (exact-pixel only, so `<meta name=fuzzy>` tests may false-FAIL),
reference chains (a reference that is itself a reftest) are not resolved
recursively, and `reftest-wait` extra settle time isn't honored beyond `load`.
`tool/wpt_reftest.py` judges reftest by parsing the pixel-difference
percentage `tool/imgdiff/imgdiff` prints, not its own pass/fail verdict (which
folds in a golden-image antialiasing tolerance inappropriate for reftest). Two
gaps remain in that percentage itself, and they are not independent of the
fuzzy limit above:
- `imgdiff`'s own per-pixel comparison already tolerates a difference of a
  few levels per channel *before a pixel counts as differing at all*. This is
  not just a rounding nuance: a systematic, small-but-real rendering
  difference that touches every pixel (e.g. a gamma/color-space or
  premultiplied-alpha rounding bug) can produce `diffCount == 0` for the
  *entire* image, so an `==` reftest would pass even though the render is
  genuinely wrong. This is currently acting as an uncontrolled substitute for
  the fuzzy tolerance the MVP doesn't implement — landing a
  `tool/imgdiff/imgdiff.cpp` "exact" mode (no channel tolerance, raw
  `diffCount` output) without also landing fuzzy support would likely flip a
  batch of currently-passing-by-accident `<meta name=fuzzy>` tests to failing
  at the same time, which would look like a regression from the imgdiff
  change rather than the fuzzy gap surfacing for the first time.
- The printed percentage is separately rounded to 2 decimals, so a handful of
  differing pixels on a large image can round to "0.00%" -- a narrower,
  independent imprecision on top of the above.

Both need the same `tool/imgdiff/imgdiff.cpp` change to close (deferred since
imgdiff is shared with the golden-image pixel suites and this would need a
rebuild); when picked up, do it together with fuzzy support, not in isolation.
The engine's own `--ref-test`/`rtDoTest` two-phase state machine is unused by
this path (it currently crashes navigating to the reference —
`WebView.cpp:816`, `referrerURL != nullptr` — and only understands
`rel=match`); removing that dead code is a candidate follow-up.

A crashtest verdict has three outcomes: **PASS** (`WPTR CRASHOK` — the engine
rendered the page to `load` without crashing), **SIGNAL_CRASH** (the shell
died on a signal — a genuine, reproducible engine crash, the highest-value
output of this suite; spot-checked cases were real `SIGABRT`/assertion
failures with backtraces into `src/core/...`), and **TIMEOUT**. TIMEOUT is the
one to interpret with care: **before treating a crashtest TIMEOUT as an engine
bug, classify it first**, because most timeouts fall into causes that are not
engine problems:

- **`wpt serve --inject-script` silently skips injection (upstream
  limitation, not ours).** `third_party/wpt/tools/serve/serve.py:63`'s
  `inject_script()` tokenizes the document with an HTML5 tokenizer to find an
  insertion point, and if the tokenizer hits a `ParseError` before that point
  it gives up with no warning and returns the page unmodified
  (`if error: return html`). Crashtests routinely contain deliberately
  malformed markup (that's the point — they try to trip a real parser), which
  hits this condition, so `tool/wpt/inject_report.js` never runs and
  `WPTR CRASHOK` can never be printed — a guaranteed TIMEOUT no matter how
  well Starfish handled the page. Confirmed by `curl`-ing served pages: e.g.
  `css/css-sizing/min-content-negative-margin-crash.html` reached
  `Window.onload` and idle mode cleanly per the engine logs, yet the served
  bytes were identical to the source (no script injected). `.svg`-served
  crashtests are skipped for the same reason via a different path (the
  injector only touches `Content-Type: text/html`). This is a
  `third_party/wpt` (pinned submodule) limitation; closing it would need a
  different completion-signal delivery mechanism, out of scope here.
- **Completion signal never fires though injection succeeded.** For a page
  carrying a wait class, the pass signal is the test's own script clearing it;
  our `inject_report.js` waits for `load` and observes the `class` attribute
  (both `test-wait` and `reftest-wait` are recognized). Tests that only signal
  via the `TestRendered` custom event still don't complete — wptrunner's own
  helper `third_party/wpt/tools/wptrunner/wptrunner/executors/test-wait.js`
  dispatches it (after `load` → `document.fonts.ready` → a double-rAF settle),
  but we never do, so a test that only listens for `TestRendered` (e.g.
  `css/css-anchor-position/long-anchor-chain-crash.html`) waits forever — as
  does any test whose async work (a `Worker`, `requestIdleCallback`, …) never
  settles in Starfish. In these the *engine* typically didn't crash; they are
  harness-coverage gaps, not engine bugs.
- **Genuinely pathological pages that really are slow/hung.** Some TIMEOUTs
  are legitimate — e.g.
  `editing/crashtests/…-collapsible-spaces.html` inserts `" ".repeat(3.3e8)`
  (marked `<meta name=timeout=long>`) and does not finish in any reasonable
  budget. Here TIMEOUT is the *correct* verdict (a crashtest catching a real
  problem), not a harness artifact.

Known completion-signal limitations of the crashtest path (all yield a false
TIMEOUT, i.e. safe over-exclusion, never a false PASS):
- The exit fires as soon as the wait class clears (or immediately if none is
  present), so this path catches load-time and *synchronous* crashes; a crash
  that would only surface on an arbitrarily-delayed later task is not
  guaranteed to be observed (true of every crashtest here, and of wptrunner
  itself — completion is signal-driven, not a fixed settle window).
- The `MutationObserver` watches only the root's `class` attribute on the node
  present at `load`. A test that signals completion by removing/replacing the
  root element outright (rather than clearing the class) is not observed and
  times out (matches upstream `test-wait.js`; rare in practice).

One earlier bug here **is now fixed**: a crashtest that removes
`document.documentElement` itself (e.g.
`dom/nodes/crashtests/documentElement-remove-*.html`,
`css/css-page/crashtests/root-element-remove-*.html`) made
`inject_report.js`'s `document.documentElement.classList` throw a `null`-deref
and hang as a false TIMEOUT; the crashtest path now null-guards the root and
treats a removed `documentElement` as "no wait class → done".

Net: a crashtest TIMEOUT is **safe to `# [auto-fail]`** (it only excludes,
never creates a false green), but it is *not* reliable evidence of an engine
defect on its own — only SIGNAL_CRASH (and a hand-verified genuine hang) is.

(Diagnosing this the first time, a `./Starfish <file-path> --hide-window`
invocation — bypassing `wpt serve` entirely — was mistaken for evidence of a
genuine engine hang. That invocation has no injected script and no exit
trigger of any kind regardless of the page, so it hangs unconditionally; it
proves nothing about the page under test. Always reproduce through
`wpt_server.py`'s `wpt_serve()` context and the real
`wpt_runner.run_one_crashtest()` path — or `test_runner.py`'s
`wpt_serve_crashtest` — never a direct file-path invocation.)

Smaller known follow-ups from tooling review, not blocking: `run_all()`'s
per-item exception backstop (`tool/wpt_runner.py`) records only `str(e)` with
no traceback, so an unexpected tooling bug (vs. an expected external failure)
is hard to tell apart in a large batch's failure histogram -- logging the
traceback to stderr (without changing the recorded reason) would help without
giving up the backstop's batch-safety property. The `wpt_domains`/`no_proxy`
Starfish-subprocess env setup is duplicated across three call sites
(`wpt_runner.py`'s `run_one`/`run_one_crashtest`, `wpt_reftest.py`'s
`_screenshot`) and could be a single shared helper. `wpt_runner.py`'s new
`_with_crashtest_marker` (via `urlsplit`/`urlunsplit`) and `wpt_audit.py`'s
existing manual URL-string reconstruction are two different conventions for
the same category of operation in the same directory; worth unifying if a
third URL-rewrite need comes up.

Out of scope (separate concerns, not run by this path):
- **Golden-image pixel tests** (`csswg`/`vendor_pixel`/`bidi` in
  `tool/test_runner.py`) — Starfish's own harness, needs expected PNGs from the
  internal `test/` submodule.
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

## Running

`tool/wpt_runner.py` starts `wpt serve`, runs each URL in the Starfish shell in
parallel, and judges the result according to `--mode` (default `testharness`):

- `testharness` (default): judges from the `WPTR` lines emitted by the
  injected report script. PASSES when the harness completes cleanly
  (`status=0`), has at least one subtest, and no subtest failed.
- `reftest`: judges from a `--screen-shot` capture of the test page and each
  MANIFEST-declared reference, diffed with `tool/imgdiff/imgdiff` (see
  `tool/wpt_reftest.py`). PASSES when every reference's relation (`==` must
  match, `!=` must not) holds.
- `crashtest`: judges from the `WPTR CRASHOK` marker the injected script emits
  once the page loads (waiting out a `test-wait` class if present) without
  crashing the shell.

```sh
# whole baseline (all lists under tool/wpt/lists/)
python3 tool/wpt_runner.py tool/wpt/lists -j8

# one list, or resume an interrupted run (results are flushed per line)
python3 tool/wpt_runner.py tool/wpt/lists/dom_basic.res
python3 tool/wpt_runner.py tool/wpt/lists --results out.txt --resume

# reftest / crashtest lists (tool/wpt/reftest_lists/, tool/wpt/crashtest_lists/)
python3 tool/wpt_runner.py tool/wpt/reftest_lists --mode reftest -j8
python3 tool/wpt_runner.py tool/wpt/crashtest_lists --mode crashtest -j8
```

Or through `test_runner.py`, which gates on the active lists — `wpt_serve_all`
runs everything, and each `wpt_serve_<module>` runs one group so a module can be
checked in isolation:

```sh
./tool/test_runner.py wpt_serve_all
./tool/test_runner.py wpt_serve_dom      # css, dom, canvas, html, xhr, fetch,
                                         # worker, idb, websocket, webrtc, svg,
                                         # intersection_observer, others
./tool/test_runner.py wpt_serve_reftest    # tool/wpt/reftest_lists/
./tool/test_runner.py wpt_serve_crashtest  # tool/wpt/crashtest_lists/
```

`wpt_serve_reftest`/`wpt_serve_crashtest` run lists generated straight from
MANIFEST.json (see below) rather than carried forward from a legacy corpus,
but they are baselined and annotated the same way as `wpt_serve_all` and gate
at ~100%. Both run in CI (`.github/workflows/x64_test.yml`). Re-baseline and
re-annotate them like any other list (see `tool/CLAUDE.md`'s WPT workflow)
after an engine fix or WPT pin bump changes what passes.

The server can also be driven standalone:

```sh
python3 tool/wpt_server.py            # serve until Ctrl-C (third_party/wpt)
```

## Tooling

All scripts live in `tool/`. Default WPT checkout is `third_party/wpt`
(override with `--wpt-root` or `$WPT_ROOT`). Data flows:

    legacy lists ──wpt_audit.py──▶ wpt/lists/*.res ──wpt_runner.py──▶ results
    (tool/reftest/cairo/wpt/)        (generated)        │                │
                                                         │                ▼
                                              wpt_server.py        wpt_annotate.py
                                              (serves both)        (results ─▶ # [auto-fail])

### tool/wpt/inject_report.js
Injected into every served page by `wpt serve --inject-script`. For a
testharness page it registers `add_completion_callback`; when the test
finishes it prints one line per subtest and a summary, then exits the shell
through the engine's `wptTestEnd()` hook (which quits when `HIDE_WINDOW` is
set):

    WPTR PASS <subtest name>
    WPTR FAIL <subtest name>
    WPTR DONE status=<0=OK|1=ERROR|2=TIMEOUT|3=PRECONDITION_FAILED> count=<n>

For a crashtest (URL carries the `__starfish_crashtest=1` query marker that
`wpt_runner.py --mode crashtest` appends), it instead waits for `load` and for
the root element's `test-wait` class to be gone (if present), then prints:

    WPTR CRASHOK

reftest has no page-side contract at all — the harness captures each page's
pixels externally via `--screen-shot` (see `tool/wpt_reftest.py`).

The query marker keeps the two page-side paths mutually exclusive, and no
other script parses page output.

### tool/wpt_server.py
On-demand `wpt serve` as a context manager, `wpt_serve(wpt_root, ...)`, plus a
CLI (`python3 tool/wpt_server.py` serves until Ctrl-C). Responsibilities:
- start `wpt serve --no-h2 --inject-script inject_report.js` in its own session;
- consider it healthy only after several consecutive good HTTP probes, and fail
  fast if it dies during boot;
- reclaim the ports first if a stale server is lingering (a leftover holding an
  alt port like 8444 makes a fresh server abort silently);
- on exit, kill the whole process group and wait for the ports to free.
Other scripts import `wpt_serve` / `DEFAULT_WPT_ROOT` from here.

### tool/wpt_runner.py — measure / gate
Runs the tests and judges them. Input: a `.res` file or a directory of them.
`--mode {testharness,reftest,crashtest}` (default `testharness`) selects the
verdict mechanism:

```sh
python3 tool/wpt_runner.py tool/wpt/lists                          # whole testharness baseline
python3 tool/wpt_runner.py .../dom_basic.res -j8 --timeout 20
python3 tool/wpt_runner.py tool/wpt/reftest_lists --mode reftest   # reftest
python3 tool/wpt_runner.py tool/wpt/crashtest_lists --mode crashtest
```

For each active URL, `testharness` mode runs `./Starfish <url> --hide-window`,
parses the `WPTR` lines, and records a verdict:
- PASS  = `DONE status=0` and `count>0` and no FAIL subtests
- FAIL  = anything else (reason: `SUBTESTS_FAILED`, `TIMEOUT`,
  `HARNESS_STATUS_1/2`, `NO_COMPLETION`, …)

`reftest` mode delegates to `tool/wpt_reftest.py`: capture the test page and
every MANIFEST-declared reference with `--screen-shot`, diff with
`tool/imgdiff/imgdiff`, and require every reference's relation to hold (reason:
`IMG_MISMATCH`, `IMG_UNEXPECTED_MATCH`, `REF_LOAD_FAIL(...)`, `NO_REFERENCE`
if the URL isn't in MANIFEST.json's `reftest` branch, …). `crashtest` mode
looks for the `WPTR CRASHOK` marker (reason: `SIGNAL_CRASH` if the shell
exited on a signal, `NO_COMPLETION`/`TIMEOUT` otherwise).

Key flags: `-j` parallel shells; `--timeout` per-test seconds; `--results FILE`
writes `PASS|FAIL <reason> <url>` per test (line-buffered); `--resume` skips URLs
already in that file and appends (survives interruption); `-f/--force` also runs
`#`-commented lines; `--no-serve` reuses an already-running server. Prints a
per-list table and a failure-reason histogram, and exits non-zero if anything in
an active list fails (so `test_runner.py wpt_serve_all` is a regression gate).

### tool/wpt_audit.py — generate / refresh the lists
Decides which legacy tests still exist in the pinned revision and (re)writes the
`tool/wpt/lists/` lists.

```sh
python3 tool/wpt_audit.py --out-dir tool/wpt/lists
```

It probes the running server for each legacy URL (HTTP 200 = served, 404 =
gone) — authoritative for WPT's *generated* tests (`*.any.html` etc.) that have
no file on disk. Missing URLs are then "remapped" by basename across three
rename patterns (directory move, `.htm`↔`.html` swap, plain→`*.any.js` /
`*.window.js`) and re-probed. With `--out-dir` it writes, per legacy list:
- `<name>.res`         — served + recovered URLs (the new list)
- `<name>.res.remap`   — `old -> new` for every recovered rename
- `<name>.res.missing` — dropped URLs with their HTTP status

Flags: `--include-commented`, `--no-remap`, `--workers`.

### tool/wpt_manifest_lists.py — generate reftest / crashtest lists
reftest and crashtest have no legacy corpus for `wpt_audit.py` to carry
forward, so this instead enumerates `third_party/wpt/MANIFEST.json` directly
per spec directory in `tool/wpt_status_targets.txt` (the same mechanism
`tool/wpt_status.py` already uses for its testharness coverage report):

```sh
python3 tool/wpt_manifest_lists.py --mode reftest --out-dir tool/wpt/reftest_lists
python3 tool/wpt_manifest_lists.py --mode crashtest --out-dir tool/wpt/crashtest_lists
```

Lists carry only test URLs, no reference/relation/fuzzy metadata — that is
resolved live from MANIFEST.json at run time (`tool/wpt_reftest.py`), so a
submodule bump that changes a reference or adds fuzzy just works without
regenerating the list.

### tool/wpt_reftest.py — reftest capture + compare
`resolve_references(manifest, test_path)` reads a reftest's references
(`[[ref_path, relation], ...]` plus any `fuzzy` extras) out of
`items["reftest"]`; `run_reftest(url, ...)` captures the test page and each
reference with `--screen-shot`, diffs with `imgdiff`, and applies the relation
(`==` must match, `!=` must not). Also runnable standalone against one URL for
debugging: `python3 tool/wpt_reftest.py <url>` (run inside a `wpt_serve`
context, e.g. via `python3 tool/wpt_server.py` in another shell).

### tool/wpt_annotate.py — mark failures
Turns a measurement into the green gate. Reads a `--results` file and, in each
`.res`, prefixes every FAIL URL with `# [auto-fail:REASON] ` (e.g.
`# [auto-fail:NO_REFERENCE]`) — the reason from the results file is kept in the
marker so a known tooling gap stays distinguishable from a real engine bug
(leaving passes active and existing comments untouched). Re-run after an
engine fix or pin bump to refresh which tests gate.

```sh
python3 tool/wpt_runner.py tool/wpt/lists --results r.txt
python3 tool/wpt_annotate.py r.txt tool/wpt/lists
```

## Test lists

`tool/wpt/lists/*.res` (testharness) — generated by `wpt_audit.py` from the
legacy `tool/reftest/cairo/wpt/*.res` against the pinned revision. Only the
`.res` files are tracked; the sibling `*.remap` (renamed tests, old→new) and
`*.missing` (dropped, with HTTP status) provenance files are regenerated by
`wpt_audit.py` and git-ignored.

`tool/wpt/reftest_lists/*.res` and `tool/wpt/crashtest_lists/*.res` —
generated by `wpt_manifest_lists.py` straight from MANIFEST.json (no legacy
corpus to carry forward), then baselined and annotated the same way as the
testharness lists (`tool/wpt_runner.py --mode ... --results` +
`wpt_annotate.py`) — see the WPT workflow in `tool/CLAUDE.md`. Re-run this
after an engine fix or WPT pin bump changes what passes.

## Upgrading the pin

```sh
git -C third_party/wpt fetch --depth 1 origin <new-sha>
git -C third_party/wpt checkout <new-sha>
python3 tool/wpt_audit.py --out-dir tool/wpt/lists   # regen lists
python3 tool/wpt_runner.py tool/wpt/lists            # re-measure
git add third_party/wpt tool/wpt/lists
```

## Baseline

Measured at the initial pin (WPT `1c4810772`, 2026-06-08): of 1919 testharness
tests, **78.1% pass**. Coverage of the legacy curated set after rename recovery
is 95.4% (1930/2024). Weak areas (engine-gap signal): webrtc, websockets, svg,
html/syntax, html/rendering, cors. Strong: css, dom, html/canvas, workers, xhr.

## Status board (wpt.fyi-comparable)

The nightly status board at <https://pages.github.sec.samsung.net/lws/starfish/>
runs *un-curated* spec directories (`tool/wpt_status_targets.txt`) to reveal
where Starfish is strong or weak per spec area — unlike the CI gate above, which
runs the curated `.res` lists at ~100% by design.

Counting matches **wpt.fyi**, so the numbers compare directly with the major
browsers. Each test is scored at the **subtest** level (`wpt_status.py:score`):
a test with subtests contributes `passing / total` subtests; a test with none
(single-page test, or a harness error that produced none) counts as `1` total,
passing only if the harness status is OK. The aggregate is the sum across tests.

> **Scope caveat:** the board counts **testharness** subtests only (reftest /
> crashtest / wdspec excluded), so its total test count looks smaller than
> wpt.fyi's full set. Read the comparison at the subtest level, not by raw
> totals.

Pipeline (`.github/workflows/wpt_status_nightly.yml`, nightly):

    wpt_status.py ──▶ report-YYYYMMDD.html  (per-category, subtest counts)
                 └──▶ metrics.json          (subtest passed/total/rate;
                                             files_passed/total + per-category
                                             breakdown for diagnostics)
    metrics.json ──wpt_update_data.py──▶ data.json ──wpt_generate_dashboard.py──▶ index.html

Run it locally:

```sh
xvfb-run -s '-screen 0 1920x1080x24' -a python3 tool/wpt_status.py \
  --only css/selectors --limit 30 -o report.html --output-json metrics.json
```

Because the metric definition is subtest-level (not the earlier per-file count),
the cumulative `data.json` history must be reset once when this lands — trigger
the workflow with `reset_history: true`.
