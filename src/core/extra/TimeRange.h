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
#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined(__StarFishTimeRange__)
#define __StarFishTimeRange__

namespace StarFish {

class TimeRange {
public:
    TimeRange(double start = 0, double end = 0)
        : m_start(start)
        , m_end(end)
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

    // NOTE Should use for already normalized TimeRanges
    static void appendToNormalizedVector(GCAtomicVector<TimeRange>& v,
                                         double start, double end)
    {
        bool merging = false;
        size_t mergingIdx = 0;
        size_t size = v.size();
        for (size_t i = 0; i < size; i++) {
            TimeRange& item = v.at(i);
            if (merging) {
                if (end < item.start()) {
                    v.at(mergingIdx).setEnd(end);
                    if (mergingIdx + 1 < i) {
                        v.erase(mergingIdx + 1, i);
                    }
                    return;
                } else if (end < item.end()) {
                    v.at(mergingIdx).setEnd(item.end());
                    v.erase(mergingIdx + 1, i + 1);
                    return;
                }
            } else if (start < item.start()) {
                if (end < item.start()) {
                    v.insert(i, TimeRange(start, end));
                    return;
                } else if (end < item.end()) {
                    item.setStart(start);
                    return;
                } else {
                    merging = true;
                    mergingIdx = i;
                }
            } else if (start < item.end()) {
                if (end > item.end()) {
                    merging = true;
                    mergingIdx = i;
                } else {
                    return;
                }
            }
        }
        if (merging) {
            v.at(mergingIdx).setEnd(end);
            if (mergingIdx + 1 < size) {
                v.erase(mergingIdx + 1, size);
            }
        } else {
            v.emplace_back(start, end);
        }
    }

private:
    double m_start;
    double m_end;
};
}

#endif
