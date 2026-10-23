# Reading a test list

```json
{
  "layer": "behavior",
  "revision": "8c1e8c78de78a9a20327bc2e631cf9726ca37bdc",
  "entries": {
    "access-inspected-object.js": {
      "expected": "fail",
      "category": "timeout"
    },
    "cpu-profiler/record-cpu-profile-with-cpu-throttling.js": {
      "expected": "pass"
    }
  }
}
```

| Field | Meaning |
| --- | --- |
| `revision` | the upstream commit the entries were built against |
| `expected` | `pass` runs every time. `fail` runs only with `--include-fails` |
| `suite` | `internal` identifies a Starfish-owned behavior test |
| `category` | how the failure was grouped, from what the run saw. Never from the expected output |
| `detail` | schema only. The error the run saw, cut at 1000 characters |

## Categories

| Category | What the run saw |
| --- | --- |
| `timeout` | the run ran out of time while the test was waiting |
| `output-differs` | the output came, and differs from `-expected.txt` |
| `not-implemented` | the command returned `-32601` |
| `bad-params` | the command returned `-32602` |
| `unprobeable` | a prior schema pass rejected the no-parameter request |
| `protocol-error` | a schema command returned another CDP error code |
| `schema-mismatch` | a successful schema response omitted a required field |
| `probe-error` | the schema probe could not obtain or parse a response |
| `no-connection` | the WebSocket did not open |
| `script-error` | the test script threw, so it never finished |
| `harness-error` | Node itself failed, so the test never ran |

`output-differs` takes every failure the categories above did not match, so
its entries have no one cause. Run the entry to see the first line that
differs.

A behavior entry keeps no detail. The first line that differs moves with
every change to the output, so the list would change in every patch that
touches the output, even when the result stays the same.

## Running one entry

Name it, or give a shell pattern:

```sh
tool/cdp_test/run.py behavior access-inspected-object.js
tool/cdp_test/run.py behavior 'dom/resolve-node-*.js'
```

A named entry runs whatever the list expects of it, and a run given a pattern
leaves the list alone.
