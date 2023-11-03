/*
 * Copyright (c) 2021-present Samsung Electronics Co., Ltd
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
#include "core/fileapi/File.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

File::File(ExecutionContext* executionContext,
           const GCVector<BufferSourceOrBlobOrDOMString>& fileBits,
           String* fileName, const FilePropertyBag& options)
    : Blob(executionContext)
    , m_fileBits(fileBits)
    , m_name(fileName)
    , m_lastModified(options.hasLastModified() ? options.lastModified() : 0)
{
    init(fileBits, options);
}

File::File(ExecutionContext* executionContext, BlobData blobData)
    : Blob(executionContext, blobData)
    , m_name(String::emptyString)
    , m_lastModified(0)
{
}

void File::init(const GCVector<BufferSourceOrBlobOrDOMString>& fileBits,
                const FilePropertyBag& options)
{
    GCVector<std::pair<void*, size_t>> bufferInfo;
    size_t totalByteLength = 0;
    for (auto& item : m_fileBits) {
        if (item.isDOMStringValue()) {
            NullableUTF8String str =
                item.getDOMStringValue()->toNullableUTF8String();
            bufferInfo.push_back(std::make_pair(
                reinterpret_cast<void*>(const_cast<char*>(str.m_buffer)),
                str.m_bufferSize));
            totalByteLength += str.m_bufferSize;
        } else if (item.isArrayBufferViewOrArrayBufferValue()) {
            STARFISH_UNIMPLEMENTED();
        } else if (item.isBlobValue()) {
            STARFISH_UNIMPLEMENTED();
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
}

ScriptBindingInstance* File::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

} // namespace Starfish
