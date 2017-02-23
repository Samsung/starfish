/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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
#include "NodeListImpl.h"
#include "dom/Element.h"
#include "dom/Document.h"

namespace StarFish {

template <typename Func>
static void getherDescendant(GCVector<Node*>* collection, Node* root, Func filter)
{
    Node* child = root->firstChild();
    while (child) {
        if (filter(child))
            collection->push_back(child);

        getherDescendant(collection, child, filter);
        child = child->nextSibling();
    }
}

unsigned long NodeListImpl::length() const
{
    if (m_canCache) {
        fillCacheIfNeed();
        return m_cachedNodeList.size();
    }
    GCVector<Node*> collection;
    getherDescendant(&collection, m_root, [this](Node* child) -> bool {
        return m_filter(child, this->m_data);
    });
    return collection.size();
}

Node* NodeListImpl::item(unsigned long index)
{
    if (m_canCache) {
        fillCacheIfNeed();
        return m_cachedNodeList[index];
    }
    GCVector<Node*> collection;
    getherDescendant(&collection, m_root, [this](Node* child) -> bool {
        return m_filter(child, this->m_data);
    });
    if (index < collection.size()) {
        return collection[index];
    }
    return nullptr;
}

void NodeListImpl::fillCacheIfNeed() const
{
    STARFISH_ASSERT(m_canCache);
    if (!m_isCacheValid) {
        getherDescendant(&m_cachedNodeList, m_root, [this](Node* child) -> bool {
            return m_filter(child, this->m_data);
        });
        m_isCacheValid = true;
    }

}

void NodeListImpl::setItems(GCVector<Element*>& elements)
{
    for (auto element : elements) {
        m_cachedNodeList.push_back(element);
    }
}

}
