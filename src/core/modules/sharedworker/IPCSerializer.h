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

#if defined(STARFISH_ENABLE_SHARED_WORKER)
#ifndef __StarfishIPCSerializer__
#define __StarfishIPCSerializer__

#include "binding/ScriptWrappable.h"
#include "core/page/Serializer.h"

namespace Starfish {

class IPCBufferWriter;
class IPCBufferReader;
class SerializeWithTransferResult;
class DeserializeWithTransferResult;

enum class IPCMessageTag : char {
    kUndefine = 0,
    kBoolean,
    kUInt32,
    kSizeNumber,
    kString,
};

class IPCMessageSerializer : public gc {
public:
    IPCMessageSerializer(const std::string& messageID);

    void writeBool(const bool value);
    void writeUInt32(const uint32_t value);
    void writeSize(const size_t value);
    void writeString(const std::string& value);

    const char* data() const;

    size_t size() const;

    bool isError() const;

private:
    IPCBufferWriter* m_writer;
};

class IPCMessageDeserializer : public gc {
public:
    IPCMessageDeserializer(const char* data, const size_t length);

    std::string messageID() const
    {
        return m_messageID;
    }

    bool checkTag(const IPCMessageTag tag);

    bool readBool();
    uint32_t readUInt32();
    size_t readSize();
    std::string readString();

    bool isError() const;

private:
    IPCBufferReader* m_reader;
    std::string m_messageID;
};

class IPCSerializedVectorData : public SerializedRawScriptValueDataInternal {
public:
    IPCSerializedVectorData();

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

class IPCSerializedData : public SerializedRawScriptValueDataInternal {
public:
    IPCSerializedData(const char* data, size_t size);

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

class IPCSerializer {
public:
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
