/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
