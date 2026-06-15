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
#include "DOMStorageDomain.h"
#include "../CDPDispatcher.h"
#include "../CDPCommand.h"
#include "core/page/WebView.h"
#include "core/page/BrowsingContext.h"
#include "core/dom/Document.h"
#include "core/dom/WebOrigin.h"
#include "core/storage/StorageNamespace.h"
#include "core/storage/StorageInternal.h"

#include "rapidjson/document.h"
#include <cstring>

namespace Starfish {

static String* toStarfishString(const std::string& s)
{
    return String::fromUTF8(s.data(), s.size());
}

static std::string fromStarfishString(String* s)
{
    return s ? s->toUTF8NonGCString() : std::string();
}

// storageId.isLocalStorage selects local vs session; the area is keyed by the
// routed page's WebOrigin so it is the very StorageInternal window.localStorage
// / window.sessionStorage operate on (StorageNamespaceImpl maps WebOrigin via
// isSameOrigin). storageId.securityOrigin is not used to reconstruct an origin:
// the page is single-origin and same-origin checks against the document origin,
// so using the document origin guarantees the shared area and avoids opaque /
// reconstruction pitfalls.
StorageInternal* DOMStorageDomain::resolveArea(CDPCommand& cmd)
{
    rapidjson::Value* params = cmd.params();
    bool isLocal = true;
    if (params && params->HasMember("storageId") &&
        (*params)["storageId"].IsObject()) {
        const rapidjson::Value& sid = (*params)["storageId"];
        if (sid.HasMember("isLocalStorage") && sid["isLocalStorage"].IsBool()) {
            isLocal = sid["isLocalStorage"].GetBool();
        }
    }

    WebView* wv = m_dispatcher->webView();
    BrowsingContext* bc = wv ? wv->mainBrowsingContext() : nullptr;
    Document* doc = bc ? bc->document() : nullptr;
    if (!doc) {
        cmd.sendError(-32000, "No document for target");
        return nullptr;
    }
    WebOrigin* origin = doc->webOrigin();
    if (!origin || origin->isOpaque()) {
        cmd.sendError(-32000,
                      "Frame has opaque origin; DOM storage unavailable");
        return nullptr;
    }

    StorageNamespace* ns =
        isLocal ? wv->localStorageNamespace() : wv->sessionStorageNamespace();
    if (!ns) {
        cmd.sendError(-32000, "Storage namespace unavailable");
        return nullptr;
    }
    return ns->storageInternal(origin);
}

void DOMStorageDomain::processMessage(CDPCommand& cmd,
                                      const std::string& method)
{
    if (method == "enable" || method == "disable") {
        cmd.sendResultEmpty();
        return;
    }

    if (method == "getDOMStorageItems") {
        StorageInternal* area = resolveArea(cmd);
        if (!area) {
            return; // resolveArea already sent the error
        }
        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value entries(rapidjson::kArrayType);

        GCVector<String*> keys = area->getKeyNames();
        for (size_t i = 0; i < keys.size(); i++) {
            String* k = keys[i];
            Optional<String*> v = area->getItem(k);
            std::string ks = fromStarfishString(k);
            std::string vs =
                v.hasValue() ? fromStarfishString(v.getValue()) : std::string();
            rapidjson::Value pair(rapidjson::kArrayType);
            pair.PushBack(rapidjson::Value(ks.c_str(), ks.size(), alloc),
                          alloc);
            pair.PushBack(rapidjson::Value(vs.c_str(), vs.size(), alloc),
                          alloc);
            entries.PushBack(pair, alloc);
        }
        result.AddMember("entries", entries, alloc);
        cmd.sendResult(result, out);
        return;
    }

    if (method == "setDOMStorageItem") {
        rapidjson::Value* params = cmd.params();
        if (!params || !params->HasMember("key") ||
            !(*params)["key"].IsString() || !params->HasMember("value") ||
            !(*params)["value"].IsString()) {
            cmd.sendError(-32602, "'key' and 'value' are required");
            return;
        }
        StorageInternal* area = resolveArea(cmd);
        if (!area) {
            return;
        }
        area->setItem(toStarfishString((*params)["key"].GetString()),
                      toStarfishString((*params)["value"].GetString()));
        cmd.sendResultEmpty();
        return;
    }

    if (method == "removeDOMStorageItem") {
        rapidjson::Value* params = cmd.params();
        if (!params || !params->HasMember("key") ||
            !(*params)["key"].IsString()) {
            cmd.sendError(-32602, "'key' is required");
            return;
        }
        StorageInternal* area = resolveArea(cmd);
        if (!area) {
            return;
        }
        area->removeItem(toStarfishString((*params)["key"].GetString()));
        cmd.sendResultEmpty();
        return;
    }

    if (method == "clear") {
        StorageInternal* area = resolveArea(cmd);
        if (!area) {
            return;
        }
        area->clear();
        cmd.sendResultEmpty();
        return;
    }

    cmd.sendError(-32601, "'method' wasn't found");
}

} // namespace Starfish

#endif
