/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
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
    /*
        bool isMap() const
        {
            return m_type == Map;
        }

        bool isSet() const
        {
            return m_type == Set;
        }
    */
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
        // Map,
        // Set,
        Array,
        PlatformObject,
        Object,
    };

private:
    uint8_t m_type;
    SerializedData* m_data;
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
};
}

#endif
