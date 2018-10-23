/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "core/fetch/stream/ReadableStreamBuffer.h"
#include "core/util/TextConverter.h"
#include "core/fileapi/Blob.h"

namespace Starfish {

void ReadableStreamBuffer::push(const char* buffer, size_t length)
{
    m_buffer.insert(m_buffer.end(), &buffer[0], &buffer[length]);
}

void ReadableStreamBuffer::clear()
{
    m_buffer.clear();
    m_buffer.shrink_to_fit();
}

void ReadableStreamBuffer::resolveWithType(Promise* promise,
                                           ScriptBindingInstance* instance,
                                           ResponseType type)
{
    size_t size = m_buffer.size();

    if (type == ResponseType::Text) {
        TextConverter textConverter(m_mimeType, String::fromUTF8("UTF-8"),
                                    m_buffer.data(), size);
        String* responseText =
            textConverter.convert(m_buffer.data(), size, true);
        promise->fulfill(createScriptValue(responseText));
    } else if (type == ResponseType::Blob) {
        void* buffer = calloc(1, size);
        memcpy(buffer, m_buffer.data(), size);
        auto blob = new Blob(instance->ownerDocument(), size, m_mimeType,
                             buffer, false, false);
        promise->fulfill(blob->scriptValue());
        free(buffer);
    } else if (type == ResponseType::Json) {
        TextConverter textConverter(m_mimeType, String::fromUTF8("UTF-8"),
                                    m_buffer.data(), size);
        String* responseText =
            textConverter.convert(m_buffer.data(), size, true);
        auto json = parseJSON(instance, responseText);
        promise->fulfill(json);
    } else if (type == ResponseType::ArrayBuffer) {
        void* buffer = calloc(1, size);
        memcpy(buffer, m_buffer.data(), size);
        auto arrayBuffer = createArrayBuffer(instance, buffer, size);
        promise->fulfill(arrayBuffer);
        free(buffer);
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }
}
}
