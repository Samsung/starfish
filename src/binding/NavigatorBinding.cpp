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

#include "Binding.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

namespace StarFish {

using namespace escargot;

static ESValue appCodeNameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Navigator);
    return toJSString(originalObj->appCodeName());
}

static ESValue appNameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Navigator);
    return toJSString(originalObj->appName());
}

static ESValue appVersionGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Navigator);
    return toJSString(originalObj->appVersion());
}

static ESValue vendorGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Navigator);
    return toJSString(originalObj->vendor());
}

static ESValue userAgentGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Navigator);
    return toJSString(originalObj->userAgent());
}

static ESValue geolocationGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Navigator);
    return originalObj->geoLocation()->scriptObject();
}

ESFunctionObject* bindingNavigator(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(Navigator, fetchData(scriptBindingInstance)
                                                   ->m_instance->globalObject()
                                                   ->objectPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorFunction->protoType().asESPointer()->asESObject(),
        ESString::create("appCodeName"), appCodeNameGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorFunction->protoType().asESPointer()->asESObject(),
        ESString::create("appName"), appNameGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorFunction->protoType().asESPointer()->asESObject(),
        ESString::create("appVersion"), appVersionGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorFunction->protoType().asESPointer()->asESObject(),
        ESString::create("vendor"), vendorGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorFunction->protoType().asESPointer()->asESObject(),
        ESString::create("userAgent"), userAgentGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorFunction->protoType().asESPointer()->asESObject(),
        ESString::create("geolocation"), geolocationGetterFunction, nullptr);

    return NavigatorFunction;
}
}
