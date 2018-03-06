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
#include "StarFish.h"

#include "core/dom/HTMLOptionsCollection.h"

#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLElement.h"
#include "core/dom/HTMLOptionElement.h"
#include "core/dom/Document.h"

namespace StarFish {

HTMLOptionsCollection::HTMLOptionsCollection(
    Node* root, NodeListImpl::FilterFunctionType filterType, void* data,
    bool canCache)
    : HTMLCollection(root, filterType, data, canCache)
{
}

ScriptBindingInstance* HTMLOptionsCollection::scriptBindingInstance()
{
    return m_nodeListImpl.root()->document()->scriptBindingInstance();
}

size_t HTMLOptionsCollection::length() const
{
    return m_nodeListImpl.length();
}

void HTMLOptionsCollection::setLength(size_t value)
{
    // TODO
    // https://html.spec.whatwg.org/multipage/common-dom-interfaces.html#dom-htmloptionscollection-length
}

int HTMLOptionsCollection::selectedIndex()
{
    for (size_t i = 0; i < m_nodeListImpl.length(); i++) {
        Element* elem = m_nodeListImpl.item(i)->asElement();
        if (elem->asHTMLOptionElement()->selectedness()) {
            return i;
        }
    }

    return -1;
}

void HTMLOptionsCollection::setSelectedIndex(int index)
{
    if (index < 0) {
        return;
    }

    for (size_t i = 0; i < m_nodeListImpl.length(); i++) {
        if (i == (size_t)index) {
            HTMLOptionElement* opt =
                m_nodeListImpl.item(i)->asHTMLOptionElement();
            opt->setSelectedness(true);
            break;
        }
    }
}
}
