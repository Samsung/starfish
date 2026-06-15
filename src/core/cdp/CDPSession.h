/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPSession__)
#define __StarfishCDPSession__

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Starfish {

// An isolated world created via Page.createIsolatedWorld. Recorded so its
// Runtime.executionContextCreated can be re-emitted after each navigation
// (puppeteer creates the utility world once and expects the browser to
// re-announce it on navigate; it never re-calls createIsolatedWorld).
struct IsolatedWorld {
    uint32_t contextId;
    std::string worldName;
    std::string uniqueId;
    // The frameId this world belongs to (main frame by default). For a child
    // iframe's utility world this is the child's frameId, and childPath is the
    // ordinal path (see ChildFrame::path) so evaluations route to its context.
    std::string frameId;
    bool isChild = false;
    std::vector<uint32_t> childPath;
};

// A child iframe frame discovered after a navigation, anywhere in the frame
// tree (direct child or deeper descendant). Each child BrowsingContext
// (HTMLIFrameElement contentDocument) gets a stable frameId and a distinct
// Runtime executionContextId, keyed by its ordinal path. The live
// BrowsingContext* is NOT stored (CDPSession is plain heap, not GC-rooted); it
// is re-resolved by walking the tree (iterateChildContext, recursively) each
// time, following the path. The path is the sequence of document-order indices
// (among iframes with a child BC) from the main document down to this frame:
// e.g. {0} is the first child iframe of the main frame, {0,1} is the second
// child iframe of that frame (a grandchild of main).
struct ChildFrame {
    std::vector<uint32_t> path; // ordinal path from main frame to this frame
    uint32_t contextId;         // Runtime executionContextId for this frame
    std::string frameId;        // CDP frameId
    std::string parentFrameId;  // CDP frameId of the immediate parent frame
    bool attachEmitted;         // Page.frameAttached already emitted
};

// A script registered via Page.addScriptToEvaluateOnNewDocument. Evaluated in
// the main world at the start of each new document, before the page's own
// inline scripts run.
struct EvaluateOnNewDocumentScript {
    std::string identifier;
    std::string source;
};

// An IO stream opened via a "ReturnAsStream" command (e.g. Page.printToPDF).
// Holds the full payload; IO.read serves it in chunks and IO.close drops it.
struct IOStream {
    std::vector<uint8_t> data;
    size_t offset = 0;
};

// A top-level navigation paused by Fetch.enable. The navigate handler emits
// Fetch.requestPaused and parks the load here instead of running it inline; the
// matching Fetch.continueRequest/fulfillRequest/failRequest resumes it. Single
// thread, so each of these arrives as its own message loop turn (no blocking).
struct PendingFetchNavigation {
    bool active = false;
    std::string requestId; // the Fetch interception id (== networkRequestId)
    std::string url;       // the URL navigate() was asked to load
    std::string sessionId; // session the navigation belongs to
};

// A resource seen on the wire by the ResourceLoader network hook. Recorded in
// document order (first response wins per requestId) so Page.getResourceTree
// can list the frame's resources and Page.getResourceContent can map a URL back
// to its captured body (networkBodies[requestId]).
struct ResourceRecord {
    std::string url;
    std::string requestId; // key into networkBodies / networkBodyBase64
    std::string type;      // CDP resource type: Document/Script/Stylesheet/...
    std::string mimeType;
};

// CDP session state. Main-thread only. GC: not inherited (plain members).
class CDPSession {
public:
    CDPSession()
        : sessionId()
        , targetId("TID-0000000001")
        , browserContextId("BID-0000000001")
        , frameId("TID-0000000001")
        , loaderId("LID-0000000001")
    {
    }

    // enable flags
    bool pageEnabled = false;
    bool runtimeEnabled = false;
    bool domEnabled = false;
    bool logEnabled = false;
    bool networkEnabled = false;
    bool cssEnabled = false;
    bool domSnapshotEnabled = false;
    bool accessibilityEnabled = false;
    bool targetDiscoverEnabled = false;
    bool targetAutoAttach = false;
    bool lifecycleEventsEnabled = false;
    bool interceptFileChooserDialog =
        false;                 // Page.setInterceptFileChooserDialog
    bool fetchEnabled = false; // Fetch.enable (request interception active)
    bool performanceEnabled = false;         // Performance.enable
    bool performanceTimelineEnabled = false; // PerformanceTimeline.enable
    bool auditsEnabled = false;              // Audits.enable
    bool animationEnabled = false;           // Animation.enable
    bool overlayEnabled = false;             // Overlay.enable
    bool profilerEnabled = false;            // Profiler.enable
    bool heapProfilerEnabled = false;        // HeapProfiler.enable
    bool debuggerEnabled = false;            // Debugger.enable

    // Profiler.start state. Escargot exposes no CPU sampling profiler through
    // its public API, so no samples are ever collected; start records a start
    // timestamp and stop returns a minimal-but-valid CPUProfile (single (root)
    // node, empty samples). startUs is longTickCount() (microseconds) at
    // Profiler.start, used for the profile's startTime/endTime.
    bool profilerRecording = false;
    uint64_t profilerStartUs = 0;
    // Profiler.startPreciseCoverage state. No coverage instrumentation exists,
    // so takePreciseCoverage/getBestEffortCoverage always report an empty
    // result.
    bool preciseCoverageActive = false;

    // Overlay.setInspectMode: the requested inspect mode string
    // ("searchForNode"/"none"/...). Headless has no visual overlay or pointer
    // hit-testing, so this is recorded only (no Overlay.inspectNodeRequested is
    // emitted); kept so a set->state round-trips for clients that probe it.
    std::string overlayInspectMode = "none";

    // Tracing.start state. The engine has no real trace recorder; start records
    // the requested transfer mode and a start timestamp, end synthesizes a
    // minimal set of Chrome trace events (see TracingDomain).
    bool tracingActive = false;
    std::string tracingTransferMode; // "ReturnAsStream" or "ReportEvents"
    uint64_t tracingStartTickUs = 0; // longTickCount() at Tracing.start

    // Animation.setPlaybackRate: stored playback rate reported back by
    // getPlaybackRate. The engine drives animations off the real wall clock
    // (tickCount) with no global time-scale hook, so this rate is recorded but
    // not applied to the running engine (see AnimationDomain).
    double animationPlaybackRate = 1.0;
    // Monotonic counter for the synthetic Animation ids handed to clients in
    // Animation.animationCreated/animationStarted events.
    uint32_t animationCounter = 0;

    // fixed identifiers (single-target MVP)
    std::string sessionId;        // "SID-..." issued on attach
    std::string targetId;         // == frameId
    std::string browserContextId; // "BID-..."
    std::string frameId;          // == targetId
    std::string loaderId;         // "LID-0000000001"
    bool attached = false;
    bool attachEmitted = false; // Target.attachedToTarget emitted once

    uint32_t executionContextId = 1; // main world context id
    // Isolated worlds (Page.createIsolatedWorld) get distinct context ids so
    // puppeteer's utility world resolves; all ids route to the single real
    // Escargot context (no true world isolation in this engine).
    uint32_t nextIsolatedContextId = 100;
    // Isolated worlds created on this session, re-announced on navigate.
    std::vector<IsolatedWorld> isolatedWorlds;
    // Child iframe frames discovered after navigation. Reset on cross-document
    // navigation and rebuilt from the live BrowsingContext tree.
    std::vector<ChildFrame> childFrames;
    // contextId base for child frames (kept above isolated worlds' range).
    uint32_t nextChildContextId = 2000;
    uint32_t childFrameCounter = 0; // monotonic for child frameId issuing
    // World names registered via Page.addScriptToEvaluateOnNewDocument with a
    // worldName (puppeteer's utility world). Each new document/frame
    // auto-creates a context for these names; for child frames we emit an
    // isolated-world executionContextCreated bound to the child frameId so
    // puppeteer's per-frame utility world resolves without an explicit
    // createIsolatedWorld call.
    std::vector<std::string> newDocumentWorldNames;
    // Scripts registered via Page.addScriptToEvaluateOnNewDocument, evaluated
    // (in order) at the start of every new document.
    std::vector<EvaluateOnNewDocumentScript> evaluateOnNewDocumentScripts;
    // Names registered via Runtime.addBinding. Each injects a window[name]
    // native function that emits Runtime.bindingCalled when invoked.
    // Re-injected into the main world after every navigation.
    std::vector<std::string> bindings;
    uint32_t evaluateOnNewDocumentCounter = 0; // monotonic for identifiers
    uint32_t sessionCounter = 0;               // monotonic for SID issuing
    // Browser contexts created via Target.createBrowserContext, tracked on the
    // initial (connection-level) session only. The default context (this
    // session's own browserContextId) is NOT listed here, matching Chrome's
    // Target.getBrowserContexts which excludes the default. Targets created
    // with Target.createTarget({browserContextId}) record that id on their
    // session; disposeBrowserContext closes them. NOTE: starfish has a single
    // shared cookie/storage context, so these ids provide grouping/lifecycle
    // only -- there is no real cookie/storage isolation between contexts.
    std::vector<std::string> browserContexts;
    uint32_t browserContextCounter =
        1;                      // monotonic for "BID-N" issuing (1 = default)
    uint32_t loaderCounter = 1; // monotonic for loaderId issuing
    // IO streams keyed by handle (Page.printToPDF transferMode:ReturnAsStream).
    std::map<std::string, IOStream> ioStreams;
    uint32_t ioStreamCounter = 0;     // monotonic for IO stream handle issuing
    uint32_t fetchRequestCounter = 0; // monotonic for Fetch interception ids
    // A navigation parked by Fetch.enable awaiting continue/fulfill/fail.
    PendingFetchNavigation pendingFetchNav;

    // requestId of the most recent navigation document request. For HTTP loads
    // this is set by the real ResourceLoader hook (== loaderId, matching
    // Chrome). For data:/synthetic loads it is set by emitNavigation.
    // Network.getResponseBody returns the stored real bytes for this id when
    // available, else serializes the live document.
    std::string lastNavigationRequestId;
    // Headers from Network.setExtraHTTPHeaders. Injected into real outgoing
    // ResourceLoader requests via the network hook (Resource::request).
    std::map<std::string, std::string> extraHTTPHeaders;

    // Real response bodies captured by the ResourceLoader network hook, keyed
    // by requestId. getResponseBody serves these. base64 flag per id marks
    // non-text payloads (e.g. images).
    std::map<std::string, std::string> networkBodies;
    std::map<std::string, bool> networkBodyBase64;
    // POST request bodies captured by the ResourceLoader network hook, keyed by
    // requestId. Network.getRequestPostData returns these. Only non-empty
    // bodies are stored (a GET / bodyless request leaves no entry, matching
    // Chrome, which errors getRequestPostData for requests without a body).
    std::map<std::string, std::string> networkRequestPostData;
    uint32_t networkRequestCounter = 0; // monotonic for subresource REQ-n ids
    // Resources seen on the wire, in arrival order, for Page.getResourceTree /
    // Page.getResourceContent. Cleared on cross-document navigation.
    std::vector<ResourceRecord> resources;

    // Network.emulateNetworkConditions: when offline is true the ResourceLoader
    // hook fails every real http(s) request (document + subresource) with
    // ERR_INTERNET_DISCONNECTED. latency/throughput are stored only (no
    // throttling implemented). Default offline=false leaves loading untouched.
    bool networkOffline = false;
    double networkLatency = 0;
    double networkDownloadThroughput = -1;
    double networkUploadThroughput = -1;
    // Network.setBlockedURLs: wildcard ('*') patterns. A real request whose URL
    // matches any pattern is failed with ERR_BLOCKED_BY_CLIENT. Empty by
    // default.
    std::vector<std::string> networkBlockedUrls;
    // Network.setCacheDisabled: when true the process HTTPCache is switched to
    // LOAD_NO_CACHE so real loads bypass the disk cache; restored to
    // LOAD_DEFAULT when re-enabled. Tracks the last requested state for
    // idempotence.
    bool networkCacheDisabled = false;

    // --- networkidle lifecycle (real in-flight tracking) ----------------
    // Number of real ResourceLoader requests (document + subresources)
    // currently in flight for this target, maintained by NetworkDomain's
    // request hook
    // (+1 on requestWillBeSent, -1 on loadingFinished/loadingFailed). When the
    // count crosses the networkidle thresholds a debounce timer is armed; after
    // 500ms of quiescence the matching Page.lifecycleEvent fires. This drives
    // page.goto(waitUntil:'networkidle0'/'networkidle2') from actual network
    // activity instead of a synthetic emit. HTTP navigations rely on this;
    // data: /about:/unknown schemes keep the synthetic immediate idle in
    // PageDomain.
    int networkInFlight = 0;
    // Debounce timer id (Timer::addTimer), TimerInvalidID when none is armed.
    size_t networkIdleTimerId = SIZE_MAX;
    // Already-emitted-this-navigation guards (reset on each new navigation) so
    // a given idle level is announced once per document.
    bool networkAlmostIdleEmitted = false;
    bool networkIdleEmitted = false;

    // --- Runtime.compileScript results -----------------------------------
    // A script compiled+persisted via
    // Runtime.compileScript(persistScript:true). Escargot's public ScriptRef is
    // GC-managed and CDPSession is plain heap (not GC-rooted), so we keep the
    // source expression string rather than the compiled ScriptRef;
    // Runtime.runScript re-evaluates it in the main world (identical result).
    // compileScript still parses up-front so syntax errors are reported there.
    // Keyed by "script-N".
    std::map<std::string, std::string> compiledScripts;
    uint32_t compiledScriptCounter = 0;     // monotonic for "script-N" ids
    uint32_t debuggerBreakpointCounter = 0; // monotonic for Debugger "bp-N" ids

    // --- DOM.performSearch results ---------------------------------------
    // Each DOM.performSearch stores its matched nodeIds under a "search-N"
    // searchId; DOM.getSearchResults serves ranges from it and
    // DOM.discardSearchResults drops it. nodeIds are issued via NodeRegistry.
    std::map<std::string, std::vector<int>> searchResults;
    uint32_t searchCounter = 0; // monotonic for "search-N" ids

    // --- Page.startScreencast state --------------------------------------
    // When active, a repetitive Timer (screencastTimerId) periodically captures
    // the renderer framebuffer and emits Page.screencastFrame. The frame number
    // (screencastSessionId) is sent as the frame's sessionId; the client echoes
    // it back via Page.screencastFrameAck. Backpressure: a new frame is only
    // emitted while screencastAcked is true (the previous frame was acked); the
    // first frame is sent unconditionally. format/quality come from
    // startScreencast (quality is jpeg-only; jpeg falls back to PNG where
    // libjpeg is absent, like captureScreenshot).
    bool screencastActive = false;
    size_t screencastTimerId = SIZE_MAX; // Timer::addTimer id, SIZE_MAX = none
    std::string screencastFormat = "png";
    int screencastQuality = 80;
    int screencastEveryNthFrame = 1;
    int screencastSessionId = 0; // monotonic frame number
    bool screencastAcked = true; // previous frame acknowledged (backpressure)
};

} // namespace Starfish

#endif
