/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "ObservableArray.h"

#include <EscargotPublic.h>

using namespace Escargot;

namespace Starfish {

// Carried as the proxy handler object's extraData. GC-allocated; the handler's
// extraData slot keeps it reachable (conservatively scanned, same as how
// ScriptWrappable stores its native pointer). `host` is kept alive by it.
struct ObservableArrayBackend : public gc {
    ObservableArrayBackend(ScriptWrappable* host,
                           const ObservableArrayCallbacks* callbacks)
        : m_host(host)
        , m_callbacks(callbacks)
    {
    }

    ScriptWrappable* m_host;
    const ObservableArrayCallbacks* m_callbacks;
};

static ObservableArrayBackend* backendOf(ValueRef* handlerThis)
{
    return (ObservableArrayBackend*)handlerThis->asObject()->extraData();
}

static bool isLengthKey(ExecutionStateRef* state, ValueRef* key)
{
    return key->isString() &&
           key->asString()->equalsWithASCIIString("length", 6);
}

// Re-seed the proxy's backing Array object so it mirrors the host's native
// backing list element-for-element.
static void seedTarget(ExecutionStateRef* state, ObjectRef* target,
                       ObservableArrayBackend* backend)
{
    uint32_t length = backend->m_callbacks->length(backend->m_host);
    target->setLength(state, length);
    for (uint32_t i = 0; i < length; i++) {
        target->setIndexedProperty(
            state, ValueRef::create(i),
            backend->m_callbacks->getIndexedValue(state, backend->m_host, i));
    }
}

// handler.set(target, P, V, Receiver)
static ValueRef* setTrap(ExecutionStateRef* state, ValueRef* thisValue,
                         size_t argc, ValueRef** argv, bool isNewExpression)
{
    ObservableArrayBackend* backend = backendOf(thisValue);
    ObjectRef* target = argv[0]->asObject();
    ValueRef* key = argv[1];
    ValueRef* value = argv[2];

    uint32_t index = key->tryToUseAsIndexProperty(state);
    if (index != ValueRef::InvalidIndexPropertyValue) {
        if (!backend->m_callbacks->setIndexedValue(state, backend->m_host,
                                                   index, value)) {
            return ValueRef::create(false);
        }
        target->setIndexedProperty(state, key, value);
        return ValueRef::create(true);
    }

    if (isLengthKey(state, key)) {
        uint32_t newLength = value->toUint32(state);
        if (!backend->m_callbacks->setLength(state, backend->m_host,
                                             newLength)) {
            return ValueRef::create(false);
        }
        target->setLength(state, newLength);
        return ValueRef::create(true);
    }

    return ValueRef::create(target->set(state, key, value));
}

// handler.defineProperty(target, P, Descriptor)
static ValueRef* definePropertyTrap(ExecutionStateRef* state,
                                    ValueRef* thisValue, size_t argc,
                                    ValueRef** argv, bool isNewExpression)
{
    ObservableArrayBackend* backend = backendOf(thisValue);
    ObjectRef* target = argv[0]->asObject();
    ValueRef* key = argv[1];
    ObjectRef* descriptor = argv[2]->asObject();
    StringRef* valueString = StringRef::createFromASCII("value");

    uint32_t index = key->tryToUseAsIndexProperty(state);
    if (index != ValueRef::InvalidIndexPropertyValue) {
        // Observable arrays only accept data descriptors carrying a value.
        if (!descriptor->has(state, valueString)) {
            return ValueRef::create(false);
        }
        ValueRef* value = descriptor->get(state, valueString);
        if (!backend->m_callbacks->setIndexedValue(state, backend->m_host,
                                                   index, value)) {
            return ValueRef::create(false);
        }
        target->defineOwnProperty(state, key, argv[2]);
        return ValueRef::create(true);
    }

    if (isLengthKey(state, key) && descriptor->has(state, valueString)) {
        uint32_t newLength =
            descriptor->get(state, valueString)->toUint32(state);
        if (!backend->m_callbacks->setLength(state, backend->m_host,
                                             newLength)) {
            return ValueRef::create(false);
        }
        target->defineOwnProperty(state, key, argv[2]);
        return ValueRef::create(true);
    }

    return ValueRef::create(target->defineOwnProperty(state, key, argv[2]));
}

// handler.deleteProperty(target, P)
static ValueRef* deletePropertyTrap(ExecutionStateRef* state,
                                    ValueRef* thisValue, size_t argc,
                                    ValueRef** argv, bool isNewExpression)
{
    ObservableArrayBackend* backend = backendOf(thisValue);
    ObjectRef* target = argv[0]->asObject();
    ValueRef* key = argv[1];

    uint32_t index = key->tryToUseAsIndexProperty(state);
    if (index != ValueRef::InvalidIndexPropertyValue) {
        if (!backend->m_callbacks->deleteIndexedValue(state, backend->m_host,
                                                      index)) {
            return ValueRef::create(false);
        }
        target->deleteOwnProperty(state, key);
        return ValueRef::create(true);
    }

    return ValueRef::create(target->deleteOwnProperty(state, key));
}

static void defineTrap(ExecutionStateRef* state, ObjectRef* handler,
                       const char* name, size_t nameLength,
                       Escargot::ScriptNativeFunctionPointer trap, size_t argc)
{
    FunctionObjectRef* fn = FunctionObjectRef::create(
        state, FunctionObjectRef::NativeFunctionInfo(
                   AtomicStringRef::create(state->context(), name, nameLength),
                   trap, argc, true /* strict */, false /* isConstructor */));
    handler->defineDataProperty(
        state, StringRef::createFromASCII(name, nameLength), fn,
        false /* writable */, false /* enumerable */, true /* configurable */);
}

Escargot::ProxyObjectRef* ObservableArray::create(
    ExecutionStateRef* state, ScriptWrappable* host,
    const ObservableArrayCallbacks* callbacks)
{
    ArrayObjectRef* target = ArrayObjectRef::create(state);
    ObservableArrayBackend* backend =
        new ObservableArrayBackend(host, callbacks);

    ObjectRef* handler = ObjectRef::create(state);
    handler->setExtraData(backend);
    defineTrap(state, handler, "set", 3, setTrap, 4);
    defineTrap(state, handler, "defineProperty", 14, definePropertyTrap, 3);
    defineTrap(state, handler, "deleteProperty", 14, deletePropertyTrap, 2);

    seedTarget(state, target, backend);

    return ProxyObjectRef::create(state, target, handler);
}

void ObservableArray::syncFromHost(ExecutionStateRef* state,
                                   Escargot::ProxyObjectRef* proxy)
{
    ObservableArrayBackend* backend =
        (ObservableArrayBackend*)proxy->handler()->extraData();
    seedTarget(state, proxy->target(), backend);
}

} // namespace Starfish
