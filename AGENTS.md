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
- When an AI coding agent authored or materially contributed to a commit,
  add a `Co-Authored-By: <Agent Name> <email>` trailer identifying it,
  placed before the `Signed-off-by` line — regardless of which agent tool
  was used.
- Branch prefixes: `feat/ fix/ docs/ style/ refactor/ chore/`.
- Never push directly or force-push to `master` — land changes through a PR.

## Code review checkers

Severity mapping for the AI review bot (Review Quality Agent), which reads
this file as the repository rule. The bot comments only on findings it
judges Major or Critical. The norms in the sections above apply as written;
the checker below is pinned explicitly so it is always enforced.

### Checker

#### 1) Test Coverage
- Test Coverage : A behavior change (new feature, bug fix) must land
  together with tests that cover it
- Test Coverage : Enabling a feature through an `.idl` change or
  implementing a new web spec (DOM, CSS, or another web API) must activate
  the relevant web-platform-tests in the `.res` lists. The lists are
  curated per spec area, so the needed list may not exist yet — in that
  case a new `.res` file must be seeded (see `docs/wpt.md`), not skipped.
  When WPT doesn't cover the behavior, an internal test must be added
  instead
- Test Coverage : When flagging missing coverage, search the pinned WPT
  corpus (`third_party/wpt/`) for tests relevant to the change and
  recommend them concretely (spec directory and test files) in the review
  comment

#### 2) Memory Efficiency
- Memory Efficiency : Low runtime memory usage is this engine's core
  constraint. Even when the code is functionally correct, actively propose
  concrete ways to reduce runtime memory — a leaner data structure or
  container choice, avoiding unnecessary copies or caching, allocating
  lazily, shrinking per-instance footprint of frequently-instantiated
  classes
- Memory Efficiency : A memory saving must not degrade rendering
  performance or responsiveness — don't propose trading speed on hot paths
  (layout, paint, style resolution, event handling) for memory; when the
  two conflict, flag the trade-off instead of picking a side

#### 3) Web Standards Compliance
- Web Standards Compliance : Review behavior changes against the relevant
  web spec (WHATWG/W3C/ECMA-262) and flag behavior that contradicts the
  spec it implements
- Web Standards Compliance : Apply this with pragmatic compromise — full
  spec coverage is often impossible under this engine's constraints, and
  features land incrementally. A deliberately scoped partial implementation
  or a stepwise landing is acceptable and should not be flagged as a
  violation; what matters is that the implemented part behaves per spec and
  that intentional deviations are visible (a why-comment citing the spec)
  rather than silent

#### 4) Security
- Security : Treat web content, network data, files, and inputs crossing
  script/native or process boundaries as attacker-controlled. Flag reachable
  paths to memory corruption, code or command injection, path traversal, or
  unauthorized data access or modification
- Security : Preserve browser security boundaries, including same-origin
  checks, CORS, CSP, cookie and storage scoping, permissions, and TLS
  certificate verification. Flag bypasses, fail-open behavior, or weaker
  defaults. Explicit developer or embedder opt-outs are acceptable only when
  they remain opt-in and cannot be enabled by web content

### Severity
- Critical : Security
- Major : Test Coverage, Memory Efficiency, Web Standards Compliance
- Minor : None

## Maintaining this file

Explanations belong at their closest home — `README.md`, `docs/`, or a code
comment; this file carries only norms and pointers. When you discover a new
non-obvious invariant, document it at its home first, and add a one-line
norm here only if it keeps tripping people up. When code referenced here
changes, update the pointer in the same change. Note this file has two
consumers: coding agents working in the repo, and the AI review bot, which
applies it as the repository rule on every PR (see Code review checkers).

This file is the shared baseline. Personal or machine-specific instructions
(your build directory, your workflow) belong in an untracked local overlay
— e.g. `CLAUDE.local.md`, `.clinerules/` — which is git-ignored, must not
contradict this file, and whose rules move here once they turn out to be
team-wide.
