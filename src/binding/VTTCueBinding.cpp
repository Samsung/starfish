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
#include "dom/DocumentFragment.h"
#include "dom/VTTCue.h"

namespace StarFish {

using namespace escargot;

// Implement for constructor
static ESValue vttcueConstructor(ESVMInstance* instance)
{
    if (!instance->currentExecutionContext()->isNewExpression()) {
        THROW_EXCEPTION(CALLED_CONSTRUCTOR_WITHOUT_NEW, "VTTCue");
    }
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 3) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_CONSTRUCT_BECAUSE_ARGS_NOT_ENOUGH, "VTTCue",
                        "3", buffer);
    }
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    ESValue arg2 = instance->currentExecutionContext()->readArgument(2);
    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();

    // Handle argument arg1
    double value1;
    value1 = arg1.toNumber();

    // Handle argument arg2
    String* value2 = String::emptyString;
    value2 = toBrowserString(arg2);

    VTTCue* result = nullptr;
    Document* callWith = fetchDocument(instance);
    // Call native function (nargs: 3)
    result = new VTTCue(callWith, value0, value1, value2);
    return result->scriptValue();
}

// Implement for attributes
static ESValue textGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(VTTCue);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->text();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue textSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(VTTCue);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setText(value0);
    return ESValue();
}

// Implement for functions
static ESValue getCueAsHTMLFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(VTTCue);
    // Declare return value (empty when void)
    DocumentFragment* result = nullptr;
    // Call native function (nargs: 0)
    result = originalObj->getCueAsHTML();

    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

ESFunctionObject* bindingVTTCue(ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* VTTCueString = ESString::create("VTTCue");
    ESFunctionObject* VTTCueFunction = ESFunctionObject::create(
        nullptr, vttcueConstructor, VTTCueString, 3, true, true);
    VTTCueFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    VTTCueFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    VTTCueFunction->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)->fnTextTrackCue()->protoType());
    VTTCueFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnTextTrackCue());
    ESObject* VTTCuePrototypeObj =
        VTTCueFunction->protoType().asESPointer()->asESObject();

    // Bind for attributes
    ESString* textString = ESString::create("text");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        VTTCuePrototypeObj, textString, textGetterFunction, textSetterFunction);

    // Bind for functions
    ESString* getCueAsHTMLString = ESString::create("getCueAsHTML");
    ESFunctionObject* getCueAsHTMLESFn = ESFunctionObject::create(
        nullptr, getCueAsHTMLFunction, getCueAsHTMLString, 0, false);
    VTTCuePrototypeObj->defineDataProperty(getCueAsHTMLString, true, true, true,
                                           getCueAsHTMLESFn);

    return VTTCueFunction;
}
}
#endif
