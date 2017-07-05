/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
