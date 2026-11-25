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
#include "../CDPServer.h"
#include "PageDomain.h"
#include "../TargetContext.h"

#include "core/page/WebView.h"
#include "core/page/BrowsingContext.h"
#include "core/dom/Document.h"
#include "core/style/ComputedStyle.h"
#include <cstring>
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

static std::string targetTitle(WebView* wv)
{
    if (wv) {
        BrowsingContext* bc = wv->mainBrowsingContext();
        if (bc && bc->document()) {
            String* t = bc->document()->title();
            if (t && !t->isEmpty()) {
                std::string s = t->toUTF8NonGCString();
                if (!s.empty()) {
                    return s;
                }
            }
            // Fallback to URL without scheme
            if (bc->document()->urlString()) {
                std::string u =
                    bc->document()->urlString()->toUTF8NonGCString();
                if (u.rfind("http://", 0) == 0) {
                    return u.substr(7);
                }
                if (u.rfind("https://", 0) == 0) {
                    return u.substr(8);
                }
                if (!u.empty()) {
                    return u;
                }
            }
        }
    }
    return "";
}

static void buildTargetInfo(CDPSession* s, WebView* wv, bool attached,
                            rapidjson::Value& out,
                            rapidjson::Document::AllocatorType& alloc)
{
    // A debugger-waiting target has not committed its initial navigation yet.
    std::string url = s->waitingForDebugger ? "" : targetUrl(wv);
    std::string title =
        s->waitingForDebugger || s->targetType == "tab" ? "" : targetTitle(wv);
    out.SetObject();
    out.AddMember(
        "targetId",
        rapidjson::Value(s->targetId.c_str(), s->targetId.size(), alloc),
        alloc);
    out.AddMember(
        "type",
        rapidjson::Value(s->targetType.c_str(), s->targetType.size(), alloc),
        alloc);
    out.AddMember("title", rapidjson::Value(title.c_str(), title.size(), alloc),
                  alloc);
    out.AddMember("url", rapidjson::Value(url.c_str(), url.size(), alloc),
                  alloc);
    out.AddMember("attached", attached, alloc);
    out.AddMember("canAccessOpener", false, alloc);
    out.AddMember("browserContextId",
                  rapidjson::Value(s->browserContextId.c_str(),
                                   s->browserContextId.size(), alloc),
                  alloc);
}

static bool filterAllows(const rapidjson::Value* filter, const char* type)
{
    if (!filter) {
        return strcmp(type, "browser") != 0 && strcmp(type, "tab") != 0;
    }
    for (auto it = filter->Begin(); it != filter->End(); ++it) {
        const rapidjson::Value& entry = *it;
        if (entry.HasMember("type") &&
            strcmp(entry["type"].GetString(), type) != 0) {
            continue;
        }
        return !entry.HasMember("exclude") || !entry["exclude"].GetBool();
    }
    return false;
}

TargetDomain::Subscription& TargetDomain::subscription(
    const std::string& sessionId)
{
    for (Subscription& entry : m_subscriptions) {
        if (entry.sessionId == sessionId) {
            return entry;
        }
    }
    m_subscriptions.emplace_back();
    m_subscriptions.back().sessionId = sessionId;
    return m_subscriptions.back();
}

bool TargetDomain::canAutoAttach(const Subscription& subscriber,
                                 CDPSession* session) const
{
    return subscriber.autoAttach &&
           (subscriber.sessionId.empty() ||
            subscriber.sessionId == m_dispatcher->browserSessionId()) &&
           (session->targetType == "tab" ? subscriber.allowTab
                                         : subscriber.allowPage);
}

void TargetDomain::reset()
{
    m_subscriptions.clear();
}

void TargetDomain::rememberTargetInfo(TargetContext* ctx)
{
    CDPSession* session = ctx->session;
    session->lastTargetInfoUrl =
        session->waitingForDebugger ? "" : targetUrl(ctx->webView);
    session->lastTargetInfoTitle =
        session->waitingForDebugger || session->targetType == "tab"
            ? ""
            : targetTitle(ctx->webView);
    session->lastTargetInfoAttached = session->attached;
    session->hasTargetInfo = true;
}

void TargetDomain::notifyTargetInfoChanged(TargetContext* ctx)
{
    if (m_subscriptions.empty()) {
        return;
    }
    CDPSession* session = ctx->session;
    if (session->waitingForDebugger) {
        return;
    }
    if (!session->hasTargetInfo) {
        rememberTargetInfo(ctx);
        return;
    }
    std::string url = targetUrl(ctx->webView);
    std::string title =
        session->targetType == "tab" ? "" : targetTitle(ctx->webView);
    if (session->hasTargetInfo && session->lastTargetInfoUrl == url &&
        session->lastTargetInfoTitle == title &&
        session->lastTargetInfoAttached == session->attached) {
        return;
    }
    session->lastTargetInfoUrl.swap(url);
    session->lastTargetInfoTitle.swap(title);
    session->lastTargetInfoAttached = session->attached;
    session->hasTargetInfo = true;
    for (const Subscription& subscriber : m_subscriptions) {
        if (!subscriber.discover &&
            !(session->attached && canAutoAttach(subscriber, session))) {
            continue;
        }
        rapidjson::Document doc;
        auto& alloc = doc.GetAllocator();
        rapidjson::Value params(rapidjson::kObjectType);
        rapidjson::Value info(rapidjson::kObjectType);
        buildTargetInfo(session, ctx->webView, session->attached, info, alloc);
        params.AddMember("targetInfo", info, alloc);
        CDPCommand event(m_dispatcher, Optional<int64_t>(),
                         subscriber.sessionId, nullptr);
        event.sendEvent("Target.targetInfoChanged", params, doc);
    }
}

void notifyTargetInfoChanged(Document* document)
{
    Optional<BrowsingContext*> browsingContext = document->browsingContext();
    if (!browsingContext) {
        return;
    }
    BrowsingContext* context = browsingContext.value();
    if (!context->isTopLevelBrowsingContext() ||
        context->document() != document) {
        return;
    }
    WebView* webView = context->webView();
    CDPServer* server = webView->cdpServer();
    if (!server) {
        return;
    }
    CDPDispatcher* dispatcher = server->dispatcher();
    TargetContext* target = dispatcher->contextForWebView(webView);
    if (target) {
        dispatcher->target()->notifyTargetInfoChanged(target);
    }
}

void TargetDomain::handleSetDiscoverTargets(CDPCommand& cmd, const std::string&)
{
    CDPSession* s = m_dispatcher->session();
    WebView* wv = m_dispatcher->webView();

    bool discover = false;
    if (cmd.params() && cmd.params()->HasMember("discover") &&
        (*cmd.params())["discover"].IsBool()) {
        discover = (*cmd.params())["discover"].GetBool();
    }
    subscription(cmd.sessionId()).discover = discover;
    cmd.sendResultEmpty();

    if (discover) {
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value params(rapidjson::kObjectType);
        rapidjson::Value ti(rapidjson::kObjectType);
        buildTargetInfo(s, wv, s->attached, ti, alloc);
        rememberTargetInfo(m_dispatcher->currentContext());
        params.AddMember("targetInfo", ti, alloc);
        CDPCommand evt(m_dispatcher, Optional<int64_t>(), cmd.sessionId(),
                       nullptr);
        evt.sendEvent("Target.targetCreated", params, doc);
    }
}

void TargetDomain::handleSetAutoAttach(CDPCommand& cmd, const std::string&)
{
    CDPSession* s = m_dispatcher->session();
    WebView* wv = m_dispatcher->webView();

    if (cmd.sessionId().empty() ||
        cmd.sessionId() == m_dispatcher->browserSessionId()) {
        bool flatten = false;
        if (cmd.params() && cmd.params()->HasMember("flatten") &&
            (*cmd.params())["flatten"].IsBool()) {
            flatten = (*cmd.params())["flatten"].GetBool();
        }
        if (!flatten) {
            cmd.sendError(-32602,
                          "Only flatten protocol is supported with browser "
                          "level auto-attach");
            return;
        }
    }

    const rapidjson::Value* filter = nullptr;
    if (cmd.params() && cmd.params()->HasMember("filter")) {
        filter = &(*cmd.params())["filter"];
        if (!filter->IsArray()) {
            cmd.sendError(-32602, "'filter' must be an array");
            return;
        }
        for (auto it = filter->Begin(); it != filter->End(); ++it) {
            const rapidjson::Value& entry = *it;
            if (!entry.IsObject() ||
                (entry.HasMember("type") && !entry["type"].IsString()) ||
                (entry.HasMember("exclude") && !entry["exclude"].IsBool())) {
                cmd.sendError(-32602, "Invalid target filter entry");
                return;
            }
        }
    }
    bool allowPage = filterAllows(filter, "page");
    bool allowTab = filterAllows(filter, "tab");
    if ((cmd.sessionId().empty() ||
         cmd.sessionId() == m_dispatcher->browserSessionId()) &&
        allowPage && allowTab) {
        cmd.sendError(-32602,
                      "Filter should not simultaneously allow \"tab\" "
                      "and \"page\", page targets are attached via tab "
                      "targets");
        return;
    }

    bool autoAttach = false;
    if (cmd.params() && cmd.params()->HasMember("autoAttach") &&
        (*cmd.params())["autoAttach"].IsBool()) {
        autoAttach = (*cmd.params())["autoAttach"].GetBool();
    }
    Subscription& subscriber = subscription(cmd.sessionId());
    subscriber.autoAttach = autoAttach;
    subscriber.allowPage = allowPage;
    subscriber.allowTab = allowTab;

    bool waitForDebugger = false;
    if (cmd.params() && cmd.params()->HasMember("waitForDebuggerOnStart") &&
        (*cmd.params())["waitForDebuggerOnStart"].IsBool()) {
        waitForDebugger = (*cmd.params())["waitForDebuggerOnStart"].GetBool();
    }
    subscriber.waitForDebugger = waitForDebugger;

    // Browser-scope auto-attach announces the existing page once, before
    // the reply. A page session's own setAutoAttach must not replace that
    // attachment and orphan the client's in-flight initialization.
    //
    // The attachedToTarget event is emitted BEFORE the setAutoAttach
    // result so that puppeteer registers the page target/session before
    // the connection-level initialize() resolves connect(); otherwise
    // browser.pages() called right after connect() races ahead of the
    // attachment and returns 0.
    if (canAutoAttach(subscriber, s) && !s->attachEmitted) {
        // Issue a session and emit Target.attachedToTarget.
        if (s->sessionId.empty()) {
            s->sessionId =
                "SID-" + std::to_string(++s->sessionCounter) + "0000000";
        }
        s->attached = true;
        s->attachEmitted = true;
        rememberTargetInfo(m_dispatcher->currentContext());

        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value params(rapidjson::kObjectType);
        params.AddMember(
            "sessionId",
            rapidjson::Value(s->sessionId.c_str(), s->sessionId.size(), alloc),
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
}

void TargetDomain::handleGetTargets(CDPCommand& cmd, const std::string&)
{
    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    rapidjson::Value infos(rapidjson::kArrayType);
    for (TargetContext* ctx : m_dispatcher->contexts()) {
        rapidjson::Value ti(rapidjson::kObjectType);
        buildTargetInfo(ctx->session, ctx->webView, ctx->session->attached, ti,
                        alloc);
        infos.PushBack(ti, alloc);
    }
    result.AddMember("targetInfos", infos, alloc);
    cmd.sendResult(result, doc);
}

void TargetDomain::handleGetBrowserContexts(CDPCommand& cmd, const std::string&)
{
    // Return the contexts created via Target.createBrowserContext. The
    // default browser context is excluded (matches Chrome). The list is
    // tracked on the initial (connection-level) session.
    CDPSession* init = m_dispatcher->initialContext()->session;
    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    rapidjson::Value ids(rapidjson::kArrayType);
    for (const std::string& bid : init->browserContexts) {
        ids.PushBack(rapidjson::Value(bid.c_str(), bid.size(), alloc), alloc);
    }
    result.AddMember("browserContextIds", ids, alloc);
    cmd.sendResult(result, doc);
}

void TargetDomain::handleCreateBrowserContext(CDPCommand& cmd,
                                              const std::string&)
{
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
                     rapidjson::Value(bid.c_str(), bid.size(), alloc), alloc);
    cmd.sendResult(result, doc);
}

void TargetDomain::handleDisposeBrowserContext(CDPCommand& cmd,
                                               const std::string&)
{
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
            params.AddMember("sessionId",
                             rapidjson::Value(vsid.c_str(), vsid.size(), alloc),
                             alloc);
            params.AddMember("targetId",
                             rapidjson::Value(vtid.c_str(), vtid.size(), alloc),
                             alloc);
            CDPCommand evt(m_dispatcher, Optional<int64_t>(), std::string(),
                           nullptr);
            evt.sendEvent("Target.detachedFromTarget", params, doc);
        }
        {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value params(rapidjson::kObjectType);
            params.AddMember("targetId",
                             rapidjson::Value(vtid.c_str(), vtid.size(), alloc),
                             alloc);
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
}

void TargetDomain::handleGetTargetInfo(CDPCommand& cmd, const std::string&)
{
    CDPSession* s = m_dispatcher->session();
    WebView* wv = m_dispatcher->webView();

    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    rapidjson::Value ti(rapidjson::kObjectType);
    buildTargetInfo(s, wv, s->attached, ti, alloc);
    result.AddMember("targetInfo", ti, alloc);
    cmd.sendResult(result, doc);
}

void TargetDomain::handleAttachToTarget(CDPCommand& cmd, const std::string&)
{
    CDPSession* s = m_dispatcher->session();

    // Keep flattened commands on the requested target's WebView.
    TargetContext* targetCtx = m_dispatcher->currentContext();
    if (cmd.params() && cmd.params()->HasMember("targetId") &&
        (*cmd.params())["targetId"].IsString()) {
        const char* targetId = (*cmd.params())["targetId"].GetString();
        for (TargetContext* ctx : m_dispatcher->contexts()) {
            if (ctx->session->targetId == targetId) {
                targetCtx = ctx;
                break;
            }
        }
    }

    CDPSession* targetSession = targetCtx->session;
    if (targetSession->sessionId.empty()) {
        CDPSession* init = m_dispatcher->initialContext()->session;
        targetSession->sessionId =
            "SID-" + std::to_string(++init->sessionCounter) + "0000000";
    }
    targetSession->attached = true;

    // Emit attachedToTarget event first.
    {
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value params(rapidjson::kObjectType);
        params.AddMember("sessionId",
                         rapidjson::Value(targetSession->sessionId.c_str(),
                                          targetSession->sessionId.size(),
                                          alloc),
                         alloc);
        rapidjson::Value ti(rapidjson::kObjectType);
        buildTargetInfo(targetSession, targetCtx->webView, true, ti, alloc);
        params.AddMember("targetInfo", ti, alloc);
        params.AddMember("waitingForDebugger", false, alloc);
        CDPCommand evt(m_dispatcher, Optional<int64_t>(), cmd.sessionId(),
                       nullptr);
        evt.sendEvent("Target.attachedToTarget", params, doc);
    }

    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    result.AddMember("sessionId",
                     rapidjson::Value(targetSession->sessionId.c_str(),
                                      targetSession->sessionId.size(), alloc),
                     alloc);
    cmd.sendResult(result, doc);
}

void TargetDomain::handleAttachToBrowserTarget(CDPCommand& cmd,
                                               const std::string&)
{
    CDPSession* s = m_dispatcher->session();

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
                     rapidjson::Value(bsid.c_str(), bsid.size(), alloc), alloc);
    cmd.sendResult(result, doc);
}

void TargetDomain::handleCreateTarget(CDPCommand& cmd, const std::string&)
{
    CDPSession* s = m_dispatcher->session();
    WebView* wv = m_dispatcher->webView();

    if (cmd.params() && cmd.params()->HasMember("hidden") &&
        (*cmd.params())["hidden"].IsBool() &&
        (*cmd.params())["hidden"].GetBool()) {
        if (cmd.params()->HasMember("background") &&
            (*cmd.params())["background"].IsBool() &&
            !(*cmd.params())["background"].GetBool()) {
            cmd.sendError(-32602,
                          "Hidden target can be created only in background");
            return;
        }
        if (cmd.params()->HasMember("newWindow") &&
            (*cmd.params())["newWindow"].IsBool() &&
            (*cmd.params())["newWindow"].GetBool()) {
            cmd.sendError(-32602,
                          "Hidden target cannot be created in a new window");
            return;
        }
        if (cmd.params()->HasMember("forTab") &&
            (*cmd.params())["forTab"].IsBool() &&
            (*cmd.params())["forTab"].GetBool()) {
            cmd.sendError(-32602, "Hidden target cannot be created for tab");
            return;
        }
    }

    // A windowed engine gets its GL context and window from the embedder;
    // a view spawned here would have neither.
    if (!wv->starfish()->isHeadless()) {
        cmd.sendError(-32000, "Target.createTarget needs a headless engine");
        return;
    }
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

    CDPSession* ns = nctx->session;
    if (cmd.params() && cmd.params()->HasMember("forTab") &&
        (*cmd.params())["forTab"].IsBool() &&
        (*cmd.params())["forTab"].GetBool()) {
        ns->targetType = "tab";
    }

    bool autoAttach = false;
    bool waitDebugger = false;
    for (const Subscription& subscriber : m_subscriptions) {
        if (canAutoAttach(subscriber, ns)) {
            autoAttach = true;
            waitDebugger |= subscriber.waitForDebugger;
        }
    }
    if (autoAttach && cmd.params() &&
        cmd.params()->HasMember("waitForDebuggerOnStart") &&
        (*cmd.params())["waitForDebuggerOnStart"].IsBool()) {
        waitDebugger = (*cmd.params())["waitForDebuggerOnStart"].GetBool();
    }
    waitDebugger = autoAttach && waitDebugger && ns->targetType != "tab";
    ns->attached = autoAttach;
    ns->attachEmitted = autoAttach;

    {
        CDPDispatcher::ScopedTarget target(m_dispatcher, nctx);
        if (waitDebugger) {
            // about:blank (including its fragment) cannot run page script.
            // Other URLs get a blank context until the debugger resumes.
            ns->waitingForDebugger = true;
            bool isBlank =
                url == "about:blank" || url.compare(0, 12, "about:blank#") == 0;
            nwv->loadHTMLDocument(isBlank
                                      ? String::fromUTF8(url.data(), url.size())
                                      : String::fromUTF8("about:blank"));
            ns->pendingNavigationUrl = url;
            // This load skips PageDomain::finishNavigation().
            m_dispatcher->rebuildDiscoveryJson();
        } else {
            CDPCommand navigation(m_dispatcher, Optional<int64_t>(),
                                  ns->sessionId, nullptr);
            m_dispatcher->page()->navigate(navigation, url);
        }
    }
    rememberTargetInfo(nctx);

    for (const Subscription& subscriber : m_subscriptions) {
        if (subscriber.discover) {
            rapidjson::Document doc;
            auto& alloc = doc.GetAllocator();
            rapidjson::Value params(rapidjson::kObjectType);
            rapidjson::Value info(rapidjson::kObjectType);
            buildTargetInfo(ns, nwv, ns->attached, info, alloc);
            params.AddMember("targetInfo", info, alloc);
            CDPCommand event(m_dispatcher, Optional<int64_t>(),
                             subscriber.sessionId, nullptr);
            event.sendEvent("Target.targetCreated", params, doc);
        }
        if (canAutoAttach(subscriber, ns)) {
            rapidjson::Document doc;
            auto& alloc = doc.GetAllocator();
            rapidjson::Value params(rapidjson::kObjectType);
            params.AddMember("sessionId",
                             rapidjson::Value(ns->sessionId.c_str(),
                                              ns->sessionId.size(), alloc),
                             alloc);
            rapidjson::Value info(rapidjson::kObjectType);
            buildTargetInfo(ns, nwv, ns->attached, info, alloc);
            params.AddMember("targetInfo", info, alloc);
            params.AddMember("waitingForDebugger", waitDebugger, alloc);
            CDPCommand event(m_dispatcher, Optional<int64_t>(),
                             subscriber.sessionId, nullptr);
            event.sendEvent("Target.attachedToTarget", params, doc);
        }
    }

    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    result.AddMember(
        "targetId",
        rapidjson::Value(ns->targetId.c_str(), ns->targetId.size(), alloc),
        alloc);
    cmd.sendResult(result, doc);
}

void TargetDomain::handleCloseTarget(CDPCommand& cmd, const std::string&)
{
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
            params.AddMember("sessionId",
                             rapidjson::Value(vsid.c_str(), vsid.size(), alloc),
                             alloc);
            params.AddMember("targetId",
                             rapidjson::Value(vtid.c_str(), vtid.size(), alloc),
                             alloc);
            CDPCommand evt(m_dispatcher, Optional<int64_t>(), std::string(),
                           nullptr);
            evt.sendEvent("Target.detachedFromTarget", params, doc);
        }
        {
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value params(rapidjson::kObjectType);
            params.AddMember("targetId",
                             rapidjson::Value(vtid.c_str(), vtid.size(), alloc),
                             alloc);
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
}

void TargetDomain::handleActivateTarget(CDPCommand& cmd, const std::string&)
{
    // Each target maps to its own independent WebView (commands are routed
    // by sessionId), so there is no shared "current target" to switch and
    // no window manager to raise. Acknowledge so the call resolves.
    cmd.sendResultEmpty();
}

void TargetDomain::handleDetachFromTarget(CDPCommand& cmd, const std::string&)
{
    CDPSession* s = m_dispatcher->session();

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
        params.AddMember(
            "targetId",
            rapidjson::Value(s->targetId.c_str(), s->targetId.size(), alloc),
            alloc);
        CDPCommand evt(m_dispatcher, Optional<int64_t>(), cmd.sessionId(),
                       nullptr);
        evt.sendEvent("Target.detachedFromTarget", params, doc);
    }
}

void TargetDomain::processMessage(CDPCommand& cmd, const std::string& method)
{
    static const CDPMethodHandler<TargetDomain> handlers[] = {
        { "setDiscoverTargets", &TargetDomain::handleSetDiscoverTargets },
        { "setAutoAttach", &TargetDomain::handleSetAutoAttach },
        { "getTargets", &TargetDomain::handleGetTargets },
        { "getBrowserContexts", &TargetDomain::handleGetBrowserContexts },
        { "createBrowserContext", &TargetDomain::handleCreateBrowserContext },
        { "disposeBrowserContext", &TargetDomain::handleDisposeBrowserContext },
        { "getTargetInfo", &TargetDomain::handleGetTargetInfo },
        { "attachToTarget", &TargetDomain::handleAttachToTarget },
        { "attachToBrowserTarget", &TargetDomain::handleAttachToBrowserTarget },
        { "createTarget", &TargetDomain::handleCreateTarget },
        { "closeTarget", &TargetDomain::handleCloseTarget },
        { "activateTarget", &TargetDomain::handleActivateTarget },
        { "detachFromTarget", &TargetDomain::handleDetachFromTarget },
    };
    if (CDPHandlerFn<TargetDomain> handler =
            findCDPMethodHandler(handlers, method)) {
        (this->*handler)(cmd, method);
        return;
    }
    cmd.sendError(-32601, "'method' wasn't found");
}

} // namespace Starfish

#endif
