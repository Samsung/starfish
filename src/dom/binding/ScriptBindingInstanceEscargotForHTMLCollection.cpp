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

ESFunctionObject* bindingHTMLCollection(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLCollection,
                                    fetchData(scriptBindingInstance)
                                        ->m_instance->globalObject()
                                        ->objectPrototype());

    /* 4.2.7.2 Interface HTMLCollection */
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLCollectionFunction->protoType().asESPointer()->asESObject(),
        ESString::create("length"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(
                ScriptWrappable::Type::HTMLCollectionObject, HTMLCollection);
            uint32_t len = originalObj->length();
            return ESValue(len);
        },
        nullptr);

    ESFunctionObject* itemFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            ESValue thisValue =
                instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue,
                         ScriptWrappable::Type::HTMLCollectionObject);
            HTMLCollection* self = (HTMLCollection*)(thisValue.asESPointer()
                                                         ->asESObject()
                                                         ->extraPointerData());

            size_t count = instance->currentExecutionContext()->argumentCount();
            if (count > 0) {
                ESValue argValue =
                    instance->currentExecutionContext()->readArgument(0);
                TO_INDEX_UINT32(argValue, idx);
                if (idx != INVALID_INDEX && idx < self->length()) {
                    Element* elem = self->item(idx);
                    STARFISH_ASSERT(elem != nullptr);
                    return elem->scriptValue();
                }
                return ESValue(ESValue::ESNull);
            } else {
                auto msg = ESString::create(
                    "Failed to execute 'hasAttribute' on Element: 1 "
                    "argument required, but only 0 present.");
                instance->throwError(ESValue(TypeError::create(msg)));
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        },
        ESString::create("item"), 1, false);
    HTMLCollectionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("item"), true, true, true,
                             itemFunction);

    ESFunctionObject* namedItemFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            ESValue thisValue =
                instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue,
                         ScriptWrappable::Type::HTMLCollectionObject);

            size_t count = instance->currentExecutionContext()->argumentCount();
            if (count > 0) {
                ESValue argValue =
                    instance->currentExecutionContext()->readArgument(0);
                ESString* argStr = argValue.toString();
                Element* elem = ((HTMLCollection*)thisValue.asESPointer()
                                     ->asESObject()
                                     ->extraPointerData())
                                    ->namedItem(toBrowserString(argStr));
                if (elem != nullptr) {
                    return elem->scriptValue();
                }
                return ESValue(ESValue::ESNull);
            } else {
                auto msg = ESString::create(
                    "Failed to execute 'namedItem' on 'HTMLCollection': 1 "
                    "argument required, but only 0 present.");
                instance->throwError(ESValue(TypeError::create(msg)));
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        },
        ESString::create("namedItem"), 1, false);
    HTMLCollectionFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("namedItem"), true, true, true,
                             namedItemFunction);

    return HTMLCollectionFunction;
}
}
