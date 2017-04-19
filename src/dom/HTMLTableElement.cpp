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

#include "dom/Document.h"
#include "dom/HTMLTableElement.h"

namespace StarFish {

virtual String* HTMLTableElement::localName()
{
    return document()
        ->window()
        ->starFish()
        ->staticStrings()
        ->m_tableTagName.localName();
}

virtual QualifiedName HTMLTableElement::name()
{
    return document()->window()->starFish()->staticStrings()->m_tableTagName;
}

String* HTMLTableElement::width()
{
    return getAttribute(
        document()->window()->starFish()->staticStrings()->m_width);
}

void HTMLTableElement::setWidth(int width)
{
    setAttribute(document()->window()->starFish()->staticStrings()->m_width,
                 String::fromInt(width));
}

String* HTMLTableElement::bgColor()
{
    return getAttribute(
        document()->window()->starFish()->staticStrings()->m_bgColor);
}

void HTMLTableElement::setBgColor(String* bgColor)
{
    setAttribute(document()->window()->starFish()->staticStrings()->m_bgColor,
                 bgColor);
}
}
