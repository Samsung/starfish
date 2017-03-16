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

ESFunctionObject* bindingKeyboardEvent(
    ScriptBindingInstance* scriptBindingInstance)
{
    /* Keyboard Events */
    DEFINE_FUNCTION_WITH_PARENTFUNC(
        KeyboardEvent, fetchData(scriptBindingInstance)->uiEvent());
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        KeyboardEventFunction->protoType().asESPointer()->asESObject(),
        ESString::create("keyCode"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject,
                                         Event);
            if (originalObj->isUIEvent() &&
                originalObj->asUIEvent()->isKeyboardEvent()) {
                return ESValue(
                    originalObj->asUIEvent()->asKeyboardEvent()->keyCode());
            }
            THROW_ILLEGAL_INVOCATION();
        },
        nullptr);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        KeyboardEventFunction->protoType().asESPointer()->asESObject(),
        ESString::create("ctrlKey"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject,
                                         Event);
            if (originalObj->isUIEvent() &&
                originalObj->asUIEvent()->isKeyboardEvent()) {
                return ESValue(
                    originalObj->asUIEvent()->asKeyboardEvent()->ctrlKey());
            }
            THROW_ILLEGAL_INVOCATION();
        },
        nullptr);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        KeyboardEventFunction->protoType().asESPointer()->asESObject(),
        ESString::create("altKey"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject,
                                         Event);
            if (originalObj->isUIEvent() &&
                originalObj->asUIEvent()->isKeyboardEvent()) {
                return ESValue(
                    originalObj->asUIEvent()->asKeyboardEvent()->altKey());
            }
            THROW_ILLEGAL_INVOCATION();
        },
        nullptr);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        KeyboardEventFunction->protoType().asESPointer()->asESObject(),
        ESString::create("shiftKey"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject,
                                         Event);
            if (originalObj->isUIEvent() &&
                originalObj->asUIEvent()->isKeyboardEvent()) {
                return ESValue(
                    originalObj->asUIEvent()->asKeyboardEvent()->shiftKey());
            }
            THROW_ILLEGAL_INVOCATION();
        },
        nullptr);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        KeyboardEventFunction->protoType().asESPointer()->asESObject(),
        ESString::create("metaKey"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject,
                                         Event);
            if (originalObj->isUIEvent() &&
                originalObj->asUIEvent()->isKeyboardEvent()) {
                return ESValue(
                    originalObj->asUIEvent()->asKeyboardEvent()->metaKey());
            }
            THROW_ILLEGAL_INVOCATION();
        },
        nullptr);
    return KeyboardEventFunction;
}
}
