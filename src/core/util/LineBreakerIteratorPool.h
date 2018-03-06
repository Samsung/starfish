/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishLineBreakerPool__
#define __StarFishLineBreakerPool__

#include <unordered_map>
#include <vector>

namespace StarFish {

enum LineBreakIteratorMode {
    LineBreakIteratorModeUAX14,
    LineBreakIteratorModeUAX14Loose,
    LineBreakIteratorModeUAX14Normal,
    LineBreakIteratorModeUAX14Strict,
};

struct BreakIteratorInfo {
    icu::Locale m_locale;
    LineBreakIteratorMode m_mode;

    BreakIteratorInfo(icu::Locale locale, LineBreakIteratorMode mode)
        : m_locale(locale)
        , m_mode(mode)
    {
    }
};

icu::BreakIterator* openLineBreakIterator(BreakIteratorInfo& info,
                                          LineBreakIteratorMode mode,
                                          bool isCJK);
void closeLineBreakIterator(icu::BreakIterator*& iter);

class LineBreakIteratorPool : public gc {
public:
    LineBreakIteratorPool(size_t capacity)
        : m_capacity(capacity)
    {
    }

    icu::BreakIterator* get(const icu::Locale& locale,
                            LineBreakIteratorMode mode, bool isCJK)
    {
        BreakIteratorInfo info(locale, mode);

        icu::BreakIterator* iterator = 0;
        for (size_t i = 0; i < m_pool.size(); ++i) {
            BreakIteratorInfo info2 = m_pool[i].first;
            if (info.m_locale == info2.m_locale &&
                info.m_mode == info2.m_mode) {
                iterator = m_pool[i].second;
                break;
            }
        }

        if (!iterator) {
            iterator = openLineBreakIterator(info, mode, isCJK);
            if (!iterator) {
                return nullptr;
            }

            put(info, iterator);
        }

        return iterator;
    }

    void put(BreakIteratorInfo& info, icu::BreakIterator* iterator)
    {
        if (m_pool.size() == m_capacity) {
            closeLineBreakIterator((*m_pool.begin()).second);
            m_pool.erase(m_pool.begin());
        }

        m_pool.emplace_back(info, iterator);
    }

private:
    size_t m_capacity;
    std::vector<std::pair<BreakIteratorInfo, icu::BreakIterator*>> m_pool;
};
}

#endif
