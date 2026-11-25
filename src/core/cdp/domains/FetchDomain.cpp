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
    addStringMember(params, "requestId", requestId, alloc);
    // networkId ties this pause to the Network.requestWillBeSent already
    // emitted for the navigation (== loaderId), so puppeteer maps it to the nav
    // request.
    addStringMember(params, "networkId", s->loaderId, alloc);
    addStringMember(params, "frameId", s->frameId, alloc);
    params.AddMember("resourceType", "Document", alloc);

    rapidjson::Value request(rapidjson::kObjectType);
    addStringMember(request, "url", url, alloc);
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

void FetchDomain::handleEnable(CDPCommand& cmd, const std::string&)
{
    CDPSession* s = m_dispatcher->session();

    s->fetchEnabled = true;
    cmd.sendResultEmpty();
}

void FetchDomain::handleDisable(CDPCommand& cmd, const std::string&)
{
    CDPSession* s = m_dispatcher->session();

    s->fetchEnabled = false;
    // If a navigation is parked, releasing interception should let it run
    // to the original URL rather than hang forever.
    if (s->pendingFetchNav.active) {
        std::string sid = s->pendingFetchNav.sessionId;
        std::string navUrl = s->pendingFetchNav.url;
        m_dispatcher->page()->completeDeferredNavigation(sid, navUrl);
    }
    cmd.sendResultEmpty();
}

void FetchDomain::handleContinueRequest(CDPCommand& cmd, const std::string&)
{
    CDPSession* s = m_dispatcher->session();

    std::string requestId = paramString(cmd, "requestId");
    cmd.sendResultEmpty();
    if (s->pendingFetchNav.active &&
        s->pendingFetchNav.requestId == requestId) {
        // continueRequest may override the URL; honor it if present.
        std::string overrideUrl = paramString(cmd, "url");
        std::string sid = s->pendingFetchNav.sessionId;
        std::string navUrl = s->pendingFetchNav.url;
        std::string target = overrideUrl.empty() ? navUrl : overrideUrl;
        m_dispatcher->page()->completeDeferredNavigation(sid, target);
    }
}

void FetchDomain::handleFulfillRequest(CDPCommand& cmd, const std::string&)
{
    CDPSession* s = m_dispatcher->session();

    std::string requestId = paramString(cmd, "requestId");
    cmd.sendResultEmpty();
    if (s->pendingFetchNav.active &&
        s->pendingFetchNav.requestId == requestId) {
        // body is base64-encoded per the CDP spec. data: URLs accept
        // base64 directly, so forward it unchanged (the document loader
        // renders the data: URL) with the client's Content-Type.
        std::string rawBody = paramString(cmd, "body");
        std::string mt = mimeFromHeaders(cmd);
        if (mt.empty()) {
            mt = "text/html";
        }
        std::string dataUrl = "data:" + mt + ";base64," + rawBody;
        std::string sid = s->pendingFetchNav.sessionId;
        m_dispatcher->page()->completeDeferredNavigation(sid, dataUrl);
    }
}

void FetchDomain::handleFailRequest(CDPCommand& cmd, const std::string&)
{
    CDPSession* s = m_dispatcher->session();

    std::string requestId = paramString(cmd, "requestId");
    std::string errorReason;
    errorReason = paramString(cmd, "errorReason", errorReason);
    cmd.sendResultEmpty();
    if (s->pendingFetchNav.active &&
        s->pendingFetchNav.requestId == requestId) {
        std::string sid = s->pendingFetchNav.sessionId;
        std::string navUrl = s->pendingFetchNav.url;
        // Map CDP errorReason to Chrome's net::ERR_* text.
        std::string errorText = "net::ERR_FAILED";
        if (errorReason == "Aborted") {
            errorText = "net::ERR_ABORTED";
        } else if (errorReason == "AddressUnreachable") {
            errorText = "net::ERR_ADDRESS_UNREACHABLE";
        } else if (errorReason == "BlockedByClient") {
            errorText = "net::ERR_BLOCKED_BY_CLIENT";
        } else if (errorReason == "BlockedByResponse") {
            errorText = "net::ERR_BLOCKED_BY_RESPONSE";
        } else if (errorReason == "ConnectionRefused") {
            errorText = "net::ERR_CONNECTION_REFUSED";
        } else if (errorReason == "ConnectionReset") {
            errorText = "net::ERR_CONNECTION_RESET";
        } else if (errorReason == "ConnectionClosed") {
            errorText = "net::ERR_CONNECTION_CLOSED";
        } else if (errorReason == "ConnectionFailed") {
            errorText = "net::ERR_CONNECTION_FAILED";
        } else if (errorReason == "ConnectionAborted") {
            errorText = "net::ERR_CONNECTION_ABORTED";
        } else if (errorReason == "Failed") {
            errorText = "net::ERR_FAILED";
        } else if (errorReason == "TimedOut") {
            errorText = "net::ERR_TIMED_OUT";
        }
        m_dispatcher->page()->failDeferredNavigation(sid, navUrl, errorText);
    }
}

void FetchDomain::processMessage(CDPCommand& cmd, const std::string& method)
{
    static const CDPMethodHandler<FetchDomain> handlers[] = {
        { "enable", &FetchDomain::handleEnable },
        { "disable", &FetchDomain::handleDisable },
        { "continueRequest", &FetchDomain::handleContinueRequest },
        { "fulfillRequest", &FetchDomain::handleFulfillRequest },
        { "failRequest", &FetchDomain::handleFailRequest },
    };
    if (CDPHandlerFn<FetchDomain> handler =
            findCDPMethodHandler(handlers, method)) {
        (this->*handler)(cmd, method);
        return;
    }
    // getResponseBody / continueWithAuth / takeResponseBodyAsStream etc. have
    // no backing in the synthetic model; ack so clients proceed.
    cmd.sendResultEmpty();
}

} // namespace Starfish

#endif
