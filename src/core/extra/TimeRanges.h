/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined(__StarfishTimeRanges__)
#define __StarfishTimeRanges__

#include "binding/ScriptWrappable.h"
#include "core/extra/TimeRange.h"

namespace Starfish {

class TimeRanges : public ScriptWrappable, public GCAtomicVector<TimeRange> {
public:
    TimeRanges(ExecutionContext* executionContext);

    TimeRanges(ExecutionContext* executionContext,
               const GCAtomicVector<TimeRange>& other);

    TimeRanges(ExecutionContext* executionContext,
               GCAtomicVector<TimeRange>&& other);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(TimeRanges)

    double start(uint32_t idx)
    {
        if (idx < size()) {
            return (*this)[idx].start();
        }
        return DBL_MAX;
    }

    double end(uint32_t idx)
    {
        if (idx < size()) {
            return (*this)[idx].end();
        }
        return DBL_MAX;
    }

    uint32_t length()
    {
        return size();
    }

private:
    ScriptBindingInstance* m_scriptBindingInstance;
};
}

#endif
