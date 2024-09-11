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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && defined(STARFISH_WEBWORKER_HOST)
#ifndef __StarfishServiceWorkerGlobalScope__
#define __StarfishServiceWorkerGlobalScope__

#include "core/modules/worker/WorkerGlobalScope.h"

namespace Starfish {

class WebWorker;
class ServiceWorker;
class ServiceWorkerData;
class ServiceWorkerScriptController;
class ResourceURL;
class ErrorEventInit;
class StorageNamespaceProvider;
class StorageNamespace;
class CustomStorage;
class Internal;

class ServiceWorkerGlobalScope final : public WorkerGlobalScope {
public:
    ServiceWorkerGlobalScope(WebWorker* webWorker, ResourceURL* url,
                             String* charSet);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isServiceWorkerGlobalScope() const override;

    void initJavaScriptGlobalBinding(
        ScriptExecutionState state,
        ScriptBindingInstance* scriptBindingInstance) override;

    // bindings
    ServiceWorker* serviceWorker();
    Promise* skipWaiting();

    void setServiceWorkerData(ServiceWorkerData* serviceWorker);
    ServiceWorkerData* serviceWorkerData()
    {
        return m_serviceWorker;
    }

    ServiceWorkerScriptController* serviceWorkerScriptController();

    CustomStorage* workerStorage();
    void initCacheStorage();
    Internal* internal();

private:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        WorkerGlobalScope::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(ServiceWorkerGlobalScope,
                                        m_storageNamespaceProvider));
        GC_set_bit(desc, GC_WORD_OFFSET(ServiceWorkerGlobalScope,
                                        m_localStorageNamespace));
    }
    ServiceWorkerData* m_serviceWorker{ nullptr };
    StorageNamespaceProvider* m_storageNamespaceProvider{ nullptr };
    StorageNamespace* m_localStorageNamespace{ nullptr };
    Internal* m_internal{ nullptr };
};
} // namespace Starfish

#endif
#endif
