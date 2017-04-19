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
#include "dom/HTMLTableCellElement.h"

namespace StarFish {

String* HTMLTableCellElement::colspan()
{
    return getAttribute(
        document()->window()->starFish()->staticStrings()->m_colspan);
}

void HTMLTableCellElement::setColspan(int colspan)
{
    setAttribute(document()->window()->starFish()->staticStrings()->m_colspan,
                 String::fromInt(colspan));
}

String* HTMLTableCellElement::rowspan()
{
    return getAttribute(
        document()->window()->starFish()->staticStrings()->m_rowspan);
}

void HTMLTableCellElement::setRowspan(int rowspan)
{
    setAttribute(document()->window()->starFish()->staticStrings()->m_colspan,
                 String::fromInt(rowspan));
}

String* HTMLTableCellElement::bgColor()
{
    return getAttribute(
        document()->window()->starFish()->staticStrings()->m_bgColor);
}

void HTMLTableCellElement::setBgColor(String* bgColor)
{
    setAttribute(document()->window()->starFish()->staticStrings()->m_bgColor,
                 bgColor);
}
}
