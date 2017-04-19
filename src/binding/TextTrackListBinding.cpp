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
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "dom/DOMException.h"
#include "dom/TextTrackList.h"

namespace StarFish {

using namespace escargot;

static ESValue lengthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackList);
    uint32_t len = originalObj->size();
    return ESValue(len);
}

static ESValue getTrackByIdGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackList);
    ESValue v = instance->currentExecutionContext()->readArgument(0);
    String* id = toBrowserString(v.toString());
    auto iter = std::find_if(
        originalObj->begin(), originalObj->end(),
        [&id](TextTrack* track) { return track->id()->equals(id); });

    if (iter != originalObj->end()) {
        return (*iter)->scriptValue();
    }
    return ESValue(ESValue::ESNull);
}

ESFunctionObject* bindingTextTrackList(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        TextTrackList, fetchData(scriptBindingInstance)->m_fnEventTarget);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackListFunction->protoType().asESPointer()->asESObject(),
        ESString::create("length"), lengthGetterFunction, nullptr);

    TextTrackListFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("getTrackById"), true, true, true,
                             ESFunctionObject::create(
                                 NULL, getTrackByIdGetterFunction,
                                 ESString::create("getTrackById"), 0, false));

    return TextTrackListFunction;
}
}
#endif
