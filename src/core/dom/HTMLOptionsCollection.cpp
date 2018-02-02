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
