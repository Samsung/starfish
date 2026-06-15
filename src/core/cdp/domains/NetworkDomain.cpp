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
#include "NetworkDomain.h"
#include "../CDPDispatcher.h"
#include "../CDPCommand.h"
#include "../CDPSession.h"
#include "../TargetContext.h"
#include "core/page/WebView.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/modules/message_loop/Timer.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "platform/loader/ResourceURL.h"
#include "platform/loader/Resource.h"
#include "platform/loader/ResourceLoader.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/fetch/RequestData.h"
#include "platform/network/curl/NetworkSharedResourceManager.h"
#if defined(STARFISH_ENABLE_HTTPCACHE)
#include "platform/network/http/HTTPCache.h"
#endif
#include "../Base64.h"

#include "rapidjson/document.h"

#include <curl/curl.h>
#include <ctime>
#include <vector>
#include <regex>
#include <algorithm>

namespace Starfish {

namespace {

    // One entry from curl's cookie jar, in the form CDP reports cookies.
    struct CookieEntry {
        std::string domain; // jar domain, may have leading '.'
        std::string path;
        bool secure = false;
        bool httpOnly = false;
        int64_t expires = 0; // epoch seconds, 0 == session cookie in the jar
        std::string name;
        std::string value;
    };

    // Split a tab-separated Netscape cookie line into its 7 fields.
    // Returns false if the line is not a cookie record (comment etc.).
    static bool parseNetscapeLine(const std::string& line, CookieEntry& out)
    {
        std::string s = line;
        bool httpOnly = false;
        const std::string httpOnlyPrefix = "#HttpOnly_";
        if (s.compare(0, httpOnlyPrefix.size(), httpOnlyPrefix) == 0) {
            httpOnly = true;
            s = s.substr(httpOnlyPrefix.size());
        } else if (!s.empty() && s[0] == '#') {
            return false; // comment line
        }

        std::vector<std::string> tok;
        size_t start = 0;
        for (size_t i = 0; i <= s.size(); ++i) {
            if (i == s.size() || s[i] == '\t') {
                tok.push_back(s.substr(start, i - start));
                start = i + 1;
            }
        }
        if (tok.size() != 7) {
            return false;
        }
        out.httpOnly = httpOnly;
        out.domain = tok[0];
        out.path = tok[2];
        out.secure = (tok[3] == "TRUE");
        out.expires = (int64_t)strtoll(tok[4].c_str(), nullptr, 10);
        out.name = tok[5];
        out.value = tok[6];
        return true;
    }

    // Read every cookie currently in the shared curl jar.
    static std::vector<CookieEntry> readAllCookies()
    {
        std::vector<CookieEntry> result;
        CURL* curl = curl_easy_init();
        if (!curl) {
            return result;
        }
        curl_easy_setopt(
            curl, CURLOPT_SHARE,
            NetworkSharedResourceManager::getInstance()->curlShareHandle());
        struct curl_slist* list = nullptr;
        curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &list);
        for (struct curl_slist* p = list; p; p = p->next) {
            if (!p->data) {
                continue;
            }
            CookieEntry e;
            if (parseNetscapeLine(p->data, e)) {
                result.push_back(e);
            }
        }
        if (list) {
            curl_slist_free_all(list);
        }
        curl_easy_cleanup(curl);
        return result;
    }

    // Write a single cookie record into the shared jar via CURLOPT_COOKIELIST.
    // Passing expires in the past removes a matching cookie.
    static void writeCookieLine(const CookieEntry& e)
    {
        CURL* curl = curl_easy_init();
        if (!curl) {
            return;
        }
        curl_easy_setopt(
            curl, CURLOPT_SHARE,
            NetworkSharedResourceManager::getInstance()->curlShareHandle());
        std::string path = e.path.empty() ? "/" : e.path;
        // domain field with leading '.' marks "subdomains allowed" (flag TRUE).
        bool includeSub = (!e.domain.empty() && e.domain[0] == '.');
        std::string line;
        if (e.httpOnly) {
            line += "#HttpOnly_";
        }
        line += e.domain;
        line += "\t";
        line += includeSub ? "TRUE" : "FALSE";
        line += "\t";
        line += path;
        line += "\t";
        line += e.secure ? "TRUE" : "FALSE";
        line += "\t";
        line += std::to_string(e.expires);
        line += "\t";
        line += e.name;
        line += "\t";
        line += e.value;
        curl_easy_setopt(curl, CURLOPT_COOKIELIST, line.c_str());
        curl_easy_cleanup(curl);
    }

    static std::string paramStr(CDPCommand& cmd, const char* name)
    {
        if (cmd.params() && cmd.params()->HasMember(name) &&
            (*cmd.params())[name].IsString()) {
            return (*cmd.params())[name].GetString();
        }
        return std::string();
    }

    static bool paramBool(CDPCommand& cmd, const char* name, bool def)
    {
        if (cmd.params() && cmd.params()->HasMember(name) &&
            (*cmd.params())[name].IsBool()) {
            return (*cmd.params())[name].GetBool();
        }
        return def;
    }

    static bool hasNumber(CDPCommand& cmd, const char* name)
    {
        return cmd.params() && cmd.params()->HasMember(name) &&
               (*cmd.params())[name].IsNumber();
    }

    static double paramNum(CDPCommand& cmd, const char* name, double def)
    {
        if (hasNumber(cmd, name)) {
            return (*cmd.params())[name].GetDouble();
        }
        return def;
    }

    static std::string currentUrl(WebView* wv)
    {
        if (wv) {
            BrowsingContext* bc = wv->mainBrowsingContext();
            if (bc && bc->document() && bc->document()->urlString()) {
                return bc->document()->urlString()->toUTF8NonGCString();
            }
        }
        return std::string();
    }

    static std::string urlHost(const std::string& url)
    {
        if (url.empty()) {
            return std::string();
        }
        ResourceURL* r = new ResourceURL(url.c_str(), url.size());
        String* h = r->hostname();
        return h ? h->toUTF8NonGCString() : std::string();
    }

    static std::string urlPath(const std::string& url)
    {
        if (url.empty()) {
            return std::string();
        }
        ResourceURL* r = new ResourceURL(url.c_str(), url.size());
        String* p = r->pathname();
        return p ? p->toUTF8NonGCString() : std::string();
    }

    // Does the jar-stored cookie domain cover host? (RFC6265 domain-match.)
    static bool cookieDomainMatches(const std::string& cookieDomain,
                                    const std::string& host)
    {
        if (host.empty() || cookieDomain.empty()) {
            return false;
        }
        std::string cd = cookieDomain;
        if (!cd.empty() && cd[0] == '.') {
            cd = cd.substr(1);
        }
        if (cd == host) {
            return true;
        }
        // host ends with "." + cd
        if (host.size() > cd.size() + 1) {
            std::string suffix = host.substr(host.size() - cd.size());
            if (suffix == cd && host[host.size() - cd.size() - 1] == '.') {
                return true;
            }
        }
        return false;
    }

    static bool cookiePathMatches(const std::string& cookiePath,
                                  const std::string& reqPath)
    {
        std::string cp = cookiePath.empty() ? "/" : cookiePath;
        std::string rp = reqPath.empty() ? "/" : reqPath;
        if (rp.compare(0, cp.size(), cp) != 0) {
            return false;
        }
        if (rp.size() == cp.size()) {
            return true;
        }
        return cp.back() == '/' || rp[cp.size()] == '/';
    }

    static void addCookieObject(rapidjson::Value& arr,
                                rapidjson::Document::AllocatorType& alloc,
                                const CookieEntry& e)
    {
        rapidjson::Value c(rapidjson::kObjectType);
        c.AddMember("name",
                    rapidjson::Value(e.name.c_str(), e.name.size(), alloc),
                    alloc);
        c.AddMember("value",
                    rapidjson::Value(e.value.c_str(), e.value.size(), alloc),
                    alloc);
        c.AddMember("domain",
                    rapidjson::Value(e.domain.c_str(), e.domain.size(), alloc),
                    alloc);
        std::string path = e.path.empty() ? "/" : e.path;
        c.AddMember("path", rapidjson::Value(path.c_str(), path.size(), alloc),
                    alloc);
        bool session = (e.expires == 0);
        c.AddMember("expires", session ? -1.0 : (double)e.expires, alloc);
        c.AddMember("size", (int)(e.name.size() + e.value.size()), alloc);
        c.AddMember("httpOnly", e.httpOnly, alloc);
        c.AddMember("secure", e.secure, alloc);
        c.AddMember("session", session, alloc);
        c.AddMember("sameSite", "None", alloc);
        arr.PushBack(c, alloc);
    }

    // Resolve the effective domain/path for a setCookie/deleteCookie request.
    // Precedence: explicit domain/path, else derived from url, else current
    // page.
    static void resolveTarget(CDPCommand& cmd, WebView* wv, std::string& domain,
                              std::string& path)
    {
        std::string url = paramStr(cmd, "url");
        std::string d = paramStr(cmd, "domain");
        std::string p = paramStr(cmd, "path");

        if (!d.empty()) {
            domain = d;
        } else if (!url.empty()) {
            domain = urlHost(url);
        } else {
            domain = urlHost(currentUrl(wv));
        }

        if (!p.empty()) {
            path = p;
        } else if (!url.empty()) {
            std::string up = urlPath(url);
            size_t slash = up.find_last_of('/');
            path = (slash == std::string::npos || slash == 0)
                       ? "/"
                       : up.substr(0, slash);
        } else {
            path = "/";
        }
        if (path.empty()) {
            path = "/";
        }
    }

    // Parse a single CDP cookie object (CookieParam form) into a CookieEntry.
    // domain/path precedence matches resolveTarget: explicit domain/path, else
    // derived from the cookie's url, else the supplied currentHost/currentPath.
    static CookieEntry cookieFromJson(const rapidjson::Value& cv,
                                      const std::string& currentHost,
                                      const std::string& currentPath)
    {
        CookieEntry e;
        if (cv.HasMember("name") && cv["name"].IsString()) {
            e.name = cv["name"].GetString();
        }
        if (cv.HasMember("value") && cv["value"].IsString()) {
            e.value = cv["value"].GetString();
        }
        std::string url, d, p;
        if (cv.HasMember("url") && cv["url"].IsString()) {
            url = cv["url"].GetString();
        }
        if (cv.HasMember("domain") && cv["domain"].IsString()) {
            d = cv["domain"].GetString();
        }
        if (cv.HasMember("path") && cv["path"].IsString()) {
            p = cv["path"].GetString();
        }
        if (!d.empty()) {
            e.domain = d;
        } else if (!url.empty()) {
            e.domain = urlHost(url);
        } else {
            e.domain = currentHost;
        }
        if (!p.empty()) {
            e.path = p;
        } else if (!url.empty()) {
            std::string up = urlPath(url);
            size_t slash = up.find_last_of('/');
            e.path = (slash == std::string::npos || slash == 0)
                         ? "/"
                         : up.substr(0, slash);
        } else {
            e.path = currentPath.empty() ? "/" : currentPath;
        }
        if (cv.HasMember("secure") && cv["secure"].IsBool()) {
            e.secure = cv["secure"].GetBool();
        }
        if (cv.HasMember("httpOnly") && cv["httpOnly"].IsBool()) {
            e.httpOnly = cv["httpOnly"].GetBool();
        }
        if (cv.HasMember("expires") && cv["expires"].IsNumber()) {
            e.expires = (int64_t)cv["expires"].GetDouble();
        }
        return e;
    }

    static const char* statusText(int status)
    {
        switch (status) {
        case 200:
            return "OK";
        case 201:
            return "Created";
        case 204:
            return "No Content";
        case 301:
            return "Moved Permanently";
        case 302:
            return "Found";
        case 304:
            return "Not Modified";
        case 400:
            return "Bad Request";
        case 401:
            return "Unauthorized";
        case 403:
            return "Forbidden";
        case 404:
            return "Not Found";
        case 500:
            return "Internal Server Error";
        case 503:
            return "Service Unavailable";
        default:
            return "";
        }
    }

    // CDP resourceType from the engine's mime type (best effort). Subresource
    // requests in this engine carry no explicit "as" attribute through the
    // hook, so classify by mime. Document is handled by the caller.
    static const char* resourceTypeFromMime(const std::string& mime)
    {
        if (mime.compare(0, 6, "image/") == 0) {
            return "Image";
        }
        if (mime.find("javascript") != std::string::npos ||
            mime.find("ecmascript") != std::string::npos) {
            return "Script";
        }
        if (mime.compare(0, 5, "text/") == 0 &&
            mime.find("css") != std::string::npos) {
            return "Stylesheet";
        }
        if (mime.find("css") != std::string::npos) {
            return "Stylesheet";
        }
        if (mime.find("font") != std::string::npos) {
            return "Font";
        }
        return "Other";
    }

    // Chrome's setBlockedURLs patterns use '*' as a wildcard matching any run
    // of characters; every other character is literal. Greedy backtracking
    // match.
    static bool wildcardMatch(const std::string& pattern,
                              const std::string& text)
    {
        size_t p = 0, t = 0, star = std::string::npos, mark = 0;
        while (t < text.size()) {
            if (p < pattern.size() && pattern[p] == '*') {
                star = p++;
                mark = t;
            } else if (p < pattern.size() && (pattern[p] == text[t])) {
                ++p;
                ++t;
            } else if (star != std::string::npos) {
                p = star + 1;
                t = ++mark;
            } else {
                return false;
            }
        }
        while (p < pattern.size() && pattern[p] == '*') {
            ++p;
        }
        return p == pattern.size();
    }

} // namespace

CDPSession* NetworkDomain::sessionForWebView(WebView* wv)
{
    TargetContext* ctx = m_dispatcher->contextForWebView(wv);
    return ctx ? ctx->session : nullptr;
}

std::string NetworkDomain::onResourceWillBeSent(WebView* wv, Resource* res,
                                                const std::string& postData)
{
    CDPSession* s = sessionForWebView(wv);
    if (!s || !s->networkEnabled || !res || !res->url()) {
        return std::string();
    }

    // The top-level document is loaded through the ResourceLoader with
    // RequestMode::Navigate (HTMLDocumentBuilder::build), using a plain
    // ResourceURL -- isDocumentURL() is false for it -- so detect by request
    // mode, not URL type. Subresources (script/img/css) use other modes.
    bool isDocument =
        res->resourceRequest() &&
        res->resourceRequest()->requestMode() == RequestMode::Navigate;
    std::string urlEarly = res->url()->urlString()
                               ? res->url()->urlString()->toUTF8NonGCString()
                               : std::string();
    bool isHttp = urlEarly.compare(0, 7, "http://") == 0 ||
                  urlEarly.compare(0, 8, "https://") == 0;

    // The top-level document is owned by the synthetic navigation path for
    // every scheme except real HTTP(S):
    //  - Fetch interception active: the deferred Fetch state machine emits the
    //    document events; a real one here would conflict.
    //  - data:/about: and other non-network schemes: PageDomain emits the
    //    synthetic document triple (there is no meaningful real transport), and
    //    its events fire synchronously so puppeteer's navigation response
    //    resolves. A real document event here would duplicate them.
    // Subresources are always surfaced regardless of scheme.
    if (isDocument && (s->fetchEnabled || !isHttp)) {
        return std::string();
    }
    // The top-level document reuses the loaderId as its requestId so
    // puppeteer's isNavigationRequest() (requestId === loaderId && type ===
    // "Document") associates the navigation response. Subresources get REQ-n
    // ids.
    std::string requestId;
    if (isDocument) {
        requestId = s->loaderId;
        s->lastNavigationRequestId = requestId;
    } else {
        requestId = "REQ-" + std::to_string(++s->networkRequestCounter);
    }

    std::string url = res->url()->urlString()
                          ? res->url()->urlString()->toUTF8NonGCString()
                          : std::string();
    std::string method = "GET";
    if (res->resourceRequest() && res->resourceRequest()->method()) {
        method = res->resourceRequest()->method()->toUTF8NonGCString();
    }

    const std::string& sessionId = s->sessionId;
    double ts = (double)longTickCount();

    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    rapidjson::Value params(rapidjson::kObjectType);
    params.AddMember(
        "requestId",
        rapidjson::Value(requestId.c_str(), requestId.size(), alloc), alloc);
    params.AddMember(
        "loaderId",
        rapidjson::Value(s->loaderId.c_str(), s->loaderId.size(), alloc),
        alloc);
    params.AddMember(
        "frameId",
        rapidjson::Value(s->frameId.c_str(), s->frameId.size(), alloc), alloc);
    params.AddMember("documentURL",
                     rapidjson::Value(url.c_str(), url.size(), alloc), alloc);
    rapidjson::Value request(rapidjson::kObjectType);
    request.AddMember("url", rapidjson::Value(url.c_str(), url.size(), alloc),
                      alloc);
    request.AddMember("method",
                      rapidjson::Value(method.c_str(), method.size(), alloc),
                      alloc);
    // Real request headers (includes any setExtraHTTPHeaders injected earlier).
    rapidjson::Value headers(rapidjson::kObjectType);
    if (res->resourceRequest()) {
        const HeaderMap& hm = res->resourceRequest()->resquestHeaderMap();
        for (const auto& kv : hm) {
            headers.AddMember(
                rapidjson::Value(kv.first.c_str(), kv.first.size(), alloc),
                rapidjson::Value(kv.second.c_str(), kv.second.size(), alloc),
                alloc);
        }
    }
    request.AddMember("headers", headers, alloc);
    // POST body: store it for getRequestPostData and reflect it in the event
    // (postData + hasPostData), matching Chrome's requestWillBeSent.
    if (!postData.empty()) {
        s->networkRequestPostData[requestId] = postData;
        request.AddMember(
            "postData",
            rapidjson::Value(postData.c_str(), postData.size(), alloc), alloc);
        request.AddMember("hasPostData", true, alloc);
    }
    request.AddMember("initialPriority",
                      rapidjson::Value(isDocument ? "VeryHigh" : "High", alloc),
                      alloc);
    request.AddMember("referrerPolicy", "strict-origin-when-cross-origin",
                      alloc);
    params.AddMember("request", request, alloc);
    rapidjson::Value initiator(rapidjson::kObjectType);
    initiator.AddMember(
        "type", rapidjson::Value(isDocument ? "other" : "parser", alloc),
        alloc);
    params.AddMember("initiator", initiator, alloc);
    params.AddMember("type",
                     rapidjson::Value(isDocument ? "Document" : "Other", alloc),
                     alloc);
    params.AddMember("timestamp", ts, alloc);
    params.AddMember("wallTime", ts, alloc);
    params.AddMember("redirectHasExtraInfo", false, alloc);
    params.AddMember("hasUserGesture", false, alloc);
    CDPCommand evt(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
    evt.sendEvent("Network.requestWillBeSent", params, doc);

    // A real request is now in flight: count it and (re)arm the networkidle
    // debounce so a freshly started request resets the 500ms quiescence window.
    ++s->networkInFlight;
    scheduleNetworkIdleCheck(wv);
    return requestId;
}

void NetworkDomain::onResourceResponse(WebView* wv,
                                       const std::string& requestId,
                                       Resource* res, ResourceRequest* rr)
{
    CDPSession* s = sessionForWebView(wv);
    if (!s || !s->networkEnabled || requestId.empty() || !res || !rr) {
        return;
    }

    bool isDocument = rr->requestMode() == RequestMode::Navigate;
    std::string url = res->url() && res->url()->urlString()
                          ? res->url()->urlString()->toUTF8NonGCString()
                          : std::string();
    int status = (int)rr->status();
    std::string mime = rr->responseMimeType()
                           ? rr->responseMimeType()->toUTF8NonGCString()
                           : std::string();
    if (mime.empty()) {
        mime = isDocument ? "text/html" : "application/octet-stream";
    }

    const std::string& sessionId = s->sessionId;
    double ts = (double)longTickCount();

    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    rapidjson::Value params(rapidjson::kObjectType);
    params.AddMember(
        "requestId",
        rapidjson::Value(requestId.c_str(), requestId.size(), alloc), alloc);
    params.AddMember(
        "loaderId",
        rapidjson::Value(s->loaderId.c_str(), s->loaderId.size(), alloc),
        alloc);
    params.AddMember(
        "frameId",
        rapidjson::Value(s->frameId.c_str(), s->frameId.size(), alloc), alloc);
    params.AddMember("timestamp", ts, alloc);
    const char* rtype = isDocument ? "Document" : resourceTypeFromMime(mime);
    params.AddMember("type", rapidjson::Value(rtype, alloc), alloc);
    rapidjson::Value response(rapidjson::kObjectType);
    response.AddMember("url", rapidjson::Value(url.c_str(), url.size(), alloc),
                       alloc);
    response.AddMember("status", status, alloc);
    response.AddMember("statusText",
                       rapidjson::Value(statusText(status), alloc), alloc);
    rapidjson::Value headers(rapidjson::kObjectType);
    const HeaderMap& hm = rr->responseHeaderMap();
    for (const auto& kv : hm) {
        headers.AddMember(
            rapidjson::Value(kv.first.c_str(), kv.first.size(), alloc),
            rapidjson::Value(kv.second.c_str(), kv.second.size(), alloc),
            alloc);
    }
    response.AddMember("headers", headers, alloc);
    response.AddMember(
        "mimeType", rapidjson::Value(mime.c_str(), mime.size(), alloc), alloc);
    response.AddMember("connectionReused", false, alloc);
    response.AddMember("connectionId", 0, alloc);
    response.AddMember("fromDiskCache", false, alloc);
    response.AddMember("fromServiceWorker", false, alloc);
    response.AddMember("encodedDataLength", 0, alloc);
    response.AddMember("securityState", "secure", alloc);
    params.AddMember("response", response, alloc);
    params.AddMember("hasExtraInfo", false, alloc);
    CDPCommand evt(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
    evt.sendEvent("Network.responseReceived", params, doc);

    // Record whether this body should be reported base64 (non-text payloads).
    bool isText = mime.compare(0, 5, "text/") == 0 ||
                  mime.find("javascript") != std::string::npos ||
                  mime.find("json") != std::string::npos ||
                  mime.find("xml") != std::string::npos ||
                  mime.find("ecmascript") != std::string::npos;
    s->networkBodyBase64[requestId] = !isText;

    // Record the resource for Page.getResourceTree / getResourceContent. Keyed
    // by requestId (first response wins); later phases (data/finished) fill the
    // body under the same requestId in networkBodies.
    bool seen = false;
    for (const ResourceRecord& r : s->resources) {
        if (r.requestId == requestId) {
            seen = true;
            break;
        }
    }
    if (!seen) {
        ResourceRecord rec;
        rec.url = url;
        rec.requestId = requestId;
        rec.type = rtype;
        rec.mimeType = mime;
        s->resources.push_back(rec);
    }
}

void NetworkDomain::onResourceData(WebView* wv, const std::string& requestId,
                                   const char* buf, size_t length)
{
    CDPSession* s = sessionForWebView(wv);
    if (!s || !s->networkEnabled || requestId.empty() || !buf || !length) {
        return;
    }
    s->networkBodies[requestId].append(buf, length);
}

void NetworkDomain::onResourceFinished(WebView* wv,
                                       const std::string& requestId)
{
    CDPSession* s = sessionForWebView(wv);
    if (!s || !s->networkEnabled || requestId.empty()) {
        return;
    }
    size_t len = 0;
    auto it = s->networkBodies.find(requestId);
    if (it != s->networkBodies.end()) {
        len = it->second.size();
    }
    const std::string& sessionId = s->sessionId;
    double ts = (double)longTickCount();
    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    rapidjson::Value params(rapidjson::kObjectType);
    params.AddMember(
        "requestId",
        rapidjson::Value(requestId.c_str(), requestId.size(), alloc), alloc);
    params.AddMember("timestamp", ts, alloc);
    params.AddMember("encodedDataLength", (double)len, alloc);
    CDPCommand evt(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
    evt.sendEvent("Network.loadingFinished", params, doc);

    if (s->networkInFlight > 0) {
        --s->networkInFlight;
    }
    scheduleNetworkIdleCheck(wv);
}

void NetworkDomain::onResourceFailed(WebView* wv, const std::string& requestId)
{
    CDPSession* s = sessionForWebView(wv);
    if (!s || !s->networkEnabled || requestId.empty()) {
        return;
    }
    const std::string& sessionId = s->sessionId;
    double ts = (double)longTickCount();
    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    rapidjson::Value params(rapidjson::kObjectType);
    params.AddMember(
        "requestId",
        rapidjson::Value(requestId.c_str(), requestId.size(), alloc), alloc);
    params.AddMember("timestamp", ts, alloc);
    params.AddMember("type", "Other", alloc);
    params.AddMember("errorText", "net::ERR_FAILED", alloc);
    params.AddMember("canceled", false, alloc);
    CDPCommand evt(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
    evt.sendEvent("Network.loadingFailed", params, doc);

    if (s->networkInFlight > 0) {
        --s->networkInFlight;
    }
    scheduleNetworkIdleCheck(wv);
}

bool NetworkDomain::shouldBlockUrl(CDPSession* s, const std::string& url,
                                   std::string& errorText)
{
    if (!s || !s->networkEnabled) {
        return false;
    }
    // Only real network (http/https) requests are subject to offline/blocking.
    // data:/about:/file: are not network transfers and load even when offline,
    // matching Chrome.
    bool isHttp =
        url.compare(0, 7, "http://") == 0 || url.compare(0, 8, "https://") == 0;
    if (!isHttp) {
        return false;
    }
    if (s->networkOffline) {
        errorText = "net::ERR_INTERNET_DISCONNECTED";
        return true;
    }
    for (const std::string& pat : s->networkBlockedUrls) {
        if (wildcardMatch(pat, url)) {
            errorText = "net::ERR_BLOCKED_BY_CLIENT";
            return true;
        }
    }
    return false;
}

bool NetworkDomain::shouldBlockRequest(WebView* wv, Resource* res,
                                       std::string& errorText)
{
    CDPSession* s = sessionForWebView(wv);
    if (!s || !res || !res->url()) {
        return false;
    }
    std::string url = res->url()->urlString()
                          ? res->url()->urlString()->toUTF8NonGCString()
                          : std::string();
    return shouldBlockUrl(s, url, errorText);
}

void NetworkDomain::emitLoadingFailed(WebView* wv, const std::string& requestId,
                                      const std::string& errorText)
{
    CDPSession* s = sessionForWebView(wv);
    if (!s || !s->networkEnabled || requestId.empty()) {
        return;
    }
    const std::string& sessionId = s->sessionId;
    double ts = (double)longTickCount();
    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    rapidjson::Value params(rapidjson::kObjectType);
    params.AddMember(
        "requestId",
        rapidjson::Value(requestId.c_str(), requestId.size(), alloc), alloc);
    params.AddMember("timestamp", ts, alloc);
    params.AddMember("type", "Other", alloc);
    params.AddMember(
        "errorText",
        rapidjson::Value(errorText.c_str(), errorText.size(), alloc), alloc);
    params.AddMember("canceled", false, alloc);
    CDPCommand evt(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
    evt.sendEvent("Network.loadingFailed", params, doc);

    // A blocked real request (offline / setBlockedURLs) that already counted
    // toward in-flight is now terminal; drop it so networkidle can settle. The
    // synthetic navigate-block path passes the document loaderId without a
    // prior requestWillBeSent (in-flight 0); the guard prevents underflow
    // there.
    if (s->networkInFlight > 0) {
        --s->networkInFlight;
    }
    scheduleNetworkIdleCheck(wv);
}

void NetworkDomain::applyExtraHTTPHeaders(WebView* wv, ResourceRequest* rr)
{
    CDPSession* s = sessionForWebView(wv);
    if (!s || s->extraHTTPHeaders.empty() || !rr) {
        return;
    }
    for (const auto& kv : s->extraHTTPHeaders) {
        rr->setRequestHeader(
            String::fromUTF8(kv.first.data(), kv.first.size()),
            String::fromUTF8(kv.second.data(), kv.second.size()));
    }
}

namespace {

    // Heap payload carried by the networkidle debounce Timer. Plain malloc (not
    // GC): freed by the handler when it fires (and on cancel via removeTimer in
    // scheduleNetworkIdleCheck before re-arming). `loaderGen` snapshots the
    // session loaderId when the timer was armed so a timer that survives a new
    // navigation is recognised as stale and ignored.
    struct NetworkIdleTimerData {
        CDPDispatcher* dispatcher;
        std::string sessionId;
        std::string loaderGen;
    };

    // Build + send a Page.lifecycleEvent for a networkidle level from a timer
    // context (no current-session routing). frameId/loaderId come from the
    // resolved session; sessionId tags the event envelope.
    static void sendIdleLifecycle(CDPDispatcher* dispatcher, CDPSession* s,
                                  const std::string& sessionId,
                                  const char* name)
    {
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value params(rapidjson::kObjectType);
        params.AddMember(
            "frameId",
            rapidjson::Value(s->frameId.c_str(), s->frameId.size(), alloc),
            alloc);
        params.AddMember(
            "loaderId",
            rapidjson::Value(s->loaderId.c_str(), s->loaderId.size(), alloc),
            alloc);
        params.AddMember("name", rapidjson::Value(name, alloc), alloc);
        params.AddMember("timestamp", (double)longTickCount(), alloc);
        CDPCommand evt(dispatcher, Optional<int64_t>(), sessionId, nullptr);
        evt.sendEvent("Page.lifecycleEvent", params, doc);
    }

} // namespace

// Static Timer callback: the 500ms quiescence window elapsed. Emit whichever
// networkidle level the current in-flight count permits and that has not yet
// fired for this navigation.
void NetworkDomain::onNetworkIdleTimer(void* data)
{
    NetworkIdleTimerData* td = static_cast<NetworkIdleTimerData*>(data);
    CDPDispatcher* dispatcher = td->dispatcher;
    std::string sessionId = td->sessionId;
    std::string loaderGen = td->loaderGen;
    delete td;

    TargetContext* ctx = dispatcher->contextForSession(sessionId);
    if (!ctx || !ctx->session) {
        return;
    }
    CDPSession* s = ctx->session;
    // The timer fired and is consumed.
    s->networkIdleTimerId = SIZE_MAX;
    // A navigation started after this timer was armed -> stale, ignore.
    if (s->loaderId != loaderGen) {
        return;
    }
    if (!s->networkEnabled) {
        return;
    }

    // networkAlmostIdle: <= 2 in-flight; networkIdle: 0 in-flight. Each once
    // per navigation. Emit almostIdle before idle (Chrome's order) when both
    // apply.
    if (s->networkInFlight <= 2 && !s->networkAlmostIdleEmitted) {
        s->networkAlmostIdleEmitted = true;
        sendIdleLifecycle(dispatcher, s, sessionId, "networkAlmostIdle");
    }
    if (s->networkInFlight == 0 && !s->networkIdleEmitted) {
        s->networkIdleEmitted = true;
        sendIdleLifecycle(dispatcher, s, sessionId, "networkIdle");
    }
}

void NetworkDomain::resetNetworkIdle(WebView* wv)
{
    CDPSession* s = sessionForWebView(wv);
    if (!s) {
        return;
    }
    if (s->networkIdleTimerId != SIZE_MAX && wv->timer()) {
        wv->timer()->removeTimer(s->networkIdleTimerId);
        s->networkIdleTimerId = SIZE_MAX;
    }
    s->networkInFlight = 0;
    s->networkAlmostIdleEmitted = false;
    s->networkIdleEmitted = false;
}

void NetworkDomain::scheduleNetworkIdleCheck(WebView* wv)
{
    CDPSession* s = sessionForWebView(wv);
    if (!s || !s->networkEnabled) {
        return;
    }
    GlobalScope* gs = wv->mainBrowsingContext()
                          ? wv->mainBrowsingContext()->window()
                          : nullptr;
    if (!wv->timer() || !gs) {
        return;
    }

    // Nothing left to announce: both idle levels already fired this navigation.
    if (s->networkAlmostIdleEmitted && s->networkIdleEmitted) {
        return;
    }

    // Cancel any pending window: the in-flight count just changed, so the 500ms
    // quiescence must be measured from now. A newly started request (count went
    // up) thereby resets the debounce; a completion re-measures from
    // completion.
    if (s->networkIdleTimerId != SIZE_MAX) {
        wv->timer()->removeTimer(s->networkIdleTimerId);
        s->networkIdleTimerId = SIZE_MAX;
    }

    // Only arm when an idle level is currently reachable and still unannounced:
    // <= 2 in-flight enables networkAlmostIdle, 0 enables networkIdle. While
    // more than 2 are in flight (or both levels already fired) no timer runs;
    // the next completion that drops the count re-evaluates.
    bool almostReachable =
        s->networkInFlight <= 2 && !s->networkAlmostIdleEmitted;
    bool idleReachable = s->networkInFlight == 0 && !s->networkIdleEmitted;
    if (!almostReachable && !idleReachable) {
        return;
    }

    NetworkIdleTimerData* td = new NetworkIdleTimerData();
    td->dispatcher = m_dispatcher;
    td->sessionId = s->sessionId;
    td->loaderGen = s->loaderId;
    s->networkIdleTimerId = wv->timer()->addTimer(
        500, gs, &NetworkDomain::onNetworkIdleTimer, td, false);
}

void NetworkDomain::processMessage(CDPCommand& cmd, const std::string& method)
{
    CDPSession* s = m_dispatcher->session();

    if (method == "enable") {
        s->networkEnabled = true;
        cmd.sendResultEmpty();
        return;
    }
    if (method == "disable") {
        s->networkEnabled = false;
        cmd.sendResultEmpty();
        return;
    }

    if (method == "setCookie") {
        WebView* wv = m_dispatcher->webView();
        CookieEntry e;
        e.name = paramStr(cmd, "name");
        e.value = paramStr(cmd, "value");
        std::string domain, path;
        resolveTarget(cmd, wv, domain, path);
        e.domain = domain;
        e.path = path;
        e.secure = paramBool(cmd, "secure", false);
        e.httpOnly = paramBool(cmd, "httpOnly", false);
        e.expires = hasNumber(cmd, "expires")
                        ? (int64_t)paramNum(cmd, "expires", 0)
                        : 0;
        bool ok = !e.name.empty() && !e.domain.empty();
        if (ok) {
            writeCookieLine(e);
        }
        rapidjson::Document doc;
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("success", ok, doc.GetAllocator());
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "setCookies") {
        WebView* wv = m_dispatcher->webView();
        if (cmd.params() && cmd.params()->HasMember("cookies") &&
            (*cmd.params())["cookies"].IsArray()) {
            rapidjson::Value& cookies = (*cmd.params())["cookies"];
            std::string curHost = urlHost(currentUrl(wv));
            for (rapidjson::SizeType i = 0; i < cookies.Size(); ++i) {
                rapidjson::Value& cv = cookies[i];
                if (!cv.IsObject()) {
                    continue;
                }
                CookieEntry e = cookieFromJson(cv, curHost, std::string());
                if (!e.name.empty() && !e.domain.empty()) {
                    writeCookieLine(e);
                }
            }
        }
        cmd.sendResultEmpty();
        return;
    }

    if (method == "getCookies" || method == "getAllCookies") {
        std::vector<CookieEntry> all = readAllCookies();

        // Build the list of (host,path) filters. getAllCookies => none.
        std::vector<std::pair<std::string, std::string>> filters;
        if (method == "getCookies") {
            if (cmd.params() && cmd.params()->HasMember("urls") &&
                (*cmd.params())["urls"].IsArray() &&
                (*cmd.params())["urls"].Size() > 0) {
                rapidjson::Value& urls = (*cmd.params())["urls"];
                for (rapidjson::SizeType i = 0; i < urls.Size(); ++i) {
                    if (urls[i].IsString()) {
                        std::string u = urls[i].GetString();
                        filters.push_back({ urlHost(u), urlPath(u) });
                    }
                }
            } else {
                std::string u = currentUrl(m_dispatcher->webView());
                filters.push_back({ urlHost(u), urlPath(u) });
            }
        }

        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value arr(rapidjson::kArrayType);
        time_t now = time(nullptr);
        for (const CookieEntry& e : all) {
            if (e.expires != 0 && (int64_t)now > e.expires) {
                continue; // expired
            }
            bool include = filters.empty();
            for (const auto& f : filters) {
                if (cookieDomainMatches(e.domain, f.first) &&
                    cookiePathMatches(e.path, f.second)) {
                    include = true;
                    break;
                }
            }
            if (include) {
                addCookieObject(arr, alloc, e);
            }
        }
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("cookies", arr, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "deleteCookies") {
        WebView* wv = m_dispatcher->webView();
        std::string name = paramStr(cmd, "name");
        std::string domain, path;
        resolveTarget(cmd, wv, domain, path);

        // Match against the jar and overwrite each match with a past expiry so
        // curl drops it. domain/path from resolveTarget pick the deletion key,
        // but the jar may store the domain with a leading '.', so match
        // loosely.
        std::vector<CookieEntry> all = readAllCookies();
        for (CookieEntry e : all) {
            if (e.name != name) {
                continue;
            }
            if (!cookieDomainMatches(e.domain, domain) && e.domain != domain) {
                continue;
            }
            std::string cp = e.path.empty() ? "/" : e.path;
            if (cp != path) {
                continue;
            }
            e.expires = 1; // in the past => removal
            writeCookieLine(e);
        }
        cmd.sendResultEmpty();
        return;
    }

    if (method == "getResponseBody") {
        std::string requestId = paramStr(cmd, "requestId");
        if (requestId.empty()) {
            cmd.sendError(-32000,
                          "No data found for resource with given identifier");
            return;
        }

        // Prefer the real bytes captured by the ResourceLoader hook (document
        // and subresources alike). base64-encode non-text payloads.
        auto bit = s->networkBodies.find(requestId);
        if (bit != s->networkBodies.end()) {
            const std::string& raw = bit->second;
            bool b64 = false;
            auto fit = s->networkBodyBase64.find(requestId);
            if (fit != s->networkBodyBase64.end()) {
                b64 = fit->second;
            }
            std::string body =
                b64 ? cdpBase64Encode(
                          reinterpret_cast<const uint8_t*>(raw.data()),
                          raw.size())
                    : raw;
            rapidjson::Document doc;
            rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
            rapidjson::Value result(rapidjson::kObjectType);
            result.AddMember("body",
                             rapidjson::Value(body.c_str(), body.size(), alloc),
                             alloc);
            result.AddMember("base64Encoded", b64, alloc);
            cmd.sendResult(result, doc);
            return;
        }

        // Fallback: no captured bytes (e.g. data: navigation, which has no real
        // network request). Serialize the live document for the latest
        // navigation request id.
        if (requestId != s->lastNavigationRequestId) {
            cmd.sendError(-32000,
                          "No data found for resource with given identifier");
            return;
        }
        std::string body;
        WebView* wv = m_dispatcher->webView();
        BrowsingContext* bc = wv ? wv->mainBrowsingContext() : nullptr;
        if (bc && bc->document() && bc->document()->documentElement()) {
            String* html = bc->document()->documentElement()->outerHTML();
            if (html) {
                body = "<!DOCTYPE html>\n" + html->toUTF8NonGCString();
            }
        }
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember(
            "body", rapidjson::Value(body.c_str(), body.size(), alloc), alloc);
        result.AddMember("base64Encoded", false, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "searchInResponseBody") {
        std::string requestId = paramStr(cmd, "requestId");
        std::string query = paramStr(cmd, "query");
        bool caseSensitive = paramBool(cmd, "caseSensitive", false);
        bool isRegex = paramBool(cmd, "isRegex", false);

        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value matches(rapidjson::kArrayType);

        // Look up the captured response body for this requestId. If none was
        // recorded (e.g. a synthetic data: navigation, or an unknown id), the
        // search yields no matches (empty result), matching Chrome.
        auto bit = s->networkBodies.find(requestId);
        if (bit != s->networkBodies.end() && !query.empty()) {
            const std::string& body = bit->second;

            // Build the matcher: regex when isRegex, otherwise a literal
            // substring search. caseSensitive=false (default) lowercases both
            // sides / passes std::regex::icase. A malformed regex matches
            // nothing rather than erroring (best-effort, like Chrome's tolerant
            // behavior for clients).
            std::regex re;
            bool reValid = false;
            std::string needleLower = query;
            if (isRegex) {
                try {
                    auto flags = std::regex::ECMAScript;
                    if (!caseSensitive) {
                        flags |= std::regex::icase;
                    }
                    re = std::regex(query, flags);
                    reValid = true;
                } catch (const std::regex_error&) {
                    reValid = false;
                }
            } else if (!caseSensitive) {
                std::transform(needleLower.begin(), needleLower.end(),
                               needleLower.begin(), ::tolower);
            }

            // Walk the body line by line (1-based lineNumber, CDP convention).
            size_t start = 0;
            int lineNumber = 0;
            while (start <= body.size()) {
                size_t nl = body.find('\n', start);
                std::string line = (nl == std::string::npos)
                                       ? body.substr(start)
                                       : body.substr(start, nl - start);
                // Strip a trailing CR (CRLF line endings).
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }

                bool hit = false;
                if (isRegex) {
                    hit = reValid && std::regex_search(line, re);
                } else if (caseSensitive) {
                    hit = line.find(query) != std::string::npos;
                } else {
                    std::string lineLower = line;
                    std::transform(lineLower.begin(), lineLower.end(),
                                   lineLower.begin(), ::tolower);
                    hit = lineLower.find(needleLower) != std::string::npos;
                }

                if (hit) {
                    rapidjson::Value m(rapidjson::kObjectType);
                    m.AddMember("lineNumber", lineNumber, alloc);
                    m.AddMember(
                        "lineContent",
                        rapidjson::Value(line.c_str(), line.size(), alloc),
                        alloc);
                    matches.PushBack(m, alloc);
                }

                if (nl == std::string::npos) {
                    break;
                }
                start = nl + 1;
                lineNumber++;
            }
        }

        result.AddMember("result", matches, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "getRequestPostData") {
        std::string requestId = paramStr(cmd, "requestId");
        auto it = s->networkRequestPostData.find(requestId);
        if (requestId.empty() || it == s->networkRequestPostData.end()) {
            // Chrome errors when the request had no body. Mirror that so
            // request.postData()/fetchPostData() behave as expected.
            cmd.sendError(-32000, "No post data available for the request");
            return;
        }
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember(
            "postData",
            rapidjson::Value(it->second.c_str(), it->second.size(), alloc),
            alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "setExtraHTTPHeaders") {
        // Store the headers on the session; the ResourceLoader network hook
        // (applyExtraHTTPHeaders) injects them into every real outgoing request
        // for this WebView.
        s->extraHTTPHeaders.clear();
        if (cmd.params() && cmd.params()->HasMember("headers") &&
            (*cmd.params())["headers"].IsObject()) {
            const rapidjson::Value& h = (*cmd.params())["headers"];
            for (auto m = h.MemberBegin(); m != h.MemberEnd(); ++m) {
                if (m->value.IsString()) {
                    s->extraHTTPHeaders[m->name.GetString()] =
                        m->value.GetString();
                }
            }
        }
        cmd.sendResultEmpty();
        return;
    }

    if (method == "emulateNetworkConditions") {
        // offline gates the ResourceLoader blocking hook (shouldBlockRequest).
        // latency/throughput are stored for completeness but not enforced --
        // no throttling is implemented in this engine.
        s->networkOffline = paramBool(cmd, "offline", false);
        s->networkLatency = paramNum(cmd, "latency", 0);
        s->networkDownloadThroughput = paramNum(cmd, "downloadThroughput", -1);
        s->networkUploadThroughput = paramNum(cmd, "uploadThroughput", -1);
        cmd.sendResultEmpty();
        return;
    }

    if (method == "setBlockedURLs") {
        s->networkBlockedUrls.clear();
        if (cmd.params() && cmd.params()->HasMember("urls") &&
            (*cmd.params())["urls"].IsArray()) {
            const rapidjson::Value& urls = (*cmd.params())["urls"];
            for (rapidjson::SizeType i = 0; i < urls.Size(); ++i) {
                if (urls[i].IsString()) {
                    s->networkBlockedUrls.push_back(urls[i].GetString());
                }
            }
        }
        cmd.sendResultEmpty();
        return;
    }

    if (method == "setCacheDisabled") {
        // Toggle the process HTTPCache between LOAD_NO_CACHE (bypass disk
        // cache) and LOAD_DEFAULT. The cache is process-wide, so the effect is
        // global even though the state is tracked per session. If HTTPCache is
        // not built or not available on this thread, fall through to an ack
        // (state stored).
        bool disabled = paramBool(cmd, "cacheDisabled", false);
        s->networkCacheDisabled = disabled;
#if defined(STARFISH_ENABLE_HTTPCACHE)
        WebView* wv = m_dispatcher->webView();
        if (wv) {
            Optional<HTTPCache*> cache = wv->starfish()->httpCache();
            if (cache) {
                cache.getValue()->setCacheMode(disabled
                                                   ? HTTPCache::LOAD_NO_CACHE
                                                   : HTTPCache::LOAD_DEFAULT);
            }
        }
#endif
        cmd.sendResultEmpty();
        return;
    }

    if (method == "clearBrowserCache") {
        // Drop every cached HTTP response. No-op ack when HTTPCache is absent.
#if defined(STARFISH_ENABLE_HTTPCACHE)
        WebView* wv = m_dispatcher->webView();
        if (wv) {
            Optional<HTTPCache*> cache = wv->starfish()->httpCache();
            if (cache) {
                cache.getValue()->clear();
            }
        }
#endif
        cmd.sendResultEmpty();
        return;
    }

    if (method == "clearBrowserCookies") {
        // Expire every cookie in the shared curl jar (same backing store as the
        // Network/Storage cookie methods).
        clearAllCookies();
        cmd.sendResultEmpty();
        return;
    }

    if (method == "getCertificate") {
        // TLS certificate chains are not collected by this engine. Return the
        // empty result shape Chrome uses when no certificate is known.
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("tableNames", rapidjson::Value(rapidjson::kArrayType),
                         alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "setUserAgentOverride") {
        // Set the per-WebView custom user agent; navigator.userAgent and real
        // outgoing request User-Agent headers read this. Empty userAgent clears
        // the override (falls back to the built-in UA).
        std::string ua = paramStr(cmd, "userAgent");
        WebView* wv = m_dispatcher->webView();
        if (wv) {
            wv->setCustomUserAgentString(
                String::fromUTF8(ua.c_str(), ua.size()));
        }
        cmd.sendResultEmpty();
        return;
    }

    if (method == "replayXHR") {
        // Re-issuing a captured XHR is not supported; ack without action.
        cmd.sendResultEmpty();
        return;
    }

    // Any remaining Network.* method (setBypassServiceWorker etc.) is accepted
    // but has no backing implementation in this MVP.
    cmd.sendResultEmpty();
}

void NetworkDomain::emitNavigation(const std::string& sessionId,
                                   const std::string& url)
{
    // Non-intercepted path: request and response phases fire back to back.
    emitNavigationRequest(sessionId, url);
    emitNavigationResponse(sessionId, url);
}

void NetworkDomain::emitNavigationRequest(const std::string& sessionId,
                                          const std::string& url)
{
    CDPSession* s = m_dispatcher->session();
    if (!s->networkEnabled) {
        return;
    }

    // Chrome assigns the top-level document request a requestId equal to the
    // loaderId; puppeteer's isNavigationRequest() (requestId === loaderId &&
    // type === "Document") relies on this to associate the navigation response.
    // Match that so page.goto() resolves a non-null navigation response.
    std::string requestId = s->loaderId;
    // Record this as the latest navigation document request so getResponseBody
    // can serve the live document under it.
    s->lastNavigationRequestId = requestId;
    double ts = (double)longTickCount();

    // Network.requestWillBeSent
    {
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value params(rapidjson::kObjectType);
        params.AddMember(
            "requestId",
            rapidjson::Value(requestId.c_str(), requestId.size(), alloc),
            alloc);
        params.AddMember(
            "loaderId",
            rapidjson::Value(s->loaderId.c_str(), s->loaderId.size(), alloc),
            alloc);
        params.AddMember(
            "frameId",
            rapidjson::Value(s->frameId.c_str(), s->frameId.size(), alloc),
            alloc);
        params.AddMember("documentURL",
                         rapidjson::Value(url.c_str(), url.size(), alloc),
                         alloc);
        rapidjson::Value request(rapidjson::kObjectType);
        request.AddMember(
            "url", rapidjson::Value(url.c_str(), url.size(), alloc), alloc);
        request.AddMember("method", "GET", alloc);
        request.AddMember("headers", rapidjson::Value(rapidjson::kObjectType),
                          alloc);
        request.AddMember("initialPriority", "VeryHigh", alloc);
        request.AddMember("referrerPolicy", "strict-origin-when-cross-origin",
                          alloc);
        params.AddMember("request", request, alloc);
        rapidjson::Value initiator(rapidjson::kObjectType);
        initiator.AddMember("type", "other", alloc);
        params.AddMember("initiator", initiator, alloc);
        params.AddMember("type", "Document", alloc);
        params.AddMember("timestamp", ts, alloc);
        params.AddMember("wallTime", ts, alloc);
        params.AddMember("redirectHasExtraInfo", false, alloc);
        params.AddMember("hasUserGesture", false, alloc);
        CDPCommand evt(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
        evt.sendEvent("Network.requestWillBeSent", params, doc);
    }
}

void NetworkDomain::emitNavigationResponse(const std::string& sessionId,
                                           const std::string& url)
{
    CDPSession* s = m_dispatcher->session();
    if (!s->networkEnabled) {
        return;
    }
    std::string requestId = s->lastNavigationRequestId;
    double ts = (double)longTickCount();

    // Network.responseReceived
    {
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value params(rapidjson::kObjectType);
        params.AddMember(
            "requestId",
            rapidjson::Value(requestId.c_str(), requestId.size(), alloc),
            alloc);
        params.AddMember(
            "loaderId",
            rapidjson::Value(s->loaderId.c_str(), s->loaderId.size(), alloc),
            alloc);
        params.AddMember(
            "frameId",
            rapidjson::Value(s->frameId.c_str(), s->frameId.size(), alloc),
            alloc);
        params.AddMember("timestamp", ts, alloc);
        params.AddMember("type", "Document", alloc);
        rapidjson::Value response(rapidjson::kObjectType);
        response.AddMember(
            "url", rapidjson::Value(url.c_str(), url.size(), alloc), alloc);
        response.AddMember("status", 200, alloc);
        response.AddMember("statusText", "OK", alloc);
        response.AddMember("headers", rapidjson::Value(rapidjson::kObjectType),
                           alloc);
        response.AddMember("mimeType", "text/html", alloc);
        response.AddMember("connectionReused", false, alloc);
        response.AddMember("connectionId", 0, alloc);
        response.AddMember("fromDiskCache", false, alloc);
        response.AddMember("fromServiceWorker", false, alloc);
        response.AddMember("encodedDataLength", 0, alloc);
        response.AddMember("securityState", "secure", alloc);
        params.AddMember("response", response, alloc);
        params.AddMember("hasExtraInfo", false, alloc);
        CDPCommand evt(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
        evt.sendEvent("Network.responseReceived", params, doc);
    }

    // Network.loadingFinished
    {
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value params(rapidjson::kObjectType);
        params.AddMember(
            "requestId",
            rapidjson::Value(requestId.c_str(), requestId.size(), alloc),
            alloc);
        params.AddMember("timestamp", ts, alloc);
        params.AddMember("encodedDataLength", 0, alloc);
        CDPCommand evt(m_dispatcher, Optional<int64_t>(), sessionId, nullptr);
        evt.sendEvent("Network.loadingFinished", params, doc);
    }
}

// --- Shared cookie-jar helpers (reused by the Storage domain) -----------

void NetworkDomain::appendAllCookies(rapidjson::Value& arr,
                                     rapidjson::Document::AllocatorType& alloc)
{
    std::vector<CookieEntry> all = readAllCookies();
    time_t now = time(nullptr);
    for (const CookieEntry& e : all) {
        if (e.expires != 0 && (int64_t)now > e.expires) {
            continue; // expired
        }
        addCookieObject(arr, alloc, e);
    }
}

void NetworkDomain::writeCookieArray(const rapidjson::Value& cookies,
                                     const std::string& currentHost,
                                     const std::string& currentPath)
{
    if (!cookies.IsArray()) {
        return;
    }
    for (rapidjson::SizeType i = 0; i < cookies.Size(); ++i) {
        const rapidjson::Value& cv = cookies[i];
        if (!cv.IsObject()) {
            continue;
        }
        CookieEntry e = cookieFromJson(cv, currentHost, currentPath);
        if (!e.name.empty() && !e.domain.empty()) {
            writeCookieLine(e);
        }
    }
}

void NetworkDomain::clearAllCookies()
{
    std::vector<CookieEntry> all = readAllCookies();
    for (CookieEntry e : all) {
        e.expires = 1; // in the past => removal
        writeCookieLine(e);
    }
}

void NetworkDomain::clearCookiesForHost(const std::string& host)
{
    if (host.empty()) {
        return;
    }
    std::vector<CookieEntry> all = readAllCookies();
    for (CookieEntry e : all) {
        if (!cookieDomainMatches(e.domain, host)) {
            continue;
        }
        e.expires = 1; // in the past => removal
        writeCookieLine(e);
    }
}

} // namespace Starfish

#endif
