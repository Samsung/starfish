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
#if defined(STARFISH_ENABLE_MULTIMEDIA)
#include "StarFishConfig.h"
#include "ScriptBindingInstance.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"
#include "dom/TextTrackCue.h"
#include "dom/TextTrackCueList.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue lengthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCueList);
    // Declare return value (empty when void)
    uint32_t result;
    result = originalObj->length();
    // Return ESValue from native value
    return ESValue(result);
}

// Implement for functions
ESFunctionObject* bindingTextTrackCueList(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* TextTrackCueListString = ESString::create("TextTrackCueList");
    ESFunctionObject* TextTrackCueListFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 TextTrackCueListString, 0, true, true);
    TextTrackCueListFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    TextTrackCueListFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    TextTrackCueListFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->set__proto__(fetchData(scriptBindingInstance)
                           ->m_instance->globalObject()
                           ->objectPrototype());
    ESObject* TextTrackCueListPrototypeObj =
        TextTrackCueListFunction->protoType().asESPointer()->asESObject();

    // Bind for attributes
    ESString* lengthString = ESString::create("length");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackCueListPrototypeObj, lengthString, lengthGetterFunction,
        nullptr);

    // Bind for functions
    return TextTrackCueListFunction;
}
}
#endif
