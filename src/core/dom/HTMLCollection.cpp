/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLElement.h"
#include "core/dom/Document.h"

namespace StarFish {

HTMLCollection::HTMLCollection(Node* root,
                               NodeListImpl::FilterFunctionType filterType,
                               void* data, bool canCache, bool includeRoot)
    : ScriptWrappable(this)
    , m_nodeListImpl(root, filterType, data, canCache, includeRoot)
{
}

ScriptBindingInstance* HTMLCollection::scriptBindingInstance()
{
    return m_nodeListImpl.root()->document()->scriptBindingInstance();
}

size_t HTMLCollection::length() const
{
    return m_nodeListImpl.length();
}

Element* HTMLCollection::item(unsigned long index)
{
    if (index >= length()) {
        return nullptr;
    }
    return m_nodeListImpl.item(index)->asElement();
}

Element* HTMLCollection::namedItem(String* key)
{
    if (key->length()) {
        for (unsigned i = 0; i < m_nodeListImpl.length(); i++) {
            Element* elem = m_nodeListImpl.item(i)->asElement();
            if (elem->hasId() && elem->id()->equals(key)) {
                return elem;
            }
            Nullable<String*> attrStr =
                elem->getAttribute(elem->starFish()->staticStrings()->m_name);
            if (attrStr.hasValue() && attrStr.getValue()->equals(key)) {
                return elem;
            }
        }
    }
    return nullptr;
}
}
