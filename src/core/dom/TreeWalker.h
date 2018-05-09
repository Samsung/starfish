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

#ifndef __StarFishTreeWalker__
#define __StarFishTreeWalker__

#include "binding/DocumentHoldable.h"
#include "core/dom/Document.h"
#include "binding/ScriptWrappable.h"

namespace StarFish {

class TreeWalker final : public ScriptWrappable {
public:
    TreeWalker(Document* document, Node* root, unsigned whatToShow,
               ScriptValue filter)
        : ScriptWrappable(this)
        , m_scriptBindingInstance(document->scriptBindingInstance())
        , m_root(root)
        , m_current(root)
        , m_whatToShow(whatToShow)
        , m_filter(filter)
    {
    }

    Node* parentNode();

    Node* root()
    {
        return m_root;
    }

    Node* firstChild();

    Node* lastChild();

    Node* previousNode();

    Node* nextNode();

    template <typename Strategy>
    Node* TraverseSiblings();

    Node* previousSibling();

    Node* nextSibling();

    void setCurrentNode(Node* node)
    {
        m_current = node;
    }

    Node* currentNode()
    {
        return m_current;
    }

    unsigned whatToShow()
    {
        return m_whatToShow;
    }

    unsigned acceptNode(Node*, bool&);

    ScriptValue filter();

    virtual ScriptBindingInstance* scriptBindingInstance()
    {
        return m_scriptBindingInstance;
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isTreeWalker() const override;

private:
    ScriptBindingInstance* m_scriptBindingInstance;
    Node* m_root;
    Node* m_current;
    unsigned m_whatToShow;
    ScriptValue m_filter;
};
}

#endif
