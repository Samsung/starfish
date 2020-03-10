/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLLIElement.h"

namespace Starfish {

void HTMLLIElement::didAttributeChanged(QualifiedName name, String* old,
                                        String* value, bool attributeCreated,
                                        bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);

    if (name == starfish()->staticStrings()->m_value) {
        document()->notifyCountingOutdated();
    } else if (name == starfish()->staticStrings()->m_type) {
        document()->notifyCountingOutdated();
    }
}

void HTMLLIElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    HTMLElement::styleForPresentationAttribute(cssValues);

    String* typeString = type();
    if (typeString != String::emptyString) {
        Node* current = parentNode();
        Node* list = nullptr;
        while (current) {
            if (current->isHTMLListContainer()) {
                list = current;
                break;
            }
            current = current->parentNode();
        }

        if (list) {
            CSSStyleValuePair pair;

            if (list->isHTMLOListElement()) {
                if (typeString->equals("1")) {
                    AtomicString strValue =
                        AtomicString::createAtomicString(starfish(), "decimal");
                    pair.setKeyKind(CSSStyleValuePair::KeyKind::ListStyleType);
                    pair.setAtomicStringValue(strValue);
                } else if (typeString->equals("a")) {
                    AtomicString strValue = AtomicString::createAtomicString(
                        starfish(), "lower-alpha");
                    pair.setKeyKind(CSSStyleValuePair::KeyKind::ListStyleType);
                    pair.setAtomicStringValue(strValue);
                } else if (typeString->equals("A")) {
                    AtomicString strValue = AtomicString::createAtomicString(
                        starfish(), "upper-alpha");
                    pair.setKeyKind(CSSStyleValuePair::KeyKind::ListStyleType);
                    pair.setAtomicStringValue(strValue);
                } else if (typeString->equals("i")) {
                    AtomicString strValue = AtomicString::createAtomicString(
                        starfish(), "lower-roman");
                    pair.setKeyKind(CSSStyleValuePair::KeyKind::ListStyleType);
                    pair.setAtomicStringValue(strValue);
                } else if (typeString->equals("I")) {
                    AtomicString strValue = AtomicString::createAtomicString(
                        starfish(), "upper-roman");
                    pair.setKeyKind(CSSStyleValuePair::KeyKind::ListStyleType);
                    pair.setAtomicStringValue(strValue);
                }
            } else {
                STARFISH_ASSERT(list->isHTMLUListElement());
                if (typeString->equals("disc") ||
                    typeString->equals("square") ||
                    typeString->equals("circle")) {
                    AtomicString strValue = AtomicString::createAtomicString(
                        starfish(), typeString);
                    pair.setKeyKind(CSSStyleValuePair::KeyKind::ListStyleType);
                    pair.setAtomicStringValue(strValue);
                }
            }

            if (pair.keyKind() != CSSStyleValuePair::KeyKind::Unknown) {
                cssValues.push_back(pair);
            }
        }
    }
}

int32_t HTMLLIElement::value()
{
    String* valueStr =
        getAttributeOrEmpty(starfish()->staticStrings()->m_value);
    return String::parseInt(valueStr);
}

void HTMLLIElement::setValue(int32_t v)
{
    setAttribute(starfish()->staticStrings()->m_value, String::fromInt(v));
}

bool HTMLLIElement::hasValue()
{
    Nullable<String*> v = getAttribute(starfish()->staticStrings()->m_value);
    if (v.hasValue()) {
        return true;
    } else {
        return false;
    }
}

String* HTMLLIElement::type()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_type);
}

void HTMLLIElement::setType(String* type)
{
    setAttribute(starfish()->staticStrings()->m_type, type);
}
}
