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

#if defined(STARFISH_ENABLE_CDP)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "CDPDispatcher.h"
#include "CDPServer.h"
#include "CDPCommand.h"
#include "CDPSession.h"
#include "TargetContext.h"
#include "NodeRegistry.h"
#include "RemoteObject.h"
#include "Base64.h"
#include "domains/TargetDomain.h"
#include "domains/PageDomain.h"
#include "domains/RuntimeDomain.h"
#include "domains/DOMDomain.h"
#include "domains/DOMDebuggerDomain.h"
#include "domains/LogDomain.h"
#include "domains/NetworkDomain.h"
#include "domains/FetchDomain.h"
#include "domains/InputDomain.h"
#include "domains/EmulationDomain.h"
#include "domains/CSSDomain.h"
#include "domains/DOMSnapshotDomain.h"
#include "domains/DOMStorageDomain.h"
#include "domains/StorageDomain.h"
#include "domains/AccessibilityDomain.h"
#include "domains/PerformanceDomain.h"
#include "domains/MemoryDomain.h"
#include "domains/AnimationDomain.h"
#include "domains/TracingDomain.h"
#include "domains/OverlayDomain.h"
#include "core/page/WebView.h"
#include "core/page/BrowsingContext.h"
#include "binding/ScriptBindingInstance.h"
#include "core/modules/message_loop/MessageLoop.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <gc.h>
#include <unistd.h>
#include <sys/utsname.h>

namespace Starfish {

// Defined in platform/network/http/HTTPTransaction.cpp. Toggles the
// process-wide TLS verify override for CDP Security.setIgnoreCertificateErrors.
// Forward declared here to avoid pulling the network header chain into this TU.
void setGlobalIgnoreSSLVerify(bool b);

// Plain heap (no GC objects), created on IO thread, deleted on main thread.
struct CDPMessageReq {
    CDPDispatcher* dispatcher;
    std::string rawJson;
};

CDPDispatcher::CDPDispatcher(CDPServer* server, WebView* webView)
    : m_server(server)
    , m_current(nullptr)
    , m_target(new TargetDomain(this))
    , m_page(new PageDomain(this))
    , m_runtime(new RuntimeDomain(this))
    , m_dom(new DOMDomain(this))
    , m_domDebugger(new DOMDebuggerDomain(this))
    , m_log(new LogDomain(this))
    , m_network(new NetworkDomain(this))
    , m_fetch(new FetchDomain(this))
    , m_input(new InputDomain(this))
    , m_emulation(new EmulationDomain(this))
    , m_css(new CSSDomain(this))
    , m_domSnapshot(new DOMSnapshotDomain(this))
    , m_domStorage(new DOMStorageDomain(this))
    , m_storage(new StorageDomain(this))
    , m_accessibility(new AccessibilityDomain(this))
    , m_performance(new PerformanceDomain(this))
    , m_memory(new MemoryDomain(this))
    , m_animation(new AnimationDomain(this))
    , m_tracing(new TracingDomain(this))
    , m_overlay(new OverlayDomain(this))
{
    // The initial target wraps the first WebView and does not own it (the
    // shell created and owns it). Its session keeps the legacy fixed ids.
    TargetContext* ctx = createContext(webView, /*ownsWebView=*/false);
    m_current = ctx;
}

CDPDispatcher::~CDPDispatcher()
{
    while (!m_contexts.empty()) {
        destroyContext(m_contexts.back());
    }
    delete m_target;
    delete m_page;
    delete m_runtime;
    delete m_dom;
    delete m_domDebugger;
    delete m_log;
    delete m_network;
    delete m_fetch;
    delete m_input;
    delete m_emulation;
    delete m_css;
    delete m_domSnapshot;
    delete m_domStorage;
    delete m_storage;
    delete m_accessibility;
    delete m_performance;
    delete m_memory;
    delete m_animation;
    delete m_tracing;
    delete m_overlay;
}

TargetContext* CDPDispatcher::createContext(WebView* wv, bool ownsWebView)
{
    // TargetContext lives on the malloc heap; its two GC-pointer members are
    // rooted individually so the conservative GC keeps the held
    // NodeRegistry/RemoteObjectStore (and the Node*/ObjectRef* they reference)
    // alive. Roots are removed in destroyContext.
    TargetContext* ctx = new TargetContext();
    ctx->webView = wv;
    ctx->session = new CDPSession();
    ctx->nodeRegistry = new NodeRegistry();
    ctx->remoteObjectStore = new RemoteObjectStore();
    ctx->ownsWebView = ownsWebView;
    // Spawned tabs do not start their own CDP server; point them at the shared
    // (initial WebView's) server so console output reaches this dispatcher.
    if (ownsWebView && !wv->cdpServer()) {
        wv->setSharedCDPServer(m_server);
    }
    GC_add_roots(&ctx->nodeRegistry, &ctx->nodeRegistry + 1);
    GC_add_roots(&ctx->remoteObjectStore, &ctx->remoteObjectStore + 1);
    m_contexts.push_back(ctx);
    return ctx;
}

void CDPDispatcher::destroyContext(TargetContext* ctx)
{
    for (auto it = m_contexts.begin(); it != m_contexts.end(); ++it) {
        if (*it == ctx) {
            m_contexts.erase(it);
            break;
        }
    }
    GC_remove_roots(&ctx->nodeRegistry, &ctx->nodeRegistry + 1);
    GC_remove_roots(&ctx->remoteObjectStore, &ctx->remoteObjectStore + 1);
    if (m_current == ctx) {
        m_current = m_contexts.empty() ? nullptr : m_contexts.front();
    }
    delete ctx->session;
    delete ctx;
}

TargetContext* CDPDispatcher::contextForSession(const std::string& sessionId)
{
    for (TargetContext* ctx : m_contexts) {
        if (!ctx->session->sessionId.empty() &&
            ctx->session->sessionId == sessionId) {
            return ctx;
        }
    }
    return nullptr;
}

TargetContext* CDPDispatcher::contextForWebView(WebView* wv)
{
    for (TargetContext* ctx : m_contexts) {
        if (ctx->webView == wv) {
            return ctx;
        }
    }
    return nullptr;
}

void CDPDispatcher::onMessageFromIO(std::string&& rawJson)
{
    CDPMessageReq* req = new CDPMessageReq{ this, std::move(rawJson) };
    // The IO thread always hands work to the initial WebView's main-thread
    // message loop (the CDP-server-owning WebView). All WebViews share that one
    // app main thread, so this is the correct (and only) main loop.
    m_contexts.front()
        ->webView->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(
            nullptr, &CDPDispatcher::onMainTrampoline, req);
}

void CDPDispatcher::onMainTrampoline(size_t, void* data)
{
    CDPMessageReq* req = static_cast<CDPMessageReq*>(data);
    req->dispatcher->dispatchOnMain(req->rawJson);
    delete req;
}

void CDPDispatcher::onConnectionClosed()
{
    // Posted from the IO thread; the actual reset must run on the main thread
    // (CDPSession/TargetContext state is main-thread only).
    m_contexts.front()
        ->webView->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(
            nullptr, &CDPDispatcher::onConnectionClosedOnMain, this);
}

void CDPDispatcher::onConnectionClosedOnMain(size_t, void* data)
{
    static_cast<CDPDispatcher*>(data)->resetConnectionState();
}

void CDPDispatcher::resetConnectionState()
{
    // Tabs spawned by the now-departed client (Target.createTarget) belong to
    // that connection; tear them down so a reconnecting client does not see
    // stale targets.
    while (m_contexts.size() > 1) {
        TargetContext* ctx = m_contexts.back();
        WebView* wv = ctx->webView;
        bool owns = ctx->ownsWebView;
        destroyContext(ctx);
        if (owns && wv) {
            wv->destroy();
        }
    }

    // Reset the initial target's connection-scoped session so the next client
    // performs a fresh attach handshake. The attach/enable flags (notably
    // attachEmitted) are per-connection; if they leaked, setAutoAttach on the
    // reconnect would skip Target.attachedToTarget and browser.pages() would
    // return empty. A fresh CDPSession also drops isolated worlds, registered
    // new-document scripts, and open IO streams from the previous client.
    TargetContext* initial = m_contexts.front();
    // Cancel an active Page.startScreencast on the persistent initial WebView
    // before dropping its session: the repetitive capture timer is bound to the
    // WebView (which survives a reconnect), so it would otherwise keep firing
    // against freed session state.
    m_page->stopScreencast(initial->session, initial->webView);
    delete initial->session;
    initial->session = new CDPSession();
    m_current = initial;
    // Drop the flat browser-target session so a reconnecting client
    // re-attaches.
    m_browserSessionId.clear();
}

void CDPDispatcher::dispatchOnMain(const std::string& rawJson)
{
    rapidjson::Document doc;
    doc.Parse(rawJson.c_str());
    if (doc.HasParseError() || !doc.IsObject()) {
        return; // -32700 not deliverable without id; drop.
    }

    Optional<int64_t> id;
    if (doc.HasMember("id") && doc["id"].IsInt64()) {
        id = doc["id"].GetInt64();
    } else if (doc.HasMember("id") && doc["id"].IsInt()) {
        id = (int64_t)doc["id"].GetInt();
    }

    std::string sessionId;
    if (doc.HasMember("sessionId") && doc["sessionId"].IsString()) {
        sessionId = doc["sessionId"].GetString();
    }

    rapidjson::Value* params = nullptr;
    if (doc.HasMember("params") && doc["params"].IsObject()) {
        params = &doc["params"];
    }

    CDPCommand cmd(this, id, sessionId, params);

    if (!doc.HasMember("method") || !doc["method"].IsString()) {
        cmd.sendError(-32600, "'method' is missing");
        return;
    }
    std::string method = doc["method"].GetString();

    // Select the target context this command operates on. Pre-attach commands
    // (empty sessionId), connection-level Target/Browser commands, and the
    // STARTUP probe operate on the initial target. A non-empty sessionId must
    // resolve to a registered target context.
    if (sessionId.empty() || sessionId == "STARTUP") {
        m_current = initialContext();
    } else if (!m_browserSessionId.empty() && sessionId == m_browserSessionId) {
        // Flat browser-target session (Playwright connectOverCDP). Browser-
        // scoped commands (Target.*, Browser.*) routed through it operate on
        // the initial context, exactly like a connection-level command.
        m_current = initialContext();
    } else {
        TargetContext* ctx = contextForSession(sessionId);
        if (!ctx) {
            cmd.sendError(-32001, "Unknown sessionId");
            return;
        }
        m_current = ctx;
    }

    // STARTUP graceful handling (Puppeteer/Stagehand probe before attach).
    if (sessionId == "STARTUP") {
        if (method == "Page.getFrameTree") {
            m_page->processMessage(cmd, "getFrameTree");
        } else {
            cmd.sendResultEmpty();
        }
        return;
    }

    size_t dot = method.find('.');
    if (dot == std::string::npos) {
        cmd.sendError(-32601, "'method' wasn't found");
        return;
    }
    std::string domain = method.substr(0, dot);
    std::string methodName = method.substr(dot + 1);

    route(cmd, domain, methodName);
}

// CDP DeviceOrientation override state. The starfish engine does not implement
// the DeviceOrientationEvent (Document::createEvent reports it unsupported and
// no sensor source dispatches it), so there is no consumer that can be driven
// from this override. The override is therefore stored as process-static state
// and reported back so DevTools/puppeteer clients that call
// setDeviceOrientationOverride / clearDeviceOrientationOverride get valid
// results; it does not fire a deviceorientation event.
static struct {
    bool active = false;
    double alpha = 0;
    double beta = 0;
    double gamma = 0;
} s_deviceOrientationOverride;

// CDP WebAuthn virtual-authenticator registry. This is a REAL in-memory CRUD
// store: addVirtualAuthenticator mints an id ("authenticator-N") and stores a
// record; add/get/remove/clearCredential(s) genuinely mutate and read it back;
// operations on an unknown id error -32000 (Chrome's exact messages). HOWEVER
// the starfish engine has NO WebAuthn implementation (no navigator.credentials
// / PublicKeyCredential, no authenticator-selection plumbing), so this registry
// is fully inspectable/manipulable over CDP but is NOT wired to any real
// WebAuthn ceremony — nothing in the engine ever consults these authenticators,
// and no credentialAdded/credentialAsserted event is ever emitted (there is no
// ceremony to assert during). Same honesty shape as
// s_deviceOrientationOverride: real stored state with no engine consumer.
// Credentials/options are stored as their raw request JSON so reads echo back
// exactly what was added.
struct WebAuthnCredential {
    std::string
        credentialId; // lookup/removal key (from credential.credentialId)
    std::string json; // raw serialized credential object, echoed verbatim
};
struct WebAuthnAuthenticator {
    std::string authenticatorId; // "authenticator-N"
    bool isUserVerified = false;
    bool automaticPresenceSimulation = true; // Chrome default
    std::vector<WebAuthnCredential> credentials;
};
static struct {
    std::vector<WebAuthnAuthenticator> authenticators;
    uint64_t nextId = 1;
} s_webAuthn;

static WebAuthnAuthenticator* findWebAuthnAuthenticator(const std::string& id)
{
    for (auto& a : s_webAuthn.authenticators) {
        if (a.authenticatorId == id)
            return &a;
    }
    return nullptr;
}

void CDPDispatcher::route(CDPCommand& cmd, const std::string& domain,
                          const std::string& method)
{
    if (domain == "Target") {
        m_target->processMessage(cmd, method);
    } else if (domain == "Page") {
        m_page->processMessage(cmd, method);
    } else if (domain == "Runtime") {
        m_runtime->processMessage(cmd, method);
    } else if (domain == "DOM") {
        m_dom->processMessage(cmd, method);
    } else if (domain == "DOMDebugger") {
        m_domDebugger->processMessage(cmd, method);
    } else if (domain == "Log") {
        m_log->processMessage(cmd, method);
    } else if (domain == "Network") {
        m_network->processMessage(cmd, method);
    } else if (domain == "Fetch") {
        m_fetch->processMessage(cmd, method);
    } else if (domain == "Input") {
        m_input->processMessage(cmd, method);
    } else if (domain == "Emulation") {
        m_emulation->processMessage(cmd, method);
    } else if (domain == "CSS") {
        m_css->processMessage(cmd, method);
    } else if (domain == "DOMSnapshot") {
        m_domSnapshot->processMessage(cmd, method);
    } else if (domain == "DOMStorage") {
        m_domStorage->processMessage(cmd, method);
    } else if (domain == "Storage") {
        m_storage->processMessage(cmd, method);
    } else if (domain == "Accessibility") {
        m_accessibility->processMessage(cmd, method);
    } else if (domain == "Performance") {
        m_performance->processMessage(cmd, method);
    } else if (domain == "Memory") {
        m_memory->processMessage(cmd, method);
    } else if (domain == "Animation") {
        m_animation->processMessage(cmd, method);
    } else if (domain == "Tracing") {
        m_tracing->processMessage(cmd, method);
    } else if (domain == "Overlay") {
        m_overlay->processMessage(cmd, method);
    } else if (domain == "IO") {
        // Minimal IO stream support backing Page.printToPDF's ReturnAsStream
        // mode. Streams are kept in the session as base64 of the full payload,
        // served in chunks. puppeteer drains via IO.read until eof, IO.close.
        CDPSession* s = session();
        if (method == "read") {
            std::string handle;
            if (cmd.params() && cmd.params()->HasMember("handle") &&
                (*cmd.params())["handle"].IsString()) {
                handle = (*cmd.params())["handle"].GetString();
            }
            auto it = s->ioStreams.find(handle);
            if (it == s->ioStreams.end()) {
                cmd.sendError(-32602, "Invalid stream handle");
                return;
            }
            IOStream& st = it->second;
            // Chunk size in raw bytes (CDP default is unbounded; cap for
            // safety).
            const size_t kChunk = 1 << 20;
            size_t remaining = st.data.size() - st.offset;
            size_t n = remaining < kChunk ? remaining : kChunk;
            std::string b64 = cdpBase64Encode(st.data.data() + st.offset, n);
            st.offset += n;
            bool eof = st.offset >= st.data.size();

            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            result.AddMember("base64Encoded", true, alloc);
            result.AddMember("data",
                             rapidjson::Value(b64.c_str(), b64.size(), alloc),
                             alloc);
            result.AddMember("eof", eof, alloc);
            cmd.sendResult(result, doc);
            return;
        }
        if (method == "close") {
            std::string handle;
            if (cmd.params() && cmd.params()->HasMember("handle") &&
                (*cmd.params())["handle"].IsString()) {
                handle = (*cmd.params())["handle"].GetString();
            }
            s->ioStreams.erase(handle);
            cmd.sendResultEmpty();
            return;
        }
        cmd.sendError(-32601, "'method' wasn't found");
    } else if (domain == "Security") {
        // setIgnoreCertificateErrors flips the process-wide TLS verify override
        // so subsequent https requests skip certificate verification
        // (self-signed certs). enable/disable/setOverrideCertificateErrors are
        // acked.
        if (method == "setIgnoreCertificateErrors") {
            bool ignore = cmd.params() && cmd.params()->HasMember("ignore") &&
                          cmd.params()->operator[]("ignore").IsBool() &&
                          cmd.params()->operator[]("ignore").GetBool();
            setGlobalIgnoreSSLVerify(ignore);
            cmd.sendResultEmpty();
        } else {
            // enable / disable / setOverrideCertificateErrors /
            // handleCertificateError
            cmd.sendResultEmpty();
        }
    } else if (domain == "Browser") {
        if (method == "getVersion") {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            result.AddMember("protocolVersion", "1.3", alloc);
            result.AddMember("product", "Starfish/1.0", alloc);
            result.AddMember("revision", "", alloc);
            result.AddMember(
                "userAgent",
                "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
                "(KHTML, like Gecko) Starfish/1.0 Safari/537.36",
                alloc);
            result.AddMember("jsVersion", "", alloc);
            cmd.sendResult(result, doc);
        } else {
            // ack other Browser.* (setDownloadBehavior, getWindowForTarget...)
            cmd.sendResultEmpty();
        }
    } else if (domain == "Profiler") {
        // Escargot's public API exposes no CPU sampling profiler or coverage
        // instrumentation (the only "Profiler.enable" in the engine is a flag
        // in its separate remote-debugger devtools server, not reachable here
        // and with no sampler behind it). So this domain acks state-changing
        // methods and returns minimal-but-valid CPUProfile/coverage results
        // that never throw on the client. No samples or coverage data are ever
        // produced.
        CDPSession* s = session();
        if (method == "enable") {
            s->profilerEnabled = true;
            cmd.sendResultEmpty();
        } else if (method == "disable") {
            s->profilerEnabled = false;
            s->profilerRecording = false;
            cmd.sendResultEmpty();
        } else if (method == "setSamplingInterval") {
            // No sampler; the requested interval has no effect. Ack.
            cmd.sendResultEmpty();
        } else if (method == "start") {
            s->profilerRecording = true;
            s->profilerStartUs = longTickCount();
            cmd.sendResultEmpty();
        } else if (method == "stop") {
            // Return a minimal valid CPUProfile: a single synthetic "(root)"
            // node with zero hit count, no samples, no time deltas. startTime/
            // endTime are microseconds (longTickCount).
            uint64_t startUs =
                s->profilerRecording ? s->profilerStartUs : longTickCount();
            uint64_t endUs = longTickCount();
            s->profilerRecording = false;

            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            rapidjson::Value profile(rapidjson::kObjectType);

            rapidjson::Value nodes(rapidjson::kArrayType);
            rapidjson::Value node(rapidjson::kObjectType);
            node.AddMember("id", 1, alloc);
            rapidjson::Value callFrame(rapidjson::kObjectType);
            callFrame.AddMember("functionName", "(root)", alloc);
            callFrame.AddMember("scriptId", "0", alloc);
            callFrame.AddMember("url", "", alloc);
            callFrame.AddMember("lineNumber", -1, alloc);
            callFrame.AddMember("columnNumber", -1, alloc);
            node.AddMember("callFrame", callFrame, alloc);
            node.AddMember("hitCount", 0, alloc);
            node.AddMember("children", rapidjson::Value(rapidjson::kArrayType),
                           alloc);
            nodes.PushBack(node, alloc);
            profile.AddMember("nodes", nodes, alloc);

            profile.AddMember("startTime", (int64_t)startUs, alloc);
            profile.AddMember("endTime", (int64_t)endUs, alloc);
            profile.AddMember("samples",
                              rapidjson::Value(rapidjson::kArrayType), alloc);
            profile.AddMember("timeDeltas",
                              rapidjson::Value(rapidjson::kArrayType), alloc);

            result.AddMember("profile", profile, alloc);
            cmd.sendResult(result, doc);
        } else if (method == "startPreciseCoverage") {
            s->preciseCoverageActive = true;
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            result.AddMember("timestamp", (double)longTickCount() / 1000000.0,
                             alloc);
            cmd.sendResult(result, doc);
        } else if (method == "takePreciseCoverage") {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            result.AddMember("result", rapidjson::Value(rapidjson::kArrayType),
                             alloc);
            result.AddMember("timestamp", (double)longTickCount() / 1000000.0,
                             alloc);
            cmd.sendResult(result, doc);
        } else if (method == "stopPreciseCoverage") {
            s->preciseCoverageActive = false;
            cmd.sendResultEmpty();
        } else if (method == "getBestEffortCoverage") {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            result.AddMember("result", rapidjson::Value(rapidjson::kArrayType),
                             alloc);
            cmd.sendResult(result, doc);
        } else {
            cmd.sendError(-32601, "'method' wasn't found");
        }
    } else if (domain == "HeapProfiler") {
        // Escargot/Boehm provide no V8 heap-snapshot serializer, allocation
        // tracker, or sampler, and no stable heap-object-ID space. So this
        // domain is honest: collectGarbage runs a REAL Boehm GC (GC_gcollect,
        // exactly like Memory.forciblyPurgeJavaScriptMemory); takeHeapSnapshot/
        // stopTrackingHeapObjects ack with NO
        // addHeapSnapshotChunk/heapStatsUpdate/ lastSeenObjectId events (no
        // serializer/tracker); stop/getSamplingProfile return a
        // minimal-but-valid SamplingHeapProfile (single (root) node, empty
        // samples) like Profiler.stop's CPUProfile; getObjectByHeapObjectId/
        // getHeapObjectId error -32000 (no heap-id map); enable/disable/
        // start{Tracking,Sampling}/addInspectedHeapObject ack. No snapshot data
        // is ever produced.
        CDPSession* s = session();
        if (method == "enable") {
            s->heapProfilerEnabled = true;
            cmd.sendResultEmpty();
        } else if (method == "disable") {
            s->heapProfilerEnabled = false;
            cmd.sendResultEmpty();
        } else if (method == "collectGarbage") {
            // REAL GC: same Boehm collector entry point as
            // Memory.forciblyPurgeJavaScriptMemory. Twice, so the second pass
            // sweeps objects made unreachable by the first.
            GC_gcollect();
            GC_gcollect();
            cmd.sendResultEmpty();
        } else if (method == "startTrackingHeapObjects") {
            cmd.sendResultEmpty(); // no allocation instrumentation; no events
        } else if (method == "stopTrackingHeapObjects") {
            cmd.sendResultEmpty(); // no snapshot streamed (no serializer)
        } else if (method == "takeHeapSnapshot") {
            // No V8 heap-snapshot serializer behind Escargot/Boehm, so emit NO
            // addHeapSnapshotChunk events. Ack only.
            cmd.sendResultEmpty();
        } else if (method == "startSampling") {
            cmd.sendResultEmpty(); // no real allocation sampler
        } else if (method == "stopSampling" || method == "getSamplingProfile") {
            // Minimal valid SamplingHeapProfile: a single synthetic (root) node
            // with zero selfSize, no children, no samples — the heap analog of
            // Profiler.stop's minimal CPUProfile.
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            rapidjson::Value profile(rapidjson::kObjectType);

            rapidjson::Value head(rapidjson::kObjectType);
            rapidjson::Value callFrame(rapidjson::kObjectType);
            callFrame.AddMember("functionName", "(root)", alloc);
            callFrame.AddMember("scriptId", "0", alloc);
            callFrame.AddMember("url", "", alloc);
            callFrame.AddMember("lineNumber", -1, alloc);
            callFrame.AddMember("columnNumber", -1, alloc);
            head.AddMember("callFrame", callFrame, alloc);
            head.AddMember("selfSize", 0, alloc);
            head.AddMember("id", 1, alloc);
            head.AddMember("children", rapidjson::Value(rapidjson::kArrayType),
                           alloc);
            profile.AddMember("head", head, alloc);
            profile.AddMember("samples",
                              rapidjson::Value(rapidjson::kArrayType), alloc);

            result.AddMember("profile", profile, alloc);
            cmd.sendResult(result, doc);
        } else if (method == "getObjectByHeapObjectId" ||
                   method == "getHeapObjectId") {
            // No heap-object-ID space (Boehm is conservative; no inspector
            // IDs). Neither id->object nor object->id is answerable.
            cmd.sendError(-32000, "Object is not available");
        } else if (method == "addInspectedHeapObject") {
            // No $0..$n heap-id console binding; ack so the call doesn't error.
            cmd.sendResultEmpty();
        } else {
            cmd.sendError(-32601, "'method' wasn't found");
        }
    } else if (domain == "WebAuthn") {
        // REAL in-memory CRUD registry of virtual authenticators + credentials
        // (see s_webAuthn above). add/get/remove genuinely mutate & read the
        // store; unknown ids error -32000 (Chrome's exact messages). HONEST
        // LIMIT: the engine has no navigator.credentials/PublicKeyCredential,
        // so this registry is inspectable/manipulable over CDP but is NOT wired
        // to any real WebAuthn ceremony — nothing consults these authenticators
        // and no credentialAdded/credentialAsserted event is emitted (no
        // ceremony to assert during).
        // setUserVerified/setAutomaticPresenceSimulation/
        // setResponseOverrideBits store their values but are inert. Credentials
        // are echoed back as the exact JSON that was added.
        rapidjson::Value* params = cmd.params(); // nullable
        if (method == "enable") {
            cmd.sendResultEmpty();
        } else if (method == "disable") {
            // Registry intentionally retained (not cleared) on disable.
            cmd.sendResultEmpty();
        } else if (method == "addVirtualAuthenticator") {
            WebAuthnAuthenticator a;
            a.authenticatorId =
                "authenticator-" + std::to_string(s_webAuthn.nextId++);
            if (params && params->HasMember("options") &&
                (*params)["options"].IsObject()) {
                const rapidjson::Value& opt = (*params)["options"];
                if (opt.HasMember("isUserVerified") &&
                    opt["isUserVerified"].IsBool())
                    a.isUserVerified = opt["isUserVerified"].GetBool();
                if (opt.HasMember("automaticPresenceSimulation") &&
                    opt["automaticPresenceSimulation"].IsBool())
                    a.automaticPresenceSimulation =
                        opt["automaticPresenceSimulation"].GetBool();
            }
            s_webAuthn.authenticators.push_back(a);

            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            result.AddMember("authenticatorId",
                             rapidjson::Value(a.authenticatorId.c_str(), alloc),
                             alloc);
            cmd.sendResult(result, doc);
        } else {
            // All remaining methods require an authenticatorId; look it up
            // once.
            std::string authId;
            if (params && params->HasMember("authenticatorId") &&
                (*params)["authenticatorId"].IsString())
                authId = (*params)["authenticatorId"].GetString();
            WebAuthnAuthenticator* auth = findWebAuthnAuthenticator(authId);
            if (!auth) {
                cmd.sendError(
                    -32000,
                    "Could not find a Virtual Authenticator matching the ID");
            } else if (method == "removeVirtualAuthenticator") {
                for (auto it = s_webAuthn.authenticators.begin();
                     it != s_webAuthn.authenticators.end(); ++it) {
                    if (it->authenticatorId == authId) {
                        s_webAuthn.authenticators.erase(it);
                        break;
                    }
                }
                cmd.sendResultEmpty();
            } else if (method == "setUserVerified") {
                if (params && params->HasMember("isUserVerified") &&
                    (*params)["isUserVerified"].IsBool())
                    auth->isUserVerified =
                        (*params)["isUserVerified"].GetBool();
                cmd.sendResultEmpty();
            } else if (method == "setAutomaticPresenceSimulation") {
                if (params && params->HasMember("enabled") &&
                    (*params)["enabled"].IsBool())
                    auth->automaticPresenceSimulation =
                        (*params)["enabled"].GetBool();
                cmd.sendResultEmpty();
            } else if (method == "setResponseOverrideBits") {
                // Bits are accepted but inert (no ceremony to apply them to).
                cmd.sendResultEmpty();
            } else if (method == "addCredential") {
                if (params && params->HasMember("credential") &&
                    (*params)["credential"].IsObject()) {
                    const rapidjson::Value& cr = (*params)["credential"];
                    WebAuthnCredential c;
                    if (cr.HasMember("credentialId") &&
                        cr["credentialId"].IsString())
                        c.credentialId = cr["credentialId"].GetString();
                    rapidjson::StringBuffer sb;
                    rapidjson::Writer<rapidjson::StringBuffer> w(sb);
                    cr.Accept(w);
                    c.json.assign(sb.GetString(), sb.GetSize());
                    auth->credentials.push_back(c);
                }
                cmd.sendResultEmpty();
            } else if (method == "getCredential") {
                std::string credId;
                if (params && params->HasMember("credentialId") &&
                    (*params)["credentialId"].IsString())
                    credId = (*params)["credentialId"].GetString();
                const WebAuthnCredential* found = nullptr;
                for (auto& c : auth->credentials) {
                    if (c.credentialId == credId) {
                        found = &c;
                        break;
                    }
                }
                if (!found) {
                    cmd.sendError(
                        -32000, "Could not find a credential matching the ID");
                } else {
                    rapidjson::Document doc;
                    rapidjson::Document::AllocatorType& alloc =
                        doc.GetAllocator();
                    rapidjson::Value result(rapidjson::kObjectType);
                    rapidjson::Document cd;
                    cd.Parse(found->json.c_str());
                    rapidjson::Value v;
                    v.CopyFrom(cd, alloc);
                    result.AddMember("credential", v, alloc);
                    cmd.sendResult(result, doc);
                }
            } else if (method == "getCredentials") {
                rapidjson::Document doc;
                rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
                rapidjson::Value result(rapidjson::kObjectType);
                rapidjson::Value creds(rapidjson::kArrayType);
                for (auto& c : auth->credentials) {
                    rapidjson::Document cd;
                    cd.Parse(c.json.c_str());
                    rapidjson::Value v;
                    v.CopyFrom(cd, alloc);
                    creds.PushBack(v, alloc);
                }
                result.AddMember("credentials", creds, alloc);
                cmd.sendResult(result, doc);
            } else if (method == "removeCredential") {
                std::string credId;
                if (params && params->HasMember("credentialId") &&
                    (*params)["credentialId"].IsString())
                    credId = (*params)["credentialId"].GetString();
                for (auto it = auth->credentials.begin();
                     it != auth->credentials.end(); ++it) {
                    if (it->credentialId == credId) {
                        auth->credentials.erase(it);
                        break; // credential-miss → idempotent no-op
                    }
                }
                cmd.sendResultEmpty();
            } else if (method == "clearCredentials") {
                auth->credentials.clear();
                cmd.sendResultEmpty();
            } else {
                cmd.sendError(-32601, "'method' wasn't found");
            }
        }
    } else if (domain == "Debugger") {
        // HANDSHAKE/SHAPE-COMPATIBLE STUB — NOT a working debugger. Escargot
        // has a debugger, but it is NOT wired to any CDP pause path in this
        // MVP: it is a separate remote TCP protocol (port 6501,
        // STARFISH_ENABLE_DEBUGGER), and there is no choke point that suspends
        // the VM and emits Debugger.paused with live call frames, so
        // BREAKPOINTS ARE ACCEPTED BUT NEVER FIRE. This block exists to satisfy
        // a CDP client's connect handshake (enable -> debuggerId) and the
        // breakpoint/source API SHAPE, nothing more.
        //   enable -> {debuggerId:"-1"} (clients require a UniqueDebuggerId;
        //   "-1" is
        //     a stable sentinel, no per-session token minted); disable acks.
        //   setBreakpointsActive/setSkipAllPauses/setPauseOnExceptions/
        //     setAsyncCallStackDepth/setBlackboxPatterns/setBlackboxedRanges ->
        //     ack, all INERT (nothing pauses, so none of these gate anything).
        //   setBreakpointByUrl -> {breakpointId:"bp-N", locations:[]}: the
        //   breakpoint
        //     is accepted and an id is minted, but it binds to no live location
        //     (empty locations) and never fires (no pause path).
        //   setBreakpoint {location} -> -32000 "Could not resolve breakpoint":
        //   needs
        //     a resolvable scriptId/line-col index we do not have; reject
        //     rather than fabricate an actualLocation.
        //   removeBreakpoint -> ack (idempotent).
        //   getScriptSource {scriptId} -> served from
        //   CDPSession::compiledScripts (the
        //     Runtime.compileScript(persistScript:true) registry — REAL source
        //     for a client-persisted script); unknown id -> -32000 "No script
        //     for id <id>". This is a source-only registry (no line/col map),
        //     which is why setBreakpoint and getPossibleBreakpoints cannot
        //     resolve locations.
        //   getPossibleBreakpoints -> {locations:[]} (no location index).
        //   resume/pause/stepOver/stepInto/stepOut/continueToLocation -> ack,
        //   no-op;
        //     NO paused/resumed event is emitted (no pause path, no live stack
        //     to synthesize).
        //   evaluateOnCallFrame -> -32000 "Can only perform operation while
        //   paused":
        //     never paused, no call frames.
        // NO events in this domain (no scriptParsed/scriptFailedToParse/paused/
        // resumed): compileScript is a pull source-registry, not a per-parse
        // notification hook, so a partial scriptParsed stream would mislead.
        // The dispatcher fabricates nothing.
        CDPSession* s = session();
        rapidjson::Value* params = cmd.params(); // nullable
        if (method == "enable") {
            s->debuggerEnabled = true;
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            result.AddMember("debuggerId", "-1", alloc);
            cmd.sendResult(result, doc);
        } else if (method == "disable") {
            s->debuggerEnabled = false;
            cmd.sendResultEmpty();
        } else if (method == "setBreakpointsActive" ||
                   method == "setSkipAllPauses" ||
                   method == "setPauseOnExceptions" ||
                   method == "setAsyncCallStackDepth" ||
                   method == "setBlackboxPatterns" ||
                   method == "setBlackboxedRanges" || method == "resume" ||
                   method == "pause" || method == "stepOver" ||
                   method == "stepInto" || method == "stepOut" ||
                   method == "continueToLocation" ||
                   method == "removeBreakpoint") {
            // All inert / no-op: nothing pauses, so these change no observable
            // state.
            cmd.sendResultEmpty();
        } else if (method == "setBreakpointByUrl") {
            // Accept the breakpoint, mint an id, but bind to nothing (never
            // fires).
            std::string bpId =
                "bp-" + std::to_string(++s->debuggerBreakpointCounter);
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            result.AddMember("breakpointId",
                             rapidjson::Value(bpId.c_str(), bpId.size(), alloc),
                             alloc);
            result.AddMember("locations",
                             rapidjson::Value(rapidjson::kArrayType), alloc);
            cmd.sendResult(result, doc);
        } else if (method == "setBreakpoint") {
            // Needs a resolvable scriptId/location; we have no location index.
            cmd.sendError(-32000, "Could not resolve breakpoint");
        } else if (method == "getScriptSource") {
            std::string scriptId;
            if (params && params->HasMember("scriptId") &&
                (*params)["scriptId"].IsString())
                scriptId = (*params)["scriptId"].GetString();
            auto it = s->compiledScripts.find(scriptId);
            if (it == s->compiledScripts.end()) {
                std::string msg = "No script for id " + scriptId;
                cmd.sendError(-32000, msg.c_str());
            } else {
                rapidjson::Document doc;
                rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
                rapidjson::Value result(rapidjson::kObjectType);
                result.AddMember("scriptSource",
                                 rapidjson::Value(it->second.c_str(),
                                                  it->second.size(), alloc),
                                 alloc);
                cmd.sendResult(result, doc);
            }
        } else if (method == "getPossibleBreakpoints") {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            result.AddMember("locations",
                             rapidjson::Value(rapidjson::kArrayType), alloc);
            cmd.sendResult(result, doc);
        } else if (method == "evaluateOnCallFrame") {
            cmd.sendError(-32000, "Can only perform operation while paused");
        } else {
            cmd.sendError(-32601, "'method' wasn't found");
        }
    } else if (domain == "Extensions") {
        // HONEST STUB: Extensions drives the unpacked-extension developer flow
        // and extension-storage inspection. The protocol is six methods —
        // loadUnpacked {path} -> {id} / uninstall {id} / getStorageItems {id,
        // storageArea, keys?} -> {data} / setStorageItems {id, storageArea,
        // values} / removeStorageItems {id, storageArea, keys} /
        // clearStorageItems {id, storageArea} — and NO events. Starfish has NO
        // extension subsystem: no extension loader, no extension-id registry,
        // no chrome.storage (session/local/sync/managed) backing store, and no
        // extension surface reachable from the CDP dispatcher, so no extension
        // can ever be loaded and no extension storage can ever exist. EVERY
        // method names a resource that cannot exist (an extension to load, or
        // an {id} addressing its storage), so all of them -> -32000 "Extensions
        // are not supported": we reject rather than mint a fake id, ack a
        // no-op, or return an empty {data} for storage that has no backend. No
        // events in this domain. The dispatcher fabricates nothing.
        cmd.sendError(-32000, "Extensions are not supported");
    } else if (domain == "Tethering") {
        // HONEST STUB: Tethering implements reverse port forwarding — a port on
        // the inspecting host is tunnelled back over the DevTools connection to
        // the inspected device. The protocol is two methods — bind {port} /
        // unbind {port} — plus one event (accepted {port, connectionId}) fired
        // when a connection arrives on a bound port. This requires the
        // transport to carry raw, multiplexed socket data alongside the JSON
        // protocol. Starfish's CDP transport is a plain JSON message channel:
        // NO reverse port-forwarding tunnel, no socket multiplexer, no
        // host-port listener, and no connectionId registry reachable from the
        // CDP dispatcher, so a bound port can never accept a connection.
        // bind/unbind ack (port NOT stored: with no tunnel to drive it, a
        // remembered port is dead state, not fidelity — nothing consumes it and
        // there is no getter). NO accepted event is emitted (no tunnel/listener
        // to source a connection, no connectionId to mint). The dispatcher
        // fabricates nothing.
        if (method == "bind" || method == "unbind") {
            cmd.sendResultEmpty();
        } else {
            cmd.sendError(-32601, "'method' wasn't found");
        }
    } else if (domain == "Cast") {
        // HONEST STUB: Cast drives the Cast / Presentation / Media Router UI —
        // discovering Cast sinks (Chromecast, smart TVs) and mirroring tabs or
        // the desktop to them. The protocol is six methods — enable
        // {presentationUrl?} / disable (discovery toggles) plus four sink ops:
        // setSinkToUse {sinkName} / startDesktopMirroring {sinkName} /
        // startTabMirroring {sinkName} / stopCasting {sinkName} — plus two
        // events (sinksUpdated {sinks}, issueUpdated {issueMessage}). Starfish
        // has NO Presentation/Cast/Media Router engine: no sink discovery
        // (mDNS/DIAL), no navigator.presentation, no tab/desktop mirroring
        // pipeline, and no sink registry reachable from the CDP main-frame
        // dispatcher, so no sink can ever exist and no cast session can ever
        // start. (There is a DIAL/SSDP CastServer under src/core/modules/cast/
        // gated by STARFISH_ENABLE_CAST_SERVICE, but it is a network-side
        // receiver wired only into the ServiceWorker agent, not a CDP sink
        // registry.) enable/ disable ack (presentationUrl not stored: with no
        // discovery engine it would be dead state, not fidelity). setSinkToUse,
        // startDesktopMirroring, startTabMirroring and stopCasting -> -32000
        // "Sink not found": there is no sink registry, so every sinkName is
        // unknown; we reject rather than falsely ack a cast action against a
        // sink that does not exist. NO sinksUpdated/issueUpdated event is
        // emitted (no Cast engine to source a sink list or an issue). The
        // dispatcher fabricates nothing.
        if (method == "enable" || method == "disable") {
            cmd.sendResultEmpty();
        } else if (method == "setSinkToUse" ||
                   method == "startDesktopMirroring" ||
                   method == "startTabMirroring" || method == "stopCasting") {
            cmd.sendError(-32000, "Sink not found");
        } else {
            cmd.sendError(-32601, "'method' wasn't found");
        }
    } else if (domain == "DeviceAccess") {
        // HONEST STUB: DeviceAccess drives the device-chooser prompt shown by
        // WebBluetooth / WebUSB / WebSerial / WebHID
        // (navigator.bluetooth.request Device(), navigator.usb.requestDevice(),
        // etc.). The protocol is four methods — enable / disable (session
        // toggles) plus two prompt-id ops: selectPrompt {id, deviceId} /
        // cancelPrompt {id} — plus one event (deviceRequestPrompted {id,
        // devices}). Starfish has NO device-chooser engine: no
        // WebBluetooth/WebUSB/WebSerial/WebHID device picker, and no prompt-id
        // (RequestId) registry reachable from the CDP main-frame dispatcher, so
        // no DeviceAccess prompt can ever be raised. enable/disable ack (no
        // session flag stored: with no prompt source it would be dead state,
        // not fidelity). selectPrompt and cancelPrompt -> -32000 "Prompt not
        // found": there is no prompt registry, so every id is unknown; we
        // reject rather than falsely ack a prompt action against a prompt that
        // does not exist. NO deviceRequestPrompted event is emitted (no chooser
        // engine to source a prompt). The dispatcher fabricates nothing.
        if (method == "enable" || method == "disable") {
            cmd.sendResultEmpty();
        } else if (method == "selectPrompt" || method == "cancelPrompt") {
            cmd.sendError(-32000, "Prompt not found");
        } else {
            cmd.sendError(-32601, "'method' wasn't found");
        }
    } else if (domain == "Database") {
        // HONEST STUB: the Database domain inspects legacy WebSQL databases
        // (those opened via window.openDatabase). The protocol is four methods
        // — enable / disable (session toggles) plus two database-id ops:
        // getDatabaseTableNames {databaseId} -> {tableNames:[]} and executeSQL
        // {databaseId, query} -> {columnNames?, values?, sqlError?} — plus one
        // event (addDatabase {database}). WebSQL is deprecated/removed from the
        // web platform, and Starfish has NO WebSQL engine: no openDatabase /
        // Database / SQLTransaction implementation reachable from the CDP main
        // frame, and no databaseId registry the dispatcher can read, so no
        // WebSQL database can ever be opened or discovered. enable/disable ack
        // (no session flag stored: with no database source it would be dead
        // state, not fidelity). getDatabaseTableNames and executeSQL -> -32000
        // "Database not found": there is no database registry, so every
        // databaseId is unknown; we reject rather than return an empty
        // tableNames (which would falsely imply a real, tableless database) or
        // fabricate a sqlError. NO addDatabase event is emitted (no WebSQL
        // engine to source a database). The dispatcher fabricates nothing.
        if (method == "enable" || method == "disable") {
            cmd.sendResultEmpty();
        } else if (method == "getDatabaseTableNames" ||
                   method == "executeSQL") {
            cmd.sendError(-32000, "Database not found");
        } else {
            cmd.sendError(-32601, "'method' wasn't found");
        }
    } else if (domain == "FedCm") {
        // HONEST STUB: FedCm drives the browser's Federated Credential
        // Management account-chooser dialog (the UI shown by
        // navigator.credentials.get({ identity })). The protocol is seven
        // methods — enable {disableRejection Delay?} / disable / resetCooldown
        // (session/global toggles) and four dialog-id ops: selectAccount
        // {dialogId, accountIndex} / clickDialogButton {dialogId, dialogButton}
        // / dismissDialog {dialogId, triggerCooldown?} / openUrl {dialogId,
        // accountIndex, accountUrlType} — plus two events (dialogShown /
        // dialogClosed). Starfish has NO FedCm engine: no navigator.credentials
        // / IdentityCredential, no account-chooser dialog, and no dialog-id
        // registry reachable from the dispatcher, so no FedCm dialog can ever
        // be shown. enable/disable/resetCooldown ack (no session flag stored:
        // with no dialog source, disableRejectionDelay would be dead state, not
        // fidelity; resetCooldown is an idempotent no-op — no cooldown to
        // clear). The four dialog-id ops -> -32000 "Dialog not found": there is
        // no dialog registry, so every dialogId is unknown; we reject rather
        // than falsely ack a dialog action. NO dialogShown/dialogClosed event
        // is emitted (no FedCm engine to source a dialog). The dispatcher
        // fabricates nothing.
        if (method == "enable" || method == "disable" ||
            method == "resetCooldown") {
            cmd.sendResultEmpty();
        } else if (method == "selectAccount" || method == "clickDialogButton" ||
                   method == "dismissDialog" || method == "openUrl") {
            cmd.sendError(-32000, "Dialog not found");
        } else {
            cmd.sendError(-32601, "'method' wasn't found");
        }
    } else if (domain == "Autofill") {
        // HONEST STUB: Autofill drives the browser's address/payment autofill —
        // trigger {fieldId, frameId?, card} forces a field to be filled from a
        // test CreditCard {number,name,expiryMonth,expiryYear,cvc};
        // setAddresses {addresses} seeds the test Address[]
        // ({fields:[{name,value}...]}) offered for filling; enable/disable
        // toggle the feature — plus one event (addressFormFilled). Starfish has
        // NO autofill engine: no address/ credit-card profile store, no
        // field-recognition pass, and no autofill driver reachable from the
        // dispatcher. So we ack all four but do NOT store the addresses: with
        // no engine to offer them and no getter to read them back, storage
        // would be dead state, not fidelity. trigger acks but fills nothing (no
        // driver, no form pipeline to apply the card to fieldId). NO
        // addressFormFilled event is emitted (no autofill engine to source a
        // fill). The dispatcher fabricates nothing.
        if (method == "trigger" || method == "setAddresses" ||
            method == "enable" || method == "disable") {
            cmd.sendResultEmpty();
        } else {
            cmd.sendError(-32601, "'method' wasn't found");
        }
    } else if (domain == "BackgroundService") {
        // HONEST STUB: BackgroundService surfaces the activity of the browser's
        // background services — backgroundFetch / backgroundSync /
        // pushMessaging / notifications / periodicBackgroundSync — so DevTools
        // can record/replay their events. The protocol is four void/ack methods
        // — startObserving {service} / stopObserving {service} / setRecording
        // {shouldRecord,service} / clearEvents {service} — plus two events
        // (recordingStateChanged / backgroundServiceEventReceived). Every one
        // of those services lives in the ServiceWorker host, which is COMPILED
        // OUT of this build (SERVICE_WORKER=0 ⇒
        // STARFISH_ENABLE_SERVICE_WORKER/STARFISH_WEBWORKER_HOST undefined; see
        // iter3). With the SW module unlinked there is NO background-service
        // activity reachable from the dispatcher — nothing to observe, nothing
        // to record, no event source. So we ack all four but do NOT store the
        // service / shouldRecord: there is no activity to gate, so storage
        // would be dead state, not fidelity. NO recordingStateChanged /
        // backgroundServiceEventReceived events are emitted (no SW host to
        // source them). The dispatcher fabricates nothing.
        if (method == "startObserving" || method == "stopObserving" ||
            method == "setRecording" || method == "clearEvents") {
            cmd.sendResultEmpty();
        } else {
            cmd.sendError(-32601, "'method' wasn't found");
        }
    } else if (domain == "EventBreakpoints") {
        // HONEST STUB: EventBreakpoints arms "instrumentation breakpoints" that
        // pause execution when a named engine event fires
        // (scriptFirstStatement, webgl.errorFired, DOM listener entry, ...).
        // The whole protocol is three void/ack methods —
        // setInstrumentationBreakpoint{eventName} /
        // removeInstrumentationBreakpoint{eventName} / disable — and NO getter,
        // so the armed set is not observable over CDP. The pause itself is
        // delivered via Debugger.paused, and Starfish's Escargot debugger is
        // NOT wired to CDP (no CDP-driven pause path). So we ack all three but
        // do NOT store the eventName: storage would be unobservable (no getter)
        // and inert (no pause path), i.e. dead state, not fidelity. Breakpoints
        // are accepted but never fire. No events exist in this domain. The
        // dispatcher fabricates nothing.
        if (method == "setInstrumentationBreakpoint" ||
            method == "removeInstrumentationBreakpoint" ||
            method == "disable") {
            cmd.sendResultEmpty();
        } else {
            cmd.sendError(-32601, "'method' wasn't found");
        }
    } else if (domain == "Preload") {
        // HONEST STUB: the Preload domain reports speculation-rules driven
        // prefetch/prerender activity. Real CDP exposes only enable/disable as
        // callable methods; everything else is event-driven (ruleSetUpdated/
        // ruleSetRemoved/preloadEnabledStateUpdated/prefetchStatusUpdated/
        // prerenderStatusUpdated/preloadingAttemptSourcesUpdated). Starfish has
        // NO speculation-rules parser and no prefetch/prerender engine
        // reachable from CDP — there is no rule-set registry, no
        // preloading-attempt source, and no prefetch/prerender driver the
        // dispatcher can read. So there is nothing to enumerate and nothing to
        // source events from. enable/disable ack (no session flag; matches
        // Media/WebAudio/LayerTree stubs). No
        // ruleSetUpdated/ruleSetRemoved/preloadEnabledStateUpdated/
        // prefetchStatusUpdated/prerenderStatusUpdated/preloadingAttemptSources
        // Updated events are emitted (no engine to source them); we never
        // fabricate rule sets, preloading attempts, or prefetch/prerender
        // statuses. The event path (CDPCommand::sendEvent) exists for a future
        // real implementation if a preloading engine becomes reachable.
        if (method == "enable" || method == "disable") {
            cmd.sendResultEmpty();
        } else {
            cmd.sendError(-32601, "'method' wasn't found");
        }
    } else if (domain == "LayerTree") {
        // HONEST STUB: LayerTree exposes the compositor's render-layer tree
        // (per-layer geometry/compositing reasons + tile snapshots that can be
        // profiled/replayed), and is otherwise event-driven
        // (layerTreeDidChange/ layerPainted). Starfish has NO CDP-reachable
        // compositor / render-layer tree and no layer-id or snapshot registry
        // the dispatcher can read (see CDP_DOMAINS.md: "hard (no compositor
        // introspection)"). So:
        //   - enable/disable ack (no session flag; matches the recent stubs).
        //   - compositingReasons returns empty arrays — structurally valid and
        //     honest: no layer is composited for any reason here.
        //   - makeSnapshot -> -32000: no layer registry, so every layerId is
        //     unknown; we reject rather than fabricate a snapshotId.
        //   - load/profile/replay/snapshotCommandLog -> -32000: no snapshot
        //     registry, so every snapshotId is unknown.
        //   - releaseSnapshot acks (idempotent no-op; releasing an id that was
        //     never minted is harmless).
        // We never fabricate layers, compositing reasons, or snapshots, and no
        // layerTreeDidChange/layerPainted events are emitted (no compositor to
        // source them). The event path (CDPCommand::sendEvent) exists for a
        // future real implementation if a compositor becomes reachable from the
        // dispatcher.
        if (method == "enable" || method == "disable" ||
            method == "releaseSnapshot") {
            cmd.sendResultEmpty();
        } else if (method == "compositingReasons") {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            result.AddMember("compositingReasons",
                             rapidjson::Value(rapidjson::kArrayType), alloc);
            result.AddMember("compositingReasonIds",
                             rapidjson::Value(rapidjson::kArrayType), alloc);
            cmd.sendResult(result, doc);
        } else if (method == "makeSnapshot") {
            cmd.sendError(-32000, "No layer with given id found");
        } else if (method == "loadSnapshot" || method == "profileSnapshot" ||
                   method == "replaySnapshot" ||
                   method == "snapshotCommandLog") {
            cmd.sendError(-32000, "No snapshot with given id found");
        } else {
            cmd.sendError(-32601, "'method' wasn't found");
        }
    } else if (domain == "Media") {
        // HONEST STUB: the Media domain is media-player logging/introspection
        // (a playerId registry plus playersCreated/playerPropertiesChanged/
        // playerEventsAdded/playerMessagesLogged/playerErrorsRaised event
        // stream). The engine does have playback
        // (src/platform/multimedia/MediaPlayer*) and HTMLMediaElement, but none
        // of it is wired to a CDP-reachable player-log registry — there is no
        // playerId namespace or per-player property/event/ message/error
        // surface the dispatcher can read, so there are no players to enumerate
        // and nothing to source events from. enable/disable ack; we never
        // fabricate a synthetic playerId or fake player
        // properties/events/messages/ errors, and no
        // playersCreated/playerPropertiesChanged/playerEventsAdded/
        // playerMessagesLogged/playerErrorsRaised events are emitted.
        if (method == "enable" || method == "disable") {
            cmd.sendResultEmpty();
        } else {
            cmd.sendError(-32601, "'method' wasn't found");
        }
    } else if (domain == "WebAudio") {
        // HONEST STUB: the engine has a WebAudio module
        // (src/core/modules/webaudio/) but it is gated behind
        // STARFISH_ENABLE_WEBAUDIO and NOT wired to CDP — there is no live
        // AudioContext/BaseAudioContext registry reachable from the inspector,
        // so there is no audio graph to enumerate and no realtime metrics to
        // read. enable/disable ack. getRealtimeData errors -32000 with Chrome's
        // exact message ("Cannot find BaseAudioContext with such id") because
        // no context can ever exist here — every contextId is unknown; we error
        // rather than fabricate currentTime/renderCapacity numbers. No
        // contextCreated/
        // contextWillBeDestroyed/contextChanged/audioListener*/audioNode*/audioParam*/
        // nodes(Dis)Connected/nodeParam(Dis)Connected events are emitted (no
        // engine to source them from).
        if (method == "enable" || method == "disable") {
            cmd.sendResultEmpty();
        } else if (method == "getRealtimeData") {
            cmd.sendError(-32000, "Cannot find BaseAudioContext with such id");
        } else {
            cmd.sendError(-32601, "'method' wasn't found");
        }
    } else if (domain == "ServiceWorker") {
        // Starfish runs ServiceWorkers in a separate worker host (gated behind
        // STARFISH_ENABLE_SERVICE_WORKER / STARFISH_WEBWORKER_HOST; see
        // src/launcher/ServiceWorkerEntry.cpp and src/public/LWEWorker*.cpp),
        // which is OFF in the CDP/headless build (SERVICE_WORKER=0). The whole
        // SW module is unlinked and Navigator::serviceWorker() is compiled out,
        // so no registration/version state is reachable from the CDP main-frame
        // context — the main frame is a client of SW registrations, not their
        // host/registry. enable/disable and every command (unregister,
        // updateRegistration, start/stop/stopAllWorkers, skipWaiting,
        // setForceUpdateOnPageLoad, inspectWorker, deliverPushMessage,
        // dispatchSyncEvent, dispatchPeriodicSyncEvent) ack as idempotent
        // no-ops. No registrations, versions, or errors are fabricated, and no
        // workerRegistrationUpdated/workerVersionUpdated/workerErrorReported
        // events are emitted. The event path (CDPCommand::sendEvent) exists for
        // a future real implementation if the worker-host registry becomes
        // reachable from the dispatcher. Every method acks identically, so the
        // body collapses to a single sendResultEmpty() (like the Inspector
        // block); unknown methods are acked too, matching
        // CacheStorage/IndexedDB.
        cmd.sendResultEmpty();
    } else if (domain == "IndexedDB") {
        // Starfish has a full IndexedDB module (src/core/modules/indexeddb/)
        // and window.indexedDB is exposed, but it is gated behind a separate
        // STARFISH_ENABLE_IDB build flag that is OFF in the CDP build, and even
        // when enabled the backend exposes no database-enumeration API
        // (IDBFactory::databases() is [Unimplemented]; MemoryBackingStore has
        // no DB-name listing). There is therefore no inspectable IDB factory in
        // the CDP main-frame context. This domain reports an empty, honest
        // view: no database names; an empty (but structurally valid) database
        // schema; no records (hasMore:false); zero metadata; mutations +
        // enable/disable ack. No fabricated databases, object stores, or
        // records are ever returned.
        if (method == "requestDatabaseNames") {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            result.AddMember("databaseNames",
                             rapidjson::Value(rapidjson::kArrayType), alloc);
            cmd.sendResult(result, doc);
        } else if (method == "requestDatabase") {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            rapidjson::Value db(rapidjson::kObjectType);
            const char* dbName = "";
            if (cmd.params() && cmd.params()->HasMember("databaseName") &&
                (*cmd.params())["databaseName"].IsString()) {
                dbName = (*cmd.params())["databaseName"].GetString();
            }
            db.AddMember("name", rapidjson::Value(dbName, alloc), alloc);
            db.AddMember("version", 1, alloc);
            db.AddMember("objectStores",
                         rapidjson::Value(rapidjson::kArrayType), alloc);
            result.AddMember("databaseWithObjectStores", db, alloc);
            cmd.sendResult(result, doc);
        } else if (method == "requestData") {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            result.AddMember("objectStoreDataEntries",
                             rapidjson::Value(rapidjson::kArrayType), alloc);
            result.AddMember("hasMore", false, alloc);
            cmd.sendResult(result, doc);
        } else if (method == "getMetadata") {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            result.AddMember("entriesCount", 0, alloc);
            result.AddMember("keyGeneratorValue", 0, alloc);
            cmd.sendResult(result, doc);
        } else if (method == "deleteDatabase" ||
                   method == "deleteObjectStoreEntries" ||
                   method == "clearObjectStore" || method == "enable" ||
                   method == "disable") {
            cmd.sendResultEmpty();
        } else {
            cmd.sendResultEmpty();
        }
    } else if (domain == "CacheStorage") {
        // Starfish's Cache API exists only as a ServiceWorker-scope JS polyfill
        // (CachePolyfillLoader, backed by IndexedDB/FetchCacheStream) and is
        // NOT reachable from the CDP main-frame context, so there is no
        // inspectable Cache Storage store here. This domain therefore reports
        // an empty, honest view: no cache names, no entries; deletes ack
        // (idempotent no-op); requestCachedResponse is a genuine miss. No
        // fabricated cache names or entries are ever returned.
        if (method == "requestCacheNames") {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            result.AddMember("caches", rapidjson::Value(rapidjson::kArrayType),
                             alloc);
            cmd.sendResult(result, doc);
        } else if (method == "requestEntries") {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            result.AddMember("cacheDataEntries",
                             rapidjson::Value(rapidjson::kArrayType), alloc);
            result.AddMember("returnCount", 0, alloc);
            cmd.sendResult(result, doc);
        } else if (method == "requestCachedResponse") {
            cmd.sendError(-32000, "cache not found");
        } else if (method == "deleteCache" || method == "deleteEntry") {
            cmd.sendResultEmpty();
        } else {
            cmd.sendResultEmpty();
        }
    } else if (domain == "Schema") {
        if (method == "getDomains") {
            // Report the set of domains this dispatcher routes. Versions are
            // the CDP stable channel ("1.3") to match Browser.getVersion.
            static const char* kDomains[] = { "Target",
                                              "Page",
                                              "Runtime",
                                              "DOM",
                                              "DOMDebugger",
                                              "DOMSnapshot",
                                              "Input",
                                              "Network",
                                              "Fetch",
                                              "Emulation",
                                              "CSS",
                                              "DOMStorage",
                                              "Storage",
                                              "Accessibility",
                                              "Security",
                                              "Performance",
                                              "PerformanceTimeline",
                                              "Audits",
                                              "Memory",
                                              "Animation",
                                              "Profiler",
                                              "Tracing",
                                              "Overlay",
                                              "Log",
                                              "IO",
                                              "Browser",
                                              "Schema",
                                              "Inspector",
                                              "SystemInfo",
                                              "DeviceOrientation",
                                              "CacheStorage",
                                              "IndexedDB",
                                              "ServiceWorker",
                                              "HeapProfiler",
                                              "WebAuthn",
                                              "WebAudio",
                                              "Media",
                                              "LayerTree",
                                              "Preload",
                                              "EventBreakpoints",
                                              "BackgroundService",
                                              "Autofill",
                                              "FedCm",
                                              "Database",
                                              "DeviceAccess",
                                              "Cast",
                                              "Tethering",
                                              "Extensions",
                                              "Debugger" };
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            rapidjson::Value domains(rapidjson::kArrayType);
            for (const char* name : kDomains) {
                rapidjson::Value d(rapidjson::kObjectType);
                d.AddMember("name", rapidjson::Value(name, alloc), alloc);
                d.AddMember("version", "1.3", alloc);
                domains.PushBack(d, alloc);
            }
            result.AddMember("domains", domains, alloc);
            cmd.sendResult(result, doc);
        } else {
            cmd.sendError(-32601, "'method' wasn't found");
        }
    } else if (domain == "Inspector") {
        // enable/disable ack. Inspector.targetCrashed/detached events would be
        // emitted from a crash handler / connection teardown; not wired here.
        cmd.sendResultEmpty();
    } else if (domain == "DeviceOrientation") {
        // The engine has no DeviceOrientationEvent, so the override has no
        // event to drive; we store/clear it as process-static state and ack.
        if (method == "setDeviceOrientationOverride") {
            s_deviceOrientationOverride.active = true;
            s_deviceOrientationOverride.alpha =
                (cmd.params() && cmd.params()->HasMember("alpha") &&
                 (*cmd.params())["alpha"].IsNumber())
                    ? (*cmd.params())["alpha"].GetDouble()
                    : 0;
            s_deviceOrientationOverride.beta =
                (cmd.params() && cmd.params()->HasMember("beta") &&
                 (*cmd.params())["beta"].IsNumber())
                    ? (*cmd.params())["beta"].GetDouble()
                    : 0;
            s_deviceOrientationOverride.gamma =
                (cmd.params() && cmd.params()->HasMember("gamma") &&
                 (*cmd.params())["gamma"].IsNumber())
                    ? (*cmd.params())["gamma"].GetDouble()
                    : 0;
            cmd.sendResultEmpty();
        } else if (method == "clearDeviceOrientationOverride") {
            s_deviceOrientationOverride.active = false;
            s_deviceOrientationOverride.alpha = 0;
            s_deviceOrientationOverride.beta = 0;
            s_deviceOrientationOverride.gamma = 0;
            cmd.sendResultEmpty();
        } else {
            // enable / disable and any other DeviceOrientation.* methods.
            cmd.sendResultEmpty();
        }
    } else if (domain == "SystemInfo") {
        if (method == "getInfo") {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            // gpu: minimal GPUInfo with empty device list.
            rapidjson::Value gpu(rapidjson::kObjectType);
            gpu.AddMember("devices", rapidjson::Value(rapidjson::kArrayType),
                          alloc);
            gpu.AddMember("auxAttributes",
                          rapidjson::Value(rapidjson::kObjectType), alloc);
            gpu.AddMember("featureStatus",
                          rapidjson::Value(rapidjson::kObjectType), alloc);
            gpu.AddMember("driverBugWorkarounds",
                          rapidjson::Value(rapidjson::kArrayType), alloc);
            result.AddMember("gpu", gpu, alloc);
            result.AddMember("modelName", "", alloc);
            result.AddMember("modelVersion", "", alloc);
            result.AddMember("commandLine", "", alloc);
            // os string from uname (e.g. "Linux 6.x x86_64").
            std::string os;
            struct utsname uts;
            if (uname(&uts) == 0) {
                os = std::string(uts.sysname) + " " + uts.release + " " +
                     uts.machine;
            }
            result.AddMember(
                "os", rapidjson::Value(os.c_str(), os.size(), alloc), alloc);
            cmd.sendResult(result, doc);
        } else if (method == "getProcessInfo") {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            rapidjson::Value arr(rapidjson::kArrayType);
            rapidjson::Value pi(rapidjson::kObjectType);
            pi.AddMember("type", "browser", alloc);
            pi.AddMember("id", (int)getpid(), alloc);
            pi.AddMember("cpuTime", 0.0, alloc);
            arr.PushBack(pi, alloc);
            result.AddMember("processInfo", arr, alloc);
            cmd.sendResult(result, doc);
        } else {
            cmd.sendError(-32601, "'method' wasn't found");
        }
    } else if (domain == "PerformanceTimeline") {
        // The engine exposes only mark/measure/resource PerformanceEntry types
        // and has no PerformanceObserver, LayoutShift, or LCP/CLS source, so no
        // PerformanceTimeline.timelineEventAdded can be produced. enable
        // records the requested eventTypes filter and acks; the event itself is
        // never emitted (nothing observable feeds it).
        if (method == "enable") {
            session()->performanceTimelineEnabled = true;
            cmd.sendResultEmpty();
        } else {
            // disable / any other PerformanceTimeline.* method.
            session()->performanceTimelineEnabled = false;
            cmd.sendResultEmpty();
        }
    } else if (domain == "Audits") {
        // Audits.getEncodedResponse reports the captured response body size for
        // a requestId; the engine performs no re-encoding, so only originalSize
        // is returned (encodedSize/body are omitted, matching a no-op encode).
        // The remaining Audits.* methods (enable/disable/checkContrast/
        // checkFormsIssues...) have no audit engine behind them and are acked
        // with valid results. Audits.issueAdded is never emitted.
        if (method == "enable") {
            session()->auditsEnabled = true;
            cmd.sendResultEmpty();
        } else if (method == "disable") {
            session()->auditsEnabled = false;
            cmd.sendResultEmpty();
        } else if (method == "getEncodedResponse") {
            std::string requestId;
            if (cmd.params() && cmd.params()->HasMember("requestId") &&
                (*cmd.params())["requestId"].IsString()) {
                requestId = (*cmd.params())["requestId"].GetString();
            }
            CDPSession* s = session();
            auto it = s->networkBodies.find(requestId);
            size_t originalSize =
                (it != s->networkBodies.end()) ? it->second.size() : 0;
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            // No re-encoding is performed, so encodedSize equals originalSize
            // and the (optional) re-encoded body is omitted.
            result.AddMember("originalSize", (int64_t)originalSize, alloc);
            result.AddMember("encodedSize", (int64_t)originalSize, alloc);
            cmd.sendResult(result, doc);
        } else if (method == "checkFormsIssues") {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            result.AddMember("formIssues",
                             rapidjson::Value(rapidjson::kArrayType), alloc);
            cmd.sendResult(result, doc);
        } else {
            // checkContrast / setEncodedResponseSizeLimit / any other Audits.*.
            cmd.sendResultEmpty();
        }
    } else {
        // MVP: domains outside the implemented ones (Emulation/WebMCP...) are
        // acked empty so DevTools clients that issue enable/disable handshakes
        // can proceed. Not a real implementation.
        cmd.sendResultEmpty();
    }
}

void CDPDispatcher::emitConsoleForWebView(WebView* webView, const char* level,
                                          const std::string& text,
                                          Escargot::ValueRef** argv,
                                          size_t argc)
{
    // Route console output to the TargetContext (session) that owns the WebView
    // the console call originated from, so each tab's console reaches its own
    // CDP session (puppeteer page.on('console')).
    TargetContext* ctx = nullptr;
    for (TargetContext* c : m_contexts) {
        if (c->webView == webView) {
            ctx = c;
            break;
        }
    }
    if (!ctx) {
        return;
    }
    CDPSession* m_session = ctx->session;
    if (!m_session->logEnabled && !m_session->runtimeEnabled) {
        return;
    }

    // Event commands carry the attached session's id (if any).
    double ts = (double)longTickCount();

    // The incoming `level` is the console method category passed by Console.cpp
    // (one of: log, info, error, warning, verbose). The two CDP targets accept
    // different enums, so map per target:
    //   Log.entryAdded.level     : verbose | info | warning | error
    //   Runtime.consoleAPICalled.type : log | debug | info | error | warning
    std::string lv(level);
    const char* logLevel = "info";   // Log.entryAdded.level
    const char* runtimeType = "log"; // Runtime.consoleAPICalled.type
    if (lv == "info") {
        logLevel = "info";
        runtimeType = "info";
    } else if (lv == "error") {
        logLevel = "error";
        runtimeType = "error";
    } else if (lv == "warning") {
        logLevel = "warning";
        runtimeType = "warning";
    } else if (lv == "verbose") { // console.debug
        logLevel = "verbose";
        runtimeType = "debug";
    } else { // "log" (and any fallthrough)
        logLevel = "info";
        runtimeType = "log";
    }

    if (m_session->logEnabled) {
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value params(rapidjson::kObjectType);
        rapidjson::Value entry(rapidjson::kObjectType);
        entry.AddMember("source", "console-api", alloc);
        entry.AddMember("level", rapidjson::Value(logLevel, alloc), alloc);
        entry.AddMember(
            "text", rapidjson::Value(text.c_str(), text.size(), alloc), alloc);
        entry.AddMember("timestamp", ts, alloc);
        entry.AddMember("url", "", alloc);
        params.AddMember("entry", entry, alloc);
        CDPCommand evt(this, Optional<int64_t>(), m_session->sessionId,
                       nullptr);
        evt.sendEvent("Log.entryAdded", params, doc);
    }

    if (m_session->runtimeEnabled) {
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value params(rapidjson::kObjectType);
        params.AddMember("type", rapidjson::Value(runtimeType, alloc), alloc);
        params.AddMember("timestamp", ts, alloc);
        params.AddMember("executionContextId",
                         (int)m_session->executionContextId, alloc);
        rapidjson::Value args(rapidjson::kArrayType);
        // Per-arg typed serialization: when the JS console call's original
        // arguments are available, serialize each as a typed RemoteObject so
        // puppeteer's msg.args() exposes number/string/object/... individually.
        BrowsingContext* bc =
            webView ? webView->mainBrowsingContext() : nullptr;
        ScriptBindingInstance* sbi = bc ? bc->scriptBindingInstance() : nullptr;
        if (argc > 0 && argv && sbi && sbi->isScriptingEnabled()) {
            for (size_t i = 0; i < argc; i++) {
                rapidjson::Value arg(rapidjson::kObjectType);
                serializeRemoteObject(sbi, ctx->remoteObjectStore, argv[i],
                                      false, arg, alloc);
                args.PushBack(arg, alloc);
            }
        } else {
            // Fallback (no original args, e.g. internal single-string console
            // calls): a single string arg holding the concatenated text.
            rapidjson::Value arg(rapidjson::kObjectType);
            arg.AddMember("type", "string", alloc);
            arg.AddMember("value",
                          rapidjson::Value(text.c_str(), text.size(), alloc),
                          alloc);
            args.PushBack(arg, alloc);
        }
        params.AddMember("args", args, alloc);
        CDPCommand evt(this, Optional<int64_t>(), m_session->sessionId,
                       nullptr);
        evt.sendEvent("Runtime.consoleAPICalled", params, doc);
    }
}

void CDPDispatcher::emitBindingCalled(WebView* webView, const std::string& name,
                                      const std::string& payload)
{
    // Route to the TargetContext (session) owning the WebView the binding call
    // originated from, mirroring emitConsoleForWebView.
    TargetContext* ctx = nullptr;
    for (TargetContext* c : m_contexts) {
        if (c->webView == webView) {
            ctx = c;
            break;
        }
    }
    if (!ctx) {
        return;
    }
    CDPSession* session = ctx->session;
    if (!session->runtimeEnabled) {
        return;
    }

    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    rapidjson::Value params(rapidjson::kObjectType);
    params.AddMember("name", rapidjson::Value(name.c_str(), name.size(), alloc),
                     alloc);
    params.AddMember("payload",
                     rapidjson::Value(payload.c_str(), payload.size(), alloc),
                     alloc);
    params.AddMember("executionContextId", (int)session->executionContextId,
                     alloc);
    CDPCommand evt(this, Optional<int64_t>(), session->sessionId, nullptr);
    evt.sendEvent("Runtime.bindingCalled", params, doc);
}

void CDPDispatcher::emitChildFrameLoaded(WebView* webView)
{
    // Route to the TargetContext owning the WebView the child frame belongs to.
    TargetContext* ctx = nullptr;
    for (TargetContext* c : m_contexts) {
        if (c->webView == webView) {
            ctx = c;
            break;
        }
    }
    if (!ctx) {
        return;
    }
    CDPSession* session = ctx->session;
    // Only meaningful once a client has attached and enabled Page/Runtime.
    if (!session->pageEnabled && !session->runtimeEnabled) {
        return;
    }

    // Child-frame discovery reads m_current (session()/webView()); point it at
    // this context for the duration of the emit, then restore. Single-threaded,
    // so no reentrancy concern.
    TargetContext* saved = m_current;
    m_current = ctx;
    m_page->discoverChildFrames(session->sessionId);
    m_current = saved;
}

void CDPDispatcher::emitJavaScriptDialogOpening(
    WebView* webView, const std::string& url, const std::string& message,
    const char* type, const std::string& defaultPrompt)
{
    // Route to the TargetContext (session) owning the WebView the dialog
    // originated from, mirroring emitConsoleForWebView. Gated on Page.enable so
    // a non-attached/non-page client doesn't receive the event.
    TargetContext* ctx = nullptr;
    for (TargetContext* c : m_contexts) {
        if (c->webView == webView) {
            ctx = c;
            break;
        }
    }
    if (!ctx) {
        return;
    }
    CDPSession* session = ctx->session;
    if (!session->pageEnabled) {
        return;
    }

    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    rapidjson::Value params(rapidjson::kObjectType);
    params.AddMember("url", rapidjson::Value(url.c_str(), url.size(), alloc),
                     alloc);
    params.AddMember("message",
                     rapidjson::Value(message.c_str(), message.size(), alloc),
                     alloc);
    params.AddMember("type", rapidjson::Value(type, alloc), alloc);
    // hasBrowserHandler:false tells the client no native UI will block; the
    // page has already proceeded with the default value (single-thread MVP).
    params.AddMember("hasBrowserHandler", false, alloc);
    params.AddMember(
        "defaultPrompt",
        rapidjson::Value(defaultPrompt.c_str(), defaultPrompt.size(), alloc),
        alloc);
    CDPCommand evt(this, Optional<int64_t>(), session->sessionId, nullptr);
    evt.sendEvent("Page.javascriptDialogOpening", params, doc);
}

} // namespace Starfish

#endif
