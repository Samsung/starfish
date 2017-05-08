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

#include "platform/location/Geolocation.h"
#include "extra/Navigator.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue geolocationGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Navigator);
    // Declare native value (empty when type is void)
    Geolocation* result = nullptr;
    result = originalObj->geolocation();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue appCodeNameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Navigator);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->appCodeName();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue appNameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Navigator);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->appName();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue appVersionGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Navigator);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->appVersion();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue userAgentGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Navigator);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->userAgent();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue vendorGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Navigator);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->vendor();
    // Return ESValue from native value
    return toJSString(result);
}

ESFunctionObject* bindingNavigator(ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* NavigatorString = ESString::create("Navigator");
    ESFunctionObject* NavigatorFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, NavigatorString, 0, true, true);
    ESObject* NavigatorPrototypeObj =
        NavigatorFunction->protoType().asESPointer()->asESObject();
    NavigatorFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    NavigatorPrototypeObj->forceNonVectorHiddenClass(false);
    NavigatorPrototypeObj->set__proto__(fetchData(scriptBindingInstance)
                                            ->m_instance->globalObject()
                                            ->objectPrototype());

    // Bind for attributes
    ESString* geolocationString = ESString::create("geolocation");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorPrototypeObj, geolocationString, geolocationGetterFunction,
        nullptr);

    ESString* appCodeNameString = ESString::create("appCodeName");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorPrototypeObj, appCodeNameString, appCodeNameGetterFunction,
        nullptr);

    ESString* appNameString = ESString::create("appName");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorPrototypeObj, appNameString, appNameGetterFunction, nullptr);

    ESString* appVersionString = ESString::create("appVersion");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorPrototypeObj, appVersionString, appVersionGetterFunction,
        nullptr);

    ESString* userAgentString = ESString::create("userAgent");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorPrototypeObj, userAgentString, userAgentGetterFunction,
        nullptr);

    ESString* vendorString = ESString::create("vendor");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorPrototypeObj, vendorString, vendorGetterFunction, nullptr);

    return NavigatorFunction;
}
}
