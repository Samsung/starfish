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
#include "DOMDebuggerDomain.h"
#include "../CDPDispatcher.h"
#include "../CDPCommand.h"
#include "../RemoteObject.h"
#include "core/page/WebView.h"
#include "core/page/BrowsingContext.h"
#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"
#include "binding/ScriptBindingInstance.h"
#include "EscargotPublic.h"

#include "rapidjson/document.h"
#include <cstring>

namespace Starfish {

// Resolve a Runtime objectId ("OBJ-<n>") to the EventTarget it handles, or
// nullptr if the handle is missing or is not an EventTarget. Window/Document/
// Element all derive from EventTarget, so node/window/document objectIds work.
static EventTarget* eventTargetFromObjectId(CDPDispatcher* disp,
                                            const char* oid)
{
    if (!oid || strncmp(oid, "OBJ-", 4) != 0) {
        return nullptr;
    }
    int id = atoi(oid + 4);
    Escargot::ObjectRef* obj = disp->remoteObjectStore()->lookup(id);
    if (!obj || !obj->extraData()) {
        return nullptr;
    }
    ScriptWrappable* w = (ScriptWrappable*)obj->extraData();
    return w->isEventTarget() ? w->asEventTarget() : nullptr;
}

void DOMDebuggerDomain::processMessage(CDPCommand& cmd,
                                       const std::string& method)
{
    if (method == "getEventListeners") {
        const char* oid = nullptr;
        if (cmd.params() && cmd.params()->HasMember("objectId") &&
            (*cmd.params())["objectId"].IsString()) {
            oid = (*cmd.params())["objectId"].GetString();
        }
        EventTarget* target = eventTargetFromObjectId(m_dispatcher, oid);

        rapidjson::Document out;
        rapidjson::Document::AllocatorType& alloc = out.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        rapidjson::Value listeners(rapidjson::kArrayType);

        if (target) {
            WebView* wv = m_dispatcher->webView();
            BrowsingContext* bc = wv ? wv->mainBrowsingContext() : nullptr;
            ScriptBindingInstance* sbi =
                bc ? bc->scriptBindingInstance() : nullptr;
            bool scripting = sbi && sbi->isScriptingEnabled();

            for (const auto& entry : target->cdpEventListeners()) {
                String* type = entry.first;
                GCVector<EventListener*>* list = entry.second;
                if (!list) {
                    continue;
                }
                std::string typeStr = type->toUTF8NonGCString();
                for (size_t i = 0; i < list->size(); i++) {
                    EventListener* l = list->at(i);
                    if (!l) {
                        continue;
                    }
                    rapidjson::Value entryObj(rapidjson::kObjectType);
                    entryObj.AddMember("type",
                                       rapidjson::Value(typeStr.c_str(),
                                                        typeStr.size(), alloc),
                                       alloc);
                    entryObj.AddMember("useCapture", l->capture(), alloc);
                    // starfish does not track the passive/once listener
                    // options; report false (CDP fields are required booleans).
                    entryObj.AddMember("passive", false, alloc);
                    entryObj.AddMember("once", false, alloc);
                    // handler: the listener callback as a function
                    // RemoteObject.
                    if (scripting) {
                        Escargot::ValueRef* fn = l->scriptValue();
                        if (fn &&
                            (fn->isCallable() || fn->isFunctionObject())) {
                            rapidjson::Value handler(rapidjson::kObjectType);
                            serializeRemoteObject(
                                sbi, m_dispatcher->remoteObjectStore(), fn,
                                false, handler, alloc);
                            entryObj.AddMember("handler", handler, alloc);
                        }
                    }
                    listeners.PushBack(entryObj, alloc);
                }
            }
        }

        result.AddMember("listeners", listeners, alloc);
        cmd.sendResult(result, out);
        return;
    }

    // Breakpoint-family methods: the MVP has no debugger attached, so these are
    // acked to keep DevTools/Puppeteer handshakes from throwing. No breakpoint
    // behavior is wired.
    //   setDOMBreakpoint / removeDOMBreakpoint
    //   setEventListenerBreakpoint / removeEventListenerBreakpoint
    //   setInstrumentationBreakpoint / removeInstrumentationBreakpoint
    //   setXHRBreakpoint / removeXHRBreakpoint
    cmd.sendResultEmpty();
}

} // namespace Starfish

#endif
