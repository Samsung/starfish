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

#ifndef __StarFishHTMLCollection__
#define __StarFishHTMLCollection__

#include "binding/ScriptWrappable.h"
#include "core/dom/NodeList.h"
#include "core/dom/NodeListImpl.h"

namespace StarFish {

class Node;
class Element;

class HTMLCollection : public ScriptWrappable {
public:
    HTMLCollection(Node* root, NodeListImpl::FilterFunctionType filterType,
                   void* data, bool canCache = false, bool includeRoot = false);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLCollection() const;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    size_t length() const;
    virtual Element* item(unsigned long index);
    virtual Element* namedItem(String* name);
    NodeListImpl& getNodeListImpl()
    {
        return m_nodeListImpl;
    }

protected:
    NodeListImpl m_nodeListImpl;
};
}

#endif
