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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/fileapi/Blob.h"
#include "core/page/Serializer.h"
#include "core/page/WebView.h"
#include "core/dom/Document.h"

namespace StarFish {

Blob::Blob(Document* document, Blob::BlobData blobData)
    : Blob(document, blobData.m_size, blobData.m_type, blobData.m_data,
           blobData.m_isClosed, blobData.m_isEntryOfBlobURLStore)
{
}

ScriptBindingInstance* Blob::scriptBindingInstance()
{
    return document()->scriptBindingInstance();
}

SerializedData* Blob::serialized()
{
    return new SerializedBlobData(m_blobData);
}

void Blob::addBlobToBlobURLStore()
{
    STARFISH_ASSERT(!m_blobData.m_isEntryOfBlobURLStore);
    m_blobData.m_isEntryOfBlobURLStore = true;
    STARFISH_ASSERT(!document()->webView()->isValidBlobURL(this));
    document()->webView()->addBlobInBlobURLStore(this);
}

void Blob::removeBlobFromBlobURLStore()
{
    STARFISH_ASSERT(document()->webView()->isValidBlobURL(this));
    document()->webView()->removeBlobFromBlobURLStore(this);
}

Blob* Blob::slice(int64_t start)
{
    return slice(start, m_blobData.m_size);
}

Blob* Blob::slice(int64_t start, int64_t end, String* contentType)
{
    // https://www.w3.org/TR/FileAPI/#slice-method-algo
    // FIXME range of int64_t and size_t not match..
    int64_t relativeStart;
    uint64_t size = m_blobData.m_size;
    if (start < 0) {
        relativeStart = std::max(start + (int64_t)size, (int64_t)0);
    } else {
        relativeStart = std::min(start, (int64_t)size);
    }
    int64_t relativeEnd;
    if (end < 0) {
        relativeEnd = std::max(((int64_t)size + end), (int64_t)0);
    } else {
        relativeEnd = std::min((int64_t)end, (int64_t)size);
    }

    String* newType = contentType;
    for (size_t i = 0; i < newType->length(); i++) {
        char32_t c = newType->charAt(i);
        if (c < 0x20 || c > 0x7E) {
            newType = String::emptyString;
            break;
        }
    }
    newType = newType->toLower();
    size_t span = (size_t)std::max(relativeEnd - relativeStart, (int64_t)0);
    STARFISH_ASSERT(relativeStart >= 0);
    void* newStart = ((char*)m_blobData.m_data) + relativeStart;
    return new Blob(document(), span, newType, newStart, m_blobData.m_isClosed,
                    false);
}
}
