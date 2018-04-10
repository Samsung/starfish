/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLUListElement.h"

namespace StarFish {
QualifiedName HTMLUListElement::name()
{
    return starFish()->staticStrings()->m_ulTagName;
}

void HTMLUListElement::didAttributeChanged(QualifiedName name, String* old,
                                           String* value, bool attributeCreated,
                                           bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);

    if (name == starFish()->staticStrings()->m_type) {
        document()->notifyCountingOutdated();
    }
}

void HTMLUListElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    HTMLElement::styleForPresentationAttribute(cssValues);

    String* typeString = type();
    if (typeString != String::emptyString) {
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::KeyKind::ListStyleType);
        if (typeString->equals("square") || typeString->equals("circle")) {
            AtomicString strValue =
                AtomicString::createAtomicString(starFish(), typeString);
            pair.setAtomicStringValue(strValue);
        } else {
            AtomicString strValue =
                AtomicString::createAtomicString(starFish(), "disc");
            pair.setAtomicStringValue(strValue);
        }
        cssValues.push_back(pair);
    }
}

String* HTMLUListElement::type()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_type);
}

void HTMLUListElement::setType(String* type)
{
    setAttribute(starFish()->staticStrings()->m_type, type);
}
}
