# Starfish CDP — Domain Coverage

Status of every Chrome DevTools Protocol domain in Starfish's CDP server.
Legend: ✅ implemented (real behavior) · 🟡 partial / stub · ❌ not implemented
(unknown methods are acked with an empty result so client handshakes proceed).

See [CDP.md](CDP.md) for the per-method table of the implemented domains.

## Implemented domains (49)

| Domain | Status | Notes |
| ------ | ------ | ----- |
| **Target** | ✅ | discovery, attach, multi-tab `createTarget`/`closeTarget`, `attachToBrowserTarget` (Playwright `connectOverCDP`) |
| **Page** | ✅ | navigate, lifecycle, history, dialogs, screenshot, printToPDF (paginated), screencast, captureSnapshot(MHTML), resource tree, setDocumentContent, setBypassCSP |
| **Runtime** | ✅ | evaluate/callFunctionOn/getProperties (objectId handles), addBinding, getHeapUsage, globalLexicalScopeNames |
| **DOM** | ✅ | tree, query, mutation, box model, file input, search, focus |
| **Input** | ✅ | mouse, key, touch, drag(ack), insertText, IME composition |
| **Network** | ✅ | **real** request/response/body (loader-hooked), cookies, headers, offline, blocked URLs, postData |
| **Fetch** | ✅ | request interception (continue/fulfill/fail) via deferred navigation |
| **Emulation** | ✅ | device metrics, UA, geolocation, emulated media (prefers-color-scheme) |
| **CSS** | ✅ | computed/inline/matched styles, stylesheet text |
| **DOMStorage** | ✅ | local/session storage items |
| **Storage** | ✅ | cookies, clearDataForOrigin, storage key |
| **Accessibility** | ✅ | getFullAXTree (synthesized from DOM) |
| **Security** | ✅ | setIgnoreCertificateErrors |
| **Performance** | ✅ | getMetrics (heap, nodes, documents) |
| **PerformanceTimeline** | 🟡 | stub — engine has only mark/measure/resource PerformanceEntry and no PerformanceObserver/LayoutShift/LCP source; enable stores the eventTypes filter + acks; `timelineEventAdded` never emitted (nothing observable feeds it) |
| **Profiler** | 🟡 | stub — Escargot exposes no CPU sampler/coverage instrumentation; enable/disable/start/setSamplingInterval ack; stop → minimal-but-valid CPUProfile (single `(root)` node, no samples); coverage methods → valid empty results; never throws on the client; no real samples/coverage ever produced |
| **Audits** | 🟡 | stub — no audit engine; enable/disable/checkContrast/checkFormsIssues ack with valid results; `getEncodedResponse` returns only `originalSize` (no re-encode); `issueAdded` never emitted |
| **Log** | ✅ | entryAdded |
| **IO** | ✅ | read/close (printToPDF / Tracing streams) |
| **Browser** | ✅ | getVersion |
| **Schema** | ✅ | getDomains |
| **Inspector** | 🟡 | enable/disable only; no `targetCrashed`/`detached` events. A future crash/detach hook would emit from `CDPDispatcher::onConnectionClosed()` (called by `CDPServer.cpp:164` on connection teardown) or an engine crash callback |
| **SystemInfo** | ✅ | getInfo / getProcessInfo (real pid, uname) |
| **Memory** | ✅ | getDOMCounters, forciblyPurgeJavaScriptMemory |
| **HeapProfiler** | 🟡 | collectGarbage runs a **real** Boehm GC (GC_gcollect, like Memory.forciblyPurgeJavaScriptMemory); no V8 snapshot serializer/allocation tracker/sampler — takeHeapSnapshot/stopTrackingHeapObjects ack with no chunks/events; stop/getSamplingProfile → minimal-but-valid SamplingHeapProfile (single (root) node, empty samples); getObjectByHeapObjectId/getHeapObjectId → -32000 (no heap-id map); enable/disable/start*/addInspectedHeapObject ack |
| **WebAuthn** | 🟡 | **real** in-memory CRUD registry of virtual authenticators + credentials (addVirtualAuthenticator mints `authenticator-N` & stores; add/get/remove/clearCredential(s) genuinely mutate & read back; unknown id → -32000 with Chrome's messages). Engine has no navigator.credentials/PublicKeyCredential, so the registry is inspectable/manipulable over CDP but NOT wired to any real WebAuthn ceremony — no authenticator is ever consulted, no credentialAdded/Asserted events; setUserVerified/setAutomaticPresenceSimulation/setResponseOverrideBits store but are inert |
| **WebAudio** | 🟡 | stub — engine has a WebAudio module (`src/core/modules/webaudio/`) but it is gated behind STARFISH_ENABLE_WEBAUDIO and NOT wired to CDP (no AudioContext/BaseAudioContext registry reachable from the inspector); enable/disable ack; getRealtimeData → -32000 "Cannot find BaseAudioContext with such id" (Chrome's exact unknown-id message; every contextId is unknown); no contextCreated/audioNode*/audioParam*/listener events emitted |
| **Media** | 🟡 | stub — reporting-only domain (enable/disable + an event stream); no media-player introspection reachable from CDP (no player registry/metrics surface the dispatcher can read), so enable/disable ack and no playersCreated/playerPropertiesChanged/playerEventsAdded/playerMessagesLogged/playerErrorsRaised events are emitted; no synthetic playerId fabricated |
| **DOMSnapshot** | ✅ | captureSnapshot / getSnapshot (flattened nodes+layout+styles) |
| **DOMDebugger** | 🟡 | getEventListeners (real); breakpoints acked |
| **Animation** | 🟡 | animationCreated/Started events (real, @keyframes); playback rate stored, not applied |
| **Tracing** | 🟡 | start/end/stream (synthetic minimal trace events) |
| **Overlay** | 🟡 | getHighlightObjectForTest (real); visual highlights acked (headless) |
| **DeviceOrientation** | 🟡 | override stored/acked (engine has no DeviceOrientationEvent) |
| **CacheStorage** | 🟡 | stub — engine Cache API is SW-scope only, not reachable; returns empty: no caches/entries; deletes ack; requestCachedResponse → not-found |
| **IndexedDB** | 🟡 | stub — engine IDB is SW/worker-scope only, gated behind STARFISH_ENABLE_IDB (off) with no enumeration API, not reachable from main-frame; enumeration empty; requestDatabase → empty schema (v1, no stores); requestData → no entries (hasMore:false); getMetadata → zeros; mutations + enable/disable ack |
| **ServiceWorker** | 🟡 | stub — engine runs SW in a separate worker host (gated behind STARFISH_WEBWORKER_HOST), whose registry is not reachable from the CDP main-frame context; enable streams no registrations; every command (unregister, updateRegistration, start/stop/stopAllWorkers, skipWaiting, setForceUpdateOnPageLoad, inspectWorker, deliverPushMessage, dispatch{,Periodic}SyncEvent) acks as idempotent no-op |
| **LayerTree** | 🟡 | stub — no CDP-reachable compositor / render-layer tree (no layer or snapshot registry); enable/disable ack; compositingReasons → empty arrays (no layer composited); makeSnapshot → -32000 (no layer registry); load/profile/replay/snapshotCommandLog → -32000 (no snapshot registry); releaseSnapshot ack (no-op); no layerTreeDidChange/layerPainted events emitted; no layers/snapshots fabricated |
| **Preload** | 🟡 | stub — no speculation-rules parser / prefetch / prerender engine reachable from CDP (no rule-set registry, no preloading-attempt source); enable/disable ack; no ruleSetUpdated/ruleSetRemoved/preloadEnabledStateUpdated/prefetchStatusUpdated/prerenderStatusUpdated/preloadingAttemptSourcesUpdated events emitted; no rule sets / attempts / statuses fabricated |
| **EventBreakpoints** | 🟡 | stub — arms instrumentation breakpoints (pause on named engine events) but the protocol is 3 void/ack methods with no getter, so the armed set is unobservable over CDP; pauses arrive via Debugger.paused, which is not CDP-wired (Escargot debugger not wired to CDP); setInstrumentationBreakpoint/removeInstrumentationBreakpoint/disable ack; eventName NOT stored (unobservable + inert = dead state); breakpoints accepted but never fire; no events in this domain; nothing fabricated |
| **BackgroundService** | 🟡 | stub — surfaces background-service activity (backgroundFetch/backgroundSync/pushMessaging/notifications/periodicBackgroundSync), all of which live in the ServiceWorker host that is compiled out (SERVICE_WORKER=0; see iter3) ⇒ no reachable activity; startObserving/stopObserving/setRecording/clearEvents ack; service/shouldRecord NOT stored (no activity to gate = dead state); no recordingStateChanged/backgroundServiceEventReceived events emitted (no SW host to source them); nothing fabricated |
| **Autofill** | 🟡 | stub — drives address/payment autofill (trigger {fieldId,frameId?,card}/setAddresses {addresses}/enable/disable); engine has NO autofill engine (no address/credit-card profile store, no field-recognition pass, no autofill driver reachable from CDP); all four ack; addresses NOT stored (no engine to offer them, no getter = dead state); trigger fills nothing (no driver/form pipeline); no addressFormFilled event emitted (no engine to source a fill); nothing fabricated |
| **FedCm** | 🟡 | stub — drives the FedCm account-chooser dialog (enable {disableRejectionDelay?}/disable/resetCooldown + dialog-id ops selectAccount/clickDialogButton/dismissDialog/openUrl); engine has NO FedCm engine (no navigator.credentials/IdentityCredential, no account-chooser dialog, no dialog-id registry reachable from CDP), so no dialog can ever be shown; enable/disable/resetCooldown ack (disableRejectionDelay NOT stored = dead state; resetCooldown no-op); the four dialog-id ops → -32000 "Dialog not found" (every dialogId unknown; reject rather than falsely ack); no dialogShown/dialogClosed events emitted (no engine to source a dialog); nothing fabricated |
| **Database** | 🟡 | stub — inspects legacy WebSQL databases (enable/disable + getDatabaseTableNames {databaseId}/executeSQL {databaseId,query}; event addDatabase); WebSQL is deprecated/removed and engine has NO WebSQL engine (no openDatabase/Database/SQLTransaction, no databaseId registry reachable from CDP), so no database can ever be opened; enable/disable ack (no flag stored = dead state); getDatabaseTableNames + executeSQL → -32000 "Database not found" (every databaseId unknown; reject rather than return empty tableNames or fabricate a sqlError); no addDatabase event emitted (no engine to source a database); nothing fabricated |
| **DeviceAccess** | 🟡 | stub — drives the device-chooser prompt for WebBluetooth/WebUSB/WebSerial/WebHID (enable/disable + prompt-id ops selectPrompt {id,deviceId}/cancelPrompt {id}; event deviceRequestPrompted); engine has NO device-chooser engine (no WebBluetooth/WebUSB/WebSerial/WebHID picker, no prompt-id registry reachable from CDP), so no prompt can ever be raised; enable/disable ack (no flag stored = dead state); selectPrompt + cancelPrompt → -32000 "Prompt not found" (every id unknown; reject rather than falsely ack); no deviceRequestPrompted event emitted (no engine to source a prompt); nothing fabricated |
| **Cast** | 🟡 | stub — drives the Cast/Presentation/Media Router UI: sink discovery + tab/desktop mirroring (enable {presentationUrl?}/disable + sink ops setSinkToUse/startDesktopMirroring/startTabMirroring/stopCasting {sinkName}; events sinksUpdated/issueUpdated); engine has NO Presentation/Cast/Media Router engine (no mDNS/DIAL discovery, no navigator.presentation, no mirroring pipeline, no sink registry reachable from CDP — the DIAL/SSDP CastServer under modules/cast is a receiver wired only into the ServiceWorker agent, gated by STARFISH_ENABLE_CAST_SERVICE), so no sink can ever exist; enable/disable ack (presentationUrl not stored = dead state); the four sink ops → -32000 "Sink not found" (every sinkName unknown; reject rather than falsely ack); no sinksUpdated/issueUpdated event emitted (no engine to source a sink or issue); nothing fabricated |
| **Tethering** | 🟡 | stub — reverse port forwarding: tunnel a host port back to the device over the DevTools connection (bind {port}/unbind {port}; event accepted {port,connectionId}); Starfish's CDP transport is a plain JSON channel with NO tunnel/socket-multiplexer/host-port listener/connectionId registry reachable from CDP, so a bound port can never accept a connection; bind/unbind ack (port not stored = dead state, no getter, nothing consumes it); no accepted event emitted (no tunnel to source a connection); nothing fabricated |
| **Extensions** | 🟡 | stub — unpacked-extension dev flow + extension-storage inspection (loadUnpacked {path}→{id}/uninstall {id}/get|set|remove|clearStorageItems {id,storageArea,...}; no events); Starfish has NO extension subsystem (no loader, no extension-id registry, no chrome.storage session/local/sync/managed backing store reachable from CDP), so no extension can be loaded and no extension storage can exist; ALL six methods → -32000 "Extensions are not supported" (reject rather than mint a fake id, ack a no-op, or return empty {data}); nothing fabricated |
| **Debugger** | 🟡 | handshake/shape stub — Escargot's debugger is NOT CDP-wired (separate remote TCP protocol), so breakpoints are accepted but NEVER FIRE (no pause path). enable→{debuggerId:"-1"}; setBreakpointsActive/setSkipAllPauses/setPauseOnExceptions/setAsyncCallStackDepth/setBlackboxPatterns/setBlackboxedRanges + resume/pause/step*/continueToLocation/removeBreakpoint→ack (all inert/no-op); setBreakpointByUrl→{breakpointId:"bp-N",locations:[]} (accepted, binds to nothing, never fires); setBreakpoint→-32000 "Could not resolve breakpoint" (no location index); getScriptSource→{scriptSource} served from the Runtime.compileScript(persistScript:true) registry (CDPSession::compiledScripts, REAL source) or -32000 "No script for id <id>"; getPossibleBreakpoints→{locations:[]}; evaluateOnCallFrame→-32000 "Can only perform operation while paused" (never paused); NO events (no scriptParsed/paused/resumed — compileScript is a pull registry, not a per-parse hook); nothing fabricated |

## Not implemented / planned (❌, acked)

| Domain | Use | Feasibility on Starfish |
| ------ | --- | ----------------------- |
| **Console** | (deprecated, use Runtime/Log) | ❌ acked |

## Notes

- Unknown domains/methods return an **empty success result** (not an error), so
  drivers that probe optional domains during connect keep working.
- "Real" Network events require the HTTP(S) loader path; `data:`/`about:` use a
  synthetic event triple.
- Pixel output (screenshot/printToPDF/screencast) is real only on a
  `*_cairo_gl` backend; `*_headless` (Mock) yields structurally-valid blank
  output.
