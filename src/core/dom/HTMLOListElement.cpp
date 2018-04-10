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
#include "core/dom/Traverse.h"
#include "core/dom/HTMLOListElement.h"

namespace StarFish {
QualifiedName HTMLOListElement::name()
{
    return starFish()->staticStrings()->m_olTagName;
}

void HTMLOListElement::didAttributeChanged(QualifiedName name, String* old,
                                           String* value, bool attributeCreated,
                                           bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);

    if (name == starFish()->staticStrings()->m_reversed) {
        document()->notifyCountingOutdated();
    } else if (name == starFish()->staticStrings()->m_start) {
        document()->notifyCountingOutdated();
    } else if (name == starFish()->staticStrings()->m_type) {
        document()->notifyCountingOutdated();
    }
}

void HTMLOListElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    HTMLElement::styleForPresentationAttribute(cssValues);

    String* typeString = type();
    if (typeString != String::emptyString) {
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::KeyKind::ListStyleType);
        if (typeString->equals("a")) {
            AtomicString strValue =
                AtomicString::createAtomicString(starFish(), "lower-alpha");
            pair.setAtomicStringValue(strValue);
        } else if (typeString->equals("A")) {
            AtomicString strValue =
                AtomicString::createAtomicString(starFish(), "upper-alpha");
            pair.setAtomicStringValue(strValue);
        } else if (typeString->equals("i")) {
            AtomicString strValue =
                AtomicString::createAtomicString(starFish(), "lower-roman");
            pair.setAtomicStringValue(strValue);
        } else if (typeString->equals("I")) {
            AtomicString strValue =
                AtomicString::createAtomicString(starFish(), "upper-roman");
            pair.setAtomicStringValue(strValue);
        } else {
            AtomicString strValue =
                AtomicString::createAtomicString(starFish(), "decimal");
            pair.setAtomicStringValue(strValue);
        }
        cssValues.push_back(pair);
    }
}

int32_t HTMLOListElement::startNumber()
{
    Nullable<String*> v = getAttribute(starFish()->staticStrings()->m_start);
    if (v.hasValue()) {
        return String::parseInt(v.getValue());
    }
    if (reversed()) {
        return itemCount();
    }
    return 1;
}

int32_t HTMLOListElement::start()
{
    Nullable<String*> v = getAttribute(starFish()->staticStrings()->m_start);
    if (v.hasValue()) {
        return String::parseInt(v.getValue());
    }
    return 1;
}

void HTMLOListElement::setStart(int32_t v)
{
    setAttribute(starFish()->staticStrings()->m_start, String::fromInt(v));
}

bool HTMLOListElement::reversed()
{
    return hasAttribute(starFish()->staticStrings()->m_reversed) != SIZE_MAX;
}

void HTMLOListElement::setReversed(bool b)
{
    if (b) {
        setAttribute(starFish()->staticStrings()->m_reversed,
                     String::emptyString);
    } else {
        removeAttribute(starFish()->staticStrings()->m_reversed);
    }
}

String* HTMLOListElement::type()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_type);
}

void HTMLOListElement::setType(String* type)
{
    setAttribute(starFish()->staticStrings()->m_type, type);
}

unsigned HTMLOListElement::itemCount()
{
    unsigned itemCount = 0;
    Node* n = firstChild();
    while (n) {
        if (n->isHTMLListContainer()) {
            n = Traverse::nextSkippingChildren(n, this);
            continue;
        }
        if (n->isHTMLLIElement()) {
            itemCount++;
        }
        n = Traverse::next(n, this);
    }
    return itemCount;
}
}
