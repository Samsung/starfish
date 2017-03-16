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

#ifdef STARFISH_ENABLE_MULTIMEDIA
#include "StarFishConfig.h"
#include "ScriptBindingInstance.h"

#include "dom/DOM.h"
#include "dom/binding/escargot/ScriptBindingInstanceDataEscargot.h"
#include "extra/SourceBuffer.h"

namespace StarFish {

using namespace escargot;

static ESValue lengthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::SourceBufferListObject,
                                 SourceBufferList);
    uint32_t len = originalObj->length();
    return ESValue(len);
}

ESFunctionObject* bindingSourceBufferList(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        SourceBufferList, fetchData(scriptBindingInstance)->m_eventTarget);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferListFunction->protoType().asESPointer()->asESObject(),
        ESString::create("length"), lengthGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferListFunction->protoType().asESPointer()->asESObject(),
        ESString::create("SourceBuffer"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(
                ScriptWrappable::Type::SourceBufferListObject,
                SourceBufferList);
            if (v.toIndex() >= originalObj->length()) {
                return ESValue(ESValue::ESUndefined);
            } else {
                SourceBuffer* buffer = (*originalObj)[v.toIndex()];
                return buffer->scriptValue();
            }

        },
        nullptr);

    return SourceBufferListFunction;
}
}
#endif
