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

#include "Binding.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

namespace StarFish {

using namespace escargot;

static ESValue vttCueTextFunction(ESVMInstance* instance)
{
    int argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 3) {
        THROW_ILLEGAL_INVOCATION();
    }
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    ESValue secondArg = instance->currentExecutionContext()->readArgument(1);
    ESValue thirdArg = instance->currentExecutionContext()->readArgument(2);
    double startTime = firstArg.toNumber();
    double endTime = secondArg.toNumber();
    if (std::isnan(startTime) || std::isnan(endTime)) {
        THROW_ILLEGAL_INVOCATION();
    }
    VTTCue* cue = new VTTCue(startTime, endTime,
                             String::fromUTF8(thirdArg.toString()->utf8Data()));
    return cue->scriptValue();
}

static ESValue textGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    if (!originalObj->isVTTCue()) {
        THROW_ILLEGAL_INVOCATION();
    }
    VTTCue* cue = (VTTCue*)originalObj;
    return toJSString(cue->text());
}

static ESValue textSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    if (!originalObj->isVTTCue() || !firstArg.isESString()) {
        THROW_ILLEGAL_INVOCATION();
    }
    VTTCue* cue = (VTTCue*)originalObj;
    cue->setText(toBrowserString(firstArg.toString()));
    return firstArg;
}

static ESValue getCueAsHTMLFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(TextTrackCue);
    if (!originalObj->isVTTCue()) {
        THROW_ILLEGAL_INVOCATION();
    }
    VTTCue* cue = (VTTCue*)originalObj;
    Document* document =
        (((Window*)instance->globalObject()->extraPointerData()))->document();
    DocumentFragment* df = cue->getCueAsHTML(document);
    if (df) {
        return df->scriptValue();
    }
    return ESValue(ESValue::ESNull);
}

ESFunctionObject* bindingVTTCue(ScriptBindingInstance* scriptBindingInstance)
{
    auto vttCueFun = ESFunctionObject::create(
        NULL, vttCueTextFunction, ESString::create("VTTCue"), 3, true, true);
    vttCueFun->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    vttCueFun->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    vttCueFun->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)->fnCharacterData()->protoType());
    vttCueFun->set__proto__(fetchData(scriptBindingInstance)->fnTextTrackCue());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        vttCueFun->protoType().asESPointer()->asESObject(),
        ESString::create("text"), textGetterFunction, textSetterFunction);

    vttCueFun->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("getCueAsHTML"), true, true, true,
        ESFunctionObject::create(NULL, getCueAsHTMLFunction,
                                 ESString::create("getCueAsHTML"), 0, false));

    return vttCueFun;
}
}
#endif
