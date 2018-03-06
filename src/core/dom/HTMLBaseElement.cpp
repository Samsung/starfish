/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
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
