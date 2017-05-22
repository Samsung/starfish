/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/modules/window/Window.h"
#include "core/style/ComputedStyle.h"

namespace StarFish {

String* HTMLBodyElement::localName()
{
    return starFish()->staticStrings()->m_bodyTagName.localName();
}

QualifiedName HTMLBodyElement::name()
{
    return starFish()->staticStrings()->m_bodyTagName;
}

DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, load);
DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, unload);

void HTMLBodyElement::didComputedStyleChanged(ComputedStyle* oldStyle,
                                              ComputedStyle* newStyle)
{
    HTMLElement::didComputedStyleChanged(oldStyle, newStyle);
    if (!newStyle->backgroundColor().isTransparent() ||
        !newStyle->backgroundImage()->equals(String::emptyString)) {
        window()->m_hasBodyElementBackground = true;
    } else {
        window()->m_hasBodyElementBackground = false;
    }

    if (oldStyle && oldStyle->overflow() != newStyle->overflow()) {
        document()->setNeedsFrameTreeBuild();
    }
}

void HTMLBodyElement::didAttributeChanged(QualifiedName name, String* old,
                                          String* value, bool attributeCreated,
                                          bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    StaticStrings* ss = starFish()->staticStrings();
    if (name == ss->m_onload) {
        window()->setAttributeEventListener(ss->m_load, value, this);
    } else if (name == ss->m_onunload) {
        window()->setAttributeEventListener(ss->m_unload, value, this);
    }
}
}
