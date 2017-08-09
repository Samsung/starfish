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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/style/ComputedStyle.h"

namespace StarFish {

QualifiedName HTMLBodyElement::name()
{
    return starFish()->staticStrings()->m_bodyTagName;
}

DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, load);

DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, message);
DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, unload);

void HTMLBodyElement::didComputedStyleChanged(ComputedStyle* oldStyle,
                                              ComputedStyle* newStyle)
{
    HTMLElement::didComputedStyleChanged(oldStyle, newStyle);
    if (!newStyle->backgroundColor().isTransparent() ||
        !newStyle->backgroundImage()->equals(String::emptyString)) {
        document()->browsingContext()->m_hasBodyElementBackground = true;
    } else {
        document()->browsingContext()->m_hasBodyElementBackground = false;
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
