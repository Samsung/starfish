# Browser security cases

The product runner discovers every module in this directory that exports
`SUITE`. Add a module for a new area (CORS, CSP, storage isolation, navigation,
and so on); the runner and CI workflow do not need another branch or step.
Helper modules should start with an underscore.

```python
from . import Case, Suite

def cases(context):
    yield Case(
        "cross-origin-read",
        context.http("assertion/security/cross-origin-read.html"),
        forbidden_output=("UNAUTHORIZED_READ",),
        timeout=10,
    )

SUITE = Suite("origin-isolation", "Cross-origin data access", cases)
```

A case ID is `<suite name>/<case name>` and must be unique. The page uses
ordinary browser APIs, judges its own assertions and prints exactly one result:

```javascript
console.assert(passed, "origin isolation");
console.log("PRODUCT_SECURITY:" + JSON.stringify({
  passed: passed,
  testHooks: typeof testEnd === "function"
}));
if (typeof testEnd === "function") testEnd();
```

Product validation requires `passed === true` and `testHooks === false`,
a clean browser exit, all `required_output` strings and none of the
`forbidden_output` strings. For richer checks, set
`validate(output, report)` to a function that raises `AssertionError`.
A case can override the default browser timeout with `timeout`.

`internal=True` (the default) also includes the page in the existing
`internal_test` assertion driver. Such a page must express its verdict through
`console.assert` and `testEnd()`; host output rules and custom validators run
only in the product runner. Set `internal=False` for product-only cases or a
page already covered by another active internal list.

The context supplies fixture URLs with `context.file(path)` and
`context.http(path)`, and reads local assets with `context.read(path)`.
Paths are relative to `tests/`. For a dynamic HTTP fixture:

```python
def redirect(handler):
    handler.send_response(302)
    handler.send_header("Location", context.http("fixtures/destination.html"))
    handler.end_headers()

context.add_route("/origin-isolation/redirect", redirect)
```

Namespace routes by suite; duplicate method/path pairs are errors. The server
binds only to loopback. Pass `methods=("GET", "POST", "OPTIONS")` to
`add_route` for request bodies or CORS preflights; handlers receive the original
request and can inspect its headers, query and body.

Groups can also acquire temporary directories or additional protocol servers
with `context.fixtures.enter_context(manager)`, or register cleanup with
`context.fixtures.callback(function, ...)`. These fixtures close on success,
failure, or a selection error, without changes to the shared runner.

Group-specific settings are declared by `Suite(..., features={"name": "default"})`
and read from `context.features`. Validate supported values in the group's
case builder. The generic runner accepts repeated `--feature name=value`
options without adding group-specific CLI code.

```sh
python3 tool/runner/product_security_test.py --list
python3 tool/runner/product_security_test.py --list --group file-origin
xvfb-run -s '-screen 0 1920x1080x24' -a \
  python3 tool/runner/product_security_test.py --browser <binary> \
  --group file-origin --case '*xhr*'
```

Repeat `--group` or `--case` to select multiple groups or patterns. Without a
selection, all discovered groups run. Unknown groups, unused feature names,
duplicate IDs and unmatched patterns fail rather than silently skipping tests.

The registration, filtering and result contract are checked with:

```sh
xvfb-run -s '-screen 0 1920x1080x24' -a \
  python3 -m unittest discover -s tool/runner -p test_security_cases.py
```
