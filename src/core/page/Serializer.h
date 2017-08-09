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

#include "StarFishConfig.h"
#include "binding/ScriptWrappable.h"

namespace StarFish {

using namespace Escargot;

class Blob;
class Document;
class SerializedPrimitiveData;
class SerializedArrayData;
class SerializedBlobData;
class SerializedObjectData;
class SerializedValue;

class SerializedData : public gc {
public:
    virtual bool isSerializedPrimitiveData() const
    {
        return false;
    }

    virtual bool isSerializedArrayData() const
    {
        return false;
    }

    virtual bool isSerializedBlobData() const
    {
        return false;
    }

    virtual bool isSerializedObjectData() const
    {
        return false;
    }

    SerializedPrimitiveData* asSerializedPrimitiveData()
    {
        STARFISH_ASSERT(isSerializedPrimitiveData());
        return (SerializedPrimitiveData*)this;
    }

    SerializedArrayData* asSerializedArrayData()
    {
        STARFISH_ASSERT(isSerializedArrayData());
        return (SerializedArrayData*)this;
    }

    SerializedBlobData* asSerializedBlobData()
    {
        STARFISH_ASSERT(isSerializedBlobData());
        return (SerializedBlobData*)this;
    }

    SerializedObjectData* asSerializedObjectData()
    {
        STARFISH_ASSERT(isSerializedObjectData());
        return (SerializedObjectData*)this;
    }
};

class SerializedPrimitiveData : public SerializedData {
public:
    SerializedPrimitiveData()
    {
    }

    bool isSerializedPrimitiveData() const override
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

    Escargot::StringRef* stringData() const
    {
        return m_data.m_stringData;
    }

    void setStringData(Escargot::StringRef* stringData)
    {
        m_data.m_stringData = stringData;
    }

private:
    union Data {
        bool m_booleanData;
        int32_t m_int32Data;
        uint32_t m_uint32Data;
        double m_numberData;
        Escargot::StringRef* m_stringData;
    };

    Data m_data;
};

class SerializedArrayData : public SerializedData {
public:
    SerializedArrayData(size_t len)
    {
        m_data.resize(len);
    }

    bool isSerializedArrayData() const override
    {
        return true;
    }

    void setValue(size_t key, SerializedValue* value)
    {
        m_data[key] = value;
    }

    size_t length() const
    {
        return m_data.size();
    }

private:
    GCVector<SerializedValue*> m_data;
};

class SerializedBlobData : public SerializedData {
public:
    SerializedBlobData(Blob* blob);

    bool isSerializedBlobData() const override
    {
        return true;
    }

    int64_t size() const
    {
        return m_size;
    }

    String* type() const
    {
        return m_type;
    }

    void* data() const
    {
        return m_data;
    }

    bool isClosed() const
    {
        return m_isClosed;
    }

    bool isEntryOfBlobURLStore() const
    {
        return m_isEntryOfBlobURLStore;
    }

private:
    uint64_t m_size;
    String* m_type;
    void* m_data;
    bool m_isClosed;
    bool m_isEntryOfBlobURLStore;
};

class SerializedObjectData : public SerializedData {
public:
    SerializedObjectData()
    {
    }

    bool isSerializedObjectData() const override
    {
        return true;
    }

    void setValue(Escargot::StringRef* key, SerializedValue* value)
    {
        m_data.emplace_back(key, value);
    }

private:
    GCVector<std::pair<StringRef*, SerializedValue*>> m_data;
};

class SerializedValue : public gc {
public:
    SerializedValue(uint8_t type, SerializedData* data)
        : m_type(type)
        , m_data(data)
    {
    }

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

    bool isInt32() const
    {
        return m_type == Int32;
    }

    bool isUint32() const
    {
        return m_type == Uint32;
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

    bool isBlob() const
    {
        return m_type == Blob;
    }

    bool isFile() const
    {
        return m_type == File;
    }

    bool isFileList() const
    {
        return m_type == FileList;
    }

    bool isImageBitmap() const
    {
        return m_type == ImageBitmap;
    }

    bool isImageData() const
    {
        return m_type == ImageData;
    }

    bool isObject() const
    {
        return m_type == Object;
    }

    SerializedData* data() const
    {
        return m_data;
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
        Int32,
        Uint32,
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
        Blob,
        File,
        FileList,
        ImageBitmap,
        ImageData,
        Object,
    };

private:
    uint8_t m_type;
    SerializedData* m_data;
};

class Serializer {
public:
    static SerializedValue* serialize(Escargot::ExecutionStateRef* state,
                                      ScriptValue value);
    static void deepcopy(Escargot::ExecutionStateRef* state,
                         SerializedData* dst, Escargot::ObjectRef* src);
    static ScriptValue deserialize(Document* document,
                                   Escargot::ExecutionStateRef* state,
                                   SerializedValue* value);
    static void deepcopy(Escargot::ExecutionStateRef* state,
                         Escargot::ObjectRef* dst, SerializedData* src);
};
}

#endif
