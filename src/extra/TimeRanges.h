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

#include "dom/EventTarget.h"
#include "extra/TimeRange.h"

namespace StarFish {

class TimeRanges : public ScriptWrappable {
public:
    TimeRanges()
        : ScriptWrappable(this)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual bool isTimeRanges() const
    {
        return true;
    }

    double start(unsigned long idx)
    {
        if (idx < m_list.size()) {
            return m_list[idx].start();
        }
        return DBL_MAX;
    }

    double end(unsigned long idx)
    {
        if (idx < m_list.size()) {
            return m_list[idx].end();
        }
        return DBL_MAX;
    }

    void push_back(TimeRange item)
    {
        m_list.push_back(item);
    }

    void push_back(double start, double end)
    {
        m_list.push_back(TimeRange(start, end));
    }

    unsigned long length()
    {
        return m_list.size();
    }

    TimeRange& at(unsigned long idx)
    {
        STARFISH_ASSERT(idx < m_list.size());
        return m_list[idx];
    }

private:
    GCVector<TimeRange> m_list;
};
}

#endif
