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
#include "RemoteObject.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptWrappable.h"
#include "core/dom/Node.h"
#include "core/util/String.h"

#include "EscargotPublic.h"

#include "rapidjson/document.h"
#include <cmath>
#include <string>

namespace Starfish {

using namespace Escargot;

RemoteObjectStore::RemoteObjectStore()
    : m_next(1)
{
}

int RemoteObjectStore::store(Escargot::ObjectRef* obj)
{
    int id = m_next++;
    m_idToObj[id] = obj;
    return id;
}

Escargot::ObjectRef* RemoteObjectStore::lookup(int id)
{
    auto it = m_idToObj.find(id);
    if (it != m_idToObj.end()) {
        return it->second;
    }
    return nullptr;
}

void RemoteObjectStore::release(int id)
{
    m_idToObj.erase(id);
}

void RemoteObjectStore::reset()
{
    m_idToObj.clear();
    m_next = 1;
}

static void addStr(rapidjson::Value& obj, const char* key, const std::string& v,
                   rapidjson::Document::AllocatorType& alloc)
{
    obj.AddMember(rapidjson::Value(key, alloc),
                  rapidjson::Value(v.c_str(), v.size(), alloc), alloc);
}

// returnByValue serialization of an object: JSON.stringify it in the page
// context and embed the parsed result under `value`. Returns false if the
// value is not JSON-serializable (e.g. functions, cycles), leaving the caller
// to fall back to the objectId handle representation.
static bool serializeByValue(ContextRef* ctx, Escargot::ValueRef* value,
                             rapidjson::Value& out,
                             rapidjson::Document::AllocatorType& alloc)
{
    Evaluator::EvaluatorResult r = Evaluator::execute(
        ctx,
        [](ExecutionStateRef* state, ValueRef* v) -> ValueRef* {
            FunctionObjectRef* stringify =
                state->context()->globalObject()->jsonStringify();
            ValueRef* argv[1] = { v };
            return stringify->call(state, ValueRef::createUndefined(), 1, argv);
        },
        value);
    if (r.error.hasValue() || !r.result->isString()) {
        return false;
    }
    std::string json = r.result->asString()->toStdUTF8String();
    rapidjson::Document parsed;
    if (parsed.Parse(json.c_str()).HasParseError()) {
        return false;
    }
    rapidjson::Value v;
    v.CopyFrom(parsed, alloc);
    out.AddMember("value", v, alloc);
    return true;
}

void serializeRemoteObject(ScriptBindingInstance* sbi, RemoteObjectStore* store,
                           Escargot::ValueRef* value, bool returnByValue,
                           rapidjson::Value& out,
                           rapidjson::Document::AllocatorType& alloc)
{
    out.SetObject();
    ContextRef* ctx = sbi->scriptContext();

    if (value->isUndefined()) {
        addStr(out, "type", "undefined", alloc);
        addStr(out, "description", "undefined", alloc);
        return;
    }
    if (value->isNull()) {
        addStr(out, "type", "object", alloc);
        addStr(out, "subtype", "null", alloc);
        out.AddMember("value", rapidjson::Value(rapidjson::kNullType), alloc);
        return;
    }
    if (value->isBoolean()) {
        addStr(out, "type", "boolean", alloc);
        out.AddMember("value", value->asBoolean(), alloc);
        return;
    }
    if (value->isNumber()) {
        addStr(out, "type", "number", alloc);
        double n = value->asNumber();
        if (std::isnan(n)) {
            addStr(out, "description", "NaN", alloc);
        } else if (std::isinf(n)) {
            addStr(out, "description", n < 0 ? "-Infinity" : "Infinity", alloc);
        } else {
            out.AddMember("value", n, alloc);
        }
        return;
    }
    if (value->isString()) {
        std::string s = value->asString()->toStdUTF8String();
        addStr(out, "type", "string", alloc);
        addStr(out, "value", s, alloc);
        return;
    }
    if (value->isSymbol()) {
        addStr(out, "type", "symbol", alloc);
        addStr(out, "description",
               value->toStringWithoutException(ctx)->toStdUTF8String(), alloc);
        return;
    }
    if (value->isBigInt()) {
        addStr(out, "type", "bigint", alloc);
        addStr(out, "description",
               value->toStringWithoutException(ctx)->toStdUTF8String(), alloc);
        return;
    }

    // From here on: object-like.
    std::string desc = value->toStringWithoutException(ctx)->toStdUTF8String();

    // DOM nodes: emit subtype "node" so puppeteer wraps the handle as an
    // ElementHandle (enabling page.$ / handle.$ / $eval). Always handle-based;
    // nodes are not value-serialized.
    if (value->isObject() && value->asObject()->extraData()) {
        ScriptWrappable* w = (ScriptWrappable*)value->asObject()->extraData();
        if (w->isNode()) {
            Node* node = w->asNode();
            addStr(out, "type", "object", alloc);
            addStr(out, "subtype", "node", alloc);
            std::string nodeName = node->nodeName()->toUTF8NonGCString();
            addStr(out, "className", nodeName, alloc);
            addStr(out, "description", nodeName, alloc);
            int id = store->store(value->asObject());
            addStr(out, "objectId", "OBJ-" + std::to_string(id), alloc);
            return;
        }
    }

    if (value->isCallable() || value->isFunctionObject()) {
        addStr(out, "type", "function", alloc);
        addStr(out, "className", "Function", alloc);
        addStr(out, "description", desc, alloc);
        if (value->isObject()) {
            int id = store->store(value->asObject());
            addStr(out, "objectId", "OBJ-" + std::to_string(id), alloc);
        }
        return;
    }

    addStr(out, "type", "object", alloc);
    if (value->isArrayObject()) {
        addStr(out, "subtype", "array", alloc);
        addStr(out, "className", "Array", alloc);
    } else if (value->isErrorObject()) {
        addStr(out, "subtype", "error", alloc);
        addStr(out, "className", "Error", alloc);
    } else {
        addStr(out, "className", "Object", alloc);
    }
    addStr(out, "description", desc, alloc);

    // returnByValue for objects: deep-serialize via JSON.stringify and embed
    // the value (puppeteer's jsonValue()/returnByValue path). If serialization
    // fails (functions, cycles, non-JSON values), fall back to a handle.
    if (returnByValue && value->isObject()) {
        if (serializeByValue(ctx, value, out, alloc)) {
            return;
        }
    }

    if (value->isObject()) {
        int id = store->store(value->asObject());
        addStr(out, "objectId", "OBJ-" + std::to_string(id), alloc);
    }
}

} // namespace Starfish

#endif
