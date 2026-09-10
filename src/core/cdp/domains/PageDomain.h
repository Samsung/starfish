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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPPageDomain__)
#define __StarfishCDPPageDomain__

#include <string>

namespace Starfish {

class CDPDispatcher;
class CDPCommand;
class BrowsingContext;
class CDPSession;
class WebView;

class PageDomain {
public:
    PageDomain(CDPDispatcher* d)
        : m_dispatcher(d)
    {
    }
    void processMessage(CDPCommand& cmd, const std::string& method);

    // Resume a navigation that Fetch.enable parked. Called from FetchDomain
    // when continueRequest/fulfillRequest/disable arrives: runs the real
    // document load (loadHTMLDocument, possibly to a fulfill data: URL) and
    // finishes the lifecycle. The pre-navigation signalling (beginNavigation)
    // already ran.
    void completeDeferredNavigation(const std::string& sessionId,
                                    const std::string& url);
    // Abandon a parked navigation (Fetch.failRequest). Emits loadingFailed and
    // finishes the lifecycle without loading a document.
    void failDeferredNavigation(const std::string& sessionId,
                                const std::string& url);

    // Page.setDocumentContent: replace the main document with `html` by loading
    // it as a data:text/html URL through the same beginNavigation /
    // loadHTMLDocument / finishNavigation path navigate uses, so the new
    // document gets a fresh loaderId, lifecycle (init..load), execution context
    // and re-announced isolated worlds.
    void setDocumentContentFromHtml(const std::string& sessionId,
                                    const std::string& html);

    // Drive the post-rewrite CDP lifecycle after the engine has already swapped
    // the document in place (puppeteer page.setContent runs
    // document.open()/write()/close() via Runtime.evaluate, which the engine
    // applies synchronously). Bumps the loaderId, emits frameStartedLoading +
    // the "init" lifecycle (so puppeteer's LifecycleWatcher sees a new loader),
    // then runs finishNavigation for the rest (contexts, lifecycle, load). No
    // document is loaded here -- the content is already present.
    void notifyDocumentRewritten(const std::string& sessionId);

    // Walk the live child BrowsingContext tree, (re)assign each iframe frame a
    // stable frameId + executionContextId in the current session, and emit
    // Page.frameAttached / Page.frameNavigated /
    // Runtime.executionContextCreated for any not yet announced. Called after a
    // top-level navigation completes (layout has run, so child contexts exist).
    void discoverChildFrames(const std::string& sessionId);

    // Ensure a ChildFrame record (frameId + contextId) exists for every live
    // child iframe, without emitting any events. Used by getFrameTree so the
    // childFrames it returns carry the same ids the events will use.
    void ensureChildFrameRecords();

    // Diff recorded child frames against the live tree: emit Page.frameDetached
    // (reason "remove") for any frame whose BrowsingContext is no longer
    // reachable (its iframe, or an ancestor's, was removed from the DOM), and
    // drop its record + child-bound isolated worlds. Called before each
    // (re)discovery and whenever the frame tree is queried.
    void sweepDetachedFrames(const std::string& sessionId);

    // Resolve a Runtime executionContextId to its child iframe BrowsingContext.
    // An unknown id returns an empty value, so the caller can use the main one.
    // Re-walks the live tree by ordinal each call (no stored GC pointers).
    Optional<BrowsingContext*> browsingContextForExecutionContextId(
        uint32_t contextId);
    Optional<BrowsingContext*> browsingContextForFrameId(
        const std::string& frameId);
    std::string frameIdForBrowsingContext(BrowsingContext* context);

    // Stop an active Page.startScreencast: cancel the repetitive capture timer
    // and clear the screencast session state. Safe to call when inactive.
    // Invoked by Page.stopScreencast and on connection teardown.
    void stopScreencast(CDPSession* s, WebView* wv);

private:
    // Emit Page.lifecycleEvent {frameId, loaderId, name, timestamp} for the
    // current frame/loader when lifecycle events are enabled.
    void emitLifecycle(const std::string& sessionId, const char* name);
    // Emit Runtime.executionContextCreated for the (new) default context.
    void emitExecutionContextCreated(const std::string& sessionId);
    // Emit Runtime.executionContextCreated for one isolated world (re-used by
    // createIsolatedWorld and by navigate to re-announce existing worlds).
    void emitIsolatedWorldCreated(const std::string& sessionId,
                                  uint32_t contextId,
                                  const std::string& worldName,
                                  const std::string& uniqueId,
                                  const std::string& frameId);

    // Pre-navigation CDP signalling shared by navigate and
    // navigateToHistoryEntry: bump loaderId, frameStartedLoading, init
    // lifecycle, and the synthetic network triple for the document load.
    void beginNavigation(const std::string& sessionId, const std::string& url);
    // Post-navigation CDP signalling shared by navigate and
    // navigateToHistoryEntry: re-inject evaluateOnNewDocument scripts and
    // bindings, reset handle registries, then emit frameNavigated /
    // documentUpdated / executionContextsCleared+Created (with isolated worlds)
    // / lifecycle / loadEventFired / frameStoppedLoading.
    void finishNavigation(const std::string& sessionId);

    // Capture `wv`'s renderer framebuffer and emit one Page.screencastFrame
    // (PNG base64 + metadata). frameNumber is sent as the frame's sessionId.
    // Reuses the captureScreenshot readback path. wv is passed explicitly so a
    // timer-driven call does not depend on the dispatcher's per-dispatch
    // current target.
    void emitScreencastFrame(WebView* wv, const std::string& sessionId,
                             int frameNumber);
    // Static Timer callback for the repetitive screencast capture timer; `data`
    // is a malloc'd ScreencastTimerData re-resolving the live session.
    static void onScreencastTimer(void* data);

    CDPDispatcher* m_dispatcher;
};

} // namespace Starfish

#endif
