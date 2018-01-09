/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#include "HTMLBaseElement.h"
#include "StarFish.h"
#include "core/dom/Document.h"

namespace StarFish {

QualifiedName HTMLBaseElement::name()
{
    return starFish()->staticStrings()->m_baseTagName;
}

String* HTMLBaseElement::href() const
{
    Nullable<String*> hrefAttr =
        getAttribute(starFish()->staticStrings()->m_href);

    if (!hrefAttr.hasValue()) {
        return document()->urlString();
    }

    return (new ResourceURL(hrefAttr.getValue()->trim(),
                            document()->fallbackBaseURL()->urlString()))
        ->urlString();
}

void HTMLBaseElement::setHref(String* href)
{
    setAttribute(starFish()->staticStrings()->m_href, href);
}

String* HTMLBaseElement::target() const
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_target);
}

void HTMLBaseElement::setTarget(String* target)
{
    setAttribute(starFish()->staticStrings()->m_target, target);
}

void HTMLBaseElement::didAttributeChanged(QualifiedName name, String* old,
                                          String* value, bool attributeCreated,
                                          bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);

    if (name == starFish()->staticStrings()->m_href ||
        name == starFish()->staticStrings()->m_target) {
        document()->processBaseElement();
    }
}
}
