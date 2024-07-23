/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_SHARED_WORKER) || defined(STARFISH_ENABLE_IDB)

#ifndef __StarfishMemorySerializer__
#define __StarfishMemorySerializer__

#include "binding/ScriptWrappable.h"
#include "core/serialize/Serializer.h"

namespace Starfish {

class SerializeWithTransferResult;
class DeserializeWithTransferResult;

class MemorySerializeWriter : public gc {
public:
    static const size_t kBufferMaxSize = 128000;

    MemorySerializeWriter();

    MemorySerializeWriter(GCVector<char>* buffer);

    template <typename T>
    void write(T value)
    {
        writeBuffer(reinterpret_cast<const char*>(&value), sizeof(T));
    }

    template <typename T>
    void write(const T* value, const size_t length)
    {
        writeBuffer(reinterpret_cast<const char*>(value), length * sizeof(T));
    }

    const GCVector<char>* buffer() const
    {
        return m_buffer;
    }

    bool isError() const
    {
        return m_isError;
    }

    void writeTerminator();

private:
    GCVector<char>* m_buffer;
    size_t m_size;
    bool m_isError;

    bool isOverflown(const size_t size);

    void writeBuffer(const char* source, const size_t size);
};

class MemorySerializeReader : public gc {
public:
    MemorySerializeReader(const char* data, const size_t length);

    template <typename T>
    void read(T& value)
    {
        size_t size = sizeof(T);

        if (m_isError || isOverflown(size)) {
            return;
        }

        value = *(reinterpret_cast<T*>(m_position));
        m_position += size;
    }

    void readRawBytes(const size_t size, char*& data);

    bool checkValue(const char value);

    bool isError() const
    {
        return m_isError;
    }

    void setError()
    {
        m_isError = true;
    }

private:
    const char* m_data;
    const char* const m_end;
    char* m_position;
    bool m_isError;

    bool isOverflown(const size_t size);
};

class MemorySerializedVectorData : public SerializedRawScriptValueDataInternal {
public:
    MemorySerializedVectorData();

    const char* data() const override
    {
        return m_vectorData->data();
    }

    size_t size() const override
    {
        return m_vectorData->size();
    }

    DEFINE_GETTER(GCVector<char>*, vectorData);

private:
    GCVector<char>* m_vectorData;
};

class MemorySerializedData : public SerializedRawScriptValueDataInternal {
public:
    MemorySerializedData(const char* data, size_t size);

    const char* data() const override
    {
        return m_data;
    }

    size_t size() const override
    {
        return m_size;
    }

private:
    const char* m_data;
    size_t m_size;
};

class MemorySerializer {
public:
    static SerializedTypedData* serialize(ExecutionContext* executionContext,
                                          ScriptValue value);

    static void serializeWithTransfer(
        ExecutionContext* executionContext, ScriptValue value,
        const GCAtomicVector<ScriptObject>& transferValues,
        SerializeWithTransferResult& result);

    static void deserializeWithTransfer(ExecutionContext* executionContext,
                                        SerializeWithTransferResult& serialized,
                                        DeserializeWithTransferResult& result);
};

} // namespace Starfish

#endif
#endif
