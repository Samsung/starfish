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
#include "core/dom/HTMLTableElement.h"

namespace StarFish {

void HTMLTableElement::didAttributeChanged(QualifiedName name, String* old,
                                           String* value, bool attributeCreated,
                                           bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    if (name == starFish()->staticStrings()->m_cellpadding) {
        if (attributeCreated) {
            m_hasCellPaddingAttribute = true;
        }
        if (attributeRemoved) {
            m_hasCellPaddingAttribute = false;
        }
        if (!old->equals(value)) {
            setAttribute(starFish()->staticStrings()->m_cellpadding, value);
            setNeedsStyleRecalc();
        }
    }
}

QualifiedName HTMLTableElement::name()
{
    return starFish()->staticStrings()->m_tableTagName;
}

String* HTMLTableElement::width()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_width);
}

void HTMLTableElement::setWidth(String* width)
{
    setAttribute(starFish()->staticStrings()->m_width, width);
}

String* HTMLTableElement::bgColor()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_bgColor);
}

void HTMLTableElement::setBgColor(String* bgColor)
{
    setAttribute(starFish()->staticStrings()->m_bgColor, bgColor);
}

String* HTMLTableElement::cellspacing()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_cellspacing);
}

void HTMLTableElement::setCellspacing(String* cellspacing)
{
    setAttribute(starFish()->staticStrings()->m_cellspacing, cellspacing);
}

String* HTMLTableElement::cellpadding()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_cellpadding);
}

void HTMLTableElement::setCellpadding(String* cellpadding)
{
    setAttribute(starFish()->staticStrings()->m_cellpadding, cellpadding);
}
}
