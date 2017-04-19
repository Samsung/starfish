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

#include "StarFishConfig.h"
#include "dom/Document.h"
#include "dom/HTMLColGroupElement.h"

namespace StarFish {
String* HTMLColGroupElement::localName()
{
    return document()
        ->window()
        ->starFish()
        ->staticStrings()
        ->m_colgroupTagName.localName();
}

QualifiedName HTMLColGroupElement::name()
{
    return document()->window()->starFish()->staticStrings()->m_colgroupTagName;
}

void HTMLColGroupElement::setSpan(int span)
{
    setAttribute(document()->window()->starFish()->staticStrings()->m_span,
                 String::fromInt(span));
}

String* HTMLColGroupElement::span()
{
    return getAttribute(
        document()->window()->starFish()->staticStrings()->m_span);
}
}
