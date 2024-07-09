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
#include <EscargotPublic.h>

#include "StarfishConfig.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptWrappable.h"
#include "core/page/Serializer.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"
#include "core/util/debug/Trace.h"
#include "core/modules/sharedworker/IPCSerializer.h"

namespace Starfish {

class IPCBufferWriter : public gc {
public:
    static const size_t kBufferMaxSize = 128000;

    IPCBufferWriter()
        : IPCBufferWriter(new GCVector<char>())
    {
    }

    IPCBufferWriter(GCVector<char>* buffer)
        : m_buffer(buffer)
        , m_size(0)
        , m_isError(false)
    {
    }

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

    void writeTerminator()
    {
        write<char>('\0');
    }

private:
    GCVector<char>* m_buffer;
    size_t m_size;
    bool m_isError;

    bool isOverflown(const size_t size)
    {
        if (m_size + size >= kBufferMaxSize) {
            m_isError = true;
            TRACE(IPC, "buffer overflow");
            return true;
        }
        return false;
    }

    void writeBuffer(const char* source, const size_t size)
    {
        if (m_isError || isOverflown(size)) {
            return;
        }

        m_buffer->insert(m_buffer->end(), source, source + size);
        m_size += size;
    }
};

class IPCBufferReader : public gc {
public:
    IPCBufferReader(const char* data, const size_t length)
        : m_data(data)
        , m_end(data + length)
        , m_position(const_cast<char*>(data))
        , m_isError(false)
    {
    }

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

    void readRawBytes(const size_t size, char*& data)
    {
        if (m_isError || isOverflown(size)) {
            return;
        }

        data = m_position;
        m_position += size;
    }

    bool checkValue(const char value)
    {
        if (m_isError || isOverflown(sizeof(char))) {
            return false;
        }

        if (value != *m_position) {
            m_isError = true;
            TRACE(IPC, "value does not match");
            return false;
        }

        return true;
    }

    bool isError() const
    {
        return m_isError;
    }

private:
    const char* m_data;
    const char* const m_end;
    char* m_position;
    bool m_isError;

    bool isOverflown(const size_t size)
    {
        if (m_position + size > m_end) {
            m_isError = true;
            TRACE(IPC, "buffer overflow");
            return true;
        }
        return false;
    }
};

IPCMessageSerializer::IPCMessageSerializer(const std::string& messageID)
    : m_writer(new IPCBufferWriter())
{
    writeString(messageID);
}

void IPCMessageSerializer::writeBool(const bool value)
{
    m_writer->write<IPCMessageTag>(IPCMessageTag::kBoolean);
    m_writer->write<bool>(value);
}

void IPCMessageSerializer::writeUInt32(const uint32_t value)
{
    m_writer->write<IPCMessageTag>(IPCMessageTag::kUInt32);
    m_writer->write<uint32_t>(value);
}

void IPCMessageSerializer::writeSize(const size_t value)
{
    m_writer->write<IPCMessageTag>(IPCMessageTag::kSizeNumber);
    m_writer->write<size_t>(value);
}

void IPCMessageSerializer::writeString(const std::string& value)
{
    m_writer->write<IPCMessageTag>(IPCMessageTag::kString);
    m_writer->write<size_t>(value.length());
    m_writer->write<char>(value.data(), value.length());
}

void IPCMessageSerializer::writeTerminator()
{
    m_writer->writeTerminator();
}

const char* IPCMessageSerializer::data() const
{
    return m_writer->buffer()->data();
}

size_t IPCMessageSerializer::size() const
{
    return m_writer->buffer()->size();
}

bool IPCMessageSerializer::isError() const
{
    return m_writer->isError();
}

IPCMessageDeserializer::IPCMessageDeserializer(const char* data,
                                               const size_t length)
    : m_reader(new IPCBufferReader(data, length))
{
    m_messageID = readString();
    TRACE(IPC, "received message:", m_messageID.c_str());
}

bool IPCMessageDeserializer::checkTag(IPCMessageTag tag)
{
    if (m_reader->checkValue(static_cast<char>(tag))) {
        char tag;
        m_reader->read<char>(tag);
        return true;
    }

    return false;
}

bool IPCMessageDeserializer::readBool()
{
    bool value = false;
    if (checkTag(IPCMessageTag::kBoolean)) {
        m_reader->read<bool>(value);
    }

    return value;
}

uint32_t IPCMessageDeserializer::readUInt32()
{
    uint32_t value = 0;
    if (checkTag(IPCMessageTag::kUInt32)) {
        m_reader->read<uint32_t>(value);
    }

    return value;
}

size_t IPCMessageDeserializer::readSize()
{
    size_t value = 0;
    if (checkTag(IPCMessageTag::kSizeNumber)) {
        m_reader->read<size_t>(value);
    }

    return value;
}

std::string IPCMessageDeserializer::readString()
{
    std::string value;
    if (checkTag(IPCMessageTag::kString)) {
        size_t length = 0;
        m_reader->read<size_t>(length);

        char* data = nullptr;
        m_reader->readRawBytes(length, data);

        value = std::string(data, length);
    }

    return value;
}

bool IPCMessageDeserializer::isError() const
{
    return m_reader->isError();
}

// Serialize ScriptValue
enum class ScriptValueSerializerTag : uint8_t {
    Undefined,
    Null,
    TruePrimitive,
    FalsePrimitive,
    Int32Primitive,
    Uint32Primitive,
    DoublePrimitive,
    OneByteStringPrimitive,
    TwoByteStringPrimitive,
    Unknown,
};

class StructuredSerialize : public gc {
public:
    StructuredSerialize(ExecutionContext* executionContext)
        : m_executionContext(executionContext)
        , m_data(new IPCSerializedVectorData())
        , m_writer(new IPCBufferWriter(m_data->vectorData()))
    {
    }

    SerializedTypedData* serialize(ScriptValue value)
    {
        serializeScriptValue(value);
        m_writer->writeTerminator();

        auto* data = new SerializedRawScriptValueData(m_data);
        return new SerializedTypedData(
            SerializedTypedData::Type::RawScriptValue, data);
    }

    IPCBufferWriter* writer()
    {
        return m_writer;
    }

private:
    ExecutionContext* m_executionContext;
    IPCSerializedVectorData* m_data;
    IPCBufferWriter* m_writer;

    void serializeScriptValue(ScriptValue value)
    {
        if (value->isUndefined()) {
            writeTag(ScriptValueSerializerTag::Undefined);
        } else if (value->isNull()) {
            writeTag(ScriptValueSerializerTag::Null);
        } else if (value->isBoolean()) {
            writeBoolean(value->asBoolean());
        } else if (value->isNumber()) {
            if (value->isInt32()) {
                writeInt32(value->asInt32());
            } else if (value->isUInt32()) {
                writeUint32(value->asUInt32());
            } else if (value->isNumber()) {
                writeDouble(value->asNumber());
            }
        } else if (value->isString()) {
            writeString(value->asString());
        } else if (value->isSymbol()) {
            throw new DOMException(m_executionContext,
                                   DOMException::DATA_CLONE_ERR);
        } else {
            STARFISH_UNSUPPORTED("Serializing values of unsupported types");
            throw new DOMException(m_executionContext,
                                   DOMException::DATA_CLONE_ERR);
        }

        if (m_writer->isError()) {
            throw new DOMException(m_executionContext,
                                   DOMException::DATA_CLONE_ERR);
        }
    }

    void writeTag(ScriptValueSerializerTag tag)
    {
        m_writer->write(static_cast<uint8_t>(tag));
    }

    void writeBoolean(bool value)
    {
        if (value) {
            writeTag(ScriptValueSerializerTag::TruePrimitive);
        } else {
            writeTag(ScriptValueSerializerTag::FalsePrimitive);
        }
    }

    void writeUint32(uint32_t value)
    {
        writeTag(ScriptValueSerializerTag::Uint32Primitive);
        m_writer->write<uint32_t>(value);
    }

    void writeInt32(int32_t value)
    {
        writeTag(ScriptValueSerializerTag::Int32Primitive);
        m_writer->write<int32_t>(value);
    }

    void writeDouble(double value)
    {
        writeTag(ScriptValueSerializerTag::DoublePrimitive);
        m_writer->write<double>(value);
    }

    void writeString(ScriptString string)
    {
        auto bufferData = string->stringBufferAccessData();
        if (bufferData.has8BitContent) {
            writeTag(ScriptValueSerializerTag::OneByteStringPrimitive);
            m_writer->write<size_t>(bufferData.length);
            m_writer->write<uint8_t>(
                static_cast<const uint8_t*>(bufferData.buffer),
                bufferData.length);
        } else {
            writeTag(ScriptValueSerializerTag::TwoByteStringPrimitive);
            m_writer->write<size_t>(bufferData.length);
            m_writer->write<uint16_t>(
                static_cast<const uint16_t*>(bufferData.buffer),
                bufferData.length);
        }
    }
};

class StructuredDeserialize : public gc {
public:
    StructuredDeserialize(ExecutionContext* executionContext, const char* data,
                          size_t len)
        : m_executionContext(executionContext)
        , m_reader(data, len)
    {
        TRACE(SERIALIZE, "size:", len);
    }

    void deserialize(DeserializeWithTransferResult& result)
    {
        deserializeScriptValue(result.m_deserialized);
    }

private:
    ExecutionContext* m_executionContext;
    IPCBufferReader m_reader;

    void deserializeScriptValue(ScriptValue& scriptValue)
    {
        ScriptValueSerializerTag tag;
        readTag(tag);

        if (tag == ScriptValueSerializerTag::Undefined) {
            scriptValue = scriptUndefined();
        } else if (tag == ScriptValueSerializerTag::Null) {
            scriptValue = scriptNull();
        } else if (tag == ScriptValueSerializerTag::TruePrimitive) {
            scriptValue = createScriptValue(true);
        } else if (tag == ScriptValueSerializerTag::FalsePrimitive) {
            scriptValue = createScriptValue(false);
        } else if (tag == ScriptValueSerializerTag::Uint32Primitive) {
            readNumber<uint32_t>(scriptValue);
        } else if (tag == ScriptValueSerializerTag::Int32Primitive) {
            readNumber<int32_t>(scriptValue);
        } else if (tag == ScriptValueSerializerTag::DoublePrimitive) {
            readNumber<double>(scriptValue);
        } else if (tag == ScriptValueSerializerTag::OneByteStringPrimitive) {
            readOneByteString(scriptValue);
        } else if (tag == ScriptValueSerializerTag::TwoByteStringPrimitive) {
            readTwoByteString(scriptValue);
        } else {
            TRACE(SERIALIZE, "Unknown Tag");
            throw new DOMException(m_executionContext,
                                   DOMException::DATA_CLONE_ERR,
                                   "unsupported value");
        }
    }

    void readTag(ScriptValueSerializerTag& tag)
    {
        uint8_t v;
        m_reader.read<uint8_t>(v);
        checkError();

        tag = static_cast<ScriptValueSerializerTag>(v);
    }

    template <typename T>
    void readNumber(ScriptValue& scriptValue)
    {
        T value = 0;
        m_reader.read<T>(value);
        checkError();

        scriptValue = createScriptValue(value);
    }

    void readOneByteString(ScriptValue& scriptValue)
    {
        size_t length = 0;
        char* data;
        m_reader.read<size_t>(length);
        m_reader.readRawBytes(length, data);
        checkError();

        scriptValue = createScriptValue(createScriptString(
            reinterpret_cast<const char*>(data), static_cast<size_t>(length)));
    }

    void readTwoByteString(ScriptValue& scriptValue)
    {
        size_t length = 0;
        char* data;
        m_reader.read<size_t>(length);
        m_reader.readRawBytes(length * sizeof(uint16_t), data);
        checkError();

        scriptValue = createScriptValue(createScriptString(
            String::fromUTF16(reinterpret_cast<const char16_t*>(data),
                              static_cast<size_t>(length))));
    }

    void checkError()
    {
        if (m_reader.isError()) {
            throw new DOMException(m_executionContext,
                                   DOMException::DATA_CLONE_ERR);
        }
    }
};

IPCSerializedVectorData::IPCSerializedVectorData()
    : m_vectorData(new GCVector<char>())
{
}

IPCSerializedData::IPCSerializedData(const char* data, size_t size)
    : m_data(data)
    , m_size(size)
{
}

void IPCSerializer::serializeWithTransfer(
    ExecutionContext* executionContext, ScriptValue value,
    const GCAtomicVector<ScriptObject>& transferValues,
    SerializeWithTransferResult& result)
{
    StructuredSerialize* serializer = new StructuredSerialize(executionContext);

    result.m_serialized = serializer->serialize(value);
    result.m_deserializer = IPCSerializer::deserializeWithTransfer;
}

void IPCSerializer::deserializeWithTransfer(
    ExecutionContext* executionContext, SerializeWithTransferResult& serialized,
    DeserializeWithTransferResult& result)
{
    SerializedRawScriptValueData* serializedData =
        serialized.m_serialized->data()->asSerializedRawScriptValueData();

    StructuredDeserialize* deserializer = new StructuredDeserialize(
        executionContext, serializedData->internal()->data(),
        serializedData->internal()->size());

    deserializer->deserialize(result);
}

} // namespace Starfish

#endif
