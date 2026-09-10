# CDP test harness structure

What each part of `tool/cdp_test` does, and what talks to what.

## One run

`run.py` is one Python process. It reads the test list, decides what to run,
and gives entries to worker threads. Below a worker everything is a child
process: one Starfish per entry, and one Node as well for a behavior entry.

```text
                run.py                 one process owns the
                        │                     list, workers and report
     ┌──────────────────┴──────────────────┐
     ▼                                     ▼
  worker thread                       worker thread    --workers N
     │                                     │
     ├─ schema entry                       ├─ behavior entry
     │    case.py                          │    behavior.py
     │      │ CDP request                  │      │ spawns
     │      ▼                              │      ▼
     │  client.py ──ws──┐                  │   node main.mjs
     │                  │                  │      │ websocket.mjs
     │                  ▼                  │      ▼
     └──────────► Starfish ◄───────────────┴─── ws://127.0.0.1:PORT
                  own port
                  own HOME
```

A worker takes one entry and stays with it until it ends. Starfish starts for
that entry and stops when the entry ends. A browser that hangs therefore
cannot affect the next entry.

## Why use Node.js to run the behavior tests

The runner is Python. Node is here because the behavior tests are Chromium's
`.js` files. We do not change those files, so we need a tool that runs
JavaScript. If we rewrote them in Python, we would also decide what each test
checks. Chromium decided that already.

## Files

| File | What it does |
| --- | --- |
| `run.py` | Command line, layer choice, worker pool, progress and stats |
| `driver/testlist.py` | Reads and writes a list, and locks it for one run |
| `driver/launcher.py` | Finds a build, starts and stops Starfish, cleans up |
| `driver/client.py` | HTTP discovery, a WebSocket, and one CDP call |
| `driver/commands.py` | Turns the protocol JSON into the schema layer's cases |
| `driver/case.py` | Runs one schema case: send, then check the response |
| `driver/behavior.py` | Runs one behavior test under Node and diffs output |
| `driver/node/main.mjs` | Loads Chromium's harness and the test under Node |
| `driver/node/websocket.mjs` | The WebSocket the Node side speaks |

No file here imports Starfish. The tests reach the build only through the CDP
endpoint, like any other client.

## What a worker does

Both layers start a browser the same way. They differ in who checks the
result.

**schema**, in Python:

1. `launcher` starts Starfish and waits for the endpoint to answer.
2. `client` opens the WebSocket and sends the command with no parameters.
3. `case` checks that every field the protocol requires is in the response.

**behavior**, in Node:

1. `launcher` starts Starfish and waits for the endpoint to answer.
2. `behavior` reads the browser WebSocket URL and starts
   `node main.mjs <ws-url> <suite-dir> <test.js> <timeout>`.
3. Node runs Chromium's test file and prints its log.
4. `behavior` compares that log with the test's `-expected.txt`. It reports
   the first line that differs.

The Node side has its own notes in
[driver/node/README.md](driver/node/README.md).

## Test data

The test cases are not written here. They come from the `test` submodule, and
each is fixed to one upstream revision.

| Path | Source | Used by |
| --- | --- | --- |
| `test/cdp/protocol/` | ChromeDevTools/devtools-protocol | schema |
| `test/cdp/inspector-protocol/` | chromium/chromium | behavior |

Each directory has a `revision.txt`. A run writes that revision into the
list, so you can tell which upstream a list was built from.

## State a run keeps

| Where | What | Removed |
| --- | --- | --- |
| `testlist-<layer>.json` | The list, and the last result of each entry | Kept: it is the output |
| `testlist-<layer>.json.lock` | The pid that holds the list | When the run ends |
| `testlist-<layer>.json.<pid>.partial` | The save in progress | On every save |
| `.tmp/test-cdp/<pid>/<random>/` | One browser's HOME: cookies, storage, cache | When the entry ends |

The list is saved after every entry. A run that is killed therefore keeps its
results so far. The other files are temporary. Ctrl-C leaves a lock file and a
storage directory behind, and the next run deletes the ones a dead pid owned.

The private homes and both sweeps come from
[`tool/common/storage.py`](../common/storage.py), which the CLI suite uses
too. The owner stamp there is one name for every suite, so whichever suite
runs next also clears what an interrupted run of the other one left.

See [TESTLIST.md](TESTLIST.md) for the fields inside a list.
