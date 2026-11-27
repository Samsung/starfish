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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPDispatcher__)
#define __StarfishCDPDispatcher__

#include <array>
#include <mutex>
#include <string>
#include <vector>
#include "CDPDomain.h"
#include "TargetContext.h"

namespace Escargot {
class ValueRef;
} // namespace Escargot

namespace Starfish {

class WebView;
class BrowsingContext;
class Document;
class Element;
class CDPServer;
class CDPSession;
class CDPCommand;
class NodeRegistry;
class RemoteObjectStore;
class TargetDomain;
class PageDomain;
class RuntimeDomain;
class DOMDomain;
class DOMDebuggerDomain;
class LogDomain;
class NetworkDomain;
class FetchDomain;
class InputDomain;
class EmulationDomain;
class CSSDomain;
class DOMSnapshotDomain;
class DOMStorageDomain;
class StorageDomain;
class AccessibilityDomain;
class PerformanceDomain;
class MemoryDomain;
class AnimationDomain;
class TracingDomain;
class OverlayDomain;

// "Domain.method" routing. Main-thread only handler execution.
// GC: not inherited; the held GC maps (NodeRegistry/RemoteObjectStore) are
// rooted via GC_add_roots in the constructor (see .cpp).
class CDPDispatcher {
public:
    CDPDispatcher(CDPServer* server, WebView* webView);
    ~CDPDispatcher();

    // Called on IO thread. raw UTF-8 only. Delegates to main thread.
    void onMessageFromIO(std::string&& rawJson);

    // Called on IO thread when the single client connection drops. Posts a
    // reset onto the main loop so a reconnecting client gets a fresh attach
    // handshake (session attach/enable flags are connection-scoped and would
    // otherwise leak across connections, leaving browser.pages() empty).
    void onConnectionClosed();

    // Main-thread processing (delegation target). rapidjson parse + routing.
    void dispatchOnMain(const std::string& rawJson);

    // Console bridge (main thread). level: "log"/"info"/"warning"/"error"/
    // "debug". Emits Log.entryAdded (if logEnabled) and
    // Runtime.consoleAPICalled (if runtimeEnabled) on the session of the
    // TargetContext that owns `webView`. No-op if the WebView has no context.
    // `text` is the concatenated message (used for Log.entryAdded.text). When
    // argv/argc are provided (argc > 0), Runtime.consoleAPICalled.args is
    // serialized per-argument as typed RemoteObjects
    // (number/string/object/...); otherwise it falls back to a single string
    // arg holding `text`.
    void emitConsoleForWebView(WebView* webView, const char* level,
                               const std::string& text,
                               Escargot::ValueRef** argv = nullptr,
                               size_t argc = 0);

    // Binding bridge (main thread). Emits Runtime.bindingCalled on the session
    // owning `webView`, for a window[name] native function injected via
    // Runtime.addBinding. Called from the injected function's native callback.
    void emitBindingCalled(WebView* webView, const std::string& name,
                           const std::string& payload);

    // Child frame bridge (main thread). Called when a child iframe's
    // BrowsingContext finishes loading (ResourceLoader load-complete path).
    // Runs PageDomain child-frame discovery on the session owning `webView`, so
    // a frame loaded asynchronously (after the synchronous navigate handler) is
    // announced via Page.frameAttached/frameNavigated +
    // executionContextCreated. No-op if the WebView has no context.
    void emitChildFrameLoaded(WebView* webView);

    void emitAnchorNavigationRequested(BrowsingContext* context, String* url);

    // Dialog bridge (main thread). Emits Page.javascriptDialogOpening on the
    // session owning `webView` when Page is enabled, for a window
    // alert/confirm/prompt call. type is "alert"/"confirm"/"prompt".
    // No-op if the WebView has no context or Page is not enabled.
    void emitJavaScriptDialogOpening(WebView* webView, const std::string& url,
                                     const std::string& message,
                                     const char* type,
                                     const std::string& defaultPrompt);

    // These resolve to the currently-routed target's state. dispatchOnMain
    // selects the target from the command's sessionId before routing, so
    // domain handlers transparently operate on the right WebView/session.
    TargetContext* currentContext()
    {
        return m_current;
    }
    TargetDomain* target()
    {
        return m_target;
    }
    CDPSession* session()
    {
        return m_current->session;
    }
    NodeRegistry* nodeRegistry()
    {
        return m_current->nodeRegistry;
    }
    RemoteObjectStore* remoteObjectStore()
    {
        return m_current->remoteObjectStore;
    }
    WebView* webView()
    {
        return m_current->webView;
    }
    CDPServer* server()
    {
        return m_server;
    }
    NetworkDomain* network()
    {
        return m_network;
    }
    FetchDomain* fetch()
    {
        return m_fetch;
    }
    PageDomain* page()
    {
        return m_page;
    }
    AccessibilityDomain* accessibility()
    {
        return m_accessibility;
    }

    // Point the per-dispatch context at `context` for as long as this object
    // lives. An event emitted outside a command (from a timer or an engine
    // hook) has no dispatch to select the context, and the domain code it
    // calls reads session()/webView() from there. Single-threaded, so the
    // save-and-restore cannot interleave.
    class ScopedTarget {
    public:
        ScopedTarget(CDPDispatcher* dispatcher, TargetContext* context)
            : m_dispatcher(dispatcher)
            , m_saved(dispatcher->m_current)
        {
            m_dispatcher->m_current = context;
        }
        ~ScopedTarget()
        {
            m_dispatcher->m_current = m_saved;
        }

    private:
        CDPDispatcher* m_dispatcher;
        TargetContext* m_saved;
    };
    RuntimeDomain* runtime()
    {
        return m_runtime;
    }
    AnimationDomain* animation()
    {
        return m_animation;
    }

    // Multi-target support. Create a new TargetContext around a freshly spawned
    // WebView (used by Target.createTarget). Returns the new context
    // (registered and GC-rooted). targetId/sessionId are issued by the caller
    // into ctx.
    TargetContext* createContext(WebView* wv, bool ownsWebView);
    void destroyContext(TargetContext* ctx);
    // Look up a context by its attached sessionId (nullptr if not found).
    TargetContext* contextForSession(const std::string& sessionId);
    // Look up the context owning a WebView (nullptr if not found). Used by the
    // network hook to route real ResourceLoader events to the right session.
    TargetContext* contextForWebView(WebView* wv);
    // The initial (first-WebView) context.
    TargetContext* initialContext()
    {
        return m_contexts.front();
    }
    // HTTP discovery body for /json and /json/list, kept fresh as tabs are
    // created or destroyed and as documents navigate (the previous
    // hardcoded single-entry body never showed spawned tabs).
    // discoveryListJson() answers on the IO thread and returns the cached
    // string; only rebuildDiscoveryJson() reads GC objects and m_contexts,
    // and it runs on the main thread exclusively.
    void rebuildDiscoveryJson();
    std::string discoveryListJson();

    const std::vector<TargetContext*>& contexts() const
    {
        return m_contexts;
    }

    // Flat browser-target session (Target.attachToBrowserTarget). Playwright's
    // connectOverCDP attaches the browser target first, then routes page
    // attaches through it; this id, when present, resolves to the initial
    // context (browser/connection-level scope). Empty until attached.
    const std::string& browserSessionId() const
    {
        return m_browserSessionId;
    }
    void setBrowserSessionId(const std::string& id)
    {
        m_browserSessionId = id;
    }

private:
    void routeIO(CDPCommand& cmd, const std::string& method);
    void routeSecurity(CDPCommand& cmd, const std::string& method);
    void routeBrowser(CDPCommand& cmd, const std::string& method);
    void routeProfiler(CDPCommand& cmd, const std::string& method);
    void routeHeapProfiler(CDPCommand& cmd, const std::string& method);
    void routeWebAuthn(CDPCommand& cmd, const std::string& method);
    void routeDebugger(CDPCommand& cmd, const std::string& method);
    void routeExtensions(CDPCommand& cmd, const std::string& method);
    void routeTethering(CDPCommand& cmd, const std::string& method);
    void routeCast(CDPCommand& cmd, const std::string& method);
    void routeDeviceAccess(CDPCommand& cmd, const std::string& method);
    void routeDatabase(CDPCommand& cmd, const std::string& method);
    void routeFedCm(CDPCommand& cmd, const std::string& method);
    void routeAutofill(CDPCommand& cmd, const std::string& method);
    void routeBackgroundService(CDPCommand& cmd, const std::string& method);
    void routeEventBreakpoints(CDPCommand& cmd, const std::string& method);
    void routePreload(CDPCommand& cmd, const std::string& method);
    void routeLayerTree(CDPCommand& cmd, const std::string& method);
    void routeMedia(CDPCommand& cmd, const std::string& method);
    void routeWebAudio(CDPCommand& cmd, const std::string& method);
    void routeServiceWorker(CDPCommand& cmd, const std::string& method);
    void routeIndexedDB(CDPCommand& cmd, const std::string& method);
    void routeCacheStorage(CDPCommand& cmd, const std::string& method);
    void routeSchema(CDPCommand& cmd, const std::string& method);
    void routeInspector(CDPCommand& cmd, const std::string& method);
    void routeDeviceOrientation(CDPCommand& cmd, const std::string& method);
    void routeSystemInfo(CDPCommand& cmd, const std::string& method);
    void routePerformanceTimeline(CDPCommand& cmd, const std::string& method);
    void routeAudits(CDPCommand& cmd, const std::string& method);
    void webAuthnEnable(CDPCommand& cmd, const std::string& method);
    void webAuthnDisable(CDPCommand& cmd, const std::string& method);
    void webAuthnAddVirtualAuthenticator(CDPCommand& cmd,
                                         const std::string& method);

    static void onMainTrampoline(size_t handle, void* data);
    static void onConnectionClosedOnMain(size_t handle, void* data);
    void resetConnectionState();
    void route(CDPCommand& cmd, const std::string& domain,
               const std::string& method);

    struct NamedDomain {
        const char* name;
        CDPDomain* domain;
    };
    static const size_t domainCount = 20;
    // Every protocol domain object, in creation order.
    std::array<NamedDomain, domainCount> domains() const;

    CDPServer* m_server;
    std::vector<TargetContext*> m_contexts; // [0] is the initial target
    TargetContext* m_current;               // selected per-dispatch
    std::string m_browserSessionId;         // Target.attachToBrowserTarget
    // Cached /json/list body: rebuilt under m_discoveryMutex on the main
    // thread, copied to the IO thread by discoveryListJson().
    std::mutex m_discoveryMutex;
    std::string m_discoveryJson;
    TargetDomain* m_target;
    PageDomain* m_page;
    RuntimeDomain* m_runtime;
    DOMDomain* m_dom;
    DOMDebuggerDomain* m_domDebugger;
    LogDomain* m_log;
    NetworkDomain* m_network;
    FetchDomain* m_fetch;
    InputDomain* m_input;
    EmulationDomain* m_emulation;
    CSSDomain* m_css;
    DOMSnapshotDomain* m_domSnapshot;
    DOMStorageDomain* m_domStorage;
    StorageDomain* m_storage;
    AccessibilityDomain* m_accessibility;
    PerformanceDomain* m_performance;
    MemoryDomain* m_memory;
    AnimationDomain* m_animation;
    TracingDomain* m_tracing;
    OverlayDomain* m_overlay;
};

// The dispatcher of the client attached to the page of document, or nullptr
// when no client is attached (the common case).
CDPDispatcher* cdpDispatcherForDocument(Document* document);
// The target of the page of document while it has a session, or nullptr.
// dispatcher is set as cdpDispatcherForDocument sets it.
TargetContext* cdpTargetForDocument(Document* document,
                                    CDPDispatcher*& dispatcher);
// The element the nodeId parameter of cmd names in the current target, or
// nullptr after replying that there is no such node.
Element* elementForNodeIdParam(CDPDispatcher* dispatcher, CDPCommand& cmd);

} // namespace Starfish

#endif
