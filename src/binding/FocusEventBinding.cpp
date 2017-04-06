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
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

namespace StarFish {

using namespace escargot;

static ESValue relatedTargetGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Event);
    if (originalObj->isFocusEvent()) {
        return ESValue(originalObj->asFocusEvent()->relatedTarget());
    }
    THROW_ILLEGAL_INVOCATION();
}

ESFunctionObject* bindingFocusEvent(
    ScriptBindingInstance* scriptBindingInstance)
{
    /* Focus Events */
    DEFINE_FUNCTION_WITH_PARENTFUNC(
        FocusEvent, fetchData(scriptBindingInstance)->uiEvent());
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        FocusEventFunction->protoType().asESPointer()->asESObject(),
        ESString::create("relatedTarget"), relatedTargetGetterFunction,
        nullptr);
    return FocusEventFunction;
}
}
