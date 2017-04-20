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
#include "dom/DOMRectReadOnly.h"

namespace StarFish {

using namespace escargot;

static ESValue xGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRectReadOnly);
    DOMRectReadOnly* rect = originalObj;
    return ESValue(rect->x());
}

static ESValue yGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRectReadOnly);
    DOMRectReadOnly* rect = originalObj;
    return ESValue(rect->y());
}

static ESValue widthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRectReadOnly);
    DOMRectReadOnly* rect = originalObj;
    return ESValue(rect->width());
}

static ESValue heightGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRectReadOnly);
    DOMRectReadOnly* rect = originalObj;
    return ESValue(rect->height());
}

static ESValue topGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRectReadOnly);
    DOMRectReadOnly* rect = originalObj;
    return ESValue(rect->top());
}

static ESValue rightGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRectReadOnly);
    DOMRectReadOnly* rect = originalObj;
    return ESValue(rect->right());
}

static ESValue bottomGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRectReadOnly);
    DOMRectReadOnly* rect = originalObj;
    return ESValue(rect->bottom());
}

static ESValue leftGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRectReadOnly);
    DOMRectReadOnly* rect = originalObj;
    return ESValue(rect->left());
}

ESFunctionObject* bindingDOMRectReadOnly(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(DOMRectReadOnly,
                                    fetchData(scriptBindingInstance)
                                        ->m_instance->globalObject()
                                        ->objectPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMRectReadOnlyFunction->protoType().asESPointer()->asESObject(),
        ESString::create("x"), xGetterFunction, nullptr, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMRectReadOnlyFunction->protoType().asESPointer()->asESObject(),
        ESString::create("y"), yGetterFunction, nullptr, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMRectReadOnlyFunction->protoType().asESPointer()->asESObject(),
        ESString::create("width"), widthGetterFunction, nullptr, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMRectReadOnlyFunction->protoType().asESPointer()->asESObject(),
        ESString::create("height"), heightGetterFunction, nullptr, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMRectReadOnlyFunction->protoType().asESPointer()->asESObject(),
        ESString::create("top"), topGetterFunction, nullptr, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMRectReadOnlyFunction->protoType().asESPointer()->asESObject(),
        ESString::create("right"), rightGetterFunction, nullptr, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMRectReadOnlyFunction->protoType().asESPointer()->asESObject(),
        ESString::create("bottom"), bottomGetterFunction, nullptr, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMRectReadOnlyFunction->protoType().asESPointer()->asESObject(),
        ESString::create("left"), leftGetterFunction, nullptr, true, true);

    return DOMRectReadOnlyFunction;
}
}
