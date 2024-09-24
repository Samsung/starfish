/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#ifndef __StarfishRange__
#define __StarfishRange__

#include "binding/ScriptWrappable.h"
#include "core/dom/Document.h"

namespace Starfish {

class Node;

class BoundaryPoint : public gc {
public:
    BoundaryPoint(Node* node)
        : m_node(node)
        , m_offset(0)
    {
    }

    BoundaryPoint(BoundaryPoint& other)
        : m_node(other.m_node)
        , m_offset(other.m_offset)
    {
    }

    BoundaryPoint(Node* node, unsigned offset)
        : m_node(node)
        , m_offset(offset)
    {
    }

    bool operator==(BoundaryPoint& other)
    {
        return (this->m_node == other.m_node &&
                this->m_offset == other.m_offset);
    }

    Node* m_node;
    unsigned m_offset;
};

class Range : public ScriptWrappable {
public:
    enum How {
        START_TO_START,
        START_TO_END,
        END_TO_END,
        END_TO_START,
    };

    Range(Document* document);
    Range(Document* document, Node* startNode, unsigned startOffset,
          Node* endNode, unsigned endOffset);

    static Range* create(Document* document);

    static Range* create(Document* document, Node* startNode,
                         unsigned startOffset, Node* endNode,
                         unsigned endOffset);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isRange() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    Node* startContainer();
    unsigned startOffset();
    Node* endContainer();
    unsigned endOffset();
    bool collapsed();
    Node* commonAncestorContainer();

    void setStart(Node* node, unsigned offset);
    void setEnd(Node* node, unsigned offset);
    void setStartBefore(Node* node);
    void setStartAfter(Node* node);
    void setEndBefore(Node* node);
    void setEndAfter(Node* node);
    void collapse(bool toStart = false);
    void selectNode(Node* node);
    void selectNodeContents(Node* node);

    short compareBoundaryPoints(unsigned how, Range* sourceRange);

    void insertNode(Node* node);

    DOMRectList* getClientRects();
    DOMRect* getBoundingClientRect(bool layoutIfNeeds = true);

    void deleteContents();
    DocumentFragment* extractContents();
    DocumentFragment* cloneContents();

    void surroundContents(Node* newParent);

    // DocumentFragment cloneContents();

    Range* cloneRange();
    void detach();

    bool isPointInRange(Node* node, unsigned offset);
    short comparePoint(Node* node, unsigned offset);

    bool intersectsNode(Node* node);

    String* toString();

    Node* firstNode();
    Node* pastLastNode();

    enum ProcessingType { Extract, Clone, Delete };
    enum ContentsProcessDirection { ProcessForward, ProcessBackward };

private:
    bool isValidOffset(Node* node, unsigned offset);
    bool compareRoots(const BoundaryPoint& bpA, const BoundaryPoint& bpB);
    short compareBoundaryPoints(const BoundaryPoint& bpA,
                                const BoundaryPoint& bpB);
    Node* root();
    void borderAndTextQuads(GCVector<DOMQuad*>& quads,
                            bool layoutIfNeeds = true);

    DocumentFragment* processContents(ProcessingType type);

    Document* m_document;
    BoundaryPoint m_start;
    BoundaryPoint m_end;
};
} // namespace Starfish

#endif
