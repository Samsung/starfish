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

#if defined(STARFISH_WEBWORKER_HOST) && \
    !defined(__StarfishServiceWorkerGlobalScope__)
#define __StarfishServiceWorkerGlobalScope__

#include "core/modules/worker/host/WorkerGlobalScope.h"

namespace Starfish {

class WebWorker;
class ServiceWorker;
class ServiceWorkerData;
class ResourceURL;
class ErrorEventInit;

class ServiceWorkerGlobalScope : public WorkerGlobalScope {
public:
    ServiceWorkerGlobalScope(WebWorker* webWorker, ResourceURL* url,
                             String* charSet);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isServiceWorkerGlobalScope() const override;

    void handleFetch(RequestData* data);

    // bindings
    ServiceWorker* serviceWorker();
    Promise* skipWaiting();

    void setServiceWorkerData(ServiceWorkerData* serviceWorker);
    ServiceWorkerData* serviceWorkerData()
    {
        return m_serviceWorker;
    }

private:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        WorkerGlobalScope::fillGCDescriptor(desc);
    }
    ServiceWorkerData* m_serviceWorker{ nullptr };
};
} // namespace Starfish

#endif
