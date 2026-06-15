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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPNetworkDomain__)
#define __StarfishCDPNetworkDomain__

#include <map>
#include <string>

#include "rapidjson/document.h"

namespace Starfish {

class CDPDispatcher;
class CDPCommand;
class CDPSession;
class WebView;
class Resource;
class ResourceRequest;

class NetworkDomain {
public:
    NetworkDomain(CDPDispatcher* d)
        : m_dispatcher(d)
    {
    }
    void processMessage(CDPCommand& cmd, const std::string& method);

    // --- Real ResourceLoader hook (main thread) -------------------------
    // Called from Resource lifecycle methods when a real network resource is
    // requested / responds / completes / fails. Resolves the CDP session that
    // owns `wv` and, if Network is enabled, emits the matching real
    // Network.* events with actual url/method/status/headers/mimeType and
    // stores the body for getResponseBody. No-op if the WebView has no session
    // or Network is not enabled.
    //
    // onResourceWillBeSent returns the requestId assigned to the resource (the
    // loaderId for the top-level document, REQ-n for subresources), to be
    // passed back into the later phase callbacks. Returns "" when not emitted.
    // postData is the encoded request entity body for this load (empty for GET
    // / bodyless requests); when non-empty it is stored for
    // Network.getRequestPostData and surfaced in the requestWillBeSent event.
    std::string onResourceWillBeSent(WebView* wv, Resource* res,
                                     const std::string& postData);
    void onResourceResponse(WebView* wv, const std::string& requestId,
                            Resource* res, ResourceRequest* rr);
    void onResourceData(WebView* wv, const std::string& requestId,
                        const char* buf, size_t length);
    void onResourceFinished(WebView* wv, const std::string& requestId);
    void onResourceFailed(WebView* wv, const std::string& requestId);

    // Decide whether a real outgoing request should be blocked, based on the
    // session's Network.emulateNetworkConditions(offline) / setBlockedURLs
    // state. Returns true and fills `errorText` (Chrome net:: error string)
    // when the request must be failed; the caller then aborts the load instead
    // of sending it. No-op (returns false) unless Network is enabled and the
    // resource is a real http(s) request -- non-network schemes (data:, etc.)
    // are never blocked, matching Chrome's offline behaviour.
    bool shouldBlockRequest(WebView* wv, Resource* res, std::string& errorText);
    // URL-string form of the same decision, for the navigate handler which must
    // decide before a Resource exists. http(s) only; fills `errorText`.
    bool shouldBlockUrl(CDPSession* s, const std::string& url,
                        std::string& errorText);
    // Emit Network.loadingFailed for a blocked request with an explicit
    // errorText (e.g. ERR_INTERNET_DISCONNECTED / ERR_BLOCKED_BY_CLIENT).
    void emitLoadingFailed(WebView* wv, const std::string& requestId,
                           const std::string& errorText);

    // Inject Network.setExtraHTTPHeaders into a real outgoing request. Called
    // from the network hook before the request is sent. No-op if no session or
    // no extra headers set.
    void applyExtraHTTPHeaders(WebView* wv, ResourceRequest* rr);

    // --- networkidle lifecycle (real in-flight) -------------------------
    // Reset in-flight tracking + idle-emitted guards for a new navigation on
    // the session owning `wv`, cancelling any pending debounce timer. Called
    // from PageDomain.beginNavigation so each document starts from a clean
    // state.
    void resetNetworkIdle(WebView* wv);
    // Arm/cancel the 500ms networkidle debounce on the session owning `wv`.
    // Called after the in-flight count changes (a request started or finished):
    // when in-flight is small enough for a networkidle level whose event has
    // not yet fired this navigation, (re)arms the timer; a still-pending
    // request keeps the timer running so a new request resets the 500ms window.
    void scheduleNetworkIdleCheck(WebView* wv);

    // Synthesize the network event triple for a top-level navigation:
    // requestWillBeSent -> responseReceived -> loadingFinished. There is no
    // real ResourceLoader hook in this MVP, so these are composed from the
    // navigation URL/loaderId to satisfy clients that observe document loads.
    // No-op unless Network is enabled.
    void emitNavigation(const std::string& sessionId, const std::string& url);

    // Split phases for the Fetch interception path: the request fires when the
    // navigation is paused, the response/loadingFinished only after it resumes.
    // emitNavigation() calls both back to back for the non-intercepted path.
    void emitNavigationRequest(const std::string& sessionId,
                               const std::string& url);
    void emitNavigationResponse(const std::string& sessionId,
                                const std::string& url);

    // --- Shared cookie-jar helpers (reused by the Storage domain) -------
    // These operate directly on the process-wide curl cookie jar
    // (NetworkSharedResourceManager), the same backing store the Network
    // cookie methods use, so Storage.getCookies/setCookies/clearCookies share
    // state with Network.getCookies/setCookie/deleteCookies. currentHost/Path
    // (when non-empty) supply the fallback domain/path for cookies whose
    // url/domain is omitted.
    //
    // Append every cookie currently in the jar to `arr` as CDP Network.Cookie
    // objects (skipping expired). Static: no per-instance state.
    static void appendAllCookies(rapidjson::Value& arr,
                                 rapidjson::Document::AllocatorType& alloc);
    // Write the cookies in the `cookies` array (CDP CookieParam form) into the
    // jar. currentHost/currentPath are used when a cookie omits url/domain.
    static void writeCookieArray(const rapidjson::Value& cookies,
                                 const std::string& currentHost,
                                 const std::string& currentPath);
    // Expire every cookie in the jar (Storage.clearCookies).
    static void clearAllCookies();
    // Expire every cookie whose stored domain covers `host` (clearDataForOrigin
    // cookies). Empty host is a no-op.
    static void clearCookiesForHost(const std::string& host);

private:
    // Resolve the CDP session that owns a WebView (nullptr if none / detached).
    CDPSession* sessionForWebView(WebView* wv);

    // Timer::addTimer callback for the 500ms networkidle debounce. `data` is a
    // heap NetworkIdleTimerData (carries dispatcher + sessionId + loader
    // generation); the handler resolves the session, ignores stale fires, and
    // emits the reached networkidle lifecycle level(s).
    static void onNetworkIdleTimer(void* data);

    CDPDispatcher* m_dispatcher;
};

} // namespace Starfish

#endif
