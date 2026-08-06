# AGENTS.md

Starfish is a lightweight Web browser engine for TV, mobile, headless and
wearable devices; low memory usage is the core constraint. The JS engine is
Escargot (`third_party/escargot`). The relevant WHATWG/W3C/ECMA-262 spec is
the source of truth for behavior, and Web Platform Tests (WPT) are the proof
of spec compliance.

## Documentation map

| Looking for | See |
|---|---|
| Build, cross-compile, per-platform steps, testing setup | `README.md` |
| C++ style (headers, formatting, classes, nullability, GC) | `docs/Coding_Style_Guide.md` |
| WPT structure, tooling, `.res` list workflow | `docs/wpt.md` |
| Full test suite list | `./tool/test_runner.py -h` |
| Source layout | `src/{core,binding,platform,browser,public,shell,launcher}` — `core/` is the engine proper; `public/` is the embedding API (`public/bridge` = per-platform bridges, `public/delegate` = API/impl separation) |

This file holds the norms; explanations live in the documents and code
comments they point to.

## Changing web-facing behavior

1. Decide behavior from the spec (WHATWG/W3C/ECMA-262), not from another
   engine's observable quirks. Comments cite the spec rationale (why), not
   the mechanics (how).
2. Interface shape comes from the `.idl` files, which mirror the specs — do
   not invent API surface, and do not modify an `.idl` without confirming
   the spec actually says so. After any `.idl` change, re-run cmake (see
   Build below).
3. Land behavior changes together with test coverage — CI gates on the
   active test lists (`.github/workflows/x64_test.yml`). If WPT covers the
   behavior, activate it: baseline the `.res` list, implement, re-measure,
   re-annotate (`docs/wpt.md`). If not, add an internal test (see Testing).
4. Heavy or optional web capabilities are compile-time gated and default
   off — follow the existing flag pattern in `CMakeLists.txt` (WEBGL,
   WEBRTC, WORKER, IDB, ENABLE_WASM, ...). That default-off posture is the
   lightweight identity of the engine; don't bypass it.

## Coding rules

Deltas and emphases on top of `docs/Coding_Style_Guide.md`:

- A value that can be absent is `Optional<T>` (`src/StarfishBase.h`), not a
  raw pointer overloaded with `nullptr`; check with implicit truthiness
  (`if (node)` / `if (!node)`). Exception: don't use `Optional<T*>` where
  "explicitly set to null" must stay distinct from "empty" — the pointer
  specialization collapses the two (see the comment at its definition).
- A plain pointer parameter or member is expected valid — this is a GC-based
  object graph. Don't add blanket `STARFISH_ASSERT(ptr != nullptr)` or
  defensive null checks (remaining ones are legacy, on their way out); assert
  only invariants the type system can't express.
- Containers of GC-managed pointers use `GCVector`/`GCTightVector`, even
  for short-lived locals — a `std::vector` buffer lives outside the GC heap
  and its elements can be collected while still in use.
- Fix root causes. Don't paper over a symptom with a defensive null check
  or a try-catch that swallows the failure.

## Build

Procedures live in `README.md`. One rule worth repeating: **editing any
`.idl` (add, modify, delete) requires re-running cmake** — incremental
`ninja` never regenerates the bindings (see the comment in
`build/starfish.cmake`).

## Testing

Wrap every test run in `xvfb-run -s '-screen 0 1920x1080x24' -a` — suites
launch Starfish 8-way in parallel, and the fixed virtual screen keeps both
system load and pixel comparisons under control (see `README.md` Testing).

After a change, run the closest suites first:

| Touched | Run | Notes |
|---|---|---|
| Any C++ | `./tool/check_tidy.py` | Same check as the PR CI `check_source` job |
| `core/dom`, DOM APIs | `./tool/test_runner.py wpt_serve_dom internal_test` | Fast |
| `core/style`, CSS/selectors | `./tool/test_runner.py wpt_serve_css` | |
| HTML parsing/elements | `./tool/test_runner.py wpt_serve_html` | |
| fetch / xhr / canvas / svg / ... | matching `wpt_serve_*` suite | |
| worker / serviceworker | `wpt_serve_testharness_worker` / `_serviceworker` | Excluded from the aggregate suite (needs daemon peers) |
| Layout, paint, rendering | `wpt_serve_reftest`, or `reftest_all` for a full pass | `reftest_all` is slow; prefer targeted suites while iterating |

Conventions:

- Internal tests assert with `console.assert` and finish with `testEnd()`;
  they must also run unmodified in a plain browser.
- Unit tests cover the public embedding API: `Starfish unit-test`.
- `test/` is a submodule; only modify existing test assets when a test is
  clearly wrong.
- `.res` list discipline (details in `docs/wpt.md`): a `#`-commented line
  is a tracked known-failure — never delete or uncomment one just to make a
  run green; `wpt_annotate.py` markers are write-once — a fixed test stays
  commented until manually uncommented; `wpt_manifest_lists.py` rewrites
  every list under `--out-dir` — diff before committing.
- If something couldn't be verified (no display, submodule not initialized,
  no build), report that explicitly instead of reporting it as done.

## Commits and branches

- `git commit -s` (DCO); subject ≤50 chars, imperative, no trailing period;
  body explains why/what, not how.
- Branch prefixes: `feat/ fix/ docs/ style/ refactor/ chore/`.
- Never push directly or force-push to `master` — land changes through a PR.

## Maintaining this file

Explanations belong at their closest home — `README.md`, `docs/`, or a code
comment; this file carries only norms and pointers. When you discover a new
non-obvious invariant, document it at its home first, and add a one-line
norm here only if it keeps tripping people up. When code referenced here
changes, update the pointer in the same change.
