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
#include "StarFish.h"
#include "core/dom/HTMLTableColElement.h"

namespace StarFish {
String* HTMLTableColElement::localName()
{
    return starFish()->staticStrings()->m_colTagName.localName();
}

QualifiedName HTMLTableColElement::name()
{
    return starFish()->staticStrings()->m_colTagName;
}

void HTMLTableColElement::setSpan(uint32_t span)
{
    setAttribute(starFish()->staticStrings()->m_span, String::fromInt(span));
}

uint32_t HTMLTableColElement::span()
{
    String* spanAttr = getAttributeOrEmpty(starFish()->staticStrings()->m_span);
    return String::parseInt(spanAttr);
}
}
