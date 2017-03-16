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

namespace StarFish {

using namespace escargot;

ESFunctionObject* bindingHTMLMediaElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        HTMLMediaElement, fetchData(scriptBindingInstance)->htmlElement());

    HTMLMediaElementFunction->asESObject()->defineDataProperty(
        ESString::create("NETWORK_EMPTY"), false, true, false,
        ESValue(HTMLMediaElement::NETWORK_EMPTY));
    HTMLMediaElementFunction->asESObject()->defineDataProperty(
        ESString::create("NETWORK_IDLE"), false, true, false,
        ESValue(HTMLMediaElement::NETWORK_IDLE));
    HTMLMediaElementFunction->asESObject()->defineDataProperty(
        ESString::create("NETWORK_LOADING"), false, true, false,
        ESValue(HTMLMediaElement::NETWORK_LOADING));
    HTMLMediaElementFunction->asESObject()->defineDataProperty(
        ESString::create("NETWORK_NO_SOURCE"), false, true, false,
        ESValue(HTMLMediaElement::NETWORK_NO_SOURCE));
    HTMLMediaElementFunction->asESObject()->defineDataProperty(
        ESString::create("HAVE_NOTHING"), false, true, false,
        ESValue(HTMLMediaElement::HAVE_NOTHING));
    HTMLMediaElementFunction->asESObject()->defineDataProperty(
        ESString::create("HAVE_METADATA"), false, true, false,
        ESValue(HTMLMediaElement::HAVE_METADATA));
    HTMLMediaElementFunction->asESObject()->defineDataProperty(
        ESString::create("HAVE_CURRENT_DATA"), false, true, false,
        ESValue(HTMLMediaElement::HAVE_CURRENT_DATA));
    HTMLMediaElementFunction->asESObject()->defineDataProperty(
        ESString::create("HAVE_FUTURE_DATA"), false, true, false,
        ESValue(HTMLMediaElement::HAVE_FUTURE_DATA));
    HTMLMediaElementFunction->asESObject()->defineDataProperty(
        ESString::create("HAVE_ENOUGH_DATA"), false, true, false,
        ESValue(HTMLMediaElement::HAVE_ENOUGH_DATA));

    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Media, src, setSrc, TYPE_STRING);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Media, currentSrc, TYPE_STRING);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Media, networkState, TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Media, preload, setPreload,
                                           TYPE_STRING);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Media, buffered, TYPE_SCRIPTVALUE);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Media, readyState, TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Media, seeking, TYPE_BOOLEAN);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Media, currentTime, setCurrentTime,
                                           TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Media, duration, TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Media, paused, TYPE_BOOLEAN);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Media, defaultPlaybackRate,
                                           setDefaultPlaybackRate, TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Media, playbackRate, setPlaybackRate,
                                           TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Media, played, TYPE_SCRIPTVALUE);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Media, seekable, TYPE_SCRIPTVALUE);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Media, ended, TYPE_BOOLEAN);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Media, autoplay, setAutoplay,
                                           TYPE_BOOLEAN);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Media, loop, setLoop, TYPE_BOOLEAN);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Media, controls, setControls,
                                           TYPE_BOOLEAN);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Media, volume, setVolume,
                                           TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Media, muted, setMuted,
                                           TYPE_BOOLEAN);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Media, textTracks, TYPE_SCRIPTVALUE);

    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, progress, Progress, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, suspend, Suspend, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, abort, Abort, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, emptied, Emptied, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, stalled, Stalled, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, loadedmetadata, Loadedmetadata,
                                      TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, loadeddata, Loadeddata,
                                      TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, loadstart, Loadstart, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, canplay, Canplay, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, canplaythrough, Canplaythrough,
                                      TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, playing, Playing, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, waiting, Waiting, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, seeking, Seeking, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, seeked, Seeked, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, ended, Ended, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, durationchange, Durationchange,
                                      TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, timeupdate, Timeupdate,
                                      TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, play, Play, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, pause, Pause, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, ratechange, Ratechange,
                                      TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, volumechange, Volumechange,
                                      TYPE_EVENT);

    HTMLMediaElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("addTextTrack"), true, true, true,
            ESFunctionObject::create(
                NULL,
                [](ESVMInstance* instance) -> ESValue {
                    GENERATE_THIS_AND_CHECK_TYPE(
                        ScriptWrappable::Type::NodeObject, Node);
                    Node* nd = originalObj;
                    if (!(nd->isElement() && nd->asElement()->isHTMLElement() &&
                          nd->asElement()
                              ->asHTMLElement()
                              ->isHTMLMediaElement())) {
                        THROW_ILLEGAL_INVOCATION();
                    }

                    ESValue arg1 =
                        instance->currentExecutionContext()->readArgument(0);
                    ESValue arg2 =
                        instance->currentExecutionContext()->readArgument(1);
                    ESValue arg3 =
                        instance->currentExecutionContext()->readArgument(2);
                    String* kind = String::emptyString;
                    String* label = String::emptyString;
                    String* language = String::emptyString;

                    // First Arg : kind
                    if (arg1.isUndefinedOrNull() || !arg1.isESString()) {
                        THROW_ILLEGAL_INVOCATION();
                    }
                    kind = toBrowserString(arg1.toString());
                    // Second Arg : label (can be omitted)
                    if (!arg2.isUndefinedOrNull() && !arg2.isESString()) {
                        THROW_ILLEGAL_INVOCATION();
                    }
                    if (!arg2.isUndefinedOrNull()) {
                        label = toBrowserString(arg2.toString());
                    }
                    // Third Arg : language (can be omitted)
                    if (!arg3.isUndefinedOrNull() && !arg3.isESString()) {
                        THROW_ILLEGAL_INVOCATION();
                    }
                    if (!arg3.isUndefinedOrNull()) {
                        language = toBrowserString(arg3.toString());
                    }

                    HTMLMediaElement* element = originalObj->asElement()
                                                    ->asHTMLElement()
                                                    ->asHTMLMediaElement();
                    TextTrack* track =
                        element->addTextTrack(kind, label, language);
                    if (!track) {
                        THROW_ILLEGAL_INVOCATION();
                    }
                    return track->scriptValue();
                },
                ESString::create("addTextTrack"), 3, false));

    HTMLMediaElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("load"), true, true, true,
            ESFunctionObject::create(
                NULL,
                [](ESVMInstance* instance) -> ESValue {
                    GENERATE_THIS_AND_CHECK_TYPE(
                        ScriptWrappable::Type::NodeObject, Node);
                    Node* nd = originalObj;
                    if (!(nd->isElement() && nd->asElement()->isHTMLElement() &&
                          nd->asElement()
                              ->asHTMLElement()
                              ->isHTMLMediaElement())) {
                        THROW_ILLEGAL_INVOCATION();
                    }
                    originalObj->asElement()
                        ->asHTMLElement()
                        ->asHTMLMediaElement()
                        ->load();
                    return ESValue(ESValue::ESUndefined);
                },
                ESString::create("load"), 0, false));

    HTMLMediaElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("canPlayType"), true, true, true,
            ESFunctionObject::create(
                NULL,
                [](ESVMInstance* instance) -> ESValue {
                    GENERATE_THIS_AND_CHECK_TYPE(
                        ScriptWrappable::Type::NodeObject, Node);
                    Node* nd = originalObj;
                    if (!(nd->isElement() && nd->asElement()->isHTMLElement() &&
                          nd->asElement()
                              ->asHTMLElement()
                              ->isHTMLMediaElement())) {
                        THROW_ILLEGAL_INVOCATION();
                    }
                    String* result =
                        originalObj->asElement()
                            ->asHTMLElement()
                            ->asHTMLMediaElement()
                            ->canPlayType(toBrowserString(v.toString()));
                    if (!result) {
                        THROW_ILLEGAL_INVOCATION();
                    }
                    return toJSString(result);
                },
                ESString::create("canPlayType"), 1, false));

    HTMLMediaElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("pause"), true, true, true,
            ESFunctionObject::create(
                NULL,
                [](ESVMInstance* instance) -> ESValue {
                    GENERATE_THIS_AND_CHECK_TYPE(
                        ScriptWrappable::Type::NodeObject, Node);
                    Node* nd = originalObj;
                    if (!(nd->isElement() && nd->asElement()->isHTMLElement() &&
                          nd->asElement()
                              ->asHTMLElement()
                              ->isHTMLMediaElement())) {
                        THROW_ILLEGAL_INVOCATION();
                    }
                    originalObj->asElement()
                        ->asHTMLElement()
                        ->asHTMLMediaElement()
                        ->pause();
                    return ESValue(ESValue::ESUndefined);
                },
                ESString::create("pause"), 0, false));

    HTMLMediaElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("play"), true, true, true,
            ESFunctionObject::create(
                NULL,
                [](ESVMInstance* instance) -> ESValue {
                    GENERATE_THIS_AND_CHECK_TYPE(
                        ScriptWrappable::Type::NodeObject, Node);
                    Node* nd = originalObj;
                    if (!(nd->isElement() && nd->asElement()->isHTMLElement() &&
                          nd->asElement()
                              ->asHTMLElement()
                              ->isHTMLMediaElement())) {
                        THROW_ILLEGAL_INVOCATION();
                    }
#ifdef USE_ES6_FEATURE
                    return originalObj->asElement()
                        ->asHTMLElement()
                        ->asHTMLMediaElement()
                        ->play()
                        ->scriptValue();
#else
                    originalObj->asElement()
                        ->asHTMLElement()
                        ->asHTMLMediaElement()
                        ->play();
                    return ESValue();
#endif
                },
                ESString::create("play"), 0, false));

    return HTMLMediaElementFunction;
}
}
#endif
