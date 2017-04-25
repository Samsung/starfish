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

static ESValue urlConstructor(ESVMInstance* instance)
{
    int argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        // throw error
    } else if (argCount == 1) {
        ESValue urlString =
            instance->currentExecutionContext()->readArgument(0);
        auto url = URL::createURL(
            String::emptyString,
            String::fromUTF8(urlString.asESString()->utf8Data()));
        return url->scriptValue();
    } else { // ignore redundant arguments
        ESValue urlString =
            instance->currentExecutionContext()->readArgument(0);
        ESValue baseURLString =
            instance->currentExecutionContext()->readArgument(1);
        // FIXME second argument can be not only string but also
        // object
        STARFISH_ASSERT(baseURLString.isESString());

        auto url = URL::createURL(
            String::fromUTF8(baseURLString.asESString()->utf8Data()),
            String::fromUTF8(urlString.asESString()->utf8Data()));
        return url->scriptValue();
    }
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
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

static ESValue createObjectURLFunction(ESVMInstance* instance)
{
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    if (arg0.isObject() &&
        (arg0.asObject()->extraData() == kEscargotObjectCheckMagic) &&
        ((ScriptWrappable*)arg0.asObject()->extraPointerData())->isBlob()) {
        Blob* b = (Blob*)arg0.toObject()->extraPointerData();
        String* url = URL::createObjectURL(b);
        return toJSString(url);
#ifdef STARFISH_ENABLE_MULTIMEDIA
    } else if (arg0.isObject() &&
               (arg0.asObject()->extraData() == kEscargotObjectCheckMagic) &&
               ((ScriptWrappable*)arg0.asObject()->extraPointerData())
                   ->isMediaSource()) {
        MediaSource* m = (MediaSource*)arg0.toObject()->extraPointerData();
        String* url = URL::createObjectURL(m);
        return toJSString(url);
#endif
    } else {
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_SIGNATURE_NOT_FOUND,
                        "createObjectURL", "URL");
    }
}

static ESValue revokeObjectURLFunction(ESVMInstance* instance)
{
    String* arg0 = toBrowserString(
        instance->currentExecutionContext()->readArgument(0).toString());
    StarFish* sf = fetchStarFish(instance);
    URL::revokeObjectURL(sf, arg0);
    return ESValue();
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
