# Web Platform Tests (WPT)

Starfish runs a curated subset of [web-platform-tests](https://github.com/web-platform-tests/wpt)
against an **on-demand** `wpt serve`, replacing the legacy always-on external
test server. WPT is version-pinned as the `third_party/wpt` submodule, so the
test corpus is reproducible and upgrades are explicit submodule bumps.

## Scope: testharness tests only

We target WPT **testharness.js** tests (JS assertions). Verdicts come straight
from the harness, with no stored expected images — so nothing here depends on
the internal `test/` submodule.

Out of scope (separate concerns, not run by this path):
- **Golden-image pixel tests** (`csswg`/`vendor_pixel`/`bidi` in
  `tool/test_runner.py`) — Starfish's own harness, needs expected PNGs from the
  internal `test/` submodule.
- **WPT native reftests** (`<link rel=match>`) — would need the engine's
  `--ref-test` two-phase compare, which currently does not reach the reference
  capture stage. Tracked separately.

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
parallel, and judges from the `WPTR` lines emitted by the injected report
script. A test PASSES when the harness completes cleanly (`status=0`), has at
least one subtest, and no subtest failed.

```sh
# whole baseline (all lists under tool/wpt/lists/)
python3 tool/wpt_runner.py tool/wpt/lists -j8

# one list, or resume an interrupted run (results are flushed per line)
python3 tool/wpt_runner.py tool/wpt/lists/dom_basic.res
python3 tool/wpt_runner.py tool/wpt/lists --results out.txt --resume
```

Or through `test_runner.py`, which gates on the active lists — `wpt_serve_all`
runs everything, and each `wpt_serve_<module>` runs one group so a module can be
checked in isolation:

```sh
./tool/test_runner.py wpt_serve_all
./tool/test_runner.py wpt_serve_dom      # css, dom, canvas, html, xhr, fetch,
                                         # worker, idb, websocket, webrtc, svg,
                                         # intersection_observer, others
```

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
Injected into every served page by `wpt serve --inject-script`. It registers a
testharness `add_completion_callback`; when the test finishes it prints one
line per subtest and a summary, then exits the shell through the engine's
`wptTestEnd()` hook (which quits when `HIDE_WINDOW` is set):

    WPTR PASS <subtest name>
    WPTR FAIL <subtest name>
    WPTR DONE status=<0=OK|1=ERROR|2=TIMEOUT|3=PRECONDITION_FAILED> count=<n>

This is the only contract between the page and the runner; no other script
parses page output.

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

```sh
python3 tool/wpt_runner.py tool/wpt/lists          # whole baseline
python3 tool/wpt_runner.py .../dom_basic.res -j8 --timeout 20
```

For each active URL it runs `./Starfish <url> --hide-window`, parses the `WPTR`
lines, and records a verdict:
- PASS  = `DONE status=0` and `count>0` and no FAIL subtests
- FAIL  = anything else (reason: `SUBTESTS_FAILED`, `TIMEOUT`,
  `HARNESS_STATUS_1/2`, `NO_COMPLETION`, …)

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

### tool/wpt_annotate.py — mark failures
Turns a measurement into the green gate. Reads a `--results` file and, in each
`.res`, prefixes every FAIL URL with `# [auto-fail] ` (leaving passes active and
existing comments untouched). Re-run after an engine fix or pin bump to refresh
which tests gate.

```sh
python3 tool/wpt_runner.py tool/wpt/lists --results r.txt
python3 tool/wpt_annotate.py r.txt tool/wpt/lists
```

## Test lists

`tool/wpt/lists/*.res` — generated by `wpt_audit.py` from the
legacy `tool/reftest/cairo/wpt/*.res` against the pinned revision. Only the
`.res` files are tracked; the sibling `*.remap` (renamed tests, old→new) and
`*.missing` (dropped, with HTTP status) provenance files are regenerated by
`wpt_audit.py` and git-ignored.

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
