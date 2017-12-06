/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "core/dom/Node.h"
#include "core/dom/NodeList.h"
#include "core/dom/NodeListImpl.h"
#include "core/dom/Document.h"

namespace StarFish {

NodeList::NodeList(Node* root, NodeListImpl::FilterFunctionType filterType,
                   void* data, bool canCache)
    : ScriptWrappable(this)
    , m_nodeListImpl(root, filterType, data, canCache)
{
}

NodeList::NodeList(Node* root, bool canCache)
    : ScriptWrappable(this)
    , m_nodeListImpl(root, canCache)
{
}

NodeList::NodeList(Node* root, NodeListFilterFunction filter, void* data,
                   bool canCache)
    : ScriptWrappable(this)
    , m_nodeListImpl(root, filter, data, canCache)
{
}

ScriptBindingInstance* NodeList::scriptBindingInstance()
{
    return m_nodeListImpl.root()->document()->scriptBindingInstance();
}

uint32_t NodeList::length() const
{
    return m_nodeListImpl.length();
}

Node* NodeList::item(uint32_t index)
{
    return m_nodeListImpl.item(index);
}
}
