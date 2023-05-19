/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_WEBWORKER_HOST) && !defined(__ExtendableEvent__)
#define __ExtendableEvent__

#include "core/dom/Event.h"

namespace Starfish {

class ServiceWorkerRegistrationData;
class ServiceWorkerData;

struct ExtendableEventInit : EventInit {
    STARFISH_MAKE_STACK_ALLOCATED();

public:
    ExtendableEventInit()
        : EventInit()
    {
    }
};

class ExtendableEvent : public Event {
public:
    ExtendableEvent(ExecutionContext* executionContext, String* eventType)
        : Event(executionContext, eventType)
    {
    }

    ExtendableEvent(ExecutionContext* executionContext, String* eventType,
                    const ExtendableEventInit& init)
        : Event(executionContext, eventType, init)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isExtendableEvent() const override;

    void waitUntil(Promise* promise);

    void enqueueWaitUntilMicrotask();

    void incrementPendingPromiseCount();
    void decrementPendingPromiseCount();
    unsigned int pendingPromisesCount();

protected:
    unsigned int m_pendingPromisesCount{ 0 };
    GCVector<Promise*> m_extendLifetimePromises;

private:
    bool isEventActive() const;
};

} // namespace Starfish
#endif
