# The Node side

The Chromium tests are JavaScript, so `behavior.py` starts Node once per test.

```text
behavior.py
  │   node main.mjs <ws-url> <suite-dir> <test.js> <target-base-url> [timeout]
  ▼
main.mjs                                   ours
  ├── session_fanout.mjs                   ours
  ├── inspector-protocol-test.js           Chromium
  └── <test.js>                            Chromium
        │
        ▼
   websocket.mjs                           ours
        │   ws://127.0.0.1:PORT
        ▼
    Starfish                               the tested build

stdout: the log, compared with <test>-expected.txt
```

`main.mjs` fills four seams the Chromium harness was written to take:

| Seam | Filled with |
| --- | --- |
| `DevToolsHost.sendMessageToEmbedder` | hand to `websocket.mjs` |
| `DevToolsAPI._log` | collect into an array |
| `DevToolsAPI._completeTest` | resolve, the run ends |
| `DevToolsAPI._fetch` | read harness helper scripts off disk |

`<ws-url>` is `webSocketDebuggerUrl` from `/json/version`. The suite makes its
own target, so the browser endpoint is required and a page endpoint fails.
`<target-base-url>` is the test directory below the behavior server's HTTP
origin. It is separate from the local file URL used to load harness helpers, so
`TestRunner.url()` navigates Starfish to resources the browser can fetch.

Log formatting runs inside Chromium's code before `_log`, so the compared text
is Chromium's own.
