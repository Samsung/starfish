/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#include "extra/XMLHttpRequest.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue onloadstartGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequestEventTarget);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onloadstart();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onloadstartSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequestEventTarget);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnloadstart(value0);
    return ESValue();
}

static ESValue onprogressGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequestEventTarget);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onprogress();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onprogressSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequestEventTarget);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnprogress(value0);
    return ESValue();
}

static ESValue onabortGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequestEventTarget);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onabort();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onabortSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequestEventTarget);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnabort(value0);
    return ESValue();
}

static ESValue onerrorGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequestEventTarget);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onerror();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onerrorSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequestEventTarget);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnerror(value0);
    return ESValue();
}

static ESValue onloadGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequestEventTarget);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onload();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onloadSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequestEventTarget);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnload(value0);
    return ESValue();
}

static ESValue ontimeoutGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequestEventTarget);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->ontimeout();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue ontimeoutSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequestEventTarget);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOntimeout(value0);
    return ESValue();
}

static ESValue onloadendGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequestEventTarget);
    // Declare native value (empty when type is void)
    EventListener* result = nullptr;
    result = originalObj->onloadend();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue onloadendSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequestEventTarget);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    EventListener* value0 = nullptr;
    value0 = EventListener::toEventListener(arg0, true);
    originalObj->setOnloadend(value0);
    return ESValue();
}

ESFunctionObject* bindingXMLHttpRequestEventTarget(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* XMLHttpRequestEventTargetString =
        ESString::create("XMLHttpRequestEventTarget");
    ESFunctionObject* XMLHttpRequestEventTargetFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 XMLHttpRequestEventTargetString, 0, true,
                                 true);
    ESObject* XMLHttpRequestEventTargetPrototypeObj =
        XMLHttpRequestEventTargetFunction->protoType()
            .asESPointer()
            ->asESObject();
    XMLHttpRequestEventTargetFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    XMLHttpRequestEventTargetPrototypeObj->forceNonVectorHiddenClass(false);
    XMLHttpRequestEventTargetPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnEventTarget()->protoType());
    XMLHttpRequestEventTargetFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnEventTarget());

    // Bind for attributes
    ESString* onloadstartString = ESString::create("onloadstart");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        XMLHttpRequestEventTargetPrototypeObj, onloadstartString,
        onloadstartGetterFunction, onloadstartSetterFunction);

    ESString* onprogressString = ESString::create("onprogress");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        XMLHttpRequestEventTargetPrototypeObj, onprogressString,
        onprogressGetterFunction, onprogressSetterFunction);

    ESString* onabortString = ESString::create("onabort");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        XMLHttpRequestEventTargetPrototypeObj, onabortString,
        onabortGetterFunction, onabortSetterFunction);

    ESString* onerrorString = ESString::create("onerror");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        XMLHttpRequestEventTargetPrototypeObj, onerrorString,
        onerrorGetterFunction, onerrorSetterFunction);

    ESString* onloadString = ESString::create("onload");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        XMLHttpRequestEventTargetPrototypeObj, onloadString,
        onloadGetterFunction, onloadSetterFunction);

    ESString* ontimeoutString = ESString::create("ontimeout");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        XMLHttpRequestEventTargetPrototypeObj, ontimeoutString,
        ontimeoutGetterFunction, ontimeoutSetterFunction);

    ESString* onloadendString = ESString::create("onloadend");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        XMLHttpRequestEventTargetPrototypeObj, onloadendString,
        onloadendGetterFunction, onloadendSetterFunction);

    return XMLHttpRequestEventTargetFunction;
}
}
