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
#include "extra/Location.h"

namespace StarFish {

using namespace escargot;

static ESValue hrefGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    return toJSString(originalObj->getHref());
}

static ESValue hrefSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    int argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    } else {
        originalObj->setHref(
            String::fromUTF8(instance->currentExecutionContext()
                                 ->readArgument(0)
                                 .toString()
                                 ->utf8Data()));
    }
    return ESValue();
}

static ESValue pathNameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    return toJSString(originalObj->getPathname());
}

static ESValue pathNameSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    int argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    } else {
        originalObj->setPathname(
            String::fromUTF8(instance->currentExecutionContext()
                                 ->readArgument(0)
                                 .toString()
                                 ->utf8Data()));
    }
    return ESValue();
}

static ESValue searchGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    return toJSString(originalObj->getSearch());
}

static ESValue searchSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    int argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    } else {
        originalObj->setSearch(
            String::fromUTF8(instance->currentExecutionContext()
                                 ->readArgument(0)
                                 .toString()
                                 ->utf8Data()));
    }
    return ESValue();
}

static ESValue hashGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    return toJSString(originalObj->getHash());
}

static ESValue hashSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    int argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    } else {
        originalObj->setHash(
            String::fromUTF8(instance->currentExecutionContext()
                                 ->readArgument(0)
                                 .toString()
                                 ->utf8Data()));
    }
    return ESValue();
}

static ESValue hostGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    return toJSString(originalObj->getHost());
}

static ESValue hostNameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    return toJSString(originalObj->getHostname());
}

static ESValue protocolGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    return toJSString(originalObj->getProtocol());
}

static ESValue protocolSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    int argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    } else {
        originalObj->setProtocol(
            String::fromUTF8(instance->currentExecutionContext()
                                 ->readArgument(0)
                                 .toString()
                                 ->utf8Data()));
    }
    return ESValue();
}

ESFunctionObject* bindingLocation(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(Location, fetchData(scriptBindingInstance)
                                                  ->m_instance->globalObject()
                                                  ->objectPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationFunction->protoType().asESPointer()->asESObject(),
        ESString::create("href"), hrefGetterFunction, hrefSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationFunction->protoType().asESPointer()->asESObject(),
        ESString::create("pathname"), pathNameGetterFunction,
        pathNameSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationFunction->protoType().asESPointer()->asESObject(),
        ESString::create("search"), searchGetterFunction, searchSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationFunction->protoType().asESPointer()->asESObject(),
        ESString::create("hash"), hashGetterFunction, hashSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationFunction->protoType().asESPointer()->asESObject(),
        ESString::create("host"), hostGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationFunction->protoType().asESPointer()->asESObject(),
        ESString::create("hostname"), hostNameGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationFunction->protoType().asESPointer()->asESObject(),
        ESString::create("protocol"), protocolGetterFunction,
        protocolSetterFunction);

    return LocationFunction;
}
}
