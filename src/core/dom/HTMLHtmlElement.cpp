/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/style/ComputedStyle.h"
#include "core/modules/window/Window.h"

namespace StarFish {
String* HTMLHtmlElement::localName()
{
    return starFish()->staticStrings()->m_htmlTagName.localName();
}

QualifiedName HTMLHtmlElement::name()
{
    return starFish()->staticStrings()->m_htmlTagName;
}

void HTMLHtmlElement::didComputedStyleChanged(ComputedStyle* oldStyle,
                                              ComputedStyle* newStyle)
{
    HTMLElement::didComputedStyleChanged(oldStyle, newStyle);
    if (!newStyle->backgroundColor().isTransparent() ||
        !newStyle->backgroundImage()->equals(String::emptyString)) {
        window()->m_hasRootElementBackground = true;
    } else {
        window()->m_hasRootElementBackground = false;
    }

    if (oldStyle && oldStyle->overflow() != newStyle->overflow()) {
        document()->setNeedsFrameTreeBuild();
    }
}
}
