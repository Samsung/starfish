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

#include "StarfishConfig.h"
#include <EscargotPublic.h>
using namespace Escargot;
#include "binding/ScriptBindingInstance.h"
#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/serialize/Serializer.h"

namespace Starfish {

void* TransferedTypedData::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(TransferedTypedData));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(TransferedTypedData)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(TransferedTypedData, m_data));
        descr =
            GC_make_descriptor(obj_bitmap, GC_WORD_LEN(TransferedTypedData));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

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

void* SerializedArrayBufferData::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SerializedArrayBufferData));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(SerializedArrayBufferData)] = { 0 };
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(SerializedArrayBufferData, m_data));
        descr = GC_make_descriptor(obj_bitmap,
                                   GC_WORD_LEN(SerializedArrayBufferData));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void* SerializedArrayBufferViewData::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SerializedArrayBufferViewData));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(SerializedArrayBufferViewData)] = {
            0
        };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(SerializedArrayBufferViewData,
                                              m_arrayBufferData));
        descr = GC_make_descriptor(obj_bitmap,
                                   GC_WORD_LEN(SerializedArrayBufferViewData));
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

void* SerializedRawScriptValueData::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SerializedRawScriptValueData));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(SerializedRawScriptValueData)] = {
            0
        };
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(SerializedRawScriptValueData, m_internal));
        descr = GC_make_descriptor(obj_bitmap,
                                   GC_WORD_LEN(SerializedRawScriptValueData));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

SerializedArrayBufferData::SerializedArrayBufferData(
    ExecutionContext* executionContext, ScriptArrayBuffer arrayBuffer)
{
    if (arrayBuffer->isDetachedBuffer()) {
        throw new DOMException(executionContext, DOMException::DATA_CLONE_ERR,
                               "ArrayBuffer is already detached.");
    }

    m_byteLength = arrayBuffer->byteLength();
    uint8_t* buffer = arrayBuffer->rawBuffer();
    m_data.insert(m_data.end(), buffer, buffer + m_byteLength);
}

SerializedArrayBufferData::SerializedArrayBufferData(uint8_t* buffer,
                                                     size_t byteLength)
    : m_byteLength(byteLength)
{
    m_data.insert(m_data.end(), buffer, buffer + m_byteLength);
}

ScriptArrayBuffer SerializedArrayBufferData::createDeserializedValue(
    ScriptExecutionState state)
{
    ArrayBufferObjectRef* arrayBuffer = ArrayBufferObjectRef::create(state);
    arrayBuffer->allocateBuffer(state, m_byteLength);
    memcpy(arrayBuffer->rawBuffer(), m_data.begin(), m_byteLength);

    return arrayBuffer;
}

SerializedArrayBufferViewData::SerializedArrayBufferViewData(
    ExecutionContext* executionContext, ScriptArrayBufferView arrayBufferView)
{
    if (arrayBufferView->isInt8ArrayObject()) {
        m_type = Type::Int8Array;
    } else if (arrayBufferView->isUint8ArrayObject()) {
        m_type = Type::Uint8Array;
    } else if (arrayBufferView->isInt16ArrayObject()) {
        m_type = Type::Int16Array;
    } else if (arrayBufferView->isUint16ArrayObject()) {
        m_type = Type::Uint16Array;
    } else if (arrayBufferView->isInt32ArrayObject()) {
        m_type = Type::Int32Array;
    } else if (arrayBufferView->isUint32ArrayObject()) {
        m_type = Type::Uint32Array;
    } else {
        STARFISH_UNSUPPORTED(
            "serialize: BigInt64Array, BigUint64Array, Float32Array, "
            "Float64Array, Uint8ClampedArray");
        throw new DOMException(executionContext, DOMException::DATA_CLONE_ERR,
                               "Data clone error");
    }

    m_byteLength = arrayBufferView->byteLength();
    m_byteOffset = arrayBufferView->byteOffset();
    m_arrayLength = arrayBufferView->arrayLength();

    m_arrayBufferData =
        new SerializedArrayBufferData(arrayBufferView->buffer()->rawBuffer(),
                                      arrayBufferView->buffer()->byteLength());
}

ScriptArrayBufferView SerializedArrayBufferViewData::createDeserializedValue(
    ExecutionContext* executionContext, ScriptExecutionState state)
{
    ArrayBufferViewRef* arrayBufferView = nullptr;
    if (m_type == Type::Int8Array) {
        arrayBufferView = Int8ArrayObjectRef::create(state);
    } else if (m_type == Type::Uint8Array) {
        arrayBufferView = Uint8ArrayObjectRef::create(state);
    } else if (m_type == Type::Int16Array) {
        arrayBufferView = Int16ArrayObjectRef::create(state);
    } else if (m_type == Type::Uint16Array) {
        arrayBufferView = Uint16ArrayObjectRef::create(state);
    } else if (m_type == Type::Int32Array) {
        arrayBufferView = Int32ArrayObjectRef::create(state);
    } else if (m_type == Type::Uint32Array) {
        arrayBufferView = Uint32ArrayObjectRef::create(state);
    } else {
        STARFISH_UNSUPPORTED(
            "deserialize: BigInt64Array, BigUint64Array, Float32Array, "
            "Float64Array, Uint8ClampedArray");
        throw new DOMException(executionContext, DOMException::DATA_CLONE_ERR,
                               "Data clone error");
    }

    ArrayBufferObjectRef* arrayBuffer =
        m_arrayBufferData->createDeserializedValue(state);

    arrayBufferView->setBuffer(arrayBuffer, m_byteOffset, m_byteLength,
                               m_arrayLength);

    return arrayBufferView;
}

SerializedRawScriptValueData::SerializedRawScriptValueData(
    SerializedRawScriptValueDataInternal* internal)
    : m_internal(internal)
{
}

static SerializedTypedData* serializeInternal(
    ExecutionContext* executionContext, ExecutionStateRef* state,
    ScriptValue value, SerializingMap& memory);
static ScriptValue deserializeInternal(ExecutionContext* executionContext,
                                       Escargot::ExecutionStateRef* state,
                                       SerializedTypedData* value,
                                       DeserializingMap& memory);

static bool deserializingDeep(ExecutionContext* executionContext,
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
            ScriptValue deserialized = deserializeInternal(
                executionContext, state, serialized, memory);
            if (!deserialized) {
                return false;
            }
            arrayobj->defineDataProperty(state,
                                         ValueRef::create(i)->toString(state),
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
                executionContext, state, propertyAndValue.second, memory);
            if (!deserialized) {
                return false;
            }
            const std::string& propertyString = propertyAndValue.first;
            ScriptString property = createScriptString(propertyString.data(),
                                                       propertyString.length());
            obj->defineDataProperty(state, property, deserialized, true, true,
                                    true);
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

static bool serializingDeep(ExecutionContext* executionContext,
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
                    executionContext, state, arrayobj->get(state, key), memory);
                if (!serialized) {
                    return false;
                }
                serializedArray->insert(i, serialized);
            }
        }
    } else if (dst->isObject()) {
        ScriptObject obj = src->asObject();
        ValueVectorRef* values = obj->ownPropertyKeys(state);
        SerializedObjectData* serializedObject =
            dst->data()->asSerializedObjectData();
        for (size_t i = 0; i < values->size(); i++) {
            ValueRef* key = values->at(i);
            if (key->isString() && obj->hasOwnProperty(state, key)) {
                SerializedTypedData* serialized = serializeInternal(
                    executionContext, state, obj->get(state, key), memory);
                if (!serialized) {
                    return false;
                }
                serializedObject->setKeyAndValue(
                    key->asString()->toStdUTF8String(), serialized);
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

static SerializedTypedData* serializeInternal(
    ExecutionContext* executionContext, ExecutionStateRef* state,
    ScriptValue value, SerializingMap& memory)
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
            data = new SerializedPrimitiveValueData(value->asUInt32());
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
            STARFISH_UNSUPPORTED("serialize: RegExpObject");
        } else if (obj->isArrayObject()) {
            type = SerializedTypedData::Array;
            ValueRef* length = obj->getOwnProperty(
                state, StringRef::createFromASCII("length"));
            data = new SerializedArrayData(length->asUInt32());
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
        } else if (obj->isPromiseObject()) {
            return nullptr;
        } else if (obj->isArrayBufferObject()) {
            type = SerializedTypedData::ArrayBuffer;
            data = new SerializedArrayBufferData(executionContext,
                                                 obj->asArrayBufferObject());
        } else if (obj->isArrayBufferView()) {
            type = SerializedTypedData::ArrayBufferView;
            data = new SerializedArrayBufferViewData(executionContext,
                                                     obj->asArrayBufferView());
        } else {
            type = SerializedTypedData::Object;
            data = new SerializedObjectData();
            deep = true;
        }
    }

    SerializedTypedData* serialized = new SerializedTypedData(type, data);
    if (!primitive) {
        memory.insert(std::make_pair(value, serialized));
    }
    if (deep &&
        !serializingDeep(executionContext, state, serialized, value, memory)) {
        return nullptr;
    }
    return serialized;
}

static ScriptValue deserializeInternal(ExecutionContext* executionContext,
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
                    ->createTransferReceivingInstance(executionContext);
            STARFISH_ASSERT(sw->isTransferable());
            sw->toTransferable()->transferReceive(data);
            result = sw->scriptValue();
        } else if (transfered->isArrayBuffer()) {
            // TODO Handle SharedArrayBuffer case (ECMAScript2018)
            STARFISH_ASSERT_NOT_REACHED();
        }
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
        result = value->data()->asSerializedStringData()->stringData();
    } else if (value->isBoolean()) {
        BooleanObjectRef* booleanObj = BooleanObjectRef::create(state);
        booleanObj->setPrimitiveValue(
            state, ValueRef::create(value->data()
                                        ->asSerializedPrimitiveValueData()
                                        ->booleanData()));
        result = booleanObj;
    } else if (value->isNumber()) {
        NumberObjectRef* numberObj = NumberObjectRef::create(state);
        numberObj->setPrimitiveValue(
            state,
            ValueRef::create(
                value->data()->asSerializedPrimitiveValueData()->numberData()));
        result = numberObj;
    } else if (value->isString()) {
        StringObjectRef* stringObj = StringObjectRef::create(state);
        stringObj->setPrimitiveValue(
            state, value->data()->asSerializedStringData()->stringData());
        result = stringObj;
    } else if (value->isDate()) {
        DateObjectRef* dateObj = DateObjectRef::create(state);
        dateObj->setTimeValue(
            state,
            ValueRef::create(
                value->data()->asSerializedPrimitiveValueData()->numberData()));
        result = dateObj;
    } else if (value->isRegExp()) {
        STARFISH_UNSUPPORTED("deserialize: RegExpObject");
        result = ValueRef::createUndefined();
    } else if (value->isArray()) {
        ArrayObjectRef* array = ArrayObjectRef::create(state);
        array->set(
            state, StringRef::createFromASCII("length"),
            ValueRef::create(value->data()->asSerializedArrayData()->length()));
        result = array;
        deep = true;
    } else if (value->isObject()) {
        result = ObjectRef::create(state);
        deep = true;
    } else if (value->isPlatformObject()) {
        result = value->data()
                     ->asSerializedPlatformObjectData()
                     ->createDeserializingInstance(executionContext)
                     ->scriptValue();
    } else if (value->isArrayBuffer()) {
        result =
            value->data()->asArrayBufferData()->createDeserializedValue(state);
    } else if (value->isArrayBufferView()) {
        result =
            value->data()->asArrayBufferViewData()->createDeserializedValue(
                executionContext, state);
    } else {
        STARFISH_UNSUPPORTED("deserialize: unsupported value");
    }
    if (result) {
        memory.insert(std::make_pair(value, result));
    }
    if (deep &&
        !deserializingDeep(executionContext, state, result, value, memory)) {
        return nullptr;
    }
    return result;
}

SerializedTypedData* Serializer::serialize(ExecutionContext* executionContext,
                                           ScriptValue value)
{
    SerializingMap initialMap;
    return serialize(executionContext, value, initialMap);
}

SerializedTypedData* Serializer::serialize(ExecutionContext* executionContext,
                                           ScriptValue value,
                                           SerializingMap& memory)
{
    SerializedTypedData* data = nullptr;
    auto result = Evaluator::execute(
        executionContext->scriptBindingInstance()->scriptContext(),
        [](ExecutionStateRef* state, SerializedTypedData** data,
           ExecutionContext* executionContext, ScriptValue value,
           SerializingMap* memory) -> ValueRef* {
            *data = serializeInternal(executionContext, state, value, *memory);
            return ValueRef::createNull();
        },
        &data, executionContext, value, &memory);

    if (!result.error.hasValue() && data) {
        return data;
    } else {
        COMPOSE_MESSAGE(
            reason, INVALID_DATA_CLONE,
            result
                .resultOrErrorToString(
                    executionContext->scriptBindingInstance()->scriptContext())
                ->toStdUTF8String()
                .data());
        throw new DOMException(executionContext, DOMException::DATA_CLONE_ERR,
                               reason);
    }
}

ScriptValue Serializer::deserialize(ExecutionContext* executionContext,
                                    SerializedTypedData* value)
{
    DeserializingMap initialMap;
    return deserialize(executionContext, value, initialMap);
}

ScriptValue Serializer::deserialize(ExecutionContext* executionContext,
                                    SerializedTypedData* value,
                                    DeserializingMap& memory)
{
    ScriptValue data = ValueRef::createUndefined();
    auto result = Evaluator::execute(
        executionContext->scriptBindingInstance()->scriptContext(),
        [](ExecutionStateRef* state, ExecutionContext* executionContext,
           SerializedTypedData* value, DeserializingMap* memory,
           ScriptValue* data) -> ValueRef* {
            *data =
                deserializeInternal(executionContext, state, value, *memory);
            return ValueRef::createNull();
        },
        executionContext, value, &memory, &data);

    if (!result.error.hasValue() && data) {
        return data;
    } else {
        COMPOSE_MESSAGE(
            reason, INVALID_DATA_CLONE,
            result
                .resultOrErrorToString(
                    executionContext->scriptBindingInstance()->scriptContext())
                ->toStdUTF8String()
                .data());
        throw new DOMException(executionContext, DOMException::DATA_CLONE_ERR,
                               reason);
    }
}

void Serializer::serializeWithTransfer(
    ExecutionContext* executionContext, ScriptValue value,
    const GCAtomicVector<ScriptObject>& transferValues,
    SerializeWithTransferResult& result)
{
    STARFISH_ASSERT(result.m_serializedTransfer.size() == 0);
    SerializingMap initialMap;
    for (size_t i = 0; i < transferValues.size(); i++) {
        ScriptObject item = transferValues[i];
        if (item->extraData()) {
            ScriptWrappable* sw =
                static_cast<ScriptWrappable*>(item->extraData());
            if (sw->isTransferable() && !sw->toTransferable()->isDetached()) {
                TransferedTypedData* placeHolder = new TransferedTypedData(
                    TransferedTypedData::PlatformObject);
                initialMap.insert(std::make_pair(item, placeHolder));
                result.m_serializedTransfer.push_back(placeHolder);
                continue;
            }
        } else if (item->isArrayBufferObject()) {
            // TODO Handle SharedArrayBuffer case (ECMAScript2018)
            STARFISH_UNSUPPORTED("serialize: transfer ArrayBuffer");
        }
    }
    SerializedTypedData* serialized =
        serialize(executionContext, value, initialMap);
    STARFISH_ASSERT(transferValues.size() ==
                    result.m_serializedTransfer.size());
    for (size_t i = 0; i < transferValues.size(); i++) {
        ScriptObject item = transferValues[i];
        TransferedTypedData* placeHolder = result.m_serializedTransfer[i];
        if (placeHolder->isPlatformObject()) {
            STARFISH_ASSERT(item->extraData());
            Transferable* tf =
                (static_cast<ScriptWrappable*>(item->extraData()))
                    ->toTransferable();
            TransferedData* dataHolder = tf->transfer();
            tf->setDetached();
            placeHolder->setPlatformObjectData(dataHolder);
        } else if (placeHolder->isArrayBuffer()) {
            // TODO Handle SharedArrayBuffer case (ECMAScript2018)
            STARFISH_ASSERT_NOT_REACHED();
        }
    }
    result.m_serialized = serialized;
    result.m_deserializer = Serializer::deserializeWithTransfer;
}

void Serializer::deserializeWithTransfer(
    ExecutionContext* executionContext, SerializeWithTransferResult& serialized,
    DeserializeWithTransferResult& result)
{
    STARFISH_ASSERT(result.m_deserializedTransfer.size() == 0);
    DeserializingMap initialMap;
    ScriptValue deserialized = nullptr;
    bool errorFound = false;
    auto sandBoxResult = Evaluator::execute(
        executionContext->scriptBindingInstance()->scriptContext(),
        [](ExecutionStateRef* state, ExecutionContext* executionContext,
           SerializeWithTransferResult* serialized,
           DeserializeWithTransferResult* result, ScriptValue* deserialized,
           DeserializingMap* initialMap, bool* errorFound) -> ValueRef* {
            *deserialized = deserializeInternal(
                executionContext, state, serialized->m_serialized, *initialMap);
            if (*deserialized) {
                for (size_t i = 0; i < serialized->m_serializedTransfer.size();
                     i++) {
                    ScriptValue v = deserializeInternal(
                        executionContext, state,
                        serialized->m_serializedTransfer[i], *initialMap);
                    if (v) {
                        result->m_deserializedTransfer.push_back(v);
                    } else {
                        *errorFound = true;
                        break;
                    }
                }
            } else {
                *errorFound = true;
            }
            return ValueRef::createNull();
        },
        executionContext, &serialized, &result, &deserialized, &initialMap,
        &errorFound);

    if (!sandBoxResult.error.hasValue() && !errorFound) {
        result.m_deserialized = deserialized;
    } else {
        throw new DOMException(executionContext, DOMException::DATA_CLONE_ERR);
    }
}
} // namespace Starfish
