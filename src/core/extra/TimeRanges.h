/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined(__StarFishTimeRanges__)
#define __StarFishTimeRanges__

#include "binding/ScriptWrappable.h"
#include "core/extra/TimeRange.h"
#include "binding/DocumentHoldable.h"

namespace StarFish {

class TimeRanges : public ScriptWrappable,
                   public GCVector<TimeRange>,
                   public DocumentHoldable {
public:
    TimeRanges(Document* document)
        : ScriptWrappable(this)
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
