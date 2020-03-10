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

#if defined(STARFISH_ENABLE_SERVICE_WORKER)
#ifndef __StarfishNotification__
#define __StarfishNotification__

#include "core/dom/EventTarget.h"
#include "core/modules/serviceworker/notification/NotificationOptions.h"

namespace Starfish {

using NotificationPermissionCallback =
    void (*)(NotificationPermission permission);

class NotificationJob;

class Notification : public EventTarget {
public:
    Notification(ExecutionContext* executionContext, String* title,
                 NotificationOptions options = NotificationOptions());

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isNotification() const override;

    virtual ExecutionContext* executionContext() const override;

    String* title();
    String* body();
    String* tag();

private:
    ExecutionContext* m_executionContext;
    NotificationJob* m_job;
};
}

#endif
#endif /* STARFISH_ENABLE_SERVICE_WORKER */
