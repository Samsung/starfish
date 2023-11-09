/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#ifdef STARFISH_SERVICE_WORKER_HOST

#include "StarfishConfig.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptBindingServiceWorkerInstance.h"
#include "binding/ScriptWrappable.h"
#include "core/dom/ErrorEvent.h"
#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"
#include <EscargotPublic.h>

namespace Starfish {

using namespace Escargot;

static OptionalRef<ValueRef> virtualIdentifierCallback(ExecutionStateRef* state,
                                                       ValueRef* key)
{
    STARFISH_ASSERT(state != nullptr);
    STARFISH_ASSERT(key != nullptr);
    String* name = toBrowserString(state, key);
    auto self = static_cast<WorkerGlobalScope*>(
        state->context()->globalObject()->extraData());

    if (name->equals("self") == true) {
        return self->scriptValue();
    }

    return OptionalRef<ValueRef>();
}

ScriptBindingServiceWorkerInstance::ScriptBindingServiceWorkerInstance(
    ScriptEngineInstance* engineInstance,
    ServiceWorkerGlobalScope* serviceWorkerGlobalScope)
    : ScriptBindingWorkerInstance<ServiceWorkerGlobalScope>(
          engineInstance, serviceWorkerGlobalScope)
{
}

void ScriptBindingServiceWorkerInstance::initJavaScriptBinding(
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
    STARFISH_ENUM_GLOBAL_BINDING_SERVICEWORKER_NAMES(DECLARE_NAME_FOR_BINDING)
#undef DECLARE_NAME_FOR_BINDING

    fnEventTarget();
    fnWorkerGlobalScope();
    fnServiceWorkerGlobalScope();

    m_ownerWorkerGlobalScope->init(this, m_ownerWorkerGlobalScope);

    context->setVirtualIdentifierCallback(virtualIdentifierCallback);
}

// TODO: Remove mockup function
#if defined(SERVICE_WORKER_USE_SEPARATE_PROCESS) && \
    defined(STARFISH_WEBWORKER_HOST)
#define BINDING_WORKER_MOCKUP_INTERFACE(F) \
    F(CSS)                                 \
    F(EventSource)                         \
    F(FormData)                            \
    F(Option)                              \
    F(Image)                               \
    F(Worker)

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
#endif

} // namespace Starfish

#endif /* STARFISH_WEBWORKER_HOST */
