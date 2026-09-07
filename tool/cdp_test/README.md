# CDP test harness

`run.py` tests the Chrome DevTools Protocol (CDP) endpoint of a Starfish
build. It starts the binary, connects over HTTP discovery and a WebSocket,
and reads the responses.

Two sets of test cases, called layers in `run.py`.

- `schema` (`test/cdp/protocol/`): checks a response for the fields the
  protocol declares required.
- `behavior` (`test/cdp/inspector-protocol/`): runs Chromium's own CDP test
  and compares its output.

## Prerequisites

- Starfish built with `STARFISH_ENABLE_CDP=1`
- Python 3
- Node 18 or later, for `behavior` tests.

## Run

```console
$ tool/cdp_test/run.py all

...

schema: 43 passed, 0 failed, 42 skipped of 85 (50.6%)
behavior: 21 passed, 0 failed, 940 skipped of 961 (2.2%)
total: 64 passed, 0 failed, 982 skipped of 1046 (6.1%)
```

```sh
run.py behavior                    # one layer
run.py behavior 'dom/*.js'         # one directory
run.py behavior --include-fails    # every entry, so one can be promoted
```

`run.py <layer> --help` documents every option and the model the lists follow.

## Test lists

Each layer keeps one file. It is both the run list and the result of the last
run, so a skip carries the reason it was skipped.

- [`testlist-schema.json`](testlist-schema.json)
- [`testlist-behavior.json`](testlist-behavior.json)

```json
{
  "layer": "schema",
  "revision": "ea39a11d80de9a08ce2af03f52125ed2e462cf84",
  "counts": { "total": 661, "pass": 43, "fail": 42 },
  "entries": {
    "Browser.getVersion": { "expected": "pass" },
    "Accessibility.queryAXTree": {
      "expected": "fail",
      "category": "not-implemented",
      "detail": "Accessibility.queryAXTree returned {'code': -32601, ...}"
    }
  }
}
```

[TESTLIST.md](TESTLIST.md) explains the fields.

## Update the protocol and tests

A revision is a commit sha or a tag. The current pin is in each
`revision.txt`; `git ls-remote <upstream-url> HEAD` gives the latest.

```sh
# The CDP protocol, for the schema layer.
# Upstream: https://github.com/ChromeDevTools/devtools-protocol
python3 test/cdp/update-protocol.py --revision <cdp-revision>
python3 tool/cdp_test/run.py schema --include-fails

# Chromium's tests, for the behavior layer.
# Upstream: https://github.com/chromium/chromium
python3 test/cdp/update-inspector-protocol.py --revision <chromium-sha>
python3 tool/cdp_test/run.py behavior --include-fails
```

Every run rewrites the list. `--include-fails` also runs the entries
expected to fail, which is what lets one be promoted back.

| Result | Line written |
| --- | --- |
| pass | active |
| fail | skip, with the reason |

A regression (an entry that used to pass and now fails) is written as a skip,
but the run still counts it as failed and exits non-zero.

## More

- [STRUCTURE.md](STRUCTURE.md): what each file does and what talks to what.
