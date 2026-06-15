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
#include "TargetDomain.h"
#include "../CDPDispatcher.h"
#include "../CDPCommand.h"
#include "../CDPSession.h"
#include "../TargetContext.h"

#include "core/page/WebView.h"
#include "core/page/BrowsingContext.h"
#include "core/dom/Document.h"
#include "core/style/ComputedStyle.h"
#include "core/util/AtomicString.h"

#include "rapidjson/document.h"

namespace Starfish {

static std::string targetUrl(WebView* wv)
{
    if (wv) {
        BrowsingContext* bc = wv->mainBrowsingContext();
        if (bc && bc->document() && bc->document()->urlString()) {
            std::string u = bc->document()->urlString()->toUTF8NonGCString();
            if (!u.empty()) {
                return u;
            }
        }
    }
    return "about:blank";
}

// Spawn a fresh, independent WebView cloning the source WebView's
// configuration (locale/timezone/font/screen/UA). The new WebView gets its own
// MessageLoop/Timer/Renderer; its glib idlers/timers register on the same
// default GMainContext as the source, so the single app main loop pumps it.
// The new WebView does NOT start a CDP server (only the source did, guarded by
// the env-gated setup in the WebView ctor + single-port bind).
static WebView* spawnWebView(WebView* src)
{
    const ScreenInfo& info = src->screenInfo();
    uint32_t w = (uint32_t)info.rect.size().width();
    uint32_t h = (uint32_t)info.rect.size().height();
    String* fontName = src->initialFontFamilyDatas()[1].m_familyName.string();
    return WebView::create(src->starfish(), src->locale().c_str(),
                           src->timezoneID()->toUTF8NonGCString().c_str(), w, h,
                           src->defaultFontSize(), fontName, info,
                           src->customUserAgentString(),
                           src->builtinPolyfillPathString());
}

static void buildTargetInfo(CDPSession* s, WebView* wv, bool attached,
                            rapidjson::Value& out,
                            rapidjson::Document::AllocatorType& alloc)
{
    std::string url = targetUrl(wv);
    out.SetObject();
    out.AddMember(
        "targetId",
        rapidjson::Value(s->targetId.c_str(), s->targetId.size(), alloc),
        alloc);
    out.AddMember("type", "page", alloc);
    out.AddMember("title", "", alloc);
    out.AddMember("url", rapidjson::Value(url.c_str(), url.size(), alloc),
                  alloc);
    out.AddMember("attached", attached, alloc);
    out.AddMember("canAccessOpener", false, alloc);
    out.AddMember("browserContextId",
                  rapidjson::Value(s->browserContextId.c_str(),
                                   s->browserContextId.size(), alloc),
                  alloc);
}

void TargetDomain::processMessage(CDPCommand& cmd, const std::string& method)
{
    CDPSession* s = m_dispatcher->session();
    WebView* wv = m_dispatcher->webView();

    if (method == "setDiscoverTargets") {
        bool discover = false;
        if (cmd.params() && cmd.params()->HasMember("discover") &&
            (*cmd.params())["discover"].IsBool()) {
            discover = (*cmd.params())["discover"].GetBool();
        }
        s->targetDiscoverEnabled = discover;
        cmd.sendResultEmpty();

        if (discover) {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value params(rapidjson::kObjectType);
            rapidjson::Value ti(rapidjson::kObjectType);
            buildTargetInfo(s, wv, false, ti, alloc);
            params.AddMember("targetInfo", ti, alloc);
            CDPCommand evt(m_dispatcher, Optional<int64_t>(), cmd.sessionId(),
                           nullptr);
            evt.sendEvent("Target.targetCreated", params, doc);
        }
        return;
    }

    if (method == "setAutoAttach") {
        bool autoAttach = false;
        if (cmd.params() && cmd.params()->HasMember("autoAttach") &&
            (*cmd.params())["autoAttach"].IsBool()) {
            autoAttach = (*cmd.params())["autoAttach"].GetBool();
        }
        s->targetAutoAttach = autoAttach;

        // Only the connection-level (sessionId-less) setAutoAttach should
        // produce the page target attachment. Puppeteer also issues
        // setAutoAttach on the page session itself once attached; emitting a
        // second attachedToTarget there would replace puppeteer's CDPSession
        // map entry and orphan in-flight page-init callbacks (hang). Guard so
        // attachedToTarget is emitted exactly once.
        //
        // The attachedToTarget event is emitted BEFORE the setAutoAttach
        // result so that puppeteer registers the page target/session before
        // the connection-level initialize() resolves connect(); otherwise
        // browser.pages() called right after connect() races ahead of the
        // attachment and returns 0.
        if (autoAttach && cmd.sessionId().empty() && !s->attachEmitted) {
            // Issue a session and emit Target.attachedToTarget.
            if (s->sessionId.empty()) {
                s->sessionId =
                    "SID-" + std::to_string(++s->sessionCounter) + "0000000";
            }
            s->attached = true;
            s->attachEmitted = true;

            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value params(rapidjson::kObjectType);
            params.AddMember("sessionId",
                             rapidjson::Value(s->sessionId.c_str(),
                                              s->sessionId.size(), alloc),
                             alloc);
            rapidjson::Value ti(rapidjson::kObjectType);
            buildTargetInfo(s, wv, true, ti, alloc);
            params.AddMember("targetInfo", ti, alloc);
            params.AddMember("waitingForDebugger", false, alloc);
            CDPCommand evt(m_dispatcher, Optional<int64_t>(), cmd.sessionId(),
                           nullptr);
            evt.sendEvent("Target.attachedToTarget", params, doc);
        }

        cmd.sendResultEmpty();
        return;
    }

    if (method == "getTargets") {
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value infos(rapidjson::kArrayType);
        for (TargetContext* ctx : m_dispatcher->contexts()) {
            rapidjson::Value ti(rapidjson::kObjectType);
            buildTargetInfo(ctx->session, ctx->webView, ctx->session->attached,
                            ti, alloc);
            infos.PushBack(ti, alloc);
        }
        result.AddMember("targetInfos", infos, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "getBrowserContexts") {
        // Return the contexts created via Target.createBrowserContext. The
        // default browser context is excluded (matches Chrome). The list is
        // tracked on the initial (connection-level) session.
        CDPSession* init = m_dispatcher->initialContext()->session;
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value ids(rapidjson::kArrayType);
        for (const std::string& bid : init->browserContexts) {
            ids.PushBack(rapidjson::Value(bid.c_str(), bid.size(), alloc),
                         alloc);
        }
        result.AddMember("browserContextIds", ids, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "createBrowserContext") {
        // Issue a fresh browser context id and track it on the initial
        // (connection-level) session. disposeOnDetach/proxyServer params are
        // accepted but not honored: starfish has a single shared cookie/storage
        // context, so this provides target grouping + lifecycle only, no real
        // isolation. createTarget({browserContextId}) associates targets with
        // it.
        CDPSession* init = m_dispatcher->initialContext()->session;
        std::string bid =
            "BID-" + std::to_string(++init->browserContextCounter) + "0000000";
        init->browserContexts.push_back(bid);

        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("browserContextId",
                         rapidjson::Value(bid.c_str(), bid.size(), alloc),
                         alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "disposeBrowserContext") {
        std::string bid;
        if (cmd.params() && cmd.params()->HasMember("browserContextId") &&
            (*cmd.params())["browserContextId"].IsString()) {
            bid = (*cmd.params())["browserContextId"].GetString();
        }
        CDPSession* init = m_dispatcher->initialContext()->session;

        // Close every spawned target associated with this browser context.
        // Collect first (destroyContext mutates the contexts vector).
        std::vector<TargetContext*> victims;
        for (TargetContext* ctx : m_dispatcher->contexts()) {
            if (ctx != m_dispatcher->initialContext() &&
                ctx->session->browserContextId == bid) {
                victims.push_back(ctx);
            }
        }
        for (TargetContext* victim : victims) {
            std::string vsid = victim->session->sessionId;
            std::string vtid = victim->session->targetId;
            WebView* vwv = victim->webView;
            bool owns = victim->ownsWebView;

            {
                rapidjson::Document doc;
                rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
                rapidjson::Value params(rapidjson::kObjectType);
                params.AddMember(
                    "sessionId",
                    rapidjson::Value(vsid.c_str(), vsid.size(), alloc), alloc);
                params.AddMember(
                    "targetId",
                    rapidjson::Value(vtid.c_str(), vtid.size(), alloc), alloc);
                CDPCommand evt(m_dispatcher, Optional<int64_t>(), std::string(),
                               nullptr);
                evt.sendEvent("Target.detachedFromTarget", params, doc);
            }
            {
                rapidjson::Document doc;
                rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
                rapidjson::Value params(rapidjson::kObjectType);
                params.AddMember(
                    "targetId",
                    rapidjson::Value(vtid.c_str(), vtid.size(), alloc), alloc);
                CDPCommand evt(m_dispatcher, Optional<int64_t>(), std::string(),
                               nullptr);
                evt.sendEvent("Target.targetDestroyed", params, doc);
            }

            m_dispatcher->destroyContext(victim);
            if (owns && vwv) {
                vwv->destroy();
            }
        }

        // Remove the context id from the tracked list.
        for (auto it = init->browserContexts.begin();
             it != init->browserContexts.end(); ++it) {
            if (*it == bid) {
                init->browserContexts.erase(it);
                break;
            }
        }

        cmd.sendResultEmpty();
        return;
    }

    if (method == "getTargetInfo") {
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value ti(rapidjson::kObjectType);
        buildTargetInfo(s, wv, s->attached, ti, alloc);
        result.AddMember("targetInfo", ti, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "attachToTarget") {
        if (s->sessionId.empty()) {
            s->sessionId =
                "SID-" + std::to_string(++s->sessionCounter) + "0000000";
        }
        s->attached = true;

        // Emit attachedToTarget event first.
        {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value params(rapidjson::kObjectType);
            params.AddMember("sessionId",
                             rapidjson::Value(s->sessionId.c_str(),
                                              s->sessionId.size(), alloc),
                             alloc);
            rapidjson::Value ti(rapidjson::kObjectType);
            buildTargetInfo(s, wv, true, ti, alloc);
            params.AddMember("targetInfo", ti, alloc);
            params.AddMember("waitingForDebugger", false, alloc);
            CDPCommand evt(m_dispatcher, Optional<int64_t>(), cmd.sessionId(),
                           nullptr);
            evt.sendEvent("Target.attachedToTarget", params, doc);
        }

        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember(
            "sessionId",
            rapidjson::Value(s->sessionId.c_str(), s->sessionId.size(), alloc),
            alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "attachToBrowserTarget") {
        // Playwright's connectOverCDP attaches the browser target first, then
        // routes page attaches (Target.attachToTarget) through the returned
        // flat session. The engine has no separate browser process/target, so
        // this mints a connection-scoped browser session id that the dispatcher
        // resolves to the initial context (browser/connection-level scope).
        std::string bsid = m_dispatcher->browserSessionId();
        if (bsid.empty()) {
            bsid = "BSID-" + std::to_string(++s->sessionCounter) + "0000000";
            m_dispatcher->setBrowserSessionId(bsid);
        }
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("sessionId",
                         rapidjson::Value(bsid.c_str(), bsid.size(), alloc),
                         alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "createTarget") {
        // Spawn a brand-new, independent WebView for this tab. It shares the
        // process Starfish + the single app glib main loop, but has its own
        // MessageLoop/Timer/Renderer and JS global, giving an independent DOM
        // and execution context.
        WebView* nwv = spawnWebView(wv);

        // Optional initial url ("about:blank" if omitted). Puppeteer's
        // newPage() passes "about:blank" and then navigates via Page.navigate.
        std::string url = "about:blank";
        if (cmd.params() && cmd.params()->HasMember("url") &&
            (*cmd.params())["url"].IsString()) {
            const char* u = (*cmd.params())["url"].GetString();
            if (u && *u) {
                url = u;
            }
        }
        nwv->loadHTMLDocument(String::fromUTF8(url.c_str(), url.size()));

        TargetContext* nctx =
            m_dispatcher->createContext(nwv, /*ownsWebView=*/true);

        // Issue fresh, unique ids off the connection-level (initial) session's
        // monotonic counters so all targets get distinct ids.
        CDPSession* init = m_dispatcher->initialContext()->session;
        uint32_t n = ++init->sessionCounter;
        std::string idNum = std::to_string(1000 + n);
        nctx->session->targetId = "TID-" + idNum;
        nctx->session->frameId = nctx->session->targetId;
        nctx->session->loaderId = "LID-" + idNum;
        // Associate the target with the requested browser context if given,
        // else with the default (initial session's) context.
        nctx->session->browserContextId = init->browserContextId;
        if (cmd.params() && cmd.params()->HasMember("browserContextId") &&
            (*cmd.params())["browserContextId"].IsString()) {
            const char* bc = (*cmd.params())["browserContextId"].GetString();
            if (bc && *bc) {
                nctx->session->browserContextId = bc;
            }
        }
        nctx->session->sessionId = "SID-" + std::to_string(n) + "1111111";
        nctx->session->attached = true;
        nctx->session->attachEmitted = true;

        CDPSession* ns = nctx->session;

        // Target.targetCreated (only if discovery is enabled on the
        // connection).
        if (init->targetDiscoverEnabled) {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value params(rapidjson::kObjectType);
            rapidjson::Value ti(rapidjson::kObjectType);
            buildTargetInfo(ns, nwv, true, ti, alloc);
            params.AddMember("targetInfo", ti, alloc);
            CDPCommand evt(m_dispatcher, Optional<int64_t>(), cmd.sessionId(),
                           nullptr);
            evt.sendEvent("Target.targetCreated", params, doc);
        }

        // Target.attachedToTarget (flatten): puppeteer maps the new session to
        // a Page. Emitted at the connection level (no parent sessionId) so the
        // flattened session is registered on the root connection.
        if (init->targetAutoAttach) {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value params(rapidjson::kObjectType);
            params.AddMember("sessionId",
                             rapidjson::Value(ns->sessionId.c_str(),
                                              ns->sessionId.size(), alloc),
                             alloc);
            rapidjson::Value ti(rapidjson::kObjectType);
            buildTargetInfo(ns, nwv, true, ti, alloc);
            params.AddMember("targetInfo", ti, alloc);
            params.AddMember("waitingForDebugger", false, alloc);
            CDPCommand evt(m_dispatcher, Optional<int64_t>(), std::string(),
                           nullptr);
            evt.sendEvent("Target.attachedToTarget", params, doc);
        }

        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember(
            "targetId",
            rapidjson::Value(ns->targetId.c_str(), ns->targetId.size(), alloc),
            alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "closeTarget") {
        std::string tid;
        if (cmd.params() && cmd.params()->HasMember("targetId") &&
            (*cmd.params())["targetId"].IsString()) {
            tid = (*cmd.params())["targetId"].GetString();
        }
        // Find and tear down the matching spawned target (never the initial).
        TargetContext* victim = nullptr;
        for (TargetContext* ctx : m_dispatcher->contexts()) {
            if (ctx != m_dispatcher->initialContext() &&
                ctx->session->targetId == tid) {
                victim = ctx;
                break;
            }
        }
        if (victim) {
            std::string vsid = victim->session->sessionId;
            std::string vtid = victim->session->targetId;
            WebView* vwv = victim->webView;
            bool owns = victim->ownsWebView;

            // detachedFromTarget + targetDestroyed before teardown.
            {
                rapidjson::Document doc;
                rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
                rapidjson::Value params(rapidjson::kObjectType);
                params.AddMember(
                    "sessionId",
                    rapidjson::Value(vsid.c_str(), vsid.size(), alloc), alloc);
                params.AddMember(
                    "targetId",
                    rapidjson::Value(vtid.c_str(), vtid.size(), alloc), alloc);
                CDPCommand evt(m_dispatcher, Optional<int64_t>(), std::string(),
                               nullptr);
                evt.sendEvent("Target.detachedFromTarget", params, doc);
            }
            {
                rapidjson::Document doc;
                rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
                rapidjson::Value params(rapidjson::kObjectType);
                params.AddMember(
                    "targetId",
                    rapidjson::Value(vtid.c_str(), vtid.size(), alloc), alloc);
                CDPCommand evt(m_dispatcher, Optional<int64_t>(), std::string(),
                               nullptr);
                evt.sendEvent("Target.targetDestroyed", params, doc);
            }

            m_dispatcher->destroyContext(victim);
            if (owns && vwv) {
                vwv->destroy();
            }
        }

        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("success", victim != nullptr, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "activateTarget") {
        // Each target maps to its own independent WebView (commands are routed
        // by sessionId), so there is no shared "current target" to switch and
        // no window manager to raise. Acknowledge so the call resolves.
        cmd.sendResultEmpty();
        return;
    }

    if (method == "detachFromTarget") {
        std::string sid = s->sessionId;
        s->attached = false;
        cmd.sendResultEmpty();

        if (!sid.empty()) {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value params(rapidjson::kObjectType);
            params.AddMember("sessionId",
                             rapidjson::Value(sid.c_str(), sid.size(), alloc),
                             alloc);
            params.AddMember("targetId",
                             rapidjson::Value(s->targetId.c_str(),
                                              s->targetId.size(), alloc),
                             alloc);
            CDPCommand evt(m_dispatcher, Optional<int64_t>(), cmd.sessionId(),
                           nullptr);
            evt.sendEvent("Target.detachedFromTarget", params, doc);
        }
        return;
    }

    cmd.sendError(-32601, "'method' wasn't found");
}

} // namespace Starfish

#endif
