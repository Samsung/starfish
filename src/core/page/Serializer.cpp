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

#include "StarFishConfig.h"
#include <EscargotPublic.h>
using namespace Escargot;
#include "core/dom/DOMException.h"
#include "core/dom/Document.h"
#include "core/page/Serializer.h"

namespace StarFish {

void* SerializedStringData::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SerializedStringData));
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
    STARFISH_ASSERT(size == sizeof(SerializedArrayData));
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
    STARFISH_ASSERT(size == sizeof(SerializedObjectData));
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
    STARFISH_ASSERT(size == sizeof(SerializedTypedData));
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

static SerializedTypedData* serializeInternal(Document* document,
                                              ExecutionStateRef* state,
                                              ScriptValue value,
                                              SerializingMap& memory);
static ScriptValue deserializeInternal(Document* document,
                                       Escargot::ExecutionStateRef* state,
                                       SerializedTypedData* value,
                                       DeserializingMap& memory);

static bool deserializingDeep(Document* document,
                              Escargot::ExecutionStateRef* state,
                              ScriptValue dst, SerializedTypedData* src,
                              DeserializingMap& memory)
{
    if (src->isArray()) {
        ScriptObject arrayobj = dst->asObject();
        SerializedArrayData* serializedArray =
            src->data()->asSerializedArrayData();
        size_t len = serializedArray->length();
        for (size_t i = 0; i < len; i++) {
            SerializedTypedData* serialized = (*serializedArray)[i];
            ScriptValue deserialized =
                deserializeInternal(document, state, serialized, memory);
            if (!deserialized) {
                return false;
            }
            arrayobj->defineDataProperty(
                state, ValueRef::create(ValueRef::create(i)->toString(state)),
                deserialized, true, true, true);
        }
    } else if (src->isObject()) {
        ScriptObject obj = dst->asObject();
        SerializedObjectData* serializedObject =
            src->data()->asSerializedObjectData();
        size_t len = serializedObject->length();
        for (size_t i = 0; i < len; i++) {
            auto& propertyAndValue = serializedObject->keyAndValue(i);
            ScriptValue deserialized = deserializeInternal(
                document, state, propertyAndValue.second, memory);
            if (!deserialized) {
                return false;
            }
            obj->defineDataProperty(state, propertyAndValue.first, deserialized,
                                    true, true, true);
        }
    } else if (src->isPlatformObject()) {
        ScriptWrappable* sw = (ScriptWrappable*)(dst->asObject()->extraData());
        STARFISH_ASSERT(sw->isSerializable());
        sw->toSerializable()->deserialize(src->data(), memory);
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }
    return true;
}

static bool serializingDeep(Document* document,
                            Escargot::ExecutionStateRef* state,
                            SerializedTypedData* dst, ScriptValue src,
                            SerializingMap& memory)
{
    if (dst->isArray()) {
        ScriptObject arrayobj = src->asObject();
        SerializedArrayData* serializedArray =
            dst->data()->asSerializedArrayData();
        for (size_t i = 0; i < serializedArray->length(); i++) {
            ValueRef* key = ValueRef::create(i);
            if (arrayobj->hasOwnProperty(state, key)) {
                SerializedTypedData* serialized = serializeInternal(
                    document, state, arrayobj->get(state, key), memory);
                if (!serialized) {
                    return false;
                }
                serializedArray->insert(i, serialized);
            }
        }
    } else if (dst->isObject()) {
        ScriptObject obj = src->asObject();
        ValueVectorRef* values = obj->getOwnPropertyKeys(state);
        SerializedObjectData* serializedObject =
            dst->data()->asSerializedObjectData();
        for (size_t i = 0; i < values->size(); i++) {
            ValueRef* key = values->at(i);
            if (key->isString() && obj->hasOwnProperty(state, key)) {
                SerializedTypedData* serialized = serializeInternal(
                    document, state, obj->get(state, key), memory);
                if (!serialized) {
                    return false;
                }
                serializedObject->setKeyAndValue(key, serialized);
            }
        }
    } else if (dst->isPlatformObject()) {
        ScriptWrappable* sw = (ScriptWrappable*)(src->asObject()->extraData());
        STARFISH_ASSERT(sw->isSerializable());
        SerializedData* result = sw->toSerializable()->serialize(memory);
        dst->setPlatformObjectData(result);
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }
    return true;
}

static SerializedTypedData* serializeInternal(Document* document,
                                              ExecutionStateRef* state,
                                              ScriptValue value,
                                              SerializingMap& memory)
{
    auto checkCycle = memory.find(value);
    if (checkCycle != memory.end()) {
        return checkCycle->second;
    }
    uint8_t type = SerializedTypedData::Undefined;
    SerializedData* data = nullptr;
    bool deep = false;
    bool primitive = true;

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
        primitive = false;
        ScriptObject obj = value->asObject();
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
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        } else if (obj->isArrayObject()) {
            type = SerializedTypedData::Array;
            ValueRef* length = obj->getOwnProperty(
                state, ValueRef::create(StringRef::fromASCII("length")));
            data = new SerializedArrayData(length->asUint32());
            deep = true;
        } else if (obj->extraData()) {
            ScriptWrappable* scriptWrappable =
                (ScriptWrappable*)(obj->extraData());
            if (scriptWrappable->isSerializable()) {
                if (scriptWrappable->isTransferable() &&
                    scriptWrappable->toTransferable()->isDetached()) {
                    return nullptr;
                }
                type = SerializedTypedData::PlatformObject;
                deep = true;
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
#ifdef ESCARGOT_ENABLE_TYPEDARRAY
        else if (obj->isArrayBufferObject()) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        } else if (obj->isArrayBufferView()) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }
#endif
        else {
            type = SerializedTypedData::Object;
            data = new SerializedObjectData();
            deep = true;
        }
    }

    SerializedTypedData* serialized = new SerializedTypedData(type, data);
    if (!primitive) {
        memory.insert(std::make_pair(value, serialized));
    }
    if (deep && !serializingDeep(document, state, serialized, value, memory)) {
        return nullptr;
    }
    return serialized;
}

static ScriptValue deserializeInternal(Document* document,
                                       Escargot::ExecutionStateRef* state,
                                       SerializedTypedData* value,
                                       DeserializingMap& memory)
{
    auto checkCycle = memory.find(value);
    if (checkCycle != memory.end()) {
        return checkCycle->second;
    }

    // NOTE result of nullptr indicates error
    ScriptValue result = nullptr;
    bool deep = false;

    if (value->isTransferedTypedData()) {
        TransferedTypedData* transfered = value->asTransferedTypedData();
        STARFISH_ASSERT(!transfered->isTransferConsumed());
        transfered->setTransferConsumed();
        if (transfered->isPlatformObject()) {
            TransferedData* data = transfered->data();
            STARFISH_ASSERT(data->isTransferedPlatformObjectData());
            ScriptWrappable* sw =
                data->asTransferedPlatformObjectData()
                    ->createTransferReceivingInstance(document);
            STARFISH_ASSERT(sw->isTransferable());
            sw->toTransferable()->transferReceive(data);
            result = sw->scriptValue();
        }
#if ESCARGOT_ENABLE_TYPEDARRAY
        else if (transfered->isArrayBuffer()) {
            // TODO Handle SharedArrayBuffer case (ECMAScript2018)
            STARFISH_ASSERT_NOT_REACHED();
        }
#endif
    } else if (value->isUndefined()) {
        result = ValueRef::createUndefined();
    } else if (value->isNull()) {
        result = ValueRef::createNull();
    } else if (value->isBooleanPrimitive()) {
        result = ValueRef::create(
            value->data()->asSerializedPrimitiveValueData()->booleanData());
    } else if (value->isInt32Primitive()) {
        result = ValueRef::create(
            value->data()->asSerializedPrimitiveValueData()->int32Data());
    } else if (value->isUint32Primitive()) {
        result = ValueRef::create(
            value->data()->asSerializedPrimitiveValueData()->uint32Data());
    } else if (value->isNumberPrimitive()) {
        result = ValueRef::create(
            value->data()->asSerializedPrimitiveValueData()->numberData());
    } else if (value->isStringPrimitive()) {
        result = ValueRef::create(
            value->data()->asSerializedStringData()->stringData());
    } else if (value->isBoolean()) {
        BooleanObjectRef* booleanObj = BooleanObjectRef::create(state);
        booleanObj->setPrimitiveValue(
            state, ValueRef::create(value->data()
                                        ->asSerializedPrimitiveValueData()
                                        ->booleanData()));
        result = ValueRef::create(booleanObj);
    } else if (value->isNumber()) {
        NumberObjectRef* numberObj = NumberObjectRef::create(state);
        numberObj->setPrimitiveValue(
            state,
            ValueRef::create(
                value->data()->asSerializedPrimitiveValueData()->numberData()));
        result = ValueRef::create(numberObj);
    } else if (value->isString()) {
        StringObjectRef* stringObj = StringObjectRef::create(state);
        stringObj->setPrimitiveValue(
            state, ValueRef::create(
                       value->data()->asSerializedStringData()->stringData()));
        result = ValueRef::create(stringObj);
    } else if (value->isDate()) {
        DateObjectRef* dateObj = DateObjectRef::create(state);
        dateObj->setTimeValue(
            state,
            ValueRef::create(
                value->data()->asSerializedPrimitiveValueData()->numberData()));
        result = ValueRef::create(dateObj);
    } else if (value->isRegExp()) {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        result = ValueRef::createUndefined();
    } else if (value->isArray()) {
        ArrayObjectRef* array = ArrayObjectRef::create(state);
        array->set(
            state, ValueRef::create(StringRef::fromASCII("length")),
            ValueRef::create(value->data()->asSerializedArrayData()->length()));
        result = ValueRef::create(array);
        deep = true;
    } else if (value->isObject()) {
        result = ValueRef::create(ObjectRef::create(state));
        deep = true;
    } else if (value->isPlatformObject()) {
        result = value->data()
                     ->asSerializedPlatformObjectData()
                     ->createDeserializingInstance(document)
                     ->scriptValue();
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
    if (result) {
        memory.insert(std::make_pair(value, result));
    }
    if (deep && !deserializingDeep(document, state, result, value, memory)) {
        return nullptr;
    }
    return result;
}

SerializedTypedData* Serializer::serialize(Document* document,
                                           ScriptValue value)
{
    SerializingMap initialMap;
    return serialize(document, value, initialMap);
}

SerializedTypedData* Serializer::serialize(Document* document,
                                           ScriptValue value,
                                           SerializingMap& memory)
{
    SandBoxRef* sandBox =
        SandBoxRef::create(document->scriptBindingInstance()->scriptContext());
    SerializedTypedData* data = nullptr;
    auto result = sandBox->run([&](ExecutionStateRef* state) -> ValueRef* {
        data = serializeInternal(document, state, value, memory);
        return ValueRef::createNull();
    });
    sandBox->destroy();

    if (result.error->isEmpty() && data) {
        return data;
    } else {
        COMPOSE_MESSAGE(reason, INVALID_DATA_CLONE,
                        result.msgStr->toStdUTF8String().data());
        throw new DOMException(document, DOMException::DATA_CLONE_ERR, reason);
    }
}

ScriptValue Serializer::deserialize(Document* document,
                                    SerializedTypedData* value)
{
    DeserializingMap initialMap;
    return deserialize(document, value, initialMap);
}

ScriptValue Serializer::deserialize(Document* document,
                                    SerializedTypedData* value,
                                    DeserializingMap& memory)
{
    SandBoxRef* sandBox =
        SandBoxRef::create(document->scriptBindingInstance()->scriptContext());
    ScriptValue data = ValueRef::createUndefined();
    auto result = sandBox->run([&](ExecutionStateRef* state) -> ValueRef* {
        data = deserializeInternal(document, state, value, memory);
        return ValueRef::createNull();
    });
    sandBox->destroy();

    if (result.error->isEmpty() && data) {
        return data;
    } else {
        COMPOSE_MESSAGE(reason, INVALID_DATA_CLONE,
                        result.msgStr->toStdUTF8String().data());
        throw new DOMException(document, DOMException::DATA_CLONE_ERR, reason);
    }
}

void Serializer::serializeWithTransfer(Document* document, ScriptValue value,
                                       GCVector<ScriptValue>& transferValues,
                                       SerializeWithTransferResult& result)
{
    STARFISH_ASSERT(result.m_serializedTransfer.size() == 0);
    SerializingMap initialMap;
    for (size_t i = 0; i < transferValues.size(); i++) {
        ScriptValue item = transferValues[i];
        if (item->isObject()) {
            if (item->asObject()->extraData()) {
                ScriptWrappable* sw =
                    (ScriptWrappable*)(item->asObject()->extraData());
                if (sw->isTransferable() &&
                    !sw->toTransferable()->isDetached()) {
                    TransferedTypedData* placeHolder = new TransferedTypedData(
                        TransferedTypedData::PlatformObject);
                    initialMap.insert(std::make_pair(item, placeHolder));
                    result.m_serializedTransfer.push_back(placeHolder);
                    continue;
                }
            }
#if ESCARGOT_ENABLE_TYPEDARRAY
            else if (item->asObject()->isArrayBufferObject()) {
                // TODO Handle SharedArrayBuffer case (ECMAScript2018)
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            }
#endif
        }
        throw new DOMException(document, DOMException::DATA_CLONE_ERR);
    }
    SerializedTypedData* serialized = serialize(document, value, initialMap);
    STARFISH_ASSERT(transferValues.size() ==
                    result.m_serializedTransfer.size());
    for (size_t i = 0; i < transferValues.size(); i++) {
        ScriptValue item = transferValues[i];
        TransferedTypedData* placeHolder = result.m_serializedTransfer[i];
        if (placeHolder->isPlatformObject()) {
            STARFISH_ASSERT(item->isObject() && item->asObject()->extraData());
            Transferable* tf =
                ((ScriptWrappable*)(item->asObject()->extraData()))
                    ->toTransferable();
            TransferedData* dataHolder = tf->transfer();
            tf->setDetached();
            placeHolder->setPlatformObjectData(dataHolder);
        }
#if ESCARGOT_ENABLE_TYPEDARRAY
        else if (placeHolder->isArrayBuffer()) {
            // TODO Handle SharedArrayBuffer case (ECMAScript2018)
            STARFISH_ASSERT_NOT_REACHED();
        }
#endif
    }
    result.m_serialized = serialized;
}

void Serializer::deserializeWithTransfer(
    Document* document, SerializeWithTransferResult& serialized,
    DeserializeWithTransferResult& result)
{
    STARFISH_ASSERT(result.m_deserializedTransfer.size() == 0);
    DeserializingMap initialMap;
    ScriptValue deserialized = nullptr;
    SandBoxRef* sandBox =
        SandBoxRef::create(document->scriptBindingInstance()->scriptContext());
    bool errorFound = false;
    auto sandBoxResult =
        sandBox->run([&](ExecutionStateRef* state) -> ValueRef* {
            deserialized = deserializeInternal(
                document, state, serialized.m_serialized, initialMap);
            if (deserialized) {
                for (size_t i = 0; i < serialized.m_serializedTransfer.size();
                     i++) {
                    ScriptValue v = deserializeInternal(
                        document, state, serialized.m_serializedTransfer[i],
                        initialMap);
                    if (v) {
                        result.m_deserializedTransfer.push_back(v);
                    } else {
                        errorFound = true;
                        break;
                    }
                }
            } else {
                errorFound = true;
            }
            return ValueRef::createNull();
        });
    sandBox->destroy();

    if (sandBoxResult.error->isEmpty() && !errorFound) {
        result.m_deserialized = deserialized;
    } else {
        throw new DOMException(document, DOMException::DATA_CLONE_ERR);
    }
}
}
