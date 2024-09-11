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
#include "core/modules/worker/DedicatedWorkerGlobalScope.h"
#include "core/modules/sharedworker/host/SharedWorkerGlobalScope.h"
#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"
#include "core/dom/ErrorEvent.h"
#include <EscargotPublic.h>

namespace Starfish {

using namespace Escargot;

template class ScriptBindingWorkerInstance<DedicatedWorkerGlobalScope>;

#if defined(STARFISH_WEBWORKER_HOST)
#if defined(STARFISH_ENABLE_SHARED_WORKER)
template class ScriptBindingWorkerInstance<SharedWorkerGlobalScope>;
#endif
#if defined(STARFISH_ENABLE_SERVICE_WORKER)
template class ScriptBindingWorkerInstance<ServiceWorkerGlobalScope>;
#endif
#endif

#define DECLARE_WORKER_NAME_FOR_BINDING(exportName)                   \
    scriptBindingInstance->defineGlobalBindingNameAccessor(           \
        state, globalObject, StringRef::createFromASCII(#exportName), \
        std::mem_fn(&ScriptBindingInstance::value##exportName),       \
        std::mem_fn(&ScriptBindingInstance::setValue##exportName));

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

void DedicatedWorkerGlobalScope::initJavaScriptGlobalBinding(
    ScriptExecutionState state, ScriptBindingInstance* scriptBindingInstance)
{
    GlobalObjectRef* globalObject = state->context()->globalObject();
    STARFISH_ENUM_GLOBAL_BINDING_DEDICATEDWORKER_NAMES(
        DECLARE_WORKER_NAME_FOR_BINDING);

    scriptBindingInstance->fnDedicatedWorkerGlobalScope();
}

#if defined(STARFISH_WEBWORKER_HOST)
#if defined(STARFISH_ENABLE_SHARED_WORKER)
void SharedWorkerGlobalScope::initJavaScriptGlobalBinding(
    ScriptExecutionState state, ScriptBindingInstance* scriptBindingInstance)
{
    GlobalObjectRef* globalObject = state->context()->globalObject();
    STARFISH_ENUM_GLOBAL_BINDING_SHAREDWORKER_NAMES(
        DECLARE_WORKER_NAME_FOR_BINDING);

    scriptBindingInstance->fnSharedWorkerGlobalScope();
}
#endif
#if defined(STARFISH_ENABLE_SERVICE_WORKER)
void ServiceWorkerGlobalScope::initJavaScriptGlobalBinding(
    ScriptExecutionState state, ScriptBindingInstance* scriptBindingInstance)
{
    GlobalObjectRef* globalObject = state->context()->globalObject();
    STARFISH_ENUM_GLOBAL_BINDING_SERVICEWORKER_NAMES(
        DECLARE_WORKER_NAME_FOR_BINDING);

    scriptBindingInstance->fnServiceWorkerGlobalScope();
}
#endif
#endif

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
    m_ownerWorkerGlobalScope->initJavaScriptGlobalBinding(state, this);

    fnEventTarget();
    fnWorkerGlobalScope();

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

#if defined(STARFISH_WEBWORKER_HOST)
// TODO: Remove mockup function
#define FOR_EACH_MOCKUP_INTERFACE(F) \
    F(CSS)                           \
    F(EventSource)                   \
    F(FormData)                      \
    F(Option)                        \
    F(Image)                         \
    F(Worker)

#define DEFINE_BINDING_FN(exportName)                 \
    Escargot::FunctionObjectRef* binding##exportName( \
        ScriptBindingInstance* scriptBindingInstance) \
    {                                                 \
        STARFISH_ASSERT_NOT_REACHED();                \
        return nullptr;                               \
    }

FOR_EACH_MOCKUP_INTERFACE(DEFINE_BINDING_FN)
#undef DEFINE_BINDING_FN
#undef FOR_EACH_MOCKUP_INTERFACE
#endif

#undef DECLARE_WORKER_NAME_FOR_BINDING

} // namespace Starfish

#endif /* STARFISH_WEBWORKER_HOST */
