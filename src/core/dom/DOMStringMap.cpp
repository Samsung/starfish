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
#include "core/dom/DOMException.h"
#include "core/dom/DOMStringMap.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"

namespace StarFish {

static bool isCustomDataAtributeName(String* attributeName)
{
    return attributeName->startsWith("data-");
}

static bool isCustomDataPropertyName(String* propertyName)
{
    size_t size = propertyName->length();
    for (size_t i = 0; i < size - 1; ++i) {
        if (propertyName->charAt(i) == '-' &&
            isASCIILower(propertyName->charAt(i + 1))) {
            return false;
        }
    }
    return true;
}

static String* generateAttributeName(String* propertyName)
{
    // Convert `camelCase` to `dash-style`
    STARFISH_ASSERT(isCustomDataPropertyName(propertyName));
    StringBuilder builder;
    builder.appendString("data-");
    size_t size = propertyName->length();
    for (size_t i = 0; i < size; i++) {
        if (isASCIIUpper(propertyName->charAt(i))) {
            builder.appendChar('-');
            builder.appendChar(toASCIILower(propertyName->charAt(i)));
        } else {
            builder.appendChar(propertyName->charAt(i));
        }
    }
    return builder.finalize();
}

static String* generatePropertyName(String* attributeName)
{
    // Convert `dash-style` to `camelCase`
    STARFISH_ASSERT(isCustomDataAtributeName(attributeName));
    StringBuilder builder;
    size_t size = attributeName->length();
    size_t lastPos = 5;
    size_t currentPos = 5;
    while (currentPos < size) {
        currentPos = attributeName->find('-', currentPos);
        if (currentPos == SIZE_MAX) {
            break;
        }
        if (currentPos + 1 < size &&
            isASCIILower(attributeName->charAt(currentPos + 1))) {
            if (lastPos != currentPos) {
                builder.appendSubString(attributeName, lastPos, currentPos);
            }
            builder.appendChar(
                toASCIIUpper(attributeName->charAt(currentPos + 1)));
            currentPos += 2;
            lastPos = currentPos;
        } else {
            currentPos++;
        }
    }
    if (lastPos < size) {
        builder.appendSubString(attributeName, lastPos, size);
    }
    return builder.finalize();
}

static bool isSameCustomDataName(String* attributeName, String* propertyName)
{
    if (!isCustomDataAtributeName(attributeName) ||
        !isCustomDataPropertyName(propertyName)) {
        return false;
    }
    return generateAttributeName(propertyName)->equals(attributeName);
}

ScriptBindingInstance* DOMStringMap::scriptBindingInstance()
{
    return m_element->document()->scriptBindingInstance();
}

Nullable<String*> DOMStringMap::defaultNamedGetter(String* key)
{
    STARFISH_ASSERT(m_element);
    size_t size = m_element->attributeCount();
    for (size_t i = 0; i < size; i++) {
        String* name = m_element->getAssuredAttributeName(i).localName();
        if (isSameCustomDataName(name, key)) {
            return Nullable<String*>(m_element->getAssuredAttribute(i));
        }
    }
    return Nullable<String*>();
}

bool DOMStringMap::defaultNamedSetter(String* key, String* value)
{
    STARFISH_ASSERT(m_element);
    if (!isCustomDataPropertyName(key)) {
        throw new DOMException(m_element->document(),
                               DOMException::Code::SYNTAX_ERR);
    }
    m_element->setAttribute(generateAttributeName(key), value);
    return true;
}

bool DOMStringMap::defaultNamedDeleter(String* key)
{
    size_t size = m_element->attributeCount();
    for (size_t i = 0; i < size; i++) {
        String* name = m_element->getAssuredAttributeName(i).localName();
        if (isSameCustomDataName(name, key)) {
            m_element->removeAttribute(i);
            return true;
        }
    }
    return false;
}

void DOMStringMap::defaultNamedEnumerator(GCVector<String*>& enums)
{
    STARFISH_ASSERT(m_element);
    size_t size = m_element->attributeCount();
    for (size_t i = 0; i < size; i++) {
        String* name = m_element->getAssuredAttributeName(i).localName();
        if (isCustomDataAtributeName(name)) {
            enums.push_back(generatePropertyName(name));
        }
    }
}
}
