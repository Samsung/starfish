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
#include "dom/TextTrack.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

namespace StarFish {

using namespace escargot;

ESFunctionObject* bindingHTMLTrackElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        HTMLTrackElement, fetchData(scriptBindingInstance)->htmlElement());

    HTMLTrackElementFunction->asESObject()->defineDataProperty(
        ESString::create("NONE"), false, true, false,
        ESValue(HTMLTrackElement::NONE));
    HTMLTrackElementFunction->asESObject()->defineDataProperty(
        ESString::create("LOADING"), false, true, false,
        ESValue(HTMLTrackElement::LOADING));
    HTMLTrackElementFunction->asESObject()->defineDataProperty(
        ESString::create("LOADED"), false, true, false,
        ESValue(HTMLTrackElement::LOADED));
    HTMLTrackElementFunction->asESObject()->defineDataProperty(
        ESString::create("ERROR"), false, true, false,
        ESValue(HTMLTrackElement::ERROR));

    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Track, kind, setKind, TYPE_STRING);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Track, src, setSrc, TYPE_STRING);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Track, srclang, setSrclang,
                                           TYPE_STRING);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Track, label, setLabel, TYPE_STRING);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Track, defaultAttr, setDefaultAttr,
                                           TYPE_BOOLEAN);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Track, readyState, TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Track, track, TYPE_SCRIPTVALUE);

    return HTMLTrackElementFunction;
}
}
#endif
