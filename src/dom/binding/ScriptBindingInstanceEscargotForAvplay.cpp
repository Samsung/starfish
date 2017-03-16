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
#include "ScriptBindingInstance.h"

#include "dom/DOM.h"
#include "dom/binding/escargot/ScriptBindingInstanceDataEscargot.h"
#include "extra/AVPlay.h"

namespace StarFish {

using namespace escargot;

ESFunctionObject* bindingavplay(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(avplay, fetchData(scriptBindingInstance)
                                                ->m_instance->globalObject()
                                                ->objectPrototype());

    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("open"), true, true, true,
        ESFunctionObject::create(
            NULL,
            [](ESVMInstance* instance) -> ESValue {
                GENERATE_THIS_AND_CHECK_TYPE(
                    ScriptWrappable::Type::avplayObject, avplay);
                ESValue firstArg =
                    instance->currentExecutionContext()->readArgument(0);
                avplay* avPlay =
                    ((Window*)instance->globalObject()->extraPointerData())
                        ->Webapis()
                        ->AVPlay();
                String* url = toBrowserString(firstArg);
                avPlay->open(url);

                return ESValue(ESValue::ESUndefined);
            },
            ESString::create("open"), 1, false));

    // webapis.avplay.prepare();
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("prepare"), true, true, true,
        ESFunctionObject::create(
            NULL,
            [](ESVMInstance* instance) -> ESValue {
                GENERATE_THIS_AND_CHECK_TYPE(
                    ScriptWrappable::Type::avplayObject, avplay);
                avplay* avPlay =
                    ((Window*)instance->globalObject()->extraPointerData())
                        ->Webapis()
                        ->AVPlay();
                avPlay->prepare();

                return ESValue(ESValue::ESUndefined);
            },
            ESString::create("prepare"), 1, false));

    // webapis.avplay.setDisplayRect(avPlayerObj.offsetLeft,
    // avPlayerObj.offsetTop, avPlayerObj.offsetWidth,
    // avPlayerObj.offsetHeight);
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("setDisplayRect"), true, true, true,
        ESFunctionObject::create(
            NULL,
            [](ESVMInstance* instance) -> ESValue {
                GENERATE_THIS_AND_CHECK_TYPE(
                    ScriptWrappable::Type::avplayObject, avplay);
                ESValue Arg1 =
                    instance->currentExecutionContext()->readArgument(0);
                ESValue Arg2 =
                    instance->currentExecutionContext()->readArgument(1);
                ESValue Arg3 =
                    instance->currentExecutionContext()->readArgument(2);
                ESValue Arg4 =
                    instance->currentExecutionContext()->readArgument(3);

                avplay* avPlay =
                    ((Window*)instance->globalObject()->extraPointerData())
                        ->Webapis()
                        ->AVPlay();
                avPlay->setDisplayRect(Arg1.toNumber(), Arg2.toNumber(),
                                       Arg3.toNumber(), Arg4.toNumber());

                return ESValue(ESValue::ESUndefined);
            },
            ESString::create("setDisplayRect"), 1, false));

    // webapis.avplay.play();
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("play"), true, true, true,
        ESFunctionObject::create(
            NULL,
            [](ESVMInstance* instance) -> ESValue {
                GENERATE_THIS_AND_CHECK_TYPE(
                    ScriptWrappable::Type::avplayObject, avplay);
                avplay* avPlay =
                    ((Window*)instance->globalObject()->extraPointerData())
                        ->Webapis()
                        ->AVPlay();
                avPlay->play();

                return ESValue(ESValue::ESUndefined);
            },
            ESString::create("play"), 1, false));

    // webapis.avplay.close();
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("close"), true, true, true,
        ESFunctionObject::create(
            NULL,
            [](ESVMInstance* instance) -> ESValue {
                GENERATE_THIS_AND_CHECK_TYPE(
                    ScriptWrappable::Type::avplayObject, avplay);
                avplay* avPlay =
                    ((Window*)instance->globalObject()->extraPointerData())
                        ->Webapis()
                        ->AVPlay();
                avPlay->close();

                return ESValue(ESValue::ESUndefined);
            },
            ESString::create("close"), 1, false));

    // webapis.avplay.pause();
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("pause"), true, true, true,
        ESFunctionObject::create(
            NULL,
            [](ESVMInstance* instance) -> ESValue {
                GENERATE_THIS_AND_CHECK_TYPE(
                    ScriptWrappable::Type::avplayObject, avplay);
                avplay* avPlay =
                    ((Window*)instance->globalObject()->extraPointerData())
                        ->Webapis()
                        ->AVPlay();
                avPlay->pause();

                return ESValue(ESValue::ESUndefined);
            },
            ESString::create("pause"), 1, false));

    // webapis.avplay.stop();
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("stop"), true, true, true,
        ESFunctionObject::create(
            NULL,
            [](ESVMInstance* instance) -> ESValue {
                GENERATE_THIS_AND_CHECK_TYPE(
                    ScriptWrappable::Type::avplayObject, avplay);
                avplay* avPlay =
                    ((Window*)instance->globalObject()->extraPointerData())
                        ->Webapis()
                        ->AVPlay();
                avPlay->stop();

                return ESValue(ESValue::ESUndefined);
            },
            ESString::create("stop"), 1, false));

    // webapis.avplay.suspend();
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("suspend"), true, true, true,
        ESFunctionObject::create(
            NULL,
            [](ESVMInstance* instance) -> ESValue {
                GENERATE_THIS_AND_CHECK_TYPE(
                    ScriptWrappable::Type::avplayObject, avplay);
                avplay* avPlay =
                    ((Window*)instance->globalObject()->extraPointerData())
                        ->Webapis()
                        ->AVPlay();
                avPlay->suspend();

                return ESValue(ESValue::ESUndefined);
            },
            ESString::create("suspend"), 1, false));

    // webapis.avplay.restore();
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("restore"), true, true, true,
        ESFunctionObject::create(
            NULL,
            [](ESVMInstance* instance) -> ESValue {
                GENERATE_THIS_AND_CHECK_TYPE(
                    ScriptWrappable::Type::avplayObject, avplay);
                avplay* avPlay =
                    ((Window*)instance->globalObject()->extraPointerData())
                        ->Webapis()
                        ->AVPlay();
                avPlay->restore();

                return ESValue(ESValue::ESUndefined);
            },
            ESString::create("restore"), 1, false));

    // webapis.avplay.getState()!=='NONE'
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("getState"), true, true, true,
        ESFunctionObject::create(
            NULL,
            [](ESVMInstance* instance) -> ESValue {
                GENERATE_THIS_AND_CHECK_TYPE(
                    ScriptWrappable::Type::avplayObject, avplay);
                avplay* avPlay =
                    ((Window*)instance->globalObject()->extraPointerData())
                        ->Webapis()
                        ->AVPlay();
                return toJSString(avPlay->getState());
            },
            ESString::create("getState"), 1, false));

    // webapis.avplay.getCurrentTime();
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("getCurrentTime"), true, true, true,
        ESFunctionObject::create(
            NULL,
            [](ESVMInstance* instance) -> ESValue {
                GENERATE_THIS_AND_CHECK_TYPE(
                    ScriptWrappable::Type::avplayObject, avplay);
                avplay* avPlay =
                    ((Window*)instance->globalObject()->extraPointerData())
                        ->Webapis()
                        ->AVPlay();
                return ESValue(avPlay->getCurrentTime());
            },
            ESString::create("getCurrentTime"), 1, false));

    // webapis.avplay.getDuration();
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("getDuration"), true, true, true,
        ESFunctionObject::create(
            NULL,
            [](ESVMInstance* instance) -> ESValue {
                GENERATE_THIS_AND_CHECK_TYPE(
                    ScriptWrappable::Type::avplayObject, avplay);
                avplay* avPlay =
                    ((Window*)instance->globalObject()->extraPointerData())
                        ->Webapis()
                        ->AVPlay();
                return ESValue(avPlay->getDuration());
            },
            ESString::create("getDuration"), 1, false));

    // webapis.avplay.setStreamingProperty("SET_MODE_3D",
    // "MODE_3D_EFFECT_SIDE_BY_SIDE");
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("setStreamingProperty"), true, true, true,
        ESFunctionObject::create(
            NULL,
            [](ESVMInstance* instance) -> ESValue {
                GENERATE_THIS_AND_CHECK_TYPE(
                    ScriptWrappable::Type::avplayObject, avplay);
                ESValue firstArg =
                    instance->currentExecutionContext()->readArgument(0);
                ESValue secondArg =
                    instance->currentExecutionContext()->readArgument(1);
                avplay* avPlay =
                    ((Window*)instance->globalObject()->extraPointerData())
                        ->Webapis()
                        ->AVPlay();
                avPlay->setStreamingProperty(toBrowserString(firstArg),
                                             toBrowserString(secondArg));
                return ESValue(ESValue::ESUndefined);
            },
            ESString::create("setStreamingProperty"), 1, false));

    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("prepareAsync"), true, true, true,
        ESFunctionObject::create(
            NULL,
            [](ESVMInstance* instance) -> ESValue {
                GENERATE_THIS_AND_CHECK_TYPE(
                    ScriptWrappable::Type::avplayObject, avplay);
                ESValue firstArg =
                    instance->currentExecutionContext()->readArgument(0);
                avplay* avPlay =
                    ((Window*)instance->globalObject()->extraPointerData())
                        ->Webapis()
                        ->AVPlay();
                avPlay->prepareAsync(firstArg);

                return ESValue(ESValue::ESUndefined);
            },
            ESString::create("prepareAsync"), 1, false));

    // webapis.avplay.setListener(listener);
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("setListener"), true, true, true,
        ESFunctionObject::create(
            NULL,
            [](ESVMInstance* instance) -> ESValue {
                GENERATE_THIS_AND_CHECK_TYPE(
                    ScriptWrappable::Type::avplayObject, avplay);
                ESValue firstArg =
                    instance->currentExecutionContext()->readArgument(0);
                avplay* avPlay =
                    ((Window*)instance->globalObject()->extraPointerData())
                        ->Webapis()
                        ->AVPlay();
                avPlay->setListener(firstArg);

                return ESValue(ESValue::ESUndefined);
            },
            ESString::create("setListener"), 1, false));

    // webapis.avplay.seekTo( _seekTime );
    avplayFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("seekTo"), true, true, true,
        ESFunctionObject::create(
            NULL,
            [](ESVMInstance* instance) -> ESValue {
                GENERATE_THIS_AND_CHECK_TYPE(
                    ScriptWrappable::Type::avplayObject, avplay);
                ESValue Arg =
                    instance->currentExecutionContext()->readArgument(0);
                avplay* avPlay =
                    ((Window*)instance->globalObject()->extraPointerData())
                        ->Webapis()
                        ->AVPlay();
                avPlay->seekTo(Arg.toNumber());
                return ESValue(ESValue::ESUndefined);
            },
            ESString::create("seekTo"), 1, false));

    return avplayFunction;
}
}
#endif
#endif
