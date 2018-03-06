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

#ifndef __StarFishNodeList__
#define __StarFishNodeList__

#include "binding/ScriptWrappable.h"
#include "core/dom/NodeListImpl.h"

namespace StarFish {

class Node;

class NodeList : public ScriptWrappable {
public:
    NodeList(Node* root, NodeListImpl::FilterFunctionType filterType,
             void* data, bool canCache = false);

    NodeList(Node* root, bool canCache = true);

    NodeList(Node* root, NodeListFilterFunction filter, void* data,
             bool canCache);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isNodeList() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    uint32_t length() const;
    Node* item(uint32_t index);
    NodeListImpl& getNodeListImpl()
    {
        return m_nodeListImpl;
    }

private:
    NodeListImpl m_nodeListImpl;
};
}

#endif
