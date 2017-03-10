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
#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined(__StarFishTimeRange__)
#define __StarFishTimeRange__

namespace StarFish {

class TimeRange {
public:
    TimeRange(double start = 0, double end = 0) : m_start(start), m_end(end)
    {
    }

    bool operator==(const TimeRange& c) const
    {
        return m_start == c.m_start && m_end == c.m_end;
    }

    bool operator!=(const TimeRange& c) const
    {
        return !this->operator==(c);
    }

    double start()
    {
        return m_start;
    }

    double end()
    {
        return m_end;
    }

    void setStart(double start)
    {
        m_start = start;
    }

    void setEnd(double end)
    {
        m_end = end;
    }

    void set(double start, double end)
    {
        setStart(start);
        setEnd(end);
    }

    bool isInRange(double pivot)
    {
        return (m_start <= pivot && m_end >= pivot);
    }

private:
    double m_start;
    double m_end;
};
}

#endif
