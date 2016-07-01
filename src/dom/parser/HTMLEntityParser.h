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

#ifndef __StarFishHTMLEntityParser__
#define __StarFishHTMLEntityParser__

#include "util/String.h"

namespace StarFish {

class DecodedHTMLEntity {
private:
    // HTML entities contain at most four UTF-16 code units.
    static const unsigned kMaxLength = 4;

public:
    DecodedHTMLEntity()
        : length(0)
    { }

    bool isEmpty() const { return !length; }

    void append(char32_t c)
    {
        STARFISH_RELEASE_ASSERT(length < kMaxLength);
        data[length++] = c;
    }

    unsigned length;
    char32_t data[kMaxLength];
};

bool consumeHTMLEntity(SegmentedString&, DecodedHTMLEntity& decodedEntity, bool& notEnoughCharacters, char32_t additionalAllowedCharacter = '\0');

}

#endif
