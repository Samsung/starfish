/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishSerializer__
#define __StarfishSerializer__

#include "binding/ScriptWrappable.h"

// https://html.spec.whatwg.org/multipage/structured-data.html#safe-passing-of-structured-data

namespace Starfish {

using namespace Escargot;

class Blob;
class ExecutionContext;
class SerializedData;
class SerializedPrimitiveValueData;
class SerializedStringData;
class SerializedArrayData;
class SerializedPlatformObjectData;
class SerializedObjectData;
class SerializedTypedData;
class SerializedArrayBufferData;
class SerializedArrayBufferViewData;
class SerializedRawScriptValueData;
class SerializeWithTransferResult;
class DeserializeWithTransferResult;
class TransferedPlatformObjectData;
class TransferedTypedData;

typedef GCUnorderedMap<void*, SerializedTypedData*> SerializingMap;
typedef GCUnorderedMap<void*, ScriptValue> DeserializingMap;

using ScriptValueSerializer = void (*)(ExecutionContext*, ScriptValue,
                                       const GCAtomicVector<ScriptObject>&,
                                       SerializeWithTransferResult&);

using ScriptValueDeserializer = void (*)(ExecutionContext*,
                                         SerializeWithTransferResult&,
                                         DeserializeWithTransferResult&);

class Serializable {
public:
    virtual ~Serializable()
    {
    }
    virtual SerializedData* serialize(SerializingMap& memory) = 0;
    virtual void deserialize(SerializedData* serialized,
                             DeserializingMap& memory) const = 0;
};

class SerializedData : public gc {
public:
    virtual ~SerializedData()
    {
    }
    virtual bool isSerializedValueData() const
    {
        return false;
    }

    virtual bool isSerializedStringData() const
    {
        return false;
    }

    virtual bool isSerializedArrayData() const
    {
        return false;
    }

    virtual bool isSerializedPlatformObjectData() const
    {
        return false;
    }

    virtual bool isSerializedObjectData() const
    {
        return false;
    }

    virtual bool isTransferedPlatformObjectData() const
    {
        return false;
    }

    virtual bool isSerializedArrayBufferData() const
    {
        return false;
    }

    virtual bool isSerializedArrayBufferViewData() const
    {
        return false;
    }

    virtual bool isSerializedRawScriptValueData() const
    {
        return false;
    }

    SerializedPrimitiveValueData* asSerializedPrimitiveValueData() const
    {
        STARFISH_ASSERT(isSerializedValueData());
        return (SerializedPrimitiveValueData*)this;
    }

    SerializedStringData* asSerializedStringData() const
    {
        STARFISH_ASSERT(isSerializedStringData());
        return (SerializedStringData*)this;
    }

    SerializedArrayData* asSerializedArrayData() const
    {
        STARFISH_ASSERT(isSerializedArrayData());
        return (SerializedArrayData*)this;
    }

    SerializedPlatformObjectData* asSerializedPlatformObjectData() const
    {
        STARFISH_ASSERT(isSerializedPlatformObjectData());
        return (SerializedPlatformObjectData*)this;
    }

    SerializedObjectData* asSerializedObjectData() const
    {
        STARFISH_ASSERT(isSerializedObjectData());
        return (SerializedObjectData*)this;
    }

    TransferedPlatformObjectData* asTransferedPlatformObjectData() const
    {
        STARFISH_ASSERT(isTransferedPlatformObjectData());
        return (TransferedPlatformObjectData*)this;
    }

    SerializedArrayBufferData* asArrayBufferData() const
    {
        STARFISH_ASSERT(isSerializedArrayBufferData());
        return (SerializedArrayBufferData*)this;
    }

    SerializedArrayBufferViewData* asArrayBufferViewData() const
    {
        STARFISH_ASSERT(isSerializedArrayBufferViewData());
        return (SerializedArrayBufferViewData*)this;
    }

    SerializedRawScriptValueData* asSerializedRawScriptValueData() const
    {
        STARFISH_ASSERT(isSerializedRawScriptValueData());
        return (SerializedRawScriptValueData*)this;
    }
};

class SerializedPrimitiveValueData : public SerializedData {
public:
    SerializedPrimitiveValueData(bool booleanData)
    {
        setBooleanData(booleanData);
    }

    SerializedPrimitiveValueData(int32_t int32Data)
    {
        setInt32Data(int32Data);
    }

    SerializedPrimitiveValueData(uint32_t uint32Data)
    {
        setUint32Data(uint32Data);
    }

    SerializedPrimitiveValueData(double numberData)
    {
        setNumberData(numberData);
    }

    bool isSerializedValueData() const override
    {
        return true;
    }

    bool booleanData() const
    {
        return m_data.m_booleanData;
    }

    void setBooleanData(bool booleanData)
    {
        m_data.m_booleanData = booleanData;
    }

    int32_t int32Data() const
    {
        return m_data.m_int32Data;
    }

    void setInt32Data(int32_t int32Data)
    {
        m_data.m_int32Data = int32Data;
    }

    uint32_t uint32Data() const
    {
        return m_data.m_uint32Data;
    }

    void setUint32Data(uint32_t uint32Data)
    {
        m_data.m_uint32Data = uint32Data;
    }

    double numberData() const
    {
        return m_data.m_numberData;
    }

    void setNumberData(double numberData)
    {
        m_data.m_numberData = numberData;
    }

private:
    union Data {
        bool m_booleanData;
        int32_t m_int32Data;
        uint32_t m_uint32Data;
        double m_numberData;
        ScriptString m_stringData;
    };

    Data m_data;
};

class SerializedStringData : public SerializedData {
public:
    SerializedStringData(ScriptString data)
        : m_data(data)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    bool isSerializedStringData() const override
    {
        return true;
    }

    ScriptString stringData() const
    {
        return m_data;
    }

    void setStringData(ScriptString data)
    {
        m_data = data;
    }

private:
    ScriptString m_data;
};

class SerializedArrayData : public SerializedData {
public:
    SerializedArrayData(size_t len)
    {
        m_data.resize(len);
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    bool isSerializedArrayData() const override
    {
        return true;
    }

    void insert(size_t key, SerializedTypedData* value)
    {
        m_data[key] = value;
    }

    SerializedTypedData*& operator[](size_t key)
    {
        return m_data[key];
    }

    size_t length() const
    {
        return m_data.size();
    }

private:
    GCVector<SerializedTypedData*> m_data;
};

class SerializedPlatformObjectData : public SerializedData {
public:
    SerializedPlatformObjectData()
    {
    }

    bool isSerializedPlatformObjectData() const override
    {
        return true;
    }

    virtual ScriptWrappable* createDeserializingInstance(
        ExecutionContext* executionContext) const = 0;
};

class SerializedObjectData : public SerializedData {
public:
    SerializedObjectData()
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    bool isSerializedObjectData() const override
    {
        return true;
    }

    void setKeyAndValue(std::string&& key, SerializedTypedData* value)
    {
        m_data.emplace_back(std::move(key), value);
    }

    const std::pair<std::string, SerializedTypedData*>& keyAndValue(
        size_t idx) const
    {
        return m_data[idx];
    }

    size_t length() const
    {
        return m_data.size();
    }

private:
    GCVector<std::pair<std::string, SerializedTypedData*>> m_data;
};

class SerializedArrayBufferData : public SerializedData {
public:
    SerializedArrayBufferData(ExecutionContext* executionContext,
                              ScriptArrayBuffer arrayBuffer);
    SerializedArrayBufferData(uint8_t* buffer, size_t byteLength);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual bool isSerializedArrayBufferData() const override
    {
        return true;
    }

    ScriptArrayBuffer createDeserializedValue(ScriptExecutionState state);

private:
    GCVector<uint8_t> m_data;
    size_t m_byteLength{ 0 };
};

class SerializedArrayBufferViewData : public SerializedData {
public:
    enum Type {
        None,
        Int8Array,
        Uint8Array,
        Int16Array,
        Uint16Array,
        Int32Array,
        Uint32Array
    };

    SerializedArrayBufferViewData(ExecutionContext* executionContext,
                                  ScriptArrayBufferView arrayBufferView);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual bool isSerializedArrayBufferViewData() const override
    {
        return true;
    }

    ScriptArrayBufferView createDeserializedValue(
        ExecutionContext* executionContext, ScriptExecutionState state);

protected:
    SerializedArrayBufferData* m_arrayBufferData{ nullptr };
    Type m_type{ Type::None };
    size_t m_byteLength{ 0 };
    size_t m_byteOffset{ 0 };
    size_t m_arrayLength{ 0 };
};

// ScriptValue is stored in a char buffer. This is used when transmitting
// ScriptValue via IPC.
class SerializedRawScriptValueDataInternal : public gc {
public:
    virtual const char* data() const = 0;
    virtual size_t size() const = 0;
};

class SerializedRawScriptValueData : public SerializedData {
public:
    SerializedRawScriptValueData(
        SerializedRawScriptValueDataInternal* internal);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    bool isSerializedRawScriptValueData() const override
    {
        return true;
    }

    SerializedRawScriptValueDataInternal* internal()
    {
        return m_internal;
    }

protected:
    SerializedRawScriptValueDataInternal* m_internal;
};

class SerializedTypedData : public gc {
public:
    SerializedTypedData(uint8_t type, SerializedData* data)
        : m_type(type)
        , m_data(data)
    {
    }
    virtual ~SerializedTypedData()
    {
    }

    virtual bool isTransferedTypedData() const
    {
        return false;
    }

    TransferedTypedData* asTransferedTypedData() const
    {
        STARFISH_ASSERT(isTransferedTypedData());
        return (TransferedTypedData*)this;
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    bool isUndefined() const
    {
        return m_type == Undefined;
    }

    bool isNull() const
    {
        return m_type == Null;
    }

    bool isBooleanPrimitive() const
    {
        return m_type == BooleanPrimitive;
    }

    bool isInt32Primitive() const
    {
        return m_type == Int32Primitive;
    }

    bool isUint32Primitive() const
    {
        return m_type == Uint32Primitive;
    }

    bool isNumberPrimitive() const
    {
        return m_type == NumberPrimitive;
    }

    bool isStringPrimitive() const
    {
        return m_type == StringPrimitive;
    }

    bool isBoolean() const
    {
        return m_type == Boolean;
    }

    bool isNumber() const
    {
        return m_type == Number;
    }

    bool isString() const
    {
        return m_type == String;
    }

    bool isDate() const
    {
        return m_type == Date;
    }

    bool isRegExp() const
    {
        return m_type == RegExp;
    }
    bool isSharedArrayBuffer() const
    {
        return m_type == SharedArrayBuffer;
    }

    bool isArrayBuffer() const
    {
        return m_type == ArrayBuffer;
    }

    bool isArrayBufferView() const
    {
        return m_type == ArrayBufferView;
    }
    bool isMap() const
    {
        return m_type == Map;
    }

    bool isSet() const
    {
        return m_type == Set;
    }

    bool isArray() const
    {
        return m_type == Array;
    }

    bool isPlatformObject() const
    {
        return m_type == PlatformObject;
    }

    bool isObject() const
    {
        return m_type == Object;
    }

    bool isRawScriptValue() const
    {
        return m_type == RawScriptValue;
    }

    SerializedData* data() const
    {
        return m_data;
    }

    void setPlatformObjectData(SerializedData* data)
    {
        STARFISH_ASSERT(m_type == PlatformObject);
        m_data = data;
    }

    enum Type {
        Undefined,
        Null,
        BooleanPrimitive,
        Int32Primitive,
        Uint32Primitive,
        NumberPrimitive,
        StringPrimitive,
        Boolean,
        Number,
        String,
        Date,
        RegExp,
        SharedArrayBuffer,
        ArrayBuffer,
        ArrayBufferView,
        Map,
        Set,
        Array,
        PlatformObject,
        Object,
        RawScriptValue,
    };

protected:
    uint8_t m_type;
    SerializedData* m_data;
};

typedef SerializedData TransferedData;

class Transferable {
public:
    Transferable()
        : m_detached(false)
    {
    }
    virtual ~Transferable()
    {
    }

    bool isDetached()
    {
        return m_detached;
    }
    void setDetached()
    {
        m_detached = true;
    }
    virtual TransferedData* transfer() = 0;
    virtual void transferReceive(TransferedData* transfered) = 0;

protected:
    bool m_detached;
};

class TransferedPlatformObjectData : public TransferedData {
public:
    TransferedPlatformObjectData()
    {
    }

    bool isTransferedPlatformObjectData() const override
    {
        return true;
    }

    virtual ScriptWrappable* createTransferReceivingInstance(
        ExecutionContext* executionContext) const = 0;
};

class TransferedTypedData : public SerializedTypedData {
public:
    TransferedTypedData(uint8_t type, TransferedData* data)
        : SerializedTypedData(type, data)
        , m_transferConsumed(false)
    {
        STARFISH_ASSERT(type == SharedArrayBuffer || type == PlatformObject);
    }

    TransferedTypedData(uint8_t type)
        : TransferedTypedData(type, nullptr)
    {
    }

    bool isTransferedTypedData() const override
    {
        return true;
    }

    bool isTransferConsumed() const
    {
        return m_transferConsumed;
    }

    void setTransferConsumed()
    {
        m_transferConsumed = true;
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    bool m_transferConsumed;
};

class SerializeWithTransferResult : public gc {
public:
    SerializedTypedData* m_serialized{ nullptr };
    GCVector<TransferedTypedData*> m_serializedTransfer;
    ScriptValueDeserializer m_deserializer{ nullptr };
};

class DeserializeWithTransferResult : public gc {
public:
    ScriptValue m_deserialized;
    GCVector<ScriptValue> m_deserializedTransfer;
};

class Serializer {
public:
    static SerializedTypedData* serialize(ExecutionContext* executionContext,
                                          ScriptValue value,
                                          SerializingMap& memory);
    static SerializedTypedData* serialize(ExecutionContext* executionContext,
                                          ScriptValue value);
    static ScriptValue deserialize(ExecutionContext* executionContext,
                                   SerializedTypedData* value,
                                   DeserializingMap& memory);
    static ScriptValue deserialize(ExecutionContext* executionContext,
                                   SerializedTypedData* value);
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
