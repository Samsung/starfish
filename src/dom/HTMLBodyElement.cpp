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
#include "Document.h"
#include "HTMLBodyElement.h"

namespace StarFish {

String* HTMLBodyElement::localName()
{
    return document()
        ->window()
        ->starFish()
        ->staticStrings()
        ->m_bodyTagName.localName();
}

QualifiedName HTMLBodyElement::name()
{
    return document()->window()->starFish()->staticStrings()->m_bodyTagName;
}

ScriptValue HTMLBodyElement::onloadEventListener()
{
    Window* window = document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_load;
    return window->attributeEventListener(attr);
}

void HTMLBodyElement::setOnloadEventListener(ScriptValue onload)
{
    Window* window = document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_load;

    if (onload.isObject()) {
        window->setAttributeEventListener(attr, onload);
    } else {
        window->clearAttributeEventListener(attr);
    }
}

ScriptValue HTMLBodyElement::onunloadEventListener()
{
    Window* window = document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_unload;
    return window->attributeEventListener(attr);
}

void HTMLBodyElement::setOnunloadEventListener(ScriptValue onunload)
{
    Window* window = document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_unload;

    if (onunload.isObject()) {
        window->setAttributeEventListener(attr, onunload);
    } else {
        window->clearAttributeEventListener(attr);
    }
}

void HTMLBodyElement::didComputedStyleChanged(ComputedStyle* oldStyle,
                                              ComputedStyle* newStyle)
{
    HTMLElement::didComputedStyleChanged(oldStyle, newStyle);
    if (!newStyle->backgroundColor().isTransparent() ||
        !newStyle->backgroundImage()->equals(String::emptyString)) {
        document()->window()->m_hasBodyElementBackground = true;
    } else {
        document()->window()->m_hasBodyElementBackground = false;
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
    StaticStrings* ss = document()->window()->starFish()->staticStrings();
    if (name == ss->m_onload) {
        document()->window()->setAttributeEventListener(ss->m_load, value,
                                                        this);
    } else if (name == ss->m_onunload) {
        document()->window()->setAttributeEventListener(ss->m_unload, value,
                                                        this);
    }
}
}
