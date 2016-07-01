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

#ifndef __StarFishHTMLEntityTable__
#define __StarFishHTMLEntityTable__

#include "util/String.h"

namespace StarFish {

struct HTMLEntityTableEntry {
    char lastCharacter() const { return entity[length - 1]; }

    const char* entity;
    int length;
    char32_t firstValue;
    char32_t secondValue;
};

class HTMLEntityTable {
public:
    static const HTMLEntityTableEntry* firstEntry();
    static const HTMLEntityTableEntry* lastEntry();

    static const HTMLEntityTableEntry* firstEntryStartingWith(char32_t);
    static const HTMLEntityTableEntry* lastEntryStartingWith(char32_t);
};

}

#endif
