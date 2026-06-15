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
#include "FetchDomain.h"
#include "PageDomain.h"
#include "../CDPDispatcher.h"
#include "../CDPCommand.h"
#include "../CDPSession.h"

#include "rapidjson/document.h"

#include <string>
#include <vector>

namespace Starfish {

namespace {

    static std::string paramStr(CDPCommand& cmd, const char* name)
    {
        if (cmd.params() && cmd.params()->HasMember(name) &&
            (*cmd.params())[name].IsString()) {
            return (*cmd.params())[name].GetString();
        }
        return std::string();
    }

    // Pull a mime type out of a Fetch.fulfillRequest responseHeaders array, if
    // the client supplied a Content-Type. Case-insensitive header name match.
    static std::string mimeFromHeaders(CDPCommand& cmd)
    {
        if (!cmd.params() || !cmd.params()->HasMember("responseHeaders") ||
            !(*cmd.params())["responseHeaders"].IsArray()) {
            return std::string();
        }
        const rapidjson::Value& hs = (*cmd.params())["responseHeaders"];
        for (rapidjson::SizeType i = 0; i < hs.Size(); ++i) {
            if (!hs[i].IsObject() || !hs[i].HasMember("name") ||
                !hs[i]["name"].IsString() || !hs[i].HasMember("value") ||
                !hs[i]["value"].IsString()) {
                continue;
            }
            std::string name = hs[i]["name"].GetString();
            for (char& c : name) {
                c = (char)tolower((unsigned char)c);
            }
            if (name == "content-type") {
                std::string v = hs[i]["value"].GetString();
                size_t semi = v.find(';');
                return semi == std::string::npos ? v : v.substr(0, semi);
            }
        }
        return std::string();
    }

} // namespace

std::string FetchDomain::emitNavigationPaused(const std::string& sessionId,
                                              const std::string& url)
{
    CDPSession* s = m_dispatcher->session();
    std::string requestId =
        "interception-job-" + std::to_string(++s->fetchRequestCounter) + ".0";

    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    rapidjson::Value params(rapidjson::kObjectType);
    params.AddMember(
        "requestId",
        rapidjson::Value(requestId.c_str(), requestId.size(), alloc), alloc);
    // networkId ties this pause to the Network.requestWillBeSent already
    // emitted for the navigation (== loaderId), so puppeteer maps it to the nav
    // request.
    params.AddMember(
        "networkId",
        rapidjson::Value(s->loaderId.c_str(), s->loaderId.size(), alloc),
        alloc);
    params.AddMember(
        "frameId",
        rapidjson::Value(s->frameId.c_str(), s->frameId.size(), alloc), alloc);
    params.AddMember("resourceType", "Document", alloc);

    rapidjson::Value request(rapidjson::kObjectType);
    request.AddMember("url", rapidjson::Value(url.c_str(), url.size(), alloc),
                      alloc);
    request.AddMember("method", "GET", alloc);
    request.AddMember("headers", rapidjson::Value(rapidjson::kObjectType),
                      alloc);
    request.AddMember("initialPriority", "VeryHigh", alloc);
    request.AddMember("referrerPolicy", "strict-origin-when-cross-origin",
                      alloc);
    params.AddMember("request", request, alloc);

    CDPCommand evt(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
    evt.sendEvent("Fetch.requestPaused", params, doc);
    return requestId;
}

void FetchDomain::processMessage(CDPCommand& cmd, const std::string& method)
{
    CDPSession* s = m_dispatcher->session();

    if (method == "enable") {
        s->fetchEnabled = true;
        cmd.sendResultEmpty();
        return;
    }
    if (method == "disable") {
        s->fetchEnabled = false;
        // If a navigation is parked, releasing interception should let it run
        // to the original URL rather than hang forever.
        if (s->pendingFetchNav.active) {
            PendingFetchNavigation nav = s->pendingFetchNav;
            s->pendingFetchNav = PendingFetchNavigation();
            m_dispatcher->page()->completeDeferredNavigation(nav.sessionId,
                                                             nav.url);
        }
        cmd.sendResultEmpty();
        return;
    }

    if (method == "continueRequest") {
        std::string requestId = paramStr(cmd, "requestId");
        cmd.sendResultEmpty();
        if (s->pendingFetchNav.active &&
            s->pendingFetchNav.requestId == requestId) {
            PendingFetchNavigation nav = s->pendingFetchNav;
            s->pendingFetchNav = PendingFetchNavigation();
            // continueRequest may override the URL; honor it if present.
            std::string overrideUrl = paramStr(cmd, "url");
            std::string target = overrideUrl.empty() ? nav.url : overrideUrl;
            m_dispatcher->page()->completeDeferredNavigation(nav.sessionId,
                                                             target);
        }
        return;
    }

    if (method == "fulfillRequest") {
        std::string requestId = paramStr(cmd, "requestId");
        cmd.sendResultEmpty();
        if (s->pendingFetchNav.active &&
            s->pendingFetchNav.requestId == requestId) {
            PendingFetchNavigation nav = s->pendingFetchNav;
            s->pendingFetchNav = PendingFetchNavigation();
            // body is base64-encoded per the CDP spec. data: URLs accept
            // base64 directly, so forward it unchanged (the document loader
            // renders the data: URL) with the client's Content-Type.
            std::string rawBody = paramStr(cmd, "body");
            std::string mt = mimeFromHeaders(cmd);
            if (mt.empty()) {
                mt = "text/html";
            }
            std::string dataUrl = "data:" + mt + ";base64," + rawBody;
            m_dispatcher->page()->completeDeferredNavigation(nav.sessionId,
                                                             dataUrl);
        }
        return;
    }

    if (method == "failRequest") {
        std::string requestId = paramStr(cmd, "requestId");
        cmd.sendResultEmpty();
        if (s->pendingFetchNav.active &&
            s->pendingFetchNav.requestId == requestId) {
            PendingFetchNavigation nav = s->pendingFetchNav;
            s->pendingFetchNav = PendingFetchNavigation();
            m_dispatcher->page()->failDeferredNavigation(nav.sessionId,
                                                         nav.url);
        }
        return;
    }

    // getResponseBody / continueWithAuth / takeResponseBodyAsStream etc. have
    // no backing in the synthetic model; ack so clients proceed.
    cmd.sendResultEmpty();
}

} // namespace Starfish

#endif
