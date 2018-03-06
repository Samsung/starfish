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

SerializedData* Blob::serialize(SerializingMap& memory)
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
    newType = newType->toASCIILower();
    size_t span = (size_t)std::max(relativeEnd - relativeStart, (int64_t)0);
    STARFISH_ASSERT(relativeStart >= 0);
    void* newStart = ((char*)m_blobData.m_data) + relativeStart;
    return new Blob(document(), span, newType, newStart, m_blobData.m_isClosed,
                    false);
}
}
