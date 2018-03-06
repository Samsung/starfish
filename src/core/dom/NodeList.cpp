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
