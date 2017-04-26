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

#include "StarFishConfig.h"
#include "ScriptBindingInstance.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"
#include "dom/DOMException.h"
#include "extra/XMLHttpRequest.h"

namespace StarFish {

using namespace escargot;

// Implement for constructor
static ESValue xmlhttprequestConstructor(ESVMInstance* instance)
{
    if (!instance->currentExecutionContext()->isNewExpression()) {
        THROW_EXCEPTION(CALLED_CONSTRUCTOR_WITHOUT_NEW, "XMLHttpRequest");
    }
    XMLHttpRequest* result = nullptr;
    Document* callWith = fetchDocument(instance);
    // Call native function (nargs: 0)
    result = new XMLHttpRequest(callWith);
    return result->scriptValue();
}

// Implement for attributes
static ESValue onreadystatechangeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);

    return originalObj->onreadystatechangeEventListener();
}

static ESValue onreadystatechangeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);

    ESValue v = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnreadystatechangeEventListener(v);

    return ESValue();
}

static ESValue readyStateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->readyState();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue timeoutGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->timeout();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue timeoutSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    uint32_t value0;
    value0 = arg0.toUint32();
    originalObj->setTimeout(value0);
    return ESValue();
}

static ESValue statusGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->status();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue responseTypeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->responseType();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue responseTypeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setResponseType(value0);
    return ESValue();
}

static ESValue responseGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    // Declare native value (empty when type is void)
    ScriptValue result;
    result = originalObj->response();
    // Return ESValue from native value
    return result;
}

static ESValue responseTextGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    try {
        result = originalObj->responseText();
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    return toJSString(result);
}

// Implement for functions
static ESValue open1Function(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 2) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH, "open1",
                        "XMLHttpRequest", "2", buffer);
    }
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Handle argument arg1
    String* value1 = String::emptyString;
    value1 = toBrowserString(arg1);

    // Call native function (nargs: 2)
    try {
        originalObj->open(value0, value1);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue open2Function(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 3) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH, "open2",
                        "XMLHttpRequest", "3", buffer);
    }
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    ESValue arg2 = instance->currentExecutionContext()->readArgument(2);
    ESValue arg3 = instance->currentExecutionContext()->readArgument(3);
    ESValue arg4 = instance->currentExecutionContext()->readArgument(4);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Handle argument arg1
    String* value1 = String::emptyString;
    value1 = toBrowserString(arg1);

    // Handle argument arg2
    bool value2;
    value2 = arg2.toBoolean();

    // Handle argument arg3
    Nullable<String*> value3;
    if (!arg3.isUndefinedOrNull()) {
        value3 = toBrowserString(arg3);
    }
    // Handle argument arg4
    Nullable<String*> value4;
    if (!arg4.isUndefinedOrNull()) {
        value4 = toBrowserString(arg4);
    }
    // Call native function (nargs: 5)
    try {
        originalObj->open(value0, value1, value2, value3, value4);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue openFunction(ESVMInstance* instance)
{
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (false) {
    } else if (argCount == 2) {
        return open1Function(instance);
    } else if (argCount >= 3 && argCount <= 5) {
        return open2Function(instance);
    } else {
        auto msg = ESString::create("Invalid arguments");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue setRequestHeaderFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 2) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "setRequestHeader", "XMLHttpRequest", "2", buffer);
    }
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Handle argument arg1
    String* value1 = String::emptyString;
    value1 = toBrowserString(arg1);

    // Call native function (nargs: 2)
    try {
        originalObj->setRequestHeader(value0, value1);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue sendFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    Nullable<String*> value0;
    if (!arg0.isUndefinedOrNull()) {
        value0 = toBrowserString(arg0);
    }
    // Call native function (nargs: 1)
    try {
        originalObj->send(value0);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue abortFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    // Declare native value (empty when type is void)
    // Call native function (nargs: 0)
    originalObj->abort();

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

ESFunctionObject* bindingXMLHttpRequest(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* XMLHttpRequestString = ESString::create("XMLHttpRequest");
    ESFunctionObject* XMLHttpRequestFunction =
        ESFunctionObject::create(nullptr, xmlhttprequestConstructor,
                                 XMLHttpRequestString, 0, true, true);
    XMLHttpRequestFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    XMLHttpRequestFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    XMLHttpRequestFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->set__proto__(fetchData(scriptBindingInstance)
                           ->fnXMLHttpRequestEventTarget()
                           ->protoType());
    XMLHttpRequestFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnXMLHttpRequestEventTarget());
    ESObject* XMLHttpRequestPrototypeObj =
        XMLHttpRequestFunction->protoType().asESPointer()->asESObject();

    // Bind for constants
    ESString* UNSENTString = ESString::create("UNSENT");
    ESValue UNSENTValue = ESValue(0);
    XMLHttpRequestPrototypeObj->defineDataProperty(UNSENTString, false, true,
                                                   false, UNSENTValue);

    XMLHttpRequestFunction->defineDataProperty(UNSENTString, false, true, false,
                                               UNSENTValue);

    ESString* OPENEDString = ESString::create("OPENED");
    ESValue OPENEDValue = ESValue(1);
    XMLHttpRequestPrototypeObj->defineDataProperty(OPENEDString, false, true,
                                                   false, OPENEDValue);

    XMLHttpRequestFunction->defineDataProperty(OPENEDString, false, true, false,
                                               OPENEDValue);

    ESString* HEADERS_RECEIVEDString = ESString::create("HEADERS_RECEIVED");
    ESValue HEADERS_RECEIVEDValue = ESValue(2);
    XMLHttpRequestPrototypeObj->defineDataProperty(
        HEADERS_RECEIVEDString, false, true, false, HEADERS_RECEIVEDValue);

    XMLHttpRequestFunction->defineDataProperty(
        HEADERS_RECEIVEDString, false, true, false, HEADERS_RECEIVEDValue);

    ESString* LOADINGString = ESString::create("LOADING");
    ESValue LOADINGValue = ESValue(3);
    XMLHttpRequestPrototypeObj->defineDataProperty(LOADINGString, false, true,
                                                   false, LOADINGValue);

    XMLHttpRequestFunction->defineDataProperty(LOADINGString, false, true,
                                               false, LOADINGValue);

    ESString* DONEString = ESString::create("DONE");
    ESValue DONEValue = ESValue(4);
    XMLHttpRequestPrototypeObj->defineDataProperty(DONEString, false, true,
                                                   false, DONEValue);

    XMLHttpRequestFunction->defineDataProperty(DONEString, false, true, false,
                                               DONEValue);

    // Bind for attributes
    ESString* onreadystatechangeString = ESString::create("onreadystatechange");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        XMLHttpRequestPrototypeObj, onreadystatechangeString,
        onreadystatechangeGetterFunction, onreadystatechangeSetterFunction);

    ESString* readyStateString = ESString::create("readyState");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        XMLHttpRequestPrototypeObj, readyStateString, readyStateGetterFunction,
        nullptr);

    ESString* timeoutString = ESString::create("timeout");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        XMLHttpRequestPrototypeObj, timeoutString, timeoutGetterFunction,
        timeoutSetterFunction);

    ESString* statusString = ESString::create("status");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        XMLHttpRequestPrototypeObj, statusString, statusGetterFunction,
        nullptr);

    ESString* responseTypeString = ESString::create("responseType");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        XMLHttpRequestPrototypeObj, responseTypeString,
        responseTypeGetterFunction, responseTypeSetterFunction);

    ESString* responseString = ESString::create("response");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        XMLHttpRequestPrototypeObj, responseString, responseGetterFunction,
        nullptr);

    ESString* responseTextString = ESString::create("responseText");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        XMLHttpRequestPrototypeObj, responseTextString,
        responseTextGetterFunction, nullptr);

    // Bind for functions
    ESString* openString = ESString::create("open");
    ESFunctionObject* openESFn =
        ESFunctionObject::create(nullptr, openFunction, openString, 0, false);
    XMLHttpRequestPrototypeObj->defineDataProperty(openString, true, true, true,
                                                   openESFn);

    ESString* setRequestHeaderString = ESString::create("setRequestHeader");
    ESFunctionObject* setRequestHeaderESFn = ESFunctionObject::create(
        nullptr, setRequestHeaderFunction, setRequestHeaderString, 2, false);
    XMLHttpRequestPrototypeObj->defineDataProperty(
        setRequestHeaderString, true, true, true, setRequestHeaderESFn);

    ESString* sendString = ESString::create("send");
    ESFunctionObject* sendESFn =
        ESFunctionObject::create(nullptr, sendFunction, sendString, 0, false);
    XMLHttpRequestPrototypeObj->defineDataProperty(sendString, true, true, true,
                                                   sendESFn);

    ESString* abortString = ESString::create("abort");
    ESFunctionObject* abortESFn =
        ESFunctionObject::create(nullptr, abortFunction, abortString, 0, false);
    XMLHttpRequestPrototypeObj->defineDataProperty(abortString, true, true,
                                                   true, abortESFn);

    return XMLHttpRequestFunction;
}
}
