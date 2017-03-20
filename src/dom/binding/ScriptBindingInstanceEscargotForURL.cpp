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

#include "dom/DOM.h"
#include "dom/binding/escargot/ScriptBindingInstanceDataEscargot.h"

namespace StarFish {

using namespace escargot;

static ESValue urlFunction(ESVMInstance* instance)
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

static ESValue createObjectURLFunction(ESVMInstance* instance)
{
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    if (arg0.isObject() &&
        (arg0.asObject()->extraData() == kEscargotObjectCheckMagic) &&
        ((ScriptWrappable*)arg0.asObject()->extraPointerData())->type() ==
            ScriptWrappable::Type::BlobObject) {
        Blob* b = (Blob*)arg0.toObject()->extraPointerData();
        String* url = URL::createObjectURL(b);
        return toJSString(url);
#ifdef STARFISH_ENABLE_MULTIMEDIA
    } else if (arg0.isObject() &&
               (arg0.asObject()->extraData() == kEscargotObjectCheckMagic) &&
               ((ScriptWrappable*)arg0.asObject()->extraPointerData())
                       ->type() == ScriptWrappable::Type::MediaSourceObject) {
        MediaSource* m = (MediaSource*)arg0.toObject()->extraPointerData();
        String* url = URL::createObjectURL(m);
        return toJSString(url);
#endif
    } else {
        ESString* msg = ESString::create(
            "Failed to execute 'createObjectURL' on 'URL': No "
            "function was found that matched the signature "
            "provided.");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue revokeObjectURLFunction(ESVMInstance* instance)
{
    String* arg0 = toBrowserString(
        instance->currentExecutionContext()->readArgument(0).toString());
    StarFish* sf =
        ((Window*)instance->globalObject()->extraPointerData())->starFish();
    URL::revokeObjectURL(sf, arg0);
    return ESValue();
}

static ESValue hrefGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
    return toJSString(originalObj->getHref());
}

static ESValue originGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
    return toJSString(originalObj->origin());
}

static ESValue protocolGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
    return toJSString(originalObj->getProtocol());
}

static ESValue usernameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
    return toJSString(originalObj->getUsername());
}

static ESValue usernameSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
    int argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    } else {
        ESValue arg = instance->currentExecutionContext()->readArgument(0);
        ESString* argString = arg.toString();
        originalObj->setUsername(String::fromUTF8(argString->utf8Data()));
    }
    return ESValue();
}

static ESValue passwordGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
    return toJSString(originalObj->getPassword());
}

static ESValue passwordSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
    int argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    } else {
        ESValue arg = instance->currentExecutionContext()->readArgument(0);
        ESString* argString = arg.toString();
        originalObj->setPassword(String::fromUTF8(argString->utf8Data()));
    }
    return ESValue();
}

static ESValue hostGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
    return toJSString(originalObj->getHost());
}

static ESValue hostnameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
    return toJSString(originalObj->getHostname());
}

static ESValue portGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
    return toJSString(originalObj->getPort());
}

static ESValue pathnameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
    return toJSString(originalObj->getPathname());
}

static ESValue pathnameSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
    int argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    } else {
        ESValue arg = instance->currentExecutionContext()->readArgument(0);
        ESString* argString = arg.toString();
        originalObj->setPathname(String::fromUTF8(argString->utf8Data()));
    }
    return ESValue();
}

static ESValue searchGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
    return toJSString(originalObj->getSearch());
}

static ESValue hashGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
    return toJSString(originalObj->getHash());
}

ESFunctionObject* bindingURL(ScriptBindingInstance* scriptBindingInstance)
{
#ifdef STARFISH_ENABLE_TEST
    ESFunctionObject* fnURL = ESFunctionObject::create(
        NULL, urlFunction, ESString::create("URL"), 2, true, true);

    fnURL->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    fnURL->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(
        false);
    fnURL->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)
            ->m_instance->globalObject()
            ->objectPrototype());
    fnURL->set__proto__(fetchData(scriptBindingInstance)
                            ->m_instance->globalObject()
                            ->objectPrototype());
#else
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(URL, fetchData(scriptBindingInstance)
                                             ->m_instance->globalObject()
                                             ->objectPrototype());
#endif
    fnURL->defineDataProperty(
        ESString::create("createObjectURL"), false, false, false,
        ESFunctionObject::create(NULL, createObjectURLFunction,
                                 ESString::create("createObjectURL"), 1,
                                 false));

    fnURL->defineDataProperty(
        ESString::create("revokeObjectURL"), false, false, false,
        ESFunctionObject::create(NULL, revokeObjectURLFunction,
                                 ESString::create("revokeObjectURL"), 1,
                                 false));

#ifdef STARFISH_ENABLE_TEST
    // FIXME setters below should not be null
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnURL->protoType().asESPointer()->asESObject(),
        ESString::create("href"), hrefGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnURL->protoType().asESPointer()->asESObject(),
        ESString::create("origin"), originGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnURL->protoType().asESPointer()->asESObject(),
        ESString::create("protocol"), protocolGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnURL->protoType().asESPointer()->asESObject(),
        ESString::create("username"), usernameGetterFunction,
        usernameSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnURL->protoType().asESPointer()->asESObject(),
        ESString::create("password"), passwordGetterFunction,
        passwordSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnURL->protoType().asESPointer()->asESObject(),
        ESString::create("host"), hostGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnURL->protoType().asESPointer()->asESObject(),
        ESString::create("hostname"), hostnameGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnURL->protoType().asESPointer()->asESObject(),
        ESString::create("port"), portGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnURL->protoType().asESPointer()->asESObject(),
        ESString::create("pathname"), pathnameGetterFunction,
        pathnameSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnURL->protoType().asESPointer()->asESObject(),
        ESString::create("search"), searchGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnURL->protoType().asESPointer()->asESObject(),
        ESString::create("hash"), hashGetterFunction, nullptr);
#endif
    return fnURL;
}
}
