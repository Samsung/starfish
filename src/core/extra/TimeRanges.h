/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined(__StarFishTimeRanges__)
#define __StarFishTimeRanges__

#include "binding/ScriptWrappable.h"
#include "core/extra/TimeRange.h"
#include "binding/DocumentHoldable.h"

namespace StarFish {

class TimeRanges : public ScriptWrappable,
                   public GCAtomicVector<TimeRange>,
                   public DocumentHoldable {
public:
    TimeRanges(Document* document)
        : ScriptWrappable(this)
        , DocumentHoldable(document)
    {
    }

    TimeRanges(Document* document, const GCAtomicVector<TimeRange>& other)
        : ScriptWrappable(this)
        , GCAtomicVector<TimeRange>(other)
        , DocumentHoldable(document)
    {
    }

    TimeRanges(Document* document, GCAtomicVector<TimeRange>&& other)
        : ScriptWrappable(this)
        , GCAtomicVector<TimeRange>(other)
        , DocumentHoldable(document)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isTimeRanges() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        return DocumentHoldable::scriptBindingInstance();
    }

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
};
}

#endif
