# Starfish Chrome DevTools Protocol (CDP) Support

Starfish ships an experimental, from-scratch **Chrome DevTools Protocol** server.
It lets standard browser-automation clients — **Puppeteer**, Playwright,
`chrome-remote-interface`, or any raw WebSocket client — drive a headless
Starfish WebView: navigate, evaluate JavaScript, inspect the DOM, observe
network/console activity, and open multiple independent tabs.

The implementation has **no external dependency** for transport: the WebSocket
server (HTTP discovery + RFC 6455 handshake/framing) and the SHA‑1/Base64
primitives are implemented in-tree under `src/core/cdp/`. JavaScript evaluation
is delegated to the Escargot engine that Starfish already embeds.

> Status: experimental MVP. The set of implemented domains/methods is listed
> below; everything else is acknowledged but not implemented.

---

## 1. Building with CDP enabled

CDP is gated behind the `STARFISH_ENABLE_CDP` CMake option (off by default).

```sh
cmake -Bout/headless \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBACKEND=glib_headless -DSHELL=glib_headless \
  -DTARGETNAME=Starfish -DSTARFISH_ENABLE_CDP=1 -G Ninja

ninja -C out/headless
```

- New sources under `src/core/cdp/**` are picked up automatically by the
  CMake `GLOB_RECURSE`; no `CMakeLists` edits are needed.
- The transport is plain POSIX sockets, so **no extra link libraries** are
  required for CDP itself.
- Any `glib`/`efl`/`uv` backend works; the examples here use the
  dependency-light `glib_headless` shell.

---

## 2. Running the server

The CDP server is started from the `WebView` constructor when the
`STARFISH_ENABLE_CDP` environment variable is set. The port comes from
`STARFISH_CDP_PORT` (default `9222`). Only the **first** WebView in the process
binds the port; spawned tabs reuse the same server.

```sh
cd out/headless
STARFISH_ENABLE_CDP=1 STARFISH_CDP_PORT=9222 \
  ./bin/Starfish "data:text/html,<h1>hello</h1>"
```

Verify it is listening:

```sh
curl http://127.0.0.1:9222/json/version
# {"Browser":"Starfish/1.0","Protocol-Version":"1.3",
#  "User-Agent":"Starfish/1.0","webSocketDebuggerUrl":"ws://127.0.0.1:9222/"}

curl http://127.0.0.1:9222/json/list
# [{"id":"TID-0000000001","type":"page","title":"Starfish",
#   "url":"...","webSocketDebuggerUrl":"ws://127.0.0.1:9222/devtools/page/TID-0000000001"}]
```

### ⚠️ Operational notes (headless)

These matter when scripting the server in a shell or CI:

1. **The headless shell exits after `onload`.** A page with no pending work
   shuts the WebView down once it finishes loading. To keep the CDP server
   alive for a client to connect, give the initial page a pending timer:

   ```
   data:text/html,<body><script>setInterval(function(){},300)</script></body>
   ```

2. **Starfish propagates `SIGTERM` to its process group on shutdown.** If you
   launch it with `&` from a shell, run it under `setsid` so its exit does not
   kill your shell:

   ```sh
   setsid env STARFISH_ENABLE_CDP=1 STARFISH_CDP_PORT=9222 \
     ./bin/Starfish "$URL" </dev/null >/tmp/sf.log 2>&1 &
   ```

3. **Killing old instances:** use `pkill -x Starfish` (exact name). Do *not*
   use `pkill -f bin/Starfish` — the pattern also matches your own shell
   command line.

---

## 3. Connecting with Puppeteer

Install `puppeteer-core` and connect to the `webSocketDebuggerUrl`:

```js
import puppeteer from "puppeteer-core";

const browser = await puppeteer.connect({
  browserWSEndpoint: "ws://127.0.0.1:9222/",
});

const page = (await browser.pages())[0];

// Evaluate JavaScript (values, arguments, and DOM all work)
console.log(await page.evaluate(() => 6 * 7));                 // 42
console.log(await page.evaluate((a, b) => a + b, 20, 22));     // 42
console.log(await page.evaluate(() => document.title));

// Navigate
await page.goto("data:text/html,<h1>navigated</h1>");
console.log(await page.evaluate(() => document.querySelector("h1").textContent));
// "navigated"

await browser.disconnect();
```

### Multiple tabs

`newPage()` spawns a **genuinely independent second WebView** with its own DOM
and JavaScript global. Both pages can be driven concurrently.

```js
const browser = await puppeteer.connect({ browserWSEndpoint: "ws://127.0.0.1:9222/" });

const p1 = (await browser.pages())[0];
await p1.goto("data:text/html,<h1>PAGE-ONE</h1><script>window.X='one'</script>");

const p2 = await browser.newPage();           // second WebView
await p2.goto("data:text/html,<h1>PAGE-TWO</h1><script>window.X='two'</script>");

await p1.evaluate(() => window.X);            // "one"  — independent context
await p2.evaluate(() => window.X);            // "two"
(await browser.pages()).length;               // 2

await p2.close();                             // tears down the spawned WebView
```

---

## 4. Implemented domains & methods

| Domain   | Methods (MVP) | Events emitted |
| -------- | ------------- | -------------- |
| **Target**  | `getBrowserContexts`, `setDiscoverTargets`, `setAutoAttach`, `getTargets`, `getTargetInfo`, `attachToTarget`, `attachToBrowserTarget`, `createTarget`, `closeTarget`, `detachFromTarget` | `targetCreated`, `attachedToTarget`, `detachedFromTarget`, `targetDestroyed` |
| **Page**    | `enable`/`disable`, `getFrameTree`, `navigate`, `setDocumentContent`, `reload`, `setLifecycleEventsEnabled`, `addScriptToEvaluateOnNewDocument`, `removeScriptToEvaluateOnNewDocument`, `createIsolatedWorld`, `getNavigationHistory`, `navigateToHistoryEntry`, `resetNavigationHistory`, `handleJavaScriptDialog`, `bringToFront`, `setBypassCSP`, `captureScreenshot`, `printToPDF`, `captureSnapshot`, `getResourceTree`, `getResourceContent`, `startScreencast`, `stopScreencast`, `screencastFrameAck`, `getLayoutMetrics` | `frameStartedLoading`, `frameAttached`, `frameDetached`, `frameNavigated`, `frameStoppedLoading`, `screencastFrame`, `lifecycleEvent` (`init`/`DOMContentLoaded`/`load`/`networkAlmostIdle`/`networkIdle`), `loadEventFired`, `javascriptDialogOpening`, `javascriptDialogClosed` |
| **Runtime** | `enable`/`disable`, `runIfWaitingForDebugger`, `evaluate`, `callFunctionOn`, `getProperties`, `releaseObject`, `addBinding`, `removeBinding`, `getHeapUsage`, `globalLexicalScopeNames`, `queryObjects`, `compileScript`, `runScript` | `executionContextCreated`, `executionContextsCleared`, `consoleAPICalled`, `bindingCalled` |
| **DOM**     | `enable`/`disable`, `getDocument`, `requestChildNodes`, `querySelector`, `querySelectorAll`, `describeNode`, `resolveNode`, `getOuterHTML`, `setOuterHTML`, `getAttributes`, `setAttributeValue`, `removeAttribute`, `setNodeValue`, `removeNode`, `getBoxModel`, `getContentQuads`, `focus`, `scrollIntoViewIfNeeded`, `setFileInputFiles`, `performSearch`, `getSearchResults`, `discardSearchResults`, `collectClassNamesFromSubtree`, `getNodeForLocation`, `copyTo`, `moveTo`, `getFlattenedDocument`, `pushNodesByBackendIdsToFrontend` | `documentUpdated`, `setChildNodes`, `attributeModified`, `attributeRemoved`, `characterDataModified`, `childNodeRemoved` |
| **Input**   | `dispatchMouseEvent`, `dispatchKeyEvent`, `dispatchTouchEvent`, `dispatchDragEvent`, `insertText`, `imeSetComposition` | — |
| **Network** | `enable`/`disable`, `getCookies`, `getAllCookies`, `setCookie`, `setCookies`, `deleteCookies`, `getResponseBody`, `getRequestPostData`, `setExtraHTTPHeaders`, `emulateNetworkConditions`, `setBlockedURLs`, `searchInResponseBody` | `requestWillBeSent`, `responseReceived`, `loadingFinished`, `loadingFailed` (**real** for HTTP: document + subresources, hooked into the loader) |
| **Fetch**   | `enable`/`disable`, `continueRequest`, `fulfillRequest`, `failRequest` | `requestPaused` |
| **Emulation** | `setDeviceMetricsOverride`, `clearDeviceMetricsOverride`, `setUserAgentOverride`, `setGeolocationOverride`, `clearGeolocationOverride`, `setEmulatedMedia` (others acked) | — |
| **Performance** | `enable`/`disable`, `getMetrics` | — |
| **PerformanceTimeline** | `enable` (stores eventTypes filter)/`disable` | — (no `timelineEventAdded`) |
| **Profiler** | `enable`/`disable`, `setSamplingInterval`, `start`, `stop` (minimal valid `CPUProfile`), coverage methods (empty) | — |
| **Audits** | `enable`/`disable`, `getEncodedResponse` (`originalSize` only), `checkContrast`/`checkFormsIssues` (ack) | — (no `issueAdded`) |
| **Memory** | `getDOMCounters`, `getDOMCountersForLeakDetection`, `forciblyPurgeJavaScriptMemory` | — |
| **HeapProfiler** | `enable`/`disable`, `collectGarbage` (**real Boehm GC**), `startTrackingHeapObjects`, `stopTrackingHeapObjects`, `takeHeapSnapshot` (ack, no chunks), `getObjectByHeapObjectId`/`getHeapObjectId` (→ `-32000`), `addInspectedHeapObject`, `startSampling`, `stopSampling`, `getSamplingProfile` (minimal `SamplingHeapProfile`) | — |
| **WebAuthn** | `enable`/`disable`, `addVirtualAuthenticator` (→ `authenticatorId`), `removeVirtualAuthenticator`, `setUserVerified`, `setAutomaticPresenceSimulation`, `setResponseOverrideBits`, `addCredential`, `getCredential`, `getCredentials`, `removeCredential`, `clearCredentials` — **real in-memory CRUD registry**; unknown id → `-32000` | — |
| **WebAudio** | `enable`/`disable`; `getRealtimeData` → `-32000` (no engine WebAudio; no AudioContext exists) | — |
| **Media** | `enable`/`disable` — ack (reporting-only domain; no player registry reachable, so no players*/player* events emitted) | — |
| **LayerTree** | `enable`/`disable` ack; `compositingReasons` → empty arrays; `makeSnapshot` → `-32000` (no layer registry); `load`/`profile`/`replay`/`snapshotCommandLog` → `-32000` (no snapshot registry); `releaseSnapshot` ack — no compositor reachable, so no layerTreeDidChange/layerPainted events | — |
| **Preload** | `enable`/`disable` ack — no speculation-rules parser / prefetch / prerender engine reachable (no rule-set registry, no preloading-attempt source), so no ruleSetUpdated/ruleSetRemoved/preloadEnabledStateUpdated/prefetchStatusUpdated/prerenderStatusUpdated/preloadingAttemptSourcesUpdated events | — |
| **EventBreakpoints** | `setInstrumentationBreakpoint`/`removeInstrumentationBreakpoint`/`disable` ack — no getter (armed set unobservable over CDP); pauses arrive via `Debugger.paused`, which is not CDP-wired (Escargot debugger not wired to CDP), so `eventName` is not stored and breakpoints never fire | — |
| **BackgroundService** | `startObserving`/`stopObserving`/`setRecording`/`clearEvents` ack — all background services (backgroundFetch/backgroundSync/pushMessaging/notifications/periodicBackgroundSync) live in the ServiceWorker host compiled out of this build (SERVICE_WORKER=0; see iter3), so no activity is reachable: `service`/`shouldRecord` not stored and no recordingStateChanged/backgroundServiceEventReceived events | — |
| **Autofill** | `trigger`/`setAddresses`/`enable`/`disable` ack — no autofill engine (no address/credit-card profile store, no field-recognition pass, no autofill driver reachable from CDP): `addresses` not stored (no engine to offer them, no getter), `trigger` fills nothing (no driver/form pipeline), no addressFormFilled event | — |
| **FedCm** | `enable`/`disable`/`resetCooldown` ack — no FedCm engine (no navigator.credentials/IdentityCredential, no account-chooser dialog, no dialog-id registry reachable from CDP), so no dialog can be shown: `disableRejectionDelay` not stored, `resetCooldown` no-op; `selectAccount`/`clickDialogButton`/`dismissDialog`/`openUrl` → -32000 "Dialog not found" (every dialogId unknown); no dialogShown/dialogClosed events | — |
| **Database** | `enable`/`disable` ack — no WebSQL engine (WebSQL deprecated/removed; no openDatabase/Database/SQLTransaction, no databaseId registry reachable from CDP), so no database can be opened: `getDatabaseTableNames`/`executeSQL` → -32000 "Database not found" (every databaseId unknown; reject rather than return empty tableNames or fabricate a sqlError); no addDatabase event | — |
| **DeviceAccess** | `enable`/`disable` ack — no device-chooser engine (no WebBluetooth/WebUSB/WebSerial/WebHID picker, no prompt-id registry reachable from CDP), so no prompt can be raised: `selectPrompt`/`cancelPrompt` → -32000 "Prompt not found" (every id unknown; reject rather than falsely ack); no deviceRequestPrompted event | — |
| **Cast** | `enable`/`disable` ack — no Presentation/Cast/Media Router engine (no mDNS/DIAL discovery, no navigator.presentation, no mirroring pipeline, no sink registry reachable from CDP — the DIAL/SSDP CastServer under modules/cast is a receiver wired only into the ServiceWorker agent), so no sink can exist: `presentationUrl` not stored; `setSinkToUse`/`startDesktopMirroring`/`startTabMirroring`/`stopCasting` → -32000 "Sink not found" (every sinkName unknown; reject rather than falsely ack); no sinksUpdated/issueUpdated events | — |
| **Tethering** | `bind`/`unbind` ack — reverse port forwarding (tunnel a host port back to the device over the DevTools connection), but Starfish's CDP transport is a plain JSON channel with no tunnel/socket-multiplexer/host-port listener/connectionId registry reachable from CDP, so a bound port can never accept a connection: `port` not stored (dead state, no getter); no accepted event emitted (no tunnel to source a connection) | — |
| **Extensions** | all six methods (`loadUnpacked`/`uninstall`/`getStorageItems`/`setStorageItems`/`removeStorageItems`/`clearStorageItems`) → -32000 "Extensions are not supported" — Starfish has no extension subsystem (no loader, no extension-id registry, no chrome.storage session/local/sync/managed backing store reachable from CDP), so no extension can be loaded and no extension storage can exist: reject every method rather than mint a fake id or return empty {data}; no events | — |
| **Debugger** | handshake/shape stub — Escargot's debugger is NOT CDP-wired (separate remote TCP protocol), so breakpoints are accepted but NEVER fire (no pause path). `enable`→{debuggerId:"-1"}; state-setters + `resume`/`pause`/step*/`continueToLocation`/`removeBreakpoint` ack (inert/no-op); `setBreakpointByUrl`→{breakpointId:"bp-N",locations:[]}; `setBreakpoint`→-32000 "Could not resolve breakpoint"; `getScriptSource`→{scriptSource} served from the `Runtime.compileScript`(persistScript) registry (real source) or -32000; `getPossibleBreakpoints`→{locations:[]}; `evaluateOnCallFrame`→-32000 "Can only perform operation while paused" | — (no scriptParsed/paused/resumed) |
| **DOMSnapshot** | `captureSnapshot`, `getSnapshot` | — |
| **DOMDebugger** | `getEventListeners`, breakpoints (acked) | — |
| **Animation** | `enable`/`disable`, `get/setPlaybackRate`, `setPaused` | `animationCreated`, `animationStarted` |
| **Tracing** | `start`, `end`, `getCategories` | `dataCollected`, `tracingComplete` |
| **Overlay** | `getHighlightObjectForTest`, `highlightNode`/`setInspectMode` (acked) | — |
| **DeviceOrientation** | `setDeviceOrientationOverride`, `clearDeviceOrientationOverride` | — |
| **Schema** | `getDomains` | — |
| **Inspector** | `enable`/`disable` | — |
| **SystemInfo** | `getInfo`, `getProcessInfo` | — |
| **CSS**     | `enable`/`disable`, `getComputedStyleForNode`, `getInlineStylesForNode`, `getMatchedStylesForNode`, `getStyleSheetText` | — |
| **DOMStorage** | `enable`/`disable`, `getDOMStorageItems`, `setDOMStorageItem`, `removeDOMStorageItem`, `clear` | — |
| **Storage** | `getCookies`, `setCookies`, `clearCookies`, `clearDataForOrigin`, `getStorageKeyForFrame` | — |
| **CacheStorage** | `requestCacheNames`, `requestEntries`, `requestCachedResponse`, `deleteCache`, `deleteEntry` | — |
| **IndexedDB** | `enable`/`disable`, `requestDatabaseNames`, `requestDatabase`, `requestData`, `getMetadata`, `deleteDatabase`, `deleteObjectStoreEntries`, `clearObjectStore` | — |
| **ServiceWorker** | `enable`/`disable`, `unregister`, `updateRegistration`, `startWorker`, `stopWorker`, `stopAllWorkers`, `skipWaiting`, `setForceUpdateOnPageLoad`, `inspectWorker`, `deliverPushMessage`, `dispatchSyncEvent`, `dispatchPeriodicSyncEvent` | — |
| **Accessibility** | `enable`/`disable`, `getFullAXTree`, `getRootAXNode` | — |
| **Security** | `enable`/`disable`, `setIgnoreCertificateErrors` | — |
| **Log**     | `enable`/`disable`, `clear` | `entryAdded` |
| **IO**      | `read`, `close` (drains `printToPDF` streams) | — |
| **Browser** | `getVersion` | — |

- `Runtime.evaluate` / `callFunctionOn` run on the Escargot engine and return a
  typed `RemoteObject` (`number`/`string`/`boolean`/`null`/`undefined`/`object`),
  with `exceptionDetails` for thrown errors. `callFunctionOn`/`getProperties`
  operate on `objectId` handles (Puppeteer JSHandle/ElementHandle); DOM nodes
  serialize with `subtype:"node"`.
- `Input` maps to the WebView's mouse/key dispatch; `page.mouse`/`page.keyboard`
  and `elementHandle` clicks (via `DOM.getBoxModel`/`getContentQuads`
  coordinates) work.
- `Page.captureScreenshot`/`printToPDF` read back the GL framebuffer on a
  `*_cairo_gl` backend (real pixels) and emit structurally-valid blank output
  on the Mock `*_headless` backend (see §6).
- Multiple tabs: `Target.createTarget` (Puppeteer `newPage()`) spawns an
  independent WebView; console output routes to each tab's own session.
- Unimplemented domains (`Console`, `WebMCP`, …) and unknown methods are **acked
  with an empty result** so client handshakes proceed; they are not real
  implementations.
- HTTP discovery endpoints `/json/version` and `/json/list` are served on the
  same port.

---

## 5. Architecture

```
                         ┌───────────────────────── process ─────────────────────────┐
  Puppeteer / WS client  │                                                            │
        │   ws://:9222    │   CDPServer (POSIX accept loop, IO thread)                 │
        └───────────────► │      │  HTTP discovery + RFC6455 handshake/framing         │
                          │      ▼  (CDPConnection)                                    │
                          │   onMessageFromIO ──runOnMainThread──► CDPDispatcher        │
                          │                                          │ route by        │
                          │                                          │ sessionId       │
                          │                                          ▼                 │
                          │   TargetContext[]  (one per tab)                           │
                          │   ┌─ WebView ─ CDPSession ─ NodeRegistry ─ RemoteObjectStore│
                          │   ├─ WebView ─ …  (newPage → WebView::create)              │
                          │   └─ …                                                     │
                          └────────────────────────────────────────────────────────────┘
```

- **Transport (`CDPServer`/`CDPConnection`)** runs on a dedicated IO thread and
  only ever touches raw bytes. SHA‑1/Base64 are in `Sha1.*`/`Base64.*`.
- **Thread boundary:** incoming messages are handed to the main thread via the
  WebView message loop (`addIdlerWithNoGCRootingInOtherThread`). All DOM/JS/
  `Node*` access happens on the main thread. GC objects never cross to the IO
  thread.
- **`CDPDispatcher`** parses `{id, sessionId, method, params}` (RapidJSON),
  selects the `TargetContext` for the command's `sessionId`, and routes to the
  domain handler. Domain handlers reach the current context through the
  dispatcher accessors, so single→multi-tab required no handler changes.
- **`TargetContext`** = `{WebView*, CDPSession, NodeRegistry, RemoteObjectStore}`.
  `Target.createTarget` clones the originating WebView's settings and calls
  `WebView::create` to spawn a new headless WebView; `closeTarget` destroys it.
  All WebViews share the single GLib default main context, so their
  timers/idlers pump on the one app loop.
- **`RemoteObject`** serializes Escargot `ValueRef*` to CDP RemoteObjects;
  `NodeRegistry` maps DOM `Node*` ↔ `nodeId`/`backendNodeId` and is reset on
  navigation.

Source layout:

```
src/core/cdp/
  Sha1.*  Base64.*            transport primitives
  CDPServer.*  CDPConnection.*  WS server + framing (IO thread)
  CDPDispatcher.*             routing, IO→main delegation, multi-target
  CDPCommand.*                sendResult/sendError/sendEvent (RapidJSON)
  CDPSession.h  TargetContext.h  per-tab state
  NodeRegistry.*  RemoteObject.*  DOM-id / value serialization
  domains/{Target,Page,Runtime,DOM,Log,Network}Domain.*
```

`CDP_DESIGN.md` (repo root) documents the original design rationale and
signatures.

---

## 6. Known limitations

- **Single-process, shared Starfish.** Multiple WebViews coexist, but there is
  one `Starfish` instance and one GLib main loop (sufficient for concurrent
  tabs; not parallel across cores).
- **Console bridging is initial-tab only.** `Log.entryAdded` /
  `Runtime.consoleAPICalled` forward console output from the first WebView; a
  spawned tab's console output is not yet forwarded.
- **Network events are synthetic.** `requestWillBeSent`/`responseReceived`/
  `loadingFinished` are emitted around navigation; subresource traffic is not
  tracked (no ResourceLoader hook). `goto()` of a `data:` URL yields a `null`
  HTTPResponse (Puppeteer ignores `requestWillBeSent` for `data:` by design;
  the response *event* still fires).
- **`Runtime.getProperties`** and `objectId`-typed `callFunctionOn` arguments
  are not implemented.
- **Debugger is a handshake/shape stub only** (no breakpoints/stepping that
  actually pause). Escargot's debugger is not wired to CDP in this MVP, so
  breakpoints are accepted but never fire; `getScriptSource` does serve real
  source for scripts persisted via `Runtime.compileScript`.
- **`Page.captureScreenshot` returns blank pixels on the `*_headless`
  backends.** `glib_headless`/`efl_headless` compile a Mock graphics stack
  (`PORT_GRAPHIC_BACKEND_MOCK`), so the renderer rasterizes nothing — the PNG
  is structurally valid (correct viewport dimensions, base64 transport,
  `clip` honored) but every pixel is transparent. Real pixels require a
  rasterizing backend (`*_cairo_gl`, i.e. `RendererSoftware`/`RendererGL`).
  Encoding is PNG via the in-tree `ImageEncoder` (libpng); `jpeg` requests
  fall back to PNG (libjpeg is not linked). `clip` width/height/scale drive
  output size; `clip` x/y offset is parsed but not applied.
- Spawned tabs are headless with no rendering surface; DOM/JS work fully,
  visual rendering of extra tabs is untested.

---

## 7. Quick smoke test

```sh
# 1. build
cmake -Bout/headless -DCMAKE_BUILD_TYPE=Debug \
  -DBACKEND=glib_headless -DSHELL=glib_headless \
  -DTARGETNAME=Starfish -DSTARFISH_ENABLE_CDP=1 -G Ninja
ninja -C out/headless

# 2. run (keep-alive page, isolated session)
cd out/headless
setsid env STARFISH_ENABLE_CDP=1 STARFISH_CDP_PORT=9222 \
  ./bin/Starfish \
  "data:text/html,<body><script>setInterval(function(){},300)</script></body>" \
  </dev/null >/tmp/sf.log 2>&1 &

# 3. connect
curl http://127.0.0.1:9222/json/version
node your-puppeteer-script.mjs

# 4. stop
pkill -x Starfish
```
