/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishReadableStreamBuffer__
#define __StarfishReadableStreamBuffer__

#include "ReadableStreamChunk.h"
#include "binding/ScriptWrappable.h"

namespace Starfish {

enum class BodyType;
class Promise;
class ExecutionContext;

#define READABLE_STREAM_BUFFER_CHUNK_SIZE 65536

class ReadableStreamBuffer final : public gc {
public:
    ReadableStreamBuffer(size_t chunkSize = READABLE_STREAM_BUFFER_CHUNK_SIZE);
    ~ReadableStreamBuffer();

    void* operator new(size_t size);
    void clearNativeResources();
    // Objects allocated via GC_finalized_malloc must not be freed with
    // GC_FREE or delete. The no-op operator delete below prevents this.
    void operator delete(void*)
    {
    }
    void operator delete[](void*) = delete;

    size_t size()
    {
        return m_buffer.size();
    }

    char* data()
    {
        return m_buffer.data();
    }

    void setType(BodyType type)
    {
        m_type = type;
    }

    BodyType type()
    {
        return m_type;
    }

    void setMimeType(String* mimeType)
    {
        m_mimeType = mimeType;
    }

    String* mineType()
    {
        return m_mimeType;
    }

    ReadableStreamChunk& buffer()
    {
        return m_buffer;
    }

    void push(const char* buffer, size_t length);

    ScriptValue dequeueValue(ScriptBindingInstance* instance);

    bool empty();

    void clear();

    void resolveWithType(Promise* promise, ExecutionContext* executionContext,
                         BodyType fetchtype);

private:
    ReadableStreamChunk m_buffer;
    size_t m_chunkSize;
    BodyType m_type;
    String* m_mimeType;
};
} // namespace Starfish

#endif
