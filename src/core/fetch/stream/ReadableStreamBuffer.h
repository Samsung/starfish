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

namespace Starfish {

enum class BodyType;
class Promise;
class ExecutionContext;

class ReadableStreamBuffer final : public gc {
public:
    ReadableStreamBuffer();
    ~ReadableStreamBuffer();

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
    void clear();

    void resolveWithType(Promise* promise, ExecutionContext* executionContext,
                         BodyType fetchtype);

private:
    ReadableStreamChunk m_buffer;
    BodyType m_type;
    String* m_mimeType;
};
} // namespace Starfish

#endif
