# 07 - Resources

> **Relevant source files**
> - `README.md`
> - `AGENTS.md`
> - `docs/Spec.md`
> - `docs/wpt.md`
> - `docs/Coding_Style_Guide.md`

## Documentation

| Document | Location | Purpose |
|----------|----------|---------|
| README.md | `README.md` | Build, run, test instructions |
| AGENTS.md | `AGENTS.md` | Coding rules, conventions, review checkers |
| Spec.md | `docs/Spec.md` | Supported web surface (HTML tags, DOM, CSS, flags) |
| WPT Guide | `docs/wpt.md` | WPT structure, tooling, `.res` list workflow |
| Coding Style | `docs/Coding_Style_Guide.md` | C++ style guidelines |
| CDP Design | `CDP_DESIGN.md` | Chrome DevTools Protocol design |

`AGENTS.md documentation map`

## Testing Resources

### Test Framework
- **Runner:** `./tool/runner/test_runner.py`
- **Display:** `xvfb-run -s '-screen 0 1920x1080x24' -a` (required for all test runs)

`README.md:295-311`, `AGENTS.md`

### Test Suites

| Suite | Command | Notes |
|-------|---------|-------|
| All tests | `test_runner.py` | Full suite |
| DOM Conformance | `test_runner.py dom_conformance` | DOM API tests |
| WPT | `test_runner.py wpt_all` | Web Platform Tests |
| WPT (CSS) | `test_runner.py wpt_[css_css21\|css_backgrounds\|...]` | CSS sub-suites |
| Vendor | `test_runner.py vendor_test` | Blink/WebKit/Gecko tests |
| Bidi | `test_runner.py bidi_test` | Bidirectional text tests |
| Internal | `test_runner.py internal_test` | Internal tests (`console.assert` + `testEnd()`) |

`README.md:307-329`

### Test Suite Mapping

| Touched Area | Run |
|--------------|-----|
| Any C++ | `./tool/lint/check_tidy.py` |
| Contract ABI | `./tool/lint/check_contract_abi.py` |
| DOM APIs | `wpt_serve_testharness_dom internal_test` |
| CSS/selectors | `wpt_serve_testharness_css` |
| HTML parsing | `wpt_serve_testharness_html` |
| Layout/paint | `wpt_serve_reftest` or `reftest_all` |
| Workers | `wpt_serve_testharness_worker` |

`AGENTS.md testing table`

### WPT Resources
- Location: `test/reftest/web-platform-tests/*`
- `.res` lists: tracked known-failures (`#`-commented lines)
- `wpt_annotate.py`: write-once markers
- `wpt_manifest_lists.py`: rewrites all lists under `--out-dir`

`README.md:342-350`, `AGENTS.md`

## Third-Party Libraries

| Library | Location | Purpose |
|---------|----------|---------|
| Escargot | `third_party/escargot` | JavaScript engine |
| robin_map | `third_party/robin_map/` | Hash map (`tsl::robin_map`) |
| libtuv | `third_party/libtuv` | Event loop (libUV port) |
| Boehm GC | (linked) | Garbage collector |

`AGENTS.md`, `third_party/`

## Lint and CI Tools

| Tool | Location | Purpose |
|------|----------|---------|
| check_tidy.py | `tool/lint/` | C++ style check (PR CI) |
| check_contract_abi.py | `tool/lint/` | UWE delegate contract ABI check |
| check_render_bmp.py | `tool/ci/` | Render pixel comparison |
| imgdiff | `tool/imgdiff/` | Image diff tool |
| test_runner.py | `tool/runner/` | Main test runner |

`tool/ directory`, `AGENTS.md`

## Coding Style

- C++ style guide: `docs/Coding_Style_Guide.md`
- Key norms (from `AGENTS.md`):
  - `Optional<T>` for nullable values
  - `GCVector`/`GCTightVector` for GC-managed containers
  - Branch prefixes: `feat/ fix/ docs/ style/ refactor/ chore/`
  - `git commit -s` (DCO), subject ≤50 chars

`AGENTS.md`