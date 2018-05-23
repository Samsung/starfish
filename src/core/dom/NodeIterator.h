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

#ifndef __StarFishNodeIterator__
#define __StarFishNodeIterator__

#include "core/dom/Document.h"
#include "binding/ScriptWrappable.h"

namespace StarFish {

class NodeIterator final : public ScriptWrappable {
private:
    class NodePointer {
    public:
        virtual ~NodePointer()
        {
        }

        NodePointer()
            : m_node(nullptr)
            , m_isPointerBeforeNode(false)
        {
        }

        NodePointer(Node*, bool);

        void clear();
        bool moveToNext(Node* root);
        bool moveToPrevious(Node* root);

        Node* m_node;
        bool m_isPointerBeforeNode;
    };

public:
    virtual ~NodeIterator()
    {
    }

    NodeIterator(Document* document, Node* root, unsigned whatToShow,
                 ScriptValue filter);

    Node* root()
    {
        return m_root;
    }

    Node* referenceNode() const
    {
        return m_referenceNode.m_node;
    }

    virtual ScriptBindingInstance* scriptBindingInstance()
    {
        return m_scriptBindingInstance;
    }

    bool pointerBeforeReferenceNode() const
    {
        return m_referenceNode.m_isPointerBeforeNode;
    }

    unsigned whatToShow()
    {
        return m_whatToShow;
    }

    void willNodeBeRemoved(Node* removedNode);
    void updateForNodeRemoval(Node*, NodePointer&);

    Node* nextNode();
    Node* previousNode();

    void detach();

    unsigned acceptNode(Node*, bool&);

    ScriptValue filter();

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;

    virtual bool isNodeIterator() const override;

private:
    ScriptBindingInstance* m_scriptBindingInstance;
    Node* m_root;
    NodePointer m_referenceNode;
    NodePointer m_candidateNode;
    unsigned m_whatToShow;
    ScriptValue m_filter;
};
}

#endif
