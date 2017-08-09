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

#include "StarFishConfig.h"
#include "core/fileapi/Blob.h"
#include "core/page/Serializer.h"

#include <EscargotPublic.h>
using namespace Escargot;

namespace StarFish {

SerializedBlobData::SerializedBlobData(Blob* blob)
    : m_size(blob->size())
    , m_type(blob->type())
    , m_data(blob->data())
    , m_isClosed(blob->isClosed())
    , m_isEntryOfBlobURLStore(blob->isEntryOfBlobURLStore())
{
}

SerializedValue* Serializer::serialize(ExecutionStateRef* state,
                                       ScriptValue value)
{
    uint8_t type = SerializedValue::Undefined;
    SerializedData* data = nullptr;

    if (value->isUndefined()) {
        type = SerializedValue::Undefined;
    } else if (value->isNull()) {
        type = SerializedValue::Null;
    } else if (value->isBoolean()) {
        type = SerializedValue::BooleanPrimitive;
        data = new SerializedPrimitiveData();
        data->asSerializedPrimitiveData()->setBooleanData(value->asBoolean());
    } else if (value->isNumber()) {
        type = SerializedValue::NumberPrimitive;
        data = new SerializedPrimitiveData();
        if (value->isInt32()) {
            data->asSerializedPrimitiveData()->setInt32Data(value->asInt32());
        } else if (value->isUInt32()) {
            data->asSerializedPrimitiveData()->setUint32Data(value->asUint32());
        } else {
            data->asSerializedPrimitiveData()->setNumberData(value->asNumber());
        }
    } else if (value->isString()) {
        type = SerializedValue::StringPrimitive;
        data = new SerializedPrimitiveData();
        data->asSerializedPrimitiveData()->setStringData(value->asString());
    } else if (value->isObject()) {
        ObjectRef* obj = value->asObject();
        if (obj->isArrayObject()) {
            ValueRef* length = obj->getOwnProperty(
                state, ValueRef::create(StringRef::fromASCII("length")));
            data = new SerializedArrayData(length->asUint32());
            deepcopy(state, data, obj);
        } else if (obj->extraData()) {
            ScriptWrappable* scriptWrappable =
                (ScriptWrappable*)(obj->extraData());
            if (scriptWrappable->isSerializable()) {
                if (scriptWrappable->isBlob()) {
                    type = SerializedValue::Blob;
                    data = new SerializedBlobData(scriptWrappable->asBlob());
                } else {
                    STARFISH_ASSERT_NOT_REACHED();
                }
            } else {
                return nullptr;
            }
        } else if (obj->isFunctionObject() || obj->isErrorObject() ||
                   obj->isGlobalObject()) {
            return nullptr;
        }
#if ESCARGOT_ENABLE_PROMISE
        else if (obj->isPromiseObject()) {
            return nullptr;
        }
#endif
        else {
            type = SerializedValue::Object;
            data = new SerializedObjectData();
            deepcopy(state, data, obj);
        }
    }

    return new SerializedValue(type, data);
}

void Serializer::deepcopy(Escargot::ExecutionStateRef* state,
                          SerializedData* dst, Escargot::ObjectRef* src)
{
    // TODO
}

ScriptValue Serializer::deserialize(Document* document,
                                    Escargot::ExecutionStateRef* state,
                                    SerializedValue* value)
{
    if (value->isUndefined()) {
        return ValueRef::createUndefined();
    } else if (value->isNull()) {
        return ValueRef::createNull();
    } else if (value->isBooleanPrimitive()) {
        return ValueRef::create(
            value->data()->asSerializedPrimitiveData()->booleanData());
    } else if (value->isInt32Primitive()) {
        return ValueRef::create(
            value->data()->asSerializedPrimitiveData()->int32Data());
    } else if (value->isUint32Primitive()) {
        return ValueRef::create(
            value->data()->asSerializedPrimitiveData()->uint32Data());
    } else if (value->isNumberPrimitive()) {
        return ValueRef::create(
            value->data()->asSerializedPrimitiveData()->numberData());
    } else if (value->isStringPrimitive()) {
        return ValueRef::create(
            value->data()->asSerializedPrimitiveData()->stringData());
    } else if (value->isArray()) {
        ArrayObjectRef* array = ArrayObjectRef::create(state);
        array->set(
            state, ValueRef::create(StringRef::fromASCII("length")),
            ValueRef::create(value->data()->asSerializedArrayData()->length()));
        deepcopy(state, array, value->data());
        return ValueRef::create(array);
    } else if (value->isObject()) {
        ObjectRef* obj = ObjectRef::create(state);
        deepcopy(state, obj, value->data());
        return ValueRef::create(obj);
    } else {
        if (value->isBlob()) {
            SerializedBlobData* blobData =
                value->data()->asSerializedBlobData();
            Blob* blob = new Blob(document, blobData);
            return blob->scriptValue();
        } else {
            STARFISH_ASSERT_NOT_REACHED();
        }
    }
}

void Serializer::deepcopy(Escargot::ExecutionStateRef* state,
                          Escargot::ObjectRef* dst, SerializedData* src)
{
    // TODO
}
}
