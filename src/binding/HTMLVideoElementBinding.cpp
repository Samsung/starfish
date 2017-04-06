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
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

namespace StarFish {

using namespace escargot;

ESFunctionObject* bindingHTMLVideoElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        HTMLVideoElement, fetchData(scriptBindingInstance)->htmlMediaElement());

    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Video, width, setWidth, TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Video, height, setHeight,
                                           TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Video, videoWidth, TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Video, videoHeight, TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Video, poster, setPoster,
                                           TYPE_STRING);

    return HTMLVideoElementFunction;
}
}
#endif
