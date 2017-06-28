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

#include "core/dom/HTMLButtonElement.h"

#include "core/dom/Document.h"
#include "core/page/BrowsingContext.h"

namespace StarFish {

HTMLButtonElement::HTMLButtonElement(Document* document)
    : HTMLElement(document)
{
    setAttribute(starFish()->staticStrings()->m_name, String::emptyString);
}

String* HTMLButtonElement::localName()
{
    return starFish()->staticStrings()->m_formTagName.localName();
}

QualifiedName HTMLButtonElement::name()
{
    return starFish()->staticStrings()->m_formTagName;
}

String* HTMLButtonElement::domName()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_name);
}

void HTMLButtonElement::setDomName(String* name)
{
    setAttribute(starFish()->staticStrings()->m_name, name);
}

String* HTMLButtonElement::type()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_type);
}

void HTMLButtonElement::setType(String* type)
{
    setAttribute(starFish()->staticStrings()->m_type, type);
}

String* HTMLButtonElement::value()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_value);
}

void HTMLButtonElement::setValue(String* value)
{
    setAttribute(starFish()->staticStrings()->m_value, value);
}
}
