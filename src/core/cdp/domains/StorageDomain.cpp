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
#include "StorageDomain.h"
#include "NetworkDomain.h"
#include "../CDPDispatcher.h"
#include "../CDPCommand.h"
#include "core/page/WebView.h"
#include "core/page/BrowsingContext.h"
#include "core/dom/Document.h"
#include "core/dom/WebOrigin.h"
#include "core/storage/StorageNamespace.h"
#include "core/storage/StorageInternal.h"
#include "platform/loader/ResourceURL.h"

#include "rapidjson/document.h"
#include <string>
#include <vector>

namespace Starfish {

namespace {

    // Host component of an origin/url string (e.g. "http://127.0.0.1:8770" ->
    // "127.0.0.1"). Empty input / no host yields "".
    static std::string hostOf(const std::string& url)
    {
        if (url.empty()) {
            return std::string();
        }
        ResourceURL* r = new ResourceURL(url.c_str(), url.size());
        String* h = r->hostname();
        return h ? h->toUTF8NonGCString() : std::string();
    }

    // Serialized origin of the routed page ("" if no document / opaque origin).
    static std::string currentOrigin(WebView* wv)
    {
        BrowsingContext* bc = wv ? wv->mainBrowsingContext() : nullptr;
        Document* doc = bc ? bc->document() : nullptr;
        if (!doc) {
            return std::string();
        }
        WebOrigin* o = doc->webOrigin();
        if (!o || o->isOpaque()) {
            return std::string();
        }
        String* s = o->serialize();
        return s ? s->toUTF8NonGCString() : std::string();
    }

    static std::string paramStr(CDPCommand& cmd, const char* name)
    {
        if (cmd.params() && cmd.params()->HasMember(name) &&
            (*cmd.params())[name].IsString()) {
            return (*cmd.params())[name].GetString();
        }
        return std::string();
    }

    // Does storageTypes (CDP comma-separated list, e.g.
    // "cookies,local_storage") request `type`? "all" matches everything.
    static bool wantsType(const std::string& storageTypes, const char* type)
    {
        if (storageTypes.empty()) {
            return false;
        }
        size_t start = 0;
        while (start <= storageTypes.size()) {
            size_t comma = storageTypes.find(',', start);
            size_t end =
                (comma == std::string::npos) ? storageTypes.size() : comma;
            // trim surrounding spaces
            size_t a = start, b = end;
            while (a < b && storageTypes[a] == ' ') {
                ++a;
            }
            while (b > a && storageTypes[b - 1] == ' ') {
                --b;
            }
            std::string tok = storageTypes.substr(a, b - a);
            if (tok == "all" || tok == type) {
                return true;
            }
            if (comma == std::string::npos) {
                break;
            }
            start = comma + 1;
        }
        return false;
    }

    // Clear the routed page's local/session storage area when `origin` names
    // that page's origin. The per-origin area is resolved through the same
    // StorageNamespace path window.localStorage / DOMStorage use, so this
    // clears exactly what page script sees. No-op when the page is a different
    // origin or has no document.
    static void clearStorageForOrigin(WebView* wv, const std::string& origin,
                                      bool local)
    {
        BrowsingContext* bc = wv ? wv->mainBrowsingContext() : nullptr;
        Document* doc = bc ? bc->document() : nullptr;
        if (!doc) {
            return;
        }
        WebOrigin* o = doc->webOrigin();
        if (!o || o->isOpaque()) {
            return;
        }
        String* ser = o->serialize();
        if (!ser || ser->toUTF8NonGCString() != origin) {
            return;
        }
        StorageNamespace* ns =
            local ? wv->localStorageNamespace() : wv->sessionStorageNamespace();
        if (!ns) {
            return;
        }
        StorageInternal* area = ns->storageInternal(o);
        if (area) {
            area->clear();
        }
    }

} // namespace

void StorageDomain::processMessage(CDPCommand& cmd, const std::string& method)
{
    if (method == "getCookies") {
        // Whole jar (browserContextId scoping not modeled: single context).
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value arr(rapidjson::kArrayType);
        NetworkDomain::appendAllCookies(arr, alloc);
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("cookies", arr, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "setCookies") {
        WebView* wv = m_dispatcher->webView();
        std::string curHost = hostOf(currentOrigin(wv));
        if (cmd.params() && cmd.params()->HasMember("cookies")) {
            NetworkDomain::writeCookieArray((*cmd.params())["cookies"], curHost,
                                            std::string());
        }
        cmd.sendResultEmpty();
        return;
    }

    if (method == "clearCookies") {
        NetworkDomain::clearAllCookies();
        cmd.sendResultEmpty();
        return;
    }

    if (method == "clearDataForOrigin") {
        std::string origin = paramStr(cmd, "origin");
        std::string storageTypes = paramStr(cmd, "storageTypes");
        if (origin.empty() || storageTypes.empty()) {
            cmd.sendError(-32602, "'origin' and 'storageTypes' are required");
            return;
        }
        WebView* wv = m_dispatcher->webView();
        if (wantsType(storageTypes, "cookies")) {
            NetworkDomain::clearCookiesForHost(hostOf(origin));
        }
        if (wantsType(storageTypes, "local_storage")) {
            clearStorageForOrigin(wv, origin, /*local=*/true);
            clearStorageForOrigin(wv, origin, /*local=*/false);
        }
        // Other storage types (indexeddb, cache_storage, websql,
        // service_workers, ...) are acked but not backed by this engine.
        cmd.sendResultEmpty();
        return;
    }

    if (method == "getStorageKeyForFrame") {
        // The storage key is the frame's serialized origin. This engine is
        // single-origin per page, so the routed page's document origin is the
        // key for any frameId it owns.
        std::string origin = currentOrigin(m_dispatcher->webView());
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("storageKey",
                         rapidjson::Value(origin.c_str(), origin.size(), alloc),
                         alloc);
        cmd.sendResult(result, doc);
        return;
    }

    if (method == "trackCacheStorageForOrigin" ||
        method == "untrackCacheStorageForOrigin" ||
        method == "trackIndexedDBForOrigin" ||
        method == "untrackIndexedDBForOrigin" ||
        method == "setStorageBucketTracking") {
        cmd.sendResultEmpty();
        return;
    }

    cmd.sendError(-32601, "'method' wasn't found");
}

} // namespace Starfish

#endif
