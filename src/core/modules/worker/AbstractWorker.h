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

#if defined(STARFISH_ENABLE_WORKER) && !defined(__StarfishAbstractWorker__)
#define __StarfishAbstractWorker__

#include "core/dom/EventTarget.h"
#include "core/modules/worker/WorkerOptions.h"

namespace Starfish {

class AbstractWorker : public EventTarget {
public:
    AbstractWorker(ExecutionContext* executionContext, String* scriptURL);
    AbstractWorker(ExecutionContext* executionContext, String* scriptURL,
                   const WorkerOptions& options);

    ResourceURL* resolveURL(String* scriptURL);

    const WorkerOptions& workerOptions() const
    {
        return m_options;
    }

    DEFINE_GETTER(ResourceURL*, scriptURL);

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(error);
#undef VIRTUAL
#undef OVERRIDE

protected:
    ExecutionContext* m_executionContext;
    ResourceURL* m_scriptURL;
    WorkerOptions m_options;

private:
};

} // namespace Starfish
#endif
