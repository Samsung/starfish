/*
 * Copyright (c) 2020-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishPerformance__
#define __StarfishPerformance__

#include "core/dom/EventTarget.h"
#include "core/extra/PerformanceResourceTiming.h"
#include "PerformanceEntry.h"

namespace Starfish {

class Performance : public EventTarget {
    Performance(ExecutionContext* executionContext);

public:
    double now();
    double timeOrigin();
    PerformanceResourceTiming* timing()
    {
        return m_performanceResourceTiming;
    }

    GCVector<PerformanceEntry*> getEntriesByType()
    {
        // TODO
        STARFISH_UNIMPLEMENTED(
            "Performance.getEntriesByType() is enabled but not yet "
            "implemented.");
        GCVector<PerformanceEntry*> entries;
        return entries;
    }

    static Performance* create(ExecutionContext* executionContext)
    {
        return new Performance(executionContext);
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isPerformance() const override;
    virtual ExecutionContext* executionContext() const override
    {
        return m_executionContext;
    }

private:
    ExecutionContext* m_executionContext;
    PerformanceResourceTiming* m_performanceResourceTiming;
};
} // namespace Starfish

#endif
