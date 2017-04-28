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
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "dom/DOMException.h"
#include "util/URL.h"

namespace StarFish {

using namespace escargot;

// Implement for constructor
static ESValue urlConstructor(ESVMInstance* instance)
{
    if (!instance->currentExecutionContext()->isNewExpression()) {
        THROW_EXCEPTION(CALLED_CONSTRUCTOR_WITHOUT_NEW, "URL");
    }
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_CONSTRUCT_BECAUSE_ARGS_NOT_ENOUGH, "URL", "1",
                        buffer);
    }
    size_t validArgCount = 2;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    // Handle argument arg1
    String* value1 = String::emptyString;
    if (arg1.isUndefined()) {
        validArgCount--;
    } else {
        value1 = toBrowserString(arg1);
    }
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    URL* result = nullptr;
    // Call native function (nargs: 1-2)
    if (validArgCount == 1) {
        result = new URL(value0);
    } else if (validArgCount == 2) {
        result = new URL(value0, value1);
    }
    return result->scriptValue();
}

// Implement for attributes
#ifdef STARFISH_ENABLE_TEST
static ESValue hrefGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->href();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue hrefSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setHref(value0);
    return ESValue();
}
#endif

#ifdef STARFISH_ENABLE_TEST
static ESValue originGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->origin();
    // Return ESValue from native value
    return toJSString(result);
}
#endif

#ifdef STARFISH_ENABLE_TEST
static ESValue protocolGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->protocol();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue protocolSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setProtocol(value0);
    return ESValue();
}
#endif

#ifdef STARFISH_ENABLE_TEST
static ESValue usernameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->username();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue usernameSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setUsername(value0);
    return ESValue();
}
#endif

#ifdef STARFISH_ENABLE_TEST
static ESValue passwordGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->password();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue passwordSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setPassword(value0);
    return ESValue();
}
#endif

#ifdef STARFISH_ENABLE_TEST
static ESValue hostGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->host();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue hostSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setHost(value0);
    return ESValue();
}
#endif

#ifdef STARFISH_ENABLE_TEST
static ESValue hostnameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->hostname();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue hostnameSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setHostname(value0);
    return ESValue();
}
#endif

#ifdef STARFISH_ENABLE_TEST
static ESValue portGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->port();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue portSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setPort(value0);
    return ESValue();
}
#endif

#ifdef STARFISH_ENABLE_TEST
static ESValue pathnameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->pathname();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue pathnameSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setPathname(value0);
    return ESValue();
}
#endif

#ifdef STARFISH_ENABLE_TEST
static ESValue searchGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->search();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue searchSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setSearch(value0);
    return ESValue();
}
#endif

#ifdef STARFISH_ENABLE_TEST
static ESValue hashGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->hash();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue hashSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setHash(value0);
    return ESValue();
}
#endif

// Implement for functions
#ifdef STARFISH_ENABLE_TEST
static ESValue toStringFunction(ESVMInstance* instance)
{
    return hrefGetterFunction(instance);
}
#endif

static bool createObjectURL1Checker(ESVMInstance* instance)
{
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    if (!_CHECK_TYPEOF(arg0, Blob)) {
        return false;
    }
    return true;
}

static ESValue createObjectURL1Function(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    Blob* value0 = nullptr;
    value0 = (Blob*)(arg0.asESPointer()->asESObject()->extraPointerData());
    // Call native function (nargs: 1)
    result = URL::createObjectURL(value0);

    // Return ESValue from native value
    return toJSString(result);
}

#ifdef STARFISH_ENABLE_MULTIMEDIA
static bool createObjectURL2Checker(ESVMInstance* instance)
{
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    if (!_CHECK_TYPEOF(arg0, MediaSource)) {
        return false;
    }
    return true;
}
#endif

#ifdef STARFISH_ENABLE_MULTIMEDIA
static ESValue createObjectURL2Function(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    MediaSource* value0 = nullptr;
    value0 =
        (MediaSource*)(arg0.asESPointer()->asESObject()->extraPointerData());
    // Call native function (nargs: 1)
    result = URL::createObjectURL(value0);

    // Return ESValue from native value
    return toJSString(result);
}
#endif

static ESValue createObjectURLFunction(ESVMInstance* instance)
{
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount >= 1 && createObjectURL1Checker(instance)) {
        return createObjectURL1Function(instance);
    }

#ifdef STARFISH_ENABLE_MULTIMEDIA
    if (argCount >= 1 && createObjectURL2Checker(instance)) {
        return createObjectURL2Function(instance);
    }
#endif

    THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_SIGNATURE_NOT_FOUND,
                    "createObjectURL", "URL");
}

static ESValue revokeObjectURLFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(URL);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "revokeObjectURL", "URL", "1", buffer);
    }
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    StarFish* callWith = fetchStarFish(instance);
    // Call native function (nargs: 1)
    URL::revokeObjectURL(callWith, value0);

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

ESFunctionObject* bindingURL(ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* URLString = ESString::create("URL");
    ESFunctionObject* URLFunction = ESFunctionObject::create(
        nullptr, urlConstructor, URLString, 1, true, true);
    URLFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    URLFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    URLFunction->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)
            ->m_instance->globalObject()
            ->objectPrototype());
    ESObject* URLPrototypeObj =
        URLFunction->protoType().asESPointer()->asESObject();

// Bind for attributes
#ifdef STARFISH_ENABLE_TEST
    ESString* hrefString = ESString::create("href");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLPrototypeObj, hrefString, hrefGetterFunction, hrefSetterFunction);
#endif

#ifdef STARFISH_ENABLE_TEST
    ESString* originString = ESString::create("origin");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLPrototypeObj, originString, originGetterFunction, nullptr);
#endif

#ifdef STARFISH_ENABLE_TEST
    ESString* protocolString = ESString::create("protocol");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLPrototypeObj, protocolString, protocolGetterFunction,
        protocolSetterFunction);
#endif

#ifdef STARFISH_ENABLE_TEST
    ESString* usernameString = ESString::create("username");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLPrototypeObj, usernameString, usernameGetterFunction,
        usernameSetterFunction);
#endif

#ifdef STARFISH_ENABLE_TEST
    ESString* passwordString = ESString::create("password");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLPrototypeObj, passwordString, passwordGetterFunction,
        passwordSetterFunction);
#endif

#ifdef STARFISH_ENABLE_TEST
    ESString* hostString = ESString::create("host");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLPrototypeObj, hostString, hostGetterFunction, hostSetterFunction);
#endif

#ifdef STARFISH_ENABLE_TEST
    ESString* hostnameString = ESString::create("hostname");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLPrototypeObj, hostnameString, hostnameGetterFunction,
        hostnameSetterFunction);
#endif

#ifdef STARFISH_ENABLE_TEST
    ESString* portString = ESString::create("port");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLPrototypeObj, portString, portGetterFunction, portSetterFunction);
#endif

#ifdef STARFISH_ENABLE_TEST
    ESString* pathnameString = ESString::create("pathname");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLPrototypeObj, pathnameString, pathnameGetterFunction,
        pathnameSetterFunction);
#endif

#ifdef STARFISH_ENABLE_TEST
    ESString* searchString = ESString::create("search");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLPrototypeObj, searchString, searchGetterFunction,
        searchSetterFunction);
#endif

#ifdef STARFISH_ENABLE_TEST
    ESString* hashString = ESString::create("hash");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLPrototypeObj, hashString, hashGetterFunction, hashSetterFunction);
#endif

// Bind for functions
#ifdef STARFISH_ENABLE_TEST
    ESString* toStringString = ESString::create("toString");
    ESFunctionObject* toStringESFn = ESFunctionObject::create(
        nullptr, toStringFunction, toStringString, 0, false);
    URLPrototypeObj->defineDataProperty(toStringString, true, true, true,
                                        toStringESFn);
#endif

    ESString* createObjectURLString = ESString::create("createObjectURL");
    ESFunctionObject* createObjectURLESFn = ESFunctionObject::create(
        nullptr, createObjectURLFunction, createObjectURLString, 1, false);
    URLFunction->defineDataProperty(createObjectURLString, true, true, true,
                                    createObjectURLESFn);

    ESString* revokeObjectURLString = ESString::create("revokeObjectURL");
    ESFunctionObject* revokeObjectURLESFn = ESFunctionObject::create(
        nullptr, revokeObjectURLFunction, revokeObjectURLString, 1, false);
    URLFunction->defineDataProperty(revokeObjectURLString, true, true, true,
                                    revokeObjectURLESFn);
    return URLFunction;
}
}
