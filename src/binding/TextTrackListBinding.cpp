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
#include "dom/TextTrack.h"
#include "dom/TextTrackList.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue lengthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackList);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->length();
    // Return ESValue from native value
    return ESValue(result);
}

// Implement for functions
static ESValue getTrackByIdFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackList);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "getTrackById", "TextTrackList",
                        reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    TextTrack* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    // Call native function (nargs: 1)
    result = originalObj->getTrackById(value0);

    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

ESFunctionObject* bindingTextTrackList(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* TextTrackListString = ESString::create("TextTrackList");
    ESFunctionObject* TextTrackListFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 TextTrackListString, 0, true, true);
    ESObject* TextTrackListPrototypeObj =
        TextTrackListFunction->protoType().asESPointer()->asESObject();
    TextTrackListFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    TextTrackListPrototypeObj->forceNonVectorHiddenClass(false);
    TextTrackListPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnEventTarget()->protoType());
    TextTrackListFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnEventTarget());

    // Bind for attributes
    ESString* lengthString = ESString::create("length");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackListPrototypeObj, lengthString, lengthGetterFunction, nullptr);

    // Bind for functions
    ESString* getTrackByIdString = ESString::create("getTrackById");
    ESFunctionObject* getTrackByIdESFn = ESFunctionObject::create(
        nullptr, getTrackByIdFunction, getTrackByIdString, 1, false);
    TextTrackListPrototypeObj->defineDataProperty(getTrackByIdString, true,
                                                  true, true, getTrackByIdESFn);

    return TextTrackListFunction;
}

void TextTrackList::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnTextTrackList()->protoType());

    postInit(instance);
}

bool TextTrackList::isTextTrackList() const
{
    return true;
}
}
#endif
