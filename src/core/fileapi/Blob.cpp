/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/fileapi/Blob.h"
#include "core/serialize/Serializer.h"
#include "core/page/WebBase.h"
#include "core/dom/ExecutionContext.h"
#include "binding/generated/ArrayBufferViewOrArrayBufferUnion.h"
#include "EscargotPublic.h"

namespace Starfish {

Blob ::Blob(ExecutionContext* executionContext)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj, void* cd) { ((Blob*)obj)->finalize(); }, NULL, NULL,
        NULL);
}

Blob::Blob(ExecutionContext* executionContext,
           const GCVector<BufferSourceOrBlobOrDOMString>& blobParts)
    : Blob(executionContext)
{
    initialize(blobParts);
}

Blob::Blob(ExecutionContext* executionContext,
           const GCVector<BufferSourceOrBlobOrDOMString>& blobParts,
           const BlobPropertyBag& options)
    : Blob(executionContext)
{
    initialize(blobParts, options);
}

Blob ::Blob(ExecutionContext* executionContext, uint64_t size, String* type,
            void* data, bool isClosed, bool isEntryOfBlobURLStore,
            bool isAllocatedByMalloc)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_blobData(size, type, data, isClosed, isEntryOfBlobURLStore,
                 isAllocatedByMalloc)
{
    // FIXME: Below is the legacy code. |isEntryOfBlobURLStore| is always
    // explicitly specified as false. therefore, calling |addBlobToBlobURLStore|
    // will never run. additionally, there is an assertion in
    // |addBlobToBlobURLStore| that is exactly the opposite of the entry
    // condition.
    if (m_blobData.m_isEntryOfBlobURLStore) {
        addBlobToBlobURLStore();
    }
    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj, void* cd) { ((Blob*)obj)->finalize(); }, NULL, NULL,
        NULL);
}

Blob::Blob(ExecutionContext* executionContext, Blob::BlobData blobData)
    : Blob(executionContext, blobData.m_size, blobData.m_type, blobData.m_data,
           blobData.m_isClosed, blobData.m_isEntryOfBlobURLStore)
{
}

void Blob::initialize(const GCVector<BufferSourceOrBlobOrDOMString>& blobParts,
                      const BlobPropertyBag& options)
{
    GCVector<std::pair<void*, size_t>> bufferInfo;
    size_t totalByteLength = 0;
    for (auto& item : blobParts) {
        if (item.isDOMStringValue()) {
            OptionalUTF8String str =
                item.getDOMStringValue()->toOptionalUTF8String();
            bufferInfo.push_back(std::make_pair(
                reinterpret_cast<void*>(const_cast<char*>(str.m_buffer)),
                str.m_bufferSize));
            totalByteLength += str.m_bufferSize;
        } else if (item.isArrayBufferViewOrArrayBufferValue()) {
            ArrayBufferViewOrArrayBuffer itemValue =
                item.getArrayBufferViewOrArrayBufferValue();
            if (itemValue.isArrayBufferViewValue()) {
                ArrayBufferViewRef* arrayBufferView =
                    itemValue.getArrayBufferViewValue();
                bufferInfo.push_back(
                    std::make_pair(reinterpret_cast<void*>(
                                       arrayBufferView->buffer()->rawBuffer()),
                                   arrayBufferView->byteLength()));
                totalByteLength += arrayBufferView->byteLength();
            } else if (itemValue.isArrayBufferValue()) {
                ArrayBufferObjectRef* arrayBuffer =
                    itemValue.getArrayBufferValue();
                bufferInfo.push_back(std::make_pair(
                    reinterpret_cast<void*>(arrayBuffer->rawBuffer()),
                    arrayBuffer->byteLength()));
                totalByteLength += arrayBuffer->byteLength();
            } else {
                STARFISH_ASSERT_NOT_REACHED();
            }
        } else if (item.isBlobValue()) {
            Blob* blob = item.getBlobValue();
            bufferInfo.push_back(std::make_pair(blob->data(), blob->size()));
            totalByteLength += blob->size();
        } else {
            STARFISH_ASSERT_NOT_REACHED();
        }
    }

    size_t offset = 0;
    char* buffer = reinterpret_cast<char*>(
        GC_MALLOC_ATOMIC_IGNORE_OFF_PAGE(totalByteLength));
    for (size_t i = 0; i < bufferInfo.size(); i++) {
        memcpy(buffer + offset, bufferInfo[i].first, bufferInfo[i].second);
        offset += bufferInfo[i].second;
    }

    m_blobData.m_size = totalByteLength;
    if (options.hasType()) {
        m_blobData.m_type = options.type();
    }
    m_blobData.m_data = buffer;
    m_blobData.m_isClosed = false;
    m_blobData.m_isEntryOfBlobURLStore = false;
    m_blobData.m_isAllocatedByMalloc = false;

    // TODO: Apply ending type.
}

ScriptBindingInstance* Blob::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

SerializedData* Blob::serialize(SerializingMap& memory)
{
    return new SerializedBlobData(m_blobData);
}

void Blob::addBlobToBlobURLStore()
{
    STARFISH_ASSERT(!m_blobData.m_isEntryOfBlobURLStore);
    m_blobData.m_isEntryOfBlobURLStore = true;
    STARFISH_ASSERT(!executionContext()->webBase()->isValidBlobURL(this));
    executionContext()->webBase()->addBlobInBlobURLStore(this);
}

void Blob::removeBlobFromBlobURLStore()
{
    STARFISH_ASSERT(executionContext()->webBase()->isValidBlobURL(this));
    executionContext()->webBase()->removeBlobFromBlobURLStore(this);
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
    return new Blob(executionContext(), span, newType, newStart,
                    m_blobData.m_isClosed, false);
}

Promise* Blob::text()
{
    Promise* promise = new Promise(m_executionContext->scriptBindingInstance());
    promise->fulfill((ScriptValue)createScriptString(
        (const char*)m_blobData.m_data, m_blobData.m_size));
    return promise;
}

Promise* Blob::arrayBuffer()
{
    Promise* promise = new Promise(m_executionContext->scriptBindingInstance());
    void* newBuffer = malloc(m_blobData.m_size);
    memcpy(newBuffer, m_blobData.m_data, m_blobData.m_size);

    ScriptArrayBuffer arrayBuffer =
        createScriptArrayBuffer(m_executionContext->scriptBindingInstance(),
                                newBuffer, m_blobData.m_size);

    promise->fulfill((ScriptValue)arrayBuffer);

    return promise;
}
} // namespace Starfish
