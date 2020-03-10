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

#ifdef STARFISH_WEBWORKER_HOST

#include "StarfishConfig.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptBindingWorkerInstance.h"
#include "binding/ScriptWrappable.h"
#include "core/modules/worker/host/WorkerGlobalScope.h"
#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"
#include "core/dom/ErrorEvent.h"
#include <EscargotPublic.h>

namespace Starfish {

using namespace Escargot;

template class ScriptBindingWorkerInstance<WorkerGlobalScope>;
template class ScriptBindingWorkerInstance<ServiceWorkerGlobalScope>;

static NullablePtr<ValueRef> virtualIdentifierCallback(ExecutionStateRef* state,
                                                       ValueRef* key)
{
    STARFISH_ASSERT(state != nullptr);
    STARFISH_ASSERT(key != nullptr);
    String* name = toBrowserString(state, key);
    auto self = fetchGlobalObject(state->context());

    if (name->equals("self") == true) {
        return self->scriptValue();
    }

    return ValueRef::createEmpty();
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

    fnEventTarget();
    fnWorkerGlobalScope();
    fnServiceWorkerGlobalScope();

    m_ownerWorkerGlobalScope->init(this, m_ownerWorkerGlobalScope);

    context->setVirtualIdentifierCallback(virtualIdentifierCallback);
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

// TODO: Remove mockup function
#define BINDING_WORKER_MOCKUP_INTERFACE(F) \
    F(CSS)                                 \
    F(CSSKeywordValue)                     \
    F(CSSNumericValue)                     \
    F(CSSStyleValue)                       \
    F(DOMStringList)                       \
    F(EventSource)                         \
    F(FormData)                            \
    F(Option)                              \
    F(Image)

#define FOR_EACH_BINDING_FN(exportName)               \
    Escargot::FunctionObjectRef* binding##exportName( \
        ScriptBindingInstance* scriptBindingInstance) \
    {                                                 \
        STARFISH_ASSERT_NOT_REACHED();                \
        return nullptr;                               \
    }

BINDING_WORKER_MOCKUP_INTERFACE(FOR_EACH_BINDING_FN)
#undef FOR_EACH_BINDING_FN
#undef BINDING_WORKER_MOCKUP_INTERFACE
}

#endif /* STARFISH_WEBWORKER_HOST */
