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

#include "extra/Location.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue hrefGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->href();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue hrefSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setHref(value0);
    return ESValue();
}

static ESValue protocolGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->protocol();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue protocolSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setProtocol(value0);
    return ESValue();
}

static ESValue hostGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->host();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue hostSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setHost(value0);
    return ESValue();
}

static ESValue hostnameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->hostname();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue hostnameSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setHostname(value0);
    return ESValue();
}

static ESValue pathnameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->pathname();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue pathnameSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setPathname(value0);
    return ESValue();
}

static ESValue searchGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->search();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue searchSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setSearch(value0);
    return ESValue();
}

static ESValue hashGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->hash();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue hashSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Location);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setHash(value0);
    return ESValue();
}

// Implement for functions
static ESValue toStringFunction(ESVMInstance* instance)
{
    return hrefGetterFunction(instance);
}

ESFunctionObject* bindingLocation(ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* LocationString = ESString::create("Location");
    ESFunctionObject* LocationFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, LocationString, 0, true, true);
    ESObject* LocationPrototypeObj =
        LocationFunction->protoType().asESPointer()->asESObject();
    LocationFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    LocationPrototypeObj->forceNonVectorHiddenClass(false);
    LocationPrototypeObj->set__proto__(fetchData(scriptBindingInstance)
                                           ->m_instance->globalObject()
                                           ->objectPrototype());

    // Bind for attributes
    ESString* hrefString = ESString::create("href");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationPrototypeObj, hrefString, hrefGetterFunction,
        hrefSetterFunction);

    ESString* protocolString = ESString::create("protocol");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationPrototypeObj, protocolString, protocolGetterFunction,
        protocolSetterFunction);

    ESString* hostString = ESString::create("host");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationPrototypeObj, hostString, hostGetterFunction,
        hostSetterFunction);

    ESString* hostnameString = ESString::create("hostname");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationPrototypeObj, hostnameString, hostnameGetterFunction,
        hostnameSetterFunction);

    ESString* pathnameString = ESString::create("pathname");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationPrototypeObj, pathnameString, pathnameGetterFunction,
        pathnameSetterFunction);

    ESString* searchString = ESString::create("search");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationPrototypeObj, searchString, searchGetterFunction,
        searchSetterFunction);

    ESString* hashString = ESString::create("hash");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationPrototypeObj, hashString, hashGetterFunction,
        hashSetterFunction);

    // Bind for functions
    ESString* toStringString = ESString::create("toString");
    ESFunctionObject* toStringESFn = ESFunctionObject::create(
        nullptr, toStringFunction, toStringString, 0, false);
    LocationPrototypeObj->defineDataProperty(toStringString, true, true, true,
                                             toStringESFn);

    return LocationFunction;
}

void Location::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnLocation()->protoType());

    postInit(instance);
}

bool Location::isLocation() const
{
    return true;
}
}
