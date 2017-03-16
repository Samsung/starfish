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

ESFunctionObject* bindingDOMRectReadOnly(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(DOMRectReadOnly,
                                    fetchData(scriptBindingInstance)
                                        ->m_instance->globalObject()
                                        ->objectPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMRectReadOnlyFunction->protoType().asESPointer()->asESObject(),
        ESString::create("x"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(
                ScriptWrappable::Type::DOMRectReadOnlyObject, DOMRectReadOnly);
            DOMRectReadOnly* rect = originalObj;
            return ESValue(rect->x());
        },
        nullptr, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMRectReadOnlyFunction->protoType().asESPointer()->asESObject(),
        ESString::create("y"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(
                ScriptWrappable::Type::DOMRectReadOnlyObject, DOMRectReadOnly);
            DOMRectReadOnly* rect = originalObj;
            return ESValue(rect->y());
        },
        nullptr, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMRectReadOnlyFunction->protoType().asESPointer()->asESObject(),
        ESString::create("width"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(
                ScriptWrappable::Type::DOMRectReadOnlyObject, DOMRectReadOnly);
            DOMRectReadOnly* rect = originalObj;
            return ESValue(rect->width());
        },
        nullptr, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMRectReadOnlyFunction->protoType().asESPointer()->asESObject(),
        ESString::create("height"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(
                ScriptWrappable::Type::DOMRectReadOnlyObject, DOMRectReadOnly);
            DOMRectReadOnly* rect = originalObj;
            return ESValue(rect->height());
        },
        nullptr, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMRectReadOnlyFunction->protoType().asESPointer()->asESObject(),
        ESString::create("top"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(
                ScriptWrappable::Type::DOMRectReadOnlyObject, DOMRectReadOnly);
            DOMRectReadOnly* rect = originalObj;
            return ESValue(rect->top());
        },
        nullptr, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMRectReadOnlyFunction->protoType().asESPointer()->asESObject(),
        ESString::create("right"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(
                ScriptWrappable::Type::DOMRectReadOnlyObject, DOMRectReadOnly);
            DOMRectReadOnly* rect = originalObj;
            return ESValue(rect->right());
        },
        nullptr, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMRectReadOnlyFunction->protoType().asESPointer()->asESObject(),
        ESString::create("bottom"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(
                ScriptWrappable::Type::DOMRectReadOnlyObject, DOMRectReadOnly);
            DOMRectReadOnly* rect = originalObj;
            return ESValue(rect->bottom());
        },
        nullptr, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMRectReadOnlyFunction->protoType().asESPointer()->asESObject(),
        ESString::create("left"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(
                ScriptWrappable::Type::DOMRectReadOnlyObject, DOMRectReadOnly);
            DOMRectReadOnly* rect = originalObj;
            return ESValue(rect->left());
        },
        nullptr, true, true);

    return DOMRectReadOnlyFunction;
}
}
