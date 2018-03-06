/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishSerializer__
#define __StarFishSerializer__

#include "binding/ScriptWrappable.h"

// https://html.spec.whatwg.org/multipage/structured-data.html#safe-passing-of-structured-data

namespace StarFish {

using namespace Escargot;

class Blob;
class Document;
class SerializedData;
class SerializedPrimitiveValueData;
class SerializedStringData;
class SerializedArrayData;
class SerializedPlatformObjectData;
class SerializedObjectData;
class SerializedTypedData;
class TransferedPlatformObjectData;
class TransferedTypedData;
class ScriptWrappable;

typedef GCUnorderedMap<void*, SerializedTypedData*> SerializingMap;
typedef GCUnorderedMap<void*, ScriptValue> DeserializingMap;
class Serializable {
public:
    virtual SerializedData* serialize(SerializingMap& memory) = 0;
    virtual void deserialize(SerializedData* serialized,
                             DeserializingMap& memory) const = 0;
};

class SerializedData : public gc {
public:
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
    SerializedStringData(StringRef* data)
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
        Document* document) const = 0;
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

    void setKeyAndValue(ScriptValue key, SerializedTypedData* value)
    {
        m_data.emplace_back(key, value);
    }

    const std::pair<ScriptValue, SerializedTypedData*>& keyAndValue(
        size_t idx) const
    {
        return m_data[idx];
    }

    size_t length() const
    {
        return m_data.size();
    }

private:
    GCVector<std::pair<ScriptValue, SerializedTypedData*>> m_data;
};

class SerializedTypedData : public gc {
public:
    SerializedTypedData(uint8_t type, SerializedData* data)
        : m_type(type)
        , m_data(data)
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
#if ESCARGOT_ENABLE_TYPEDARRAY
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
#endif
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
#if ESCARGOT_ENABLE_TYPEDARRAY
        SharedArrayBuffer,
        ArrayBuffer,
        ArrayBufferView,
#endif
        Map,
        Set,
        Array,
        PlatformObject,
        Object,
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
        Document* document) const = 0;
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

protected:
    bool m_transferConsumed;
};

class SerializeWithTransferResult : public gc {
public:
    SerializedTypedData* m_serialized;
    GCVector<TransferedTypedData*> m_serializedTransfer;
};

class DeserializeWithTransferResult : public gc {
public:
    ScriptValue m_deserialized;
    GCVector<ScriptValue> m_deserializedTransfer;
};

class Serializer {
public:
    static SerializedTypedData* serialize(Document* document, ScriptValue value,
                                          SerializingMap& memory);
    static SerializedTypedData* serialize(Document* document,
                                          ScriptValue value);
    static ScriptValue deserialize(Document* document,
                                   SerializedTypedData* value,
                                   DeserializingMap& memory);
    static ScriptValue deserialize(Document* document,
                                   SerializedTypedData* value);
    static void serializeWithTransfer(Document* document, ScriptValue value,
                                      GCVector<ScriptValue>& transferValues,
                                      SerializeWithTransferResult& result);
    static void deserializeWithTransfer(Document* document,
                                        SerializeWithTransferResult& serialized,
                                        DeserializeWithTransferResult& result);
};
}

#endif
