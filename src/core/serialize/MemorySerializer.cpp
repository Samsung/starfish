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

#include <EscargotPublic.h>

#include "StarfishConfig.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptWrappable.h"
#include "core/util/debug/Trace.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"
#include "core/serialize/MemorySerializer.h"

namespace Starfish {

MemorySerializeWriter::MemorySerializeWriter()
    : MemorySerializeWriter(new GCVector<char>())
{
}

MemorySerializeWriter::MemorySerializeWriter(GCVector<char>* buffer)
    : m_buffer(buffer)
    , m_size(0)
    , m_isError(false)
{
}

bool MemorySerializeWriter::isOverflown(const size_t size)
{
    if (m_size + size >= kBufferMaxSize) {
        m_isError = true;
        TRACE(SERIALIZE, "buffer overflow");
        return true;
    }
    return false;
}

void MemorySerializeWriter::writeBuffer(const char* source, const size_t size)
{
    if (m_isError || isOverflown(size)) {
        return;
    }

    m_buffer->insert(m_buffer->end(), source, source + size);
    m_size += size;
}

void MemorySerializeWriter::writeTerminator()
{
    write<char>('\0');
}

MemorySerializeReader::MemorySerializeReader(const char* data,
                                             const size_t length)
    : m_end(data + length)
    , m_position(const_cast<char*>(data))
    , m_isError(false)
{
}

void MemorySerializeReader::readRawBytes(const size_t size, char*& data)
{
    if (m_isError || isOverflown(size)) {
        return;
    }

    data = m_position;
    m_position += size;
}

bool MemorySerializeReader::checkValue(const char value)
{
    if (m_isError || isOverflown(sizeof(char))) {
        return false;
    }

    if (value != *m_position) {
        m_isError = true;
        TRACE(SERIALIZE, "value does not match");
        return false;
    }

    return true;
}

bool MemorySerializeReader::isMatchingValue(const char value)
{
    if (m_isError || isOverflown(sizeof(char))) {
        return false;
    }

    return (value == *m_position);
}

bool MemorySerializeReader::isOverflown(const size_t size)
{
    if (m_position + size > m_end) {
        m_isError = true;
        TRACE(SERIALIZE, "buffer overflow");
        return true;
    }
    return false;
}

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
    BeginObject,
    EndObject,
    BeginArrayObject,
    EndArrayObject,
    ArrayBuffer,
    ArrayBufferView,
    Int8Array,
    Uint8Array,
    Int16Array,
    Uint16Array,
    Int32Array,
    Uint32Array,
    Unknown,
};

class StructuredSerialize : public gc {
public:
    StructuredSerialize(ExecutionContext* executionContext)
        : m_executionContext(executionContext)
        , m_data(new MemorySerializedVectorData())
        , m_writer(new MemorySerializeWriter(m_data->vectorData()))
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

    MemorySerializeWriter* writer()
    {
        return m_writer;
    }

private:
    ExecutionContext* m_executionContext;
    MemorySerializedVectorData* m_data;
    MemorySerializeWriter* m_writer;

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
        } else if (value->isObject()) {
            ScriptObject obj = value->asObject();
            if (obj->isArrayObject()) {
                writeArrayObject(obj->asArrayObject());
            } else if (obj->isArrayBufferObject()) {
                writeArrayBufferObject(obj->asArrayBufferObject());
            } else if (obj->isTypedArrayObject()) {
                writeTypedArrayObject(obj->asArrayBufferView());
            } else if (obj->isBooleanObject() || obj->isNumberObject() ||
                       obj->isBigIntObject() || obj->isStringObject() ||
                       obj->isDateObject() || obj->isRegExpObject() ||
                       obj->isSharedArrayBufferObject() ||
                       obj->isFunctionObject() || obj->isErrorObject() ||
                       obj->isGlobalObject() || obj->isPromiseObject() ||
                       obj->isProxyObject() || obj->isArrayBufferView()) {
                STARFISH_UNIMPLEMENTED();
                throw new DOMException(m_executionContext,
                                       DOMException::DATA_CLONE_ERR);
            } else {
                writeObject(obj);
            }
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

    void writeRawBuffer(const uint8_t* buffer, size_t byteLength)
    {
        m_writer->write<size_t>(byteLength);
        m_writer->write<uint8_t>(buffer, byteLength);
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

    void writeArrayObject(ScriptArrayObject arrayObject)
    {
        writeTag(ScriptValueSerializerTag::BeginArrayObject);

        auto result = Evaluator::execute(
            m_executionContext->scriptBindingInstance()->scriptContext(),
            [](ScriptExecutionState state, StructuredSerialize* self,
               ScriptArrayObject arrayObject) {
                uint64_t length = arrayObject->length(state);
                self->writer()->write<uint64_t>(length);

                for (uint64_t i = 0; i < length; i++) {
                    auto key = ValueRef::create(i);
                    if (arrayObject->hasOwnProperty(state, key)) {
                        self->serializeScriptValue(
                            arrayObject->get(state, key));
                    }
                }

                return scriptUndefined();
            },
            this, arrayObject);

        if (!result.isSuccessful()) {
            throw new DOMException(m_executionContext,
                                   DOMException::DATA_CLONE_ERR,
                                   "Failed to serialize an array object.");
        }

        writeTag(ScriptValueSerializerTag::EndArrayObject);
    }

    void writeArrayBufferObject(ScriptArrayBuffer arrayBuffer)
    {
        writeTag(ScriptValueSerializerTag::ArrayBuffer);
        TRACE(SERIALIZE, arrayBuffer->byteLength());
        writeRawBuffer(arrayBuffer->rawBuffer(), arrayBuffer->byteLength());
    }

    void writeTypedArrayObject(ScriptArrayBufferView arrayBufferView)
    {
        writeTag(ScriptValueSerializerTag::ArrayBuffer);
        writeRawBuffer(arrayBufferView->rawBuffer(),
                       arrayBufferView->byteLength());

        writeArrayBufferView(arrayBufferView);
    }

    void writeArrayBufferView(ArrayBufferViewRef* arrayBufferView)
    {
        writeTag(ScriptValueSerializerTag::ArrayBufferView);

        ScriptValueSerializerTag typeTag = ScriptValueSerializerTag::Unknown;

        if (arrayBufferView->isUint8ArrayObject()) {
            typeTag = ScriptValueSerializerTag::Uint8Array;
        } else if (arrayBufferView->isInt8ArrayObject()) {
            typeTag = ScriptValueSerializerTag::Int8Array;
        } else if (arrayBufferView->isUint16ArrayObject()) {
            typeTag = ScriptValueSerializerTag::Uint16Array;
        } else if (arrayBufferView->isInt16ArrayObject()) {
            typeTag = ScriptValueSerializerTag::Int16Array;
        } else if (arrayBufferView->isUint32ArrayObject()) {
            typeTag = ScriptValueSerializerTag::Uint32Array;
        } else if (arrayBufferView->isInt32ArrayObject()) {
            typeTag = ScriptValueSerializerTag::Int32Array;
        } else {
            STARFISH_UNIMPLEMENTED();
            throw new DOMException(m_executionContext,
                                   DOMException::DATA_CLONE_ERR);
        }

        writeTag(typeTag);
        m_writer->write<size_t>(arrayBufferView->byteOffset());
        m_writer->write<size_t>(arrayBufferView->arrayLength());
    }

    void writeObject(ScriptObject object)
    {
        writeTag(ScriptValueSerializerTag::BeginObject);

        auto result = Escargot::Evaluator::execute(
            m_executionContext->scriptBindingInstance()->scriptContext(),
            [](ScriptExecutionState state, StructuredSerialize* self,
               ScriptObject object) -> ScriptValue {
                Escargot::ValueVectorRef* keys = object->ownPropertyKeys(state);

                for (size_t i = 0; i < keys->size(); i++) {
                    ScriptValue key = keys->at(i);
                    if (key->isString() && object->hasOwnProperty(state, key)) {
                        auto property = object->getOwnProperty(state, key);
                        self->writeString(key->asString());
                        self->serializeScriptValue(property);
                    }
                }
                return scriptUndefined();
            },
            this, object);

        if (!result.isSuccessful()) {
            throw new DOMException(m_executionContext,
                                   DOMException::DATA_CLONE_ERR,
                                   "Failed to serialize an object.");
            return;
        }

        writeTag(ScriptValueSerializerTag::EndObject);
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

    void deserialize(ScriptValue& scriptValue)
    {
        deserializeScriptValue(scriptValue);
    }

private:
    ExecutionContext* m_executionContext;
    MemorySerializeReader m_reader;

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
        } else if (tag == ScriptValueSerializerTag::BeginObject) {
            readObject(scriptValue);
        } else if (tag == ScriptValueSerializerTag::BeginArrayObject) {
            readArrayObject(scriptValue);
        } else if (tag == ScriptValueSerializerTag::ArrayBuffer) {
            readArrayBufferObject(scriptValue);

            if (isMatchingTag(ScriptValueSerializerTag::ArrayBufferView)) {
                auto arrayBufferScriptValue =
                    scriptValue->asArrayBufferObject();
                readTag(tag);
                readArrayBufferView(scriptValue, arrayBufferScriptValue);
            }
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

    bool isMatchingTag(ScriptValueSerializerTag tag)
    {
        return m_reader.isMatchingValue(static_cast<uint8_t>(tag));
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

    void readArrayObject(ScriptValue& scriptValue)
    {
        uint64_t length = 0;
        m_reader.read<uint64_t>(length);

        ValueVectorRef* elements = ValueVectorRef::create();

        for (size_t i = 0; i < length; i++) {
            ScriptValue value;
            deserializeScriptValue(value);
            elements->pushBack(value);
        }

        auto result = Evaluator::execute(
            m_executionContext->scriptBindingInstance()->scriptContext(),
            [](ScriptExecutionState state,
               ValueVectorRef* elements) -> ScriptValue {
                return ArrayObjectRef::create(state, elements);
            },
            elements);

        if (!result.isSuccessful()) {
            throw new DOMException(m_executionContext,
                                   DOMException::DATA_CLONE_ERR,
                                   "Failed to deserialize an array object.");
        }

        scriptValue = result.result;

        ScriptValueSerializerTag tag;
        readTag(tag);
        if (tag != ScriptValueSerializerTag::EndArrayObject) {
            throw new DOMException(m_executionContext,
                                   DOMException::DATA_CLONE_ERR,
                                   "Failed to deserialize an array object.");
        }

        checkError();
    }

    void readArrayBufferObject(ScriptValue& scriptValue)
    {
        size_t byteLength = 0;
        char* bytes = nullptr;

        m_reader.read<size_t>(byteLength);
        m_reader.readRawBytes(byteLength, bytes);
        checkError();

        auto scriptArrayBuffer = createScriptArrayBuffer(
            m_executionContext->scriptBindingInstance(), byteLength);
        memcpy(scriptArrayBuffer->rawBuffer(), bytes, byteLength);

        scriptValue = scriptArrayBuffer;
    }

    static size_t getByteSize(ScriptValueSerializerTag tag)
    {
        switch (tag) {
        case ScriptValueSerializerTag::Int8Array:
            return sizeof(int8_t);
        case ScriptValueSerializerTag::Uint8Array:
            return sizeof(uint8_t);
        case ScriptValueSerializerTag::Int16Array:
            return sizeof(int16_t);
        case ScriptValueSerializerTag::Uint16Array:
            return sizeof(uint16_t);
        case ScriptValueSerializerTag::Int32Array:
            return sizeof(int32_t);
        case ScriptValueSerializerTag::Uint32Array:
            return sizeof(uint32_t);
        default:
            break;
        }

        return 0;
    }

    void readArrayBufferView(ScriptValue& scriptValue,
                             ScriptArrayBuffer arrayBuffer)
    {
        ScriptValueSerializerTag typeTag;
        size_t byteOffset = 0;
        size_t arrayLength = 0;

        readTag(typeTag);
        m_reader.read<size_t>(byteOffset);
        m_reader.read<size_t>(arrayLength);
        checkError();

        ScriptArrayBufferView arrayBufferView;
        if (typeTag == ScriptValueSerializerTag::Uint8Array) {
            arrayBufferView = createEmptyUint8Array(
                m_executionContext->scriptBindingInstance());
        } else if (typeTag == ScriptValueSerializerTag::Int8Array) {
            arrayBufferView = createEmptyInt8Array(
                m_executionContext->scriptBindingInstance());
        } else if (typeTag == ScriptValueSerializerTag::Uint16Array) {
            arrayBufferView = createEmptyUint16Array(
                m_executionContext->scriptBindingInstance());
        } else if (typeTag == ScriptValueSerializerTag::Int16Array) {
            arrayBufferView = createEmptyInt16Array(
                m_executionContext->scriptBindingInstance());
        } else if (typeTag == ScriptValueSerializerTag::Uint32Array) {
            arrayBufferView = createEmptyUint32Array(
                m_executionContext->scriptBindingInstance());
        } else if (typeTag == ScriptValueSerializerTag::Int32Array) {
            arrayBufferView = createEmptyInt32Array(
                m_executionContext->scriptBindingInstance());
        } else {
            STARFISH_UNIMPLEMENTED();
            throw new DOMException(m_executionContext,
                                   DOMException::DATA_CLONE_ERR);
        }

        size_t byteSize = getByteSize(typeTag);
        if (byteSize == 0) {
            throw new DOMException(
                m_executionContext, DOMException::DATA_CLONE_ERR,
                "Failed to deserialize an array buffer view.");
        }

        arrayBufferView->setBuffer(arrayBuffer, byteOffset,
                                   byteSize * arrayLength, arrayLength);

        scriptValue = arrayBufferView;
    }

    void readObject(ScriptValue& scriptValue)
    {
        auto result = Escargot::Evaluator::execute(
            m_executionContext->scriptBindingInstance()->scriptContext(),
            [](ScriptExecutionState state) -> ScriptValue {
                return Escargot::ObjectRef::create(state);
            });

        if (!result.isSuccessful()) {
            throw new DOMException(m_executionContext,
                                   DOMException::DATA_CLONE_ERR,
                                   "Failed to deserialize an object.");
        }
        scriptValue = result.result;

        while (!isMatchingTag(ScriptValueSerializerTag::EndObject)) {
            ScriptValue keyValue;
            deserializeScriptValue(keyValue);

            ScriptValue propertyValue;
            deserializeScriptValue(propertyValue);

            auto result = Escargot::Evaluator::execute(
                m_executionContext->scriptBindingInstance()->scriptContext(),
                [](ScriptExecutionState state, ScriptObject object,
                   ScriptValue keyValue,
                   ScriptValue propertyValue) -> ScriptValue {
                    object->defineDataProperty(state, keyValue, propertyValue,
                                               true, true, true);
                    return scriptUndefined();
                },
                scriptValue->asObject(), keyValue, propertyValue);

            if (!result.isSuccessful()) {
                throw new DOMException(m_executionContext,
                                       DOMException::DATA_CLONE_ERR,
                                       "Failed to deserialize an object.");
            }
        }

        ScriptValueSerializerTag tag;
        readTag(tag);
        if (tag != ScriptValueSerializerTag::EndObject) {
            throw new DOMException(m_executionContext,
                                   DOMException::DATA_CLONE_ERR,
                                   "Failed to deserialize an object.");
        }
        checkError();
    }

    void checkError()
    {
        if (m_reader.isError()) {
            throw new DOMException(m_executionContext,
                                   DOMException::DATA_CLONE_ERR);
        }
    }
};

MemorySerializedVectorData::MemorySerializedVectorData()
    : m_vectorData(new GCVector<char>())
{
}

MemorySerializedData::MemorySerializedData(const char* data, size_t size)
    : m_data(data)
    , m_size(size)
{
}

SerializedTypedData* MemorySerializer::serialize(
    ExecutionContext* executionContext, ScriptValue value)
{
    StructuredSerialize serializer(executionContext);
    return serializer.serialize(value);
}

ScriptValue MemorySerializer::deserialize(ExecutionContext* executionContext,
                                          const char* data, size_t size)
{
    StructuredDeserialize deserializer(executionContext, data, size);

    ScriptValue result = scriptNull();
    deserializer.deserialize(result);

    return result;
}

void MemorySerializer::serializeWithTransfer(
    ExecutionContext* executionContext, ScriptValue value,
    const GCVector<ScriptObject>& transferValues,
    SerializeWithTransferResult& result)
{
    StructuredSerialize* serializer = new StructuredSerialize(executionContext);

    result.m_serialized = serializer->serialize(value);
    result.m_deserializer = MemorySerializer::deserializeWithTransfer;
}

void MemorySerializer::deserializeWithTransfer(
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
