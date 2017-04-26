/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#include "StarFish.h"
#include "binding/ScriptWrappable.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"
#include "dom/Document.h"
#include "dom/Element.h"
#include "layout/Frame.h"
#include "layout/FrameBox.h"
#include "platform/message_loop/MessageLoop.h"
#include "platform/window/Window.h"
#include "style/CSSStyleLookupTrie.h"
#include "vm/ESVMInstance.h"

namespace StarFish {

using namespace escargot;

void defineNativeAccessorPropertyButNeedToGenerateJSFunction(
    ESObject* obj, ESString* propertyName, NativeFunctionType getter,
    NativeFunctionType setter, bool isEnumerable, bool isConfigurable)
{
    bool isWritable = setter;

    ESPropertyAccessorData* accData = new ESPropertyAccessorData();
    accData->setJSGetter(ESFunctionObject::create(
        nullptr, getter, ESVMInstance::currentInstance()->strings().emptyString,
        0, false, false));
    if (setter) {
        accData->setJSSetter(ESFunctionObject::create(
            nullptr, setter,
            ESVMInstance::currentInstance()->strings().emptyString, 1, false,
            false));
    }
    obj->defineAccessorProperty(propertyName, accData, isWritable, isEnumerable,
                                isConfigurable);
}

ScriptBindingInstanceDataEscargot* fetchData(ScriptBindingInstance* instance)
{
    return (ScriptBindingInstanceDataEscargot*)instance->data();
}

Document* fetchDocument(ESVMInstance* instance)
{
    Window* window = (Window*)instance->globalObject()->extraPointerData();
    return window->document();
}

StarFish* fetchStarFish(ESVMInstance* instance)
{
    Window* window = ((Window*)instance->globalObject()->extraPointerData());
    return window->starFish();
}

String* toBrowserString(const ESValue& v)
{
    return toBrowserString(v.toString());
}

String* toBrowserString(const ESString* v)
{
    escargot::NullableUTF8String s = v->toNullableUTF8String();
    String* newStr = String::fromUTF8(s.m_buffer, s.m_bufferSize);
    // NOTE: input string contains whitecharacters as is, i.e., "\n" is stored
    // as '\','n'
    // The right way is, input string should already have '\n', and white spaces
    // should be removed from here.
    // For time being, we simply remove "\n" and other whitespaces strings.
    // newStr = newStr->replaceAll(String::fromUTF8("\n"), String::spaceString);
    // newStr = newStr->replaceAll(String::fromUTF8("\t"), String::spaceString);
    // newStr = newStr->replaceAll(String::fromUTF8("\f"), String::spaceString);
    // newStr = newStr->replaceAll(String::fromUTF8("\r"), String::spaceString);
    return newStr;
}

ESValue toJSString(String* v)
{
    return createScriptString(v);
}

ESValue defaultFunction(ESVMInstance* instance)
{
    return ESValue();
}

ESValue errorOnConstructorFunction(ESVMInstance* instance)
{
    ESVMInstance::currentInstance()->throwError(
        ESValue(TypeError::create(ESString::create("Illegal constructor"))));
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
    return ESValue();
}

ScriptWrappable::ScriptWrappable(void* extraPointerData)
{
    STARFISH_ASSERT(!((size_t)extraPointerData & (size_t)1));
    m_object = (ESFunctionObject*)((size_t)extraPointerData | (size_t)1);
}

ScriptObject ScriptWrappable::scriptObjectSlowCase()
{
    void* extraPointerData = (void*)((size_t)m_object - 1);
    m_object = ESObject::create(0);
    STARFISH_ASSERT(!((size_t)m_object & (size_t)1));
    m_object->setExtraPointerData(extraPointerData);
    m_object->setExtraData(kEscargotObjectCheckMagic);

    Window* window = (Window*)ESVMInstance::currentInstance()
                         ->globalObject()
                         ->extraPointerData();
    init(window->scriptBindingInstance());

    return m_object;
}

bool ScriptWrappable::hasProperty(String* name)
{
    return m_object->ESObject::hasProperty(createScriptString(name));
}

static int utf32ToUtf16(char32_t i, char16_t* u)
{
    if (i < 0xffff) {
        *u = (char16_t)(i & 0xffff);
        return 1;
    } else if (i < 0x10ffff) {
        i -= 0x10000;
        *u++ = 0xd800 | (i >> 10);
        *u = 0xdc00 | (i & 0x3ff);
        return 2;
    } else {
        // produce error char
        *u = 0xFFFD;
        return 1;
    }
}

ScriptValue createScriptString(String* str)
{
    if (str->isASCIIString()) {
        escargot::ASCIIString s(str->asASCIIString()->begin(),
                                str->asASCIIString()->end());
        return ESString::create(std::move(s));
    } else {
        escargot::UTF16String out;
        for (size_t i = 0; i < str->length(); i++) {
            char32_t src = str->charAt(i);
            char16_t dst[2];
            int ret = utf32ToUtf16(src, dst);

            if (ret == 1) {
                out.push_back(src);
            } else if (ret == 2) {
                out.push_back(dst[0]);
                out.push_back(dst[1]);
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }

        return ESString::create(std::move(out));
    }
}

ScriptValue createScriptFunction(String** argNames, size_t argc,
                                 String* functionBody, bool& error)
{
    error = false;
    ESVMInstance* instance = ESVMInstance::currentInstance();

    ESValueVector arg(0);
    for (size_t i = 0; i < argc; i++) {
        arg.push_back(createScriptString(argNames[i]));
    }

    arg.push_back(createScriptString(functionBody));

    ScriptValue result;
    std::jmp_buf tryPosition;
    if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
        result = ESFunctionObject::call(instance,
                                        instance->globalObject()->function(),
                                        ESValue(), arg.data(), argc + 1, false);
        instance->unregisterTryPos(&tryPosition);
    } else {
        result = instance->getCatchedError();
        error = true;
        STARFISH_LOG_INFO("Uncaught %s\n", result.toString()->utf8Data());
    }
    return result;
}

struct AttributeStringEventFunctionInnerData : public gc {
    ESValue function;
    Element* m_target;
};

static ESValue globalObjectReadCallbackFunction(const ESValue& key,
                                                ESObject* obj)
{
    Window* wnd = (Window*)ESVMInstance::currentInstance()
                      ->globalObject()
                      ->extraPointerData();
    Element* e =
        wnd->document()
            ->elementExecutionStackForAttributeStringEventFunctionObject()
            .back();
    if (e->scriptValue().asESPointer()->asESObject()->hasOwnProperty(key,
                                                                     true)) {
        return e->scriptValue().asESPointer()->asESObject()->getOwnProperty(
            key);
    }
    return ESValue(ESValue::ESDeletedValue);
}

static ESValue attributeStringEventFunction(ESVMInstance* instance)
{
    FunctionEnvironmentRecordWithArgumentsObject* record =
        (FunctionEnvironmentRecordWithArgumentsObject*)
            ESVMInstance::currentInstance()
                ->currentExecutionContext()
                ->environment()
                ->record();
    ESFunctionObject* callee = record->callee();
    STARFISH_ASSERT(callee->extraData() == kEventStringAttributeCheckMagic);
    AttributeStringEventFunctionInnerData* data =
        (AttributeStringEventFunctionInnerData*)callee->extraPointerData();

    Window* wnd = (Window*)ESVMInstance::currentInstance()
                      ->globalObject()
                      ->extraPointerData();
    wnd->document()
        ->elementExecutionStackForAttributeStringEventFunctionObject()
        .push_back(data->m_target);

    ESVMInstance::currentInstance()->globalObject()->setIdentifierInterceptor(
        globalObjectReadCallbackFunction);

    std::jmp_buf tryPosition;
    bool hasError = false;
    ESValue result;
    if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
        result = ESFunctionObject::call(ESVMInstance::currentInstance(),
                                        data->function,
                                        ESVMInstance::currentInstance()
                                            ->currentExecutionContext()
                                            ->resolveThisBinding(),
                                        ESVMInstance::currentInstance()
                                            ->currentExecutionContext()
                                            ->arguments(),
                                        ESVMInstance::currentInstance()
                                            ->currentExecutionContext()
                                            ->argumentCount(),
                                        false);
        instance->unregisterTryPos(&tryPosition);
        hasError = false;
    } else {
        hasError = true;
        result = instance->getCatchedError();
    }

    wnd->document()
        ->elementExecutionStackForAttributeStringEventFunctionObject()
        .pop_back();

    if (wnd->document()
            ->elementExecutionStackForAttributeStringEventFunctionObject()
            .size() == 0) {
        ESVMInstance::currentInstance()
            ->globalObject()
            ->setIdentifierInterceptor(nullptr);
    }

    if (hasError) {
        instance->throwError(result);
    }

    return result;
}

ScriptValue createAttributeStringEventFunction(Element* target,
                                               String* functionBody,
                                               bool& result)
{
    String* name[] = { String::createASCIIString("event") };
    ESValue fn = createScriptFunction(name, 1, functionBody, result);
    ESFunctionObject* wrapper = ESFunctionObject::create(
        NULL, attributeStringEventFunction, ESString::create(""), 0, false);

    wrapper->codeBlock()->m_needsToPrepareGenerateArgumentsObject = true;
    wrapper->setExtraData(kEventStringAttributeCheckMagic);
    AttributeStringEventFunctionInnerData* data =
        new AttributeStringEventFunctionInnerData();
    data->m_target = target;
    data->function = fn;
    wrapper->setExtraPointerData(data);
    return wrapper;
}

ScriptValue callScriptFunction(ScriptValue fn, ScriptValue* argv, size_t argc,
                               ScriptValue thisValue)
{
    ScriptValue result;
    if (fn.isESPointer() && fn.asESPointer()->isESFunctionObject()) {
        ESVMInstance* instance = ESVMInstance::currentInstance();
        std::jmp_buf tryPosition;
        if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
            result = ESFunctionObject::call(instance, fn, thisValue, argv, argc,
                                            false);
            instance->unregisterTryPos(&tryPosition);
        } else {
            result = instance->getCatchedError();
            STARFISH_LOG_INFO("Uncaught %s\n", result.toString()->utf8Data());
        }
    }
    return result;
}

ScriptValue createArrayBuffer(void* bufferSrc, size_t len)
{
#ifdef USE_ES6_FEATURE
    ESArrayBufferObject* obj = ESArrayBufferObject::create();
    obj->attachArrayBuffer(bufferSrc, len);
    return obj;
#else
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
#endif
}

ScriptValue parseJSON(String* jsonData)
{
    ScriptValue ret;
    ESVMInstance* instance = ESVMInstance::currentInstance();
    ScriptValue json_arg[1] = { ScriptValue(createScriptString(jsonData)) };
    ScriptValue json_parse_fn = instance->globalObject()->json()->get(
        ScriptValue(createScriptString(String::fromUTF8("parse"))));
    return callScriptFunction(json_parse_fn, json_arg, 1,
                              instance->globalObject()->json());
}

bool isCallableScriptValue(ScriptValue v)
{
    if (v.isESPointer() && v.asESPointer()->isESFunctionObject()) {
        return true;
    }
    return false;
}

#ifdef USE_ES6_FEATURE
Promise::Promise()
    : m_scriptValue(ESPromiseObject::create())
{
    // TODO remove below line if escargot fixed
    m_scriptValue.asESPointer()->asESPromiseObject()->set__proto__(
        ESVMInstanceCurrentInstance()->globalObject()->promisePrototype());
}

void Promise::fulfill(ScriptValue v)
{
    m_scriptValue.asESPointer()->asESPromiseObject()->fulfillPromise(
        ESVMInstanceCurrentInstance(), v);
}

void Promise::reject(ScriptValue v)
{
    m_scriptValue.asESPointer()->asESPromiseObject()->rejectPromise(
        ESVMInstanceCurrentInstance(), v);
}
#endif
}
