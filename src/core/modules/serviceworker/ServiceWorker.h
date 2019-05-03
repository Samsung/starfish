/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && \
    !defined(__StarfishServiceWorker__)
#define __StarfishServiceWorker__

#include "core/dom/EventTarget.h"
#include "core/util/Archivable.h"
#include "core/modules/serviceworker/ServiceWorkerData.h"

namespace Starfish {

class ServiceWorker : public EventTarget {
public:
    ServiceWorker(ExecutionContext* executionContext)
        : EventTarget()
        , m_executionContext(executionContext)
        , m_data(new ServiceWorkerData())
    {
        STARFISH_ASSERT(m_data != nullptr);
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isServiceWorker() const override;

    virtual ExecutionContext* executionContext() const override;

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(statechange);
#undef VIRTUAL
#undef OVERRIDE

    DEFINE_GETTER_SETTER(ServiceWorkerData*, data, Data);

    String* scriptURL() const; // binding interface
    String* state() const;     // binding interface

private:
    ExecutionContext* m_executionContext;
    ServiceWorkerData* m_data;
};
} // namespace Starfish

#endif
