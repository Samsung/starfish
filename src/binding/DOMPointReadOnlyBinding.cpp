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

#include "dom/DOMPointReadOnly.h"

namespace StarFish {

using namespace escargot;

static ESValue xGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPointReadOnly);
    DOMPointReadOnly* point = originalObj;
    return ESValue(point->x());
}

static ESValue yGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPointReadOnly);
    DOMPointReadOnly* point = originalObj;
    return ESValue(point->y());
}

static ESValue zGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPointReadOnly);
    DOMPointReadOnly* point = originalObj;
    return ESValue(point->z());
}

static ESValue wGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPointReadOnly);
    DOMPointReadOnly* point = originalObj;
    return ESValue(point->w());
}

ESFunctionObject* bindingDOMPointReadOnly(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(DOMPointReadOnly,
                                    fetchData(scriptBindingInstance)
                                        ->m_instance->globalObject()
                                        ->objectPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMPointReadOnlyFunction->protoType().asESPointer()->asESObject(),
        ESString::create("x"), xGetterFunction, nullptr, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMPointReadOnlyFunction->protoType().asESPointer()->asESObject(),
        ESString::create("y"), yGetterFunction, nullptr, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMPointReadOnlyFunction->protoType().asESPointer()->asESObject(),
        ESString::create("z"), zGetterFunction, nullptr, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMPointReadOnlyFunction->protoType().asESPointer()->asESObject(),
        ESString::create("w"), wGetterFunction, nullptr, true, true);

    return DOMPointReadOnlyFunction;
}
}
