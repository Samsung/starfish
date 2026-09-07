# Reading a test list

```json
{
  "layer": "behavior",
  "revision": "8c1e8c78de78a9a20327bc2e631cf9726ca37bdc",
  "counts": { "total": 961, "pass": 21, "fail": 940 },
  "entries": {
    "access-inspected-object.js": {
      "expected": "fail",
      "category": "timeout",
      "detail": "line 2: expected '{', got 'HARNESS TIMEOUT'"
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
| `counts.total` | every case upstream has. For schema, every CDP command |
| `expected` | `pass` runs every time. `fail` runs only with `--include-fails` |
| `category` | how the failure was grouped, from what the run saw. Never from the expected output |
| `detail` | the first line that differs, cut at 1000 characters |

## Categories

| Category | What the run saw |
| --- | --- |
| `timeout` | the run ran out of time while the test was waiting |
| `output-differs` | the output came, and differs from `-expected.txt` |
| `not-implemented` | the command returned `-32601` |
| `bad-params` | the command returned `-32602` |
| `no-connection` | the WebSocket did not open |
| `script-error` | the test script threw, so it never finished |
| `harness-error` | Node itself failed, so the test never ran |
| `failed` | the schema layer's only category. The response failed the check |

`output-differs` takes every failure the categories above did not match, so
its entries have no one cause. Read the detail.

## Running one entry

Name it, or give a shell pattern:

```sh
tool/cdp_test/run.py behavior access-inspected-object.js
tool/cdp_test/run.py behavior 'dom/resolve-node-*.js'
```

A named entry runs whatever the list expects of it, and a run given a pattern
leaves the list alone.
