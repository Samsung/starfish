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
#include <EscargotPublic.h>
#include "core/page/Serializer.h"

namespace StarFish {

void* SerializedStringData::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(SerializedStringData)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(SerializedStringData, m_data));
        descr =
            GC_make_descriptor(obj_bitmap, GC_WORD_LEN(SerializedStringData));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void* SerializedArrayData::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(SerializedArrayData)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(SerializedArrayData, m_data));
        descr =
            GC_make_descriptor(obj_bitmap, GC_WORD_LEN(SerializedArrayData));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void* SerializedObjectData::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(SerializedObjectData)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(SerializedObjectData, m_data));
        descr =
            GC_make_descriptor(obj_bitmap, GC_WORD_LEN(SerializedObjectData));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void* SerializedTypedData::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(SerializedTypedData)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(SerializedTypedData, m_data));
        descr =
            GC_make_descriptor(obj_bitmap, GC_WORD_LEN(SerializedTypedData));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

SerializedTypedData* Serializer::serialize(ExecutionStateRef* state,
                                           ScriptValue value)
{
    uint8_t type = SerializedTypedData::Undefined;
    SerializedData* data = nullptr;

    if (value->isUndefined()) {
        type = SerializedTypedData::Undefined;
    } else if (value->isNull()) {
        type = SerializedTypedData::Null;
    } else if (value->isBoolean()) {
        type = SerializedTypedData::BooleanPrimitive;
        data = new SerializedPrimitiveValueData(value->asBoolean());
    } else if (value->isNumber()) {
        if (value->isInt32()) {
            type = SerializedTypedData::Int32Primitive;
            data = new SerializedPrimitiveValueData(value->asInt32());
        } else if (value->isUInt32()) {
            type = SerializedTypedData::Uint32Primitive;
            data = new SerializedPrimitiveValueData(value->asUint32());
        } else if (value->isNumber()) {
            type = SerializedTypedData::NumberPrimitive;
            data = new SerializedPrimitiveValueData(value->asNumber());
        }
    } else if (value->isString()) {
        type = SerializedTypedData::StringPrimitive;
        data = new SerializedStringData(value->asString());
    } else if (value->isObject()) {
        ObjectRef* obj = value->asObject();
        if (obj->isBooleanObject()) {
            type = SerializedTypedData::Boolean;
            data = new SerializedPrimitiveValueData(
                obj->asBooleanObject()->primitiveValue());
        } else if (obj->isNumberObject()) {
            type = SerializedTypedData::Number;
            data = new SerializedPrimitiveValueData(
                obj->asNumberObject()->primitiveValue());
        } else if (obj->isStringObject()) {
            type = SerializedTypedData::String;
            data = new SerializedStringData(
                obj->asStringObject()->primitiveValue());
        } else if (obj->isDateObject()) {
            type = SerializedTypedData::Date;
            data = new SerializedPrimitiveValueData(
                obj->asDateObject()->primitiveValue());
        } else if (obj->isRegExpObject()) {
            type = SerializedTypedData::RegExp;
            STARFISH_ASSERT_NOT_REACHED();
        } else if (obj->isArrayObject()) {
            type = SerializedTypedData::Array;
            ValueRef* length = obj->getOwnProperty(
                state, ValueRef::create(StringRef::fromASCII("length")));
            data = new SerializedArrayData(length->asUint32());
            deepcopy(state, data, obj);
        } else if (obj->extraData()) {
            ScriptWrappable* scriptWrappable =
                (ScriptWrappable*)(obj->extraData());
            if (scriptWrappable->isSerializable()) {
                type = SerializedTypedData::PlatformObject;
                data = scriptWrappable->toSerializable()->serialized();
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
            type = SerializedTypedData::Object;
            data = new SerializedObjectData();
            deepcopy(state, data, obj);
        }
    }

    return new SerializedTypedData(type, data);
}

void Serializer::deepcopy(Escargot::ExecutionStateRef* state,
                          SerializedData* dst, Escargot::ObjectRef* src)
{
    ValueVectorRef* values = src->getOwnPropertyKeys(state);
    if (dst->isSerializedArrayData()) {
        SerializedArrayData* serializedArray = dst->asSerializedArrayData();
        for (size_t i = 0; i < serializedArray->length(); i++) {
            ValueRef* key = ValueRef::create(i);
            if (src->hasOwnProperty(state, key)) {
                serializedArray->insert(i,
                                        serialize(state, src->get(state, key)));
            }
        }
    } else {
        SerializedObjectData* serializedObject = dst->asSerializedObjectData();
        for (size_t i = 0; i < values->size(); i++) {
            ValueRef* key = values->at(i);
            if (key->isString() && src->hasOwnProperty(state, key)) {
                serializedObject->setKeyAndValue(
                    key, serialize(state, src->get(state, key)));
            }
        }
    }
}

ScriptValue Serializer::deserialize(Document* document,
                                    Escargot::ExecutionStateRef* state,
                                    SerializedTypedData* value)
{
    if (value->isUndefined()) {
        return ValueRef::createUndefined();
    } else if (value->isNull()) {
        return ValueRef::createNull();
    } else if (value->isBooleanPrimitive()) {
        return ValueRef::create(
            value->data()->asSerializedPrimitiveValueData()->booleanData());
    } else if (value->isInt32Primitive()) {
        return ValueRef::create(
            value->data()->asSerializedPrimitiveValueData()->int32Data());
    } else if (value->isUint32Primitive()) {
        return ValueRef::create(
            value->data()->asSerializedPrimitiveValueData()->uint32Data());
    } else if (value->isNumberPrimitive()) {
        return ValueRef::create(
            value->data()->asSerializedPrimitiveValueData()->numberData());
    } else if (value->isStringPrimitive()) {
        return ValueRef::create(
            value->data()->asSerializedStringData()->stringData());
    } else if (value->isBoolean()) {
        BooleanObjectRef* booleanObj = BooleanObjectRef::create(state);
        booleanObj->setPrimitiveValue(
            state, ValueRef::create(value->data()
                                        ->asSerializedPrimitiveValueData()
                                        ->booleanData()));
        return ValueRef::create(booleanObj);
    } else if (value->isNumber()) {
        NumberObjectRef* numberObj = NumberObjectRef::create(state);
        numberObj->setPrimitiveValue(
            state,
            ValueRef::create(
                value->data()->asSerializedPrimitiveValueData()->numberData()));
        return ValueRef::create(numberObj);
    } else if (value->isString()) {
        StringObjectRef* stringObj = StringObjectRef::create(state);
        stringObj->setPrimitiveValue(
            state, ValueRef::create(
                       value->data()->asSerializedStringData()->stringData()));
        return ValueRef::create(stringObj);
    } else if (value->isDate()) {
        DateObjectRef* dateObj = DateObjectRef::create(state);
        dateObj->setTimeValue(
            state,
            ValueRef::create(
                value->data()->asSerializedPrimitiveValueData()->numberData()));
        return ValueRef::create(dateObj);
    } else if (value->isRegExp()) {
        STARFISH_ASSERT_NOT_REACHED();
    } else if (value->isArray()) {
        ArrayObjectRef* array = ArrayObjectRef::create(state);
        array->set(
            state, ValueRef::create(StringRef::fromASCII("length")),
            ValueRef::create(value->data()->asSerializedArrayData()->length()));
        deepcopy(document, state, array, value->data());
        return ValueRef::create(array);
    } else if (value->isObject()) {
        ObjectRef* obj = ObjectRef::create(state);
        deepcopy(document, state, obj, value->data());
        return ValueRef::create(obj);
    } else {
        STARFISH_ASSERT(value->isPlatformObject());
        return value->data()
            ->asSerializedPlatformObjectData()
            ->deserialized(document)
            ->scriptValue();
    }
}

void Serializer::deepcopy(Document* document,
                          Escargot::ExecutionStateRef* state,
                          Escargot::ObjectRef* dst, SerializedData* src)
{
    if (src->isSerializedArrayData()) {
        SerializedArrayData* serializedArray = src->asSerializedArrayData();
        size_t len = serializedArray->length();
        for (size_t i = 0; i < len; i++) {
            SerializedTypedData* serialized = (*serializedArray)[i];
            ValueRef* value = deserialize(document, state, serialized);
            dst->defineDataProperty(
                state, ValueRef::create(ValueRef::create(i)->toString(state)),
                value, true, true, true);
        }
    } else {
        SerializedObjectData* serializedObject = src->asSerializedObjectData();
        size_t len = serializedObject->length();
        for (size_t i = 0; i < len; i++) {
            auto& propertyAndValue = serializedObject->keyAndValue(i);
            ValueRef* value =
                deserialize(document, state, propertyAndValue.second);
            dst->defineDataProperty(state, propertyAndValue.first, value, true,
                                    true, true);
        }
    }
}
}
