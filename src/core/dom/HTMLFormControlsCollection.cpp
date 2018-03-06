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
#include "core/dom/HTMLFormControlsCollection.h"
#include "core/dom/HTMLElement.h"

namespace StarFish {
HTMLFormControlsCollection::HTMLFormControlsCollection(
    Node* root, NodeListImpl::FilterFunctionType filterType)
    : HTMLCollection(root, filterType, nullptr, false)
{
    STARFISH_ASSERT(filterType == NodeListImpl::FormElementsFiliter);
}

Element* HTMLFormControlsCollection::namedItem(String* name)
{
    // https://html.spec.whatwg.org/multipage/common-dom-interfaces.html#dom-htmlformcontrolscollection-nameditem
    if (name->length() && !name->equals(String::emptyString)) {
        for (unsigned i = 0; i < m_nodeListImpl.length(); i++) {
            Element* elem = m_nodeListImpl.item(i)->asElement();
            if (elem->asHTMLElement()->hasId() &&
                elem->asHTMLElement()->id()->equals(name)) {
                return elem;
            }
            Nullable<String*> attrStr =
                elem->getAttribute(elem->starFish()->staticStrings()->m_name);
            if (attrStr.hasValue() && attrStr.getValue()->equals(name)) {
                return elem;
            }
        }
    }
    // TODO :
    // 4. Otherwise, create a new RadioNodeList object representing a live view
    //   of the HTMLFormControlsCollection object, further filtered so that the
    //   only nodes in the RadioNodeList object are those that have either an id
    //   attribute or a name attribute equal to name. The nodes in the
    //   RadioNodeList object must be sorted in tree order.
    // 5. Return that RadioNodeList object.
    return nullptr;
}
}
