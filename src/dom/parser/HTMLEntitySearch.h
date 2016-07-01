/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#ifndef __StarFishHTMLEntitySearch__
#define __StarFishHTMLEntitySearch__

#include "util/String.h"

namespace StarFish {

struct HTMLEntityTableEntry;

class HTMLEntitySearch {
public:
    HTMLEntitySearch();

    void advance(char32_t);

    bool isEntityPrefix() const { return !!m_first; }
    int currentLength() const { return m_currentLength; }

    const HTMLEntityTableEntry* mostRecentMatch() const { return m_mostRecentMatch; }

private:
    enum CompareResult {
        Before,
        Prefix,
        After,
    };

    CompareResult compare(const HTMLEntityTableEntry*, char32_t) const;
    const HTMLEntityTableEntry* findFirst(char32_t) const;
    const HTMLEntityTableEntry* findLast(char32_t) const;

    void fail()
    {
        m_first = 0;
        m_last = 0;
    }

    int m_currentLength;

    const HTMLEntityTableEntry* m_mostRecentMatch;
    const HTMLEntityTableEntry* m_first;
    const HTMLEntityTableEntry* m_last;
};

}

#endif
