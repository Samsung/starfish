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

#ifdef STARFISH_ENABLE_MULTIMEDIA

#include "StarFish.h"
#include "core/dom/HTMLSourceElement.h"

namespace StarFish {

String* HTMLSourceElement::localName()
{
    return starFish()->staticStrings()->m_sourceTagName.localName();
}

QualifiedName HTMLSourceElement::name()
{
    return starFish()->staticStrings()->m_sourceTagName;
}

String* HTMLSourceElement::src()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_src);
}

String* HTMLSourceElement::type()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_type);
}

void HTMLSourceElement::setSrc(String* src)
{
    setAttribute(starFish()->staticStrings()->m_src, src);
}

void HTMLSourceElement::setType(String* type)
{
    setAttribute(starFish()->staticStrings()->m_type, type);
}
}

#endif
