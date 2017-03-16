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
#include "extra/Navigator.h"

namespace StarFish {

using namespace escargot;

ESFunctionObject* bindingNavigator(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(Navigator, fetchData(scriptBindingInstance)
                                                   ->m_instance->globalObject()
                                                   ->objectPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorFunction->protoType().asESPointer()->asESObject(),
        ESString::create("appCodeName"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NavigatorObject,
                                         Navigator);
            return toJSString(originalObj->appCodeName());
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorFunction->protoType().asESPointer()->asESObject(),
        ESString::create("appName"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NavigatorObject,
                                         Navigator);
            return toJSString(originalObj->appName());
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorFunction->protoType().asESPointer()->asESObject(),
        ESString::create("appVersion"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NavigatorObject,
                                         Navigator);
            return toJSString(originalObj->appVersion());
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorFunction->protoType().asESPointer()->asESObject(),
        ESString::create("vendor"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NavigatorObject,
                                         Navigator);
            return toJSString(originalObj->vendor());
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorFunction->protoType().asESPointer()->asESObject(),
        ESString::create("userAgent"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NavigatorObject,
                                         Navigator);
            return toJSString(originalObj->userAgent());
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorFunction->protoType().asESPointer()->asESObject(),
        ESString::create("geolocation"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NavigatorObject,
                                         Navigator);
            return originalObj->geoLocation()->scriptObject();
        },
        nullptr);

    return NavigatorFunction;
}
}
