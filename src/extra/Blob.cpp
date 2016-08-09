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

#include "StarFishConfig.h"
#include "Blob.h"
#include "StarFish.h"

namespace StarFish {

void Blob::addBlobToBlobURLStore()
{
    STARFISH_ASSERT(!m_isEntryOfBlobURLStore);
    m_isEntryOfBlobURLStore = true;
    STARFISH_ASSERT(!m_starFish->isValidBlobURL(this));
    m_starFish->addBlobInBlobURLStore(this);
}

void Blob::removeBlobFromBlobURLStore()
{
    STARFISH_ASSERT(m_starFish->isValidBlobURL(this));
    m_starFish->removeBlobFromBlobURLStore(this);
}

Blob* Blob::slice(int64_t start, int64_t end, String* contentType)
{
    // https://www.w3.org/TR/FileAPI/#slice-method-algo
    // FIXME range of int64_t and size_t not match..
    int64_t relativeStart;
    if (start < 0) {
        relativeStart = std::max(start + (int64_t)m_size, (int64_t)0);
    } else {
        relativeStart = std::min(start, (int64_t)m_size);
    }
    int64_t relativeEnd;
    if (end < 0) {
        relativeEnd = std::max(((int64_t)m_size + end), (int64_t)0);
    } else {
        relativeEnd = std::min((int64_t)end, (int64_t)m_size);
    }

    String* newType = contentType;
    for (size_t i = 0; i < newType->length(); i ++) {
        char32_t c = newType->charAt(i);
        if (c < 0x20 || c > 0x7E) {
            newType = String::emptyString;
            break;
        }
    }
    newType = newType->toLower();
    size_t span = (size_t)std::max(relativeEnd - relativeStart, (int64_t)0);
    STARFISH_ASSERT(relativeStart >= 0);
    void* newStart = ((char*)m_data) + relativeStart;
    return new Blob(m_starFish, span, newType, newStart, m_isClosed, false);
}

}
