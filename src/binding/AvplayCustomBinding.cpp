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
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
#include "StarFishConfig.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "core/dom/DOMException.h"
#include "core/extra/Avplay.h"

namespace StarFish {

using namespace escargot;

static ESValue openFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Avplay);
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    String* url = toBrowserString(firstArg);
    originalObj->open(url);

    return ESValue(ESValue::ESUndefined);
}

static ESValue prepareFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Avplay);
    originalObj->prepare();

    return ESValue(ESValue::ESUndefined);
}

static ESValue setDisplayRectFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Avplay);
    ESValue Arg1 = instance->currentExecutionContext()->readArgument(0);
    ESValue Arg2 = instance->currentExecutionContext()->readArgument(1);
    ESValue Arg3 = instance->currentExecutionContext()->readArgument(2);
    ESValue Arg4 = instance->currentExecutionContext()->readArgument(3);

    originalObj->setDisplayRect(Arg1.toNumber(), Arg2.toNumber(),
                                Arg3.toNumber(), Arg4.toNumber());

    return ESValue(ESValue::ESUndefined);
}

static ESValue playFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Avplay);
    originalObj->play();

    return ESValue(ESValue::ESUndefined);
}

static ESValue closeFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Avplay);
    originalObj->close();

    return ESValue(ESValue::ESUndefined);
}

static ESValue pauseFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Avplay);
    originalObj->pause();

    return ESValue(ESValue::ESUndefined);
}

static ESValue stopFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Avplay);
    originalObj->stop();

    return ESValue(ESValue::ESUndefined);
}

static ESValue suspendFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Avplay);
    originalObj->suspend();

    return ESValue(ESValue::ESUndefined);
}

static ESValue restoreFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Avplay);
    originalObj->restore();

    return ESValue(ESValue::ESUndefined);
}

static ESValue getStateFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Avplay);
    return toJSString(originalObj->getState());
}

static ESValue getCurrentTimeFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Avplay);
    return ESValue(originalObj->getCurrentTime());
}

static ESValue getDurationFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Avplay);
    return ESValue(originalObj->getDuration());
}

static ESValue setStreamingPropertyFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Avplay);
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    ESValue secondArg = instance->currentExecutionContext()->readArgument(1);
    originalObj->setStreamingProperty(toBrowserString(firstArg),
                                      toBrowserString(secondArg));
    return ESValue(ESValue::ESUndefined);
}

static ESValue prepareAsyncFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Avplay);
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    Avplay* avPlay = ((Window*)instance->globalObject()->extraPointerData())
                         ->Webapis()
                         ->avplay();
    avPlay->prepareAsync(firstArg);

    return ESValue(ESValue::ESUndefined);
}

static ESValue setListenerFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Avplay);
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
    originalObj->setListener(firstArg);

    return ESValue(ESValue::ESUndefined);
}

static ESValue seekToFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Avplay);
    ESValue Arg = instance->currentExecutionContext()->readArgument(0);
    originalObj->seekTo(Arg.toNumber());
    return ESValue(ESValue::ESUndefined);
}

ESFunctionObject* bindingavplay(ScriptBindingInstance* scriptBindingInstance)
{
    ESString* avplayString = ESString::create("avplay");
    ESFunctionObject* avplayFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, avplayString, 0, true, true);
    ESObject* avplayPrototypeObj =
        avplayFunction->protoType().asESPointer()->asESObject();
    avplayFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    avplayPrototypeObj->forceNonVectorHiddenClass(false);
    avplayFunction->set__proto__(fetchData(scriptBindingInstance)
                                     ->m_instance->globalObject()
                                     ->objectPrototype());

    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("open"), true, true, true,
        ESFunctionObject::create(NULL, openFunction, ESString::create("open"),
                                 1, false));

    // webapis.avplay.prepare();
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("prepare"), true, true, true,
        ESFunctionObject::create(NULL, prepareFunction,
                                 ESString::create("prepare"), 1, false));

    // webapis.avplay.setDisplayRect(avPlayerObj.offsetLeft,
    // avPlayerObj.offsetTop, avPlayerObj.offsetWidth,
    // avPlayerObj.offsetHeight);
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("setDisplayRect"), true, true, true,
        ESFunctionObject::create(NULL, setDisplayRectFunction,
                                 ESString::create("setDisplayRect"), 1, false));

    // webapis.avplay.play();
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("play"), true, true, true,
        ESFunctionObject::create(NULL, playFunction, ESString::create("play"),
                                 1, false));

    // webapis.avplay.close();
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("close"), true, true, true,
        ESFunctionObject::create(NULL, closeFunction, ESString::create("close"),
                                 1, false));

    // webapis.avplay.pause();
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("pause"), true, true, true,
        ESFunctionObject::create(NULL, pauseFunction, ESString::create("pause"),
                                 1, false));

    // webapis.avplay.stop();
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("stop"), true, true, true,
        ESFunctionObject::create(NULL, stopFunction, ESString::create("stop"),
                                 1, false));

    // webapis.avplay.suspend();
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("suspend"), true, true, true,
        ESFunctionObject::create(NULL, suspendFunction,
                                 ESString::create("suspend"), 1, false));

    // webapis.avplay.restore();
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("restore"), true, true, true,
        ESFunctionObject::create(NULL, restoreFunction,
                                 ESString::create("restore"), 1, false));

    // webapis.avplay.getState()!=='NONE'
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("getState"), true, true, true,
        ESFunctionObject::create(NULL, getStateFunction,
                                 ESString::create("getState"), 1, false));

    // webapis.avplay.getCurrentTime();
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("getCurrentTime"), true, true, true,
        ESFunctionObject::create(NULL, getCurrentTimeFunction,
                                 ESString::create("getCurrentTime"), 1, false));

    // webapis.avplay.getDuration();
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("getDuration"), true, true, true,
        ESFunctionObject::create(NULL, getDurationFunction,
                                 ESString::create("getDuration"), 1, false));

    // webapis.avplay.setStreamingProperty("SET_MODE_3D",
    // "MODE_3D_EFFECT_SIDE_BY_SIDE");
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("setStreamingProperty"), true, true, true,
        ESFunctionObject::create(NULL, setStreamingPropertyFunction,
                                 ESString::create("setStreamingProperty"), 1,
                                 false));

    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("prepareAsync"), true, true, true,
        ESFunctionObject::create(NULL, prepareAsyncFunction,
                                 ESString::create("prepareAsync"), 1, false));

    // webapis.avplay.setListener(listener);
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("setListener"), true, true, true,
        ESFunctionObject::create(NULL, setListenerFunction,
                                 ESString::create("setListener"), 1, false));

    // webapis.avplay.seekTo( _seekTime );
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("seekTo"), true, true, true,
        ESFunctionObject::create(NULL, seekToFunction,
                                 ESString::create("seekTo"), 1, false));

    return avplayFunction;
}
}
#endif
#endif
