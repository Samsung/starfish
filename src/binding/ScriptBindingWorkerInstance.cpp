/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifdef STARFISH_ENABLE_WORKER

#include "StarfishConfig.h"
#include "binding/ScriptBindingWorkerInstance.h"
#include "binding/ScriptWrappable.h"
#include "core/modules/worker/host/DedicatedWorkerGlobalScope.h"
#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"
#include "core/dom/ErrorEvent.h"
#include <EscargotPublic.h>

namespace Starfish {

using namespace Escargot;

template class ScriptBindingWorkerInstance<DedicatedWorkerGlobalScope>;
#if defined(STARFISH_SERVICE_WORKER_HOST)
template class ScriptBindingWorkerInstance<ServiceWorkerGlobalScope>;
#endif

template <typename T>
static OptionalRef<ValueRef> virtualIdentifierCallback(ExecutionStateRef* state,
                                                       ValueRef* key)
{
    STARFISH_ASSERT(state != nullptr);
    STARFISH_ASSERT(key != nullptr);
    String* name = toBrowserString(state, key);
    auto self = static_cast<T*>(state->context()->globalObject()->extraData());

    if (name->equals("self") == true) {
        return self->scriptValue();
    }

    return OptionalRef<ValueRef>();
}

template <typename T>
ScriptBindingWorkerInstance<T>::ScriptBindingWorkerInstance(
    ScriptEngineInstance* engineInstance, T* workerGlobalScope)
    : ScriptBindingInstance(engineInstance)
    , m_ownerWorkerGlobalScope(workerGlobalScope)
{
    STARFISH_ASSERT(engineInstance != nullptr);
    STARFISH_ASSERT(workerGlobalScope != nullptr);
}

template <typename T>
void ScriptBindingWorkerInstance<T>::initJavaScriptBinding(
    Escargot::ContextRef* context, Escargot::ExecutionStateRef* state)
{
    STARFISH_ASSERT(context != nullptr);
    STARFISH_ASSERT(state != nullptr);
    ScriptBindingInstance::initJavaScriptBinding(context, state);

    GlobalObjectRef* globalObject = context->globalObject();
#define DECLARE_NAME_FOR_BINDING(exportName)                          \
    defineGlobalBindingNameAccessor(                                  \
        state, globalObject, StringRef::createFromASCII(#exportName), \
        std::mem_fn(&ScriptBindingInstance::value##exportName),       \
        std::mem_fn(&ScriptBindingInstance::setValue##exportName));
    STARFISH_ENUM_GLOBAL_BINDING_WORKER_NAMES(DECLARE_NAME_FOR_BINDING)
#undef DECLARE_NAME_FOR_BINDING

    fnEventTarget();
    fnWorkerGlobalScope();
    fnDedicatedWorkerGlobalScope();

    m_ownerWorkerGlobalScope->init(this, m_ownerWorkerGlobalScope);

    context->setVirtualIdentifierCallback(virtualIdentifierCallback<T>);
}

template <typename T>
void ScriptBindingWorkerInstance<T>::destroy()
{
    ScriptBindingInstance::destroy();
}

template <typename T>
Window* ScriptBindingWorkerInstance<T>::ownerWindow()
{
    STARFISH_ASSERT_NOT_REACHED();
    return nullptr;
}

template <typename T>
Document* ScriptBindingWorkerInstance<T>::ownerDocument()
{
    STARFISH_ASSERT_NOT_REACHED();
    return nullptr;
}

template <typename T>
void ScriptBindingWorkerInstance<T>::dispatchErrorEventToGlobalScope(
    ErrorEventInit& errorInfo)
{
    m_ownerWorkerGlobalScope->dispatchErrorEvent(errorInfo);
}

} // namespace Starfish

#endif /* STARFISH_WEBWORKER_HOST */
