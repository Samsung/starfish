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
#include "StarFish.h"
#include "core/dom/Range.h"
#include "core/dom/Traverse.h"
#include "core/dom/DOMException.h"
#include "core/dom/CharacterData.h"

namespace StarFish {

Range::Range(Document* document, Node* startContainer, unsigned startOffset,
             Node* endContainer, unsigned endOffset)
    : ScriptWrappable(this)
    , m_document(document)
    , m_start(document)
    , m_end(document)
{
    m_document->appendRange(this);
    setStart(startContainer, startOffset);
    setEnd(endContainer, endOffset);
}

Range* Range::create(Document* document)
{
    return new Range(document);
}

Range* Range::create(Document* document, Node* startContainer,
                     unsigned startOffset, Node* endContainer,
                     unsigned endOffset)
{
    return new Range(document, startContainer, startOffset, endContainer,
                     endOffset);
}

ScriptBindingInstance* Range::scriptBindingInstance()
{
    return m_document->scriptBindingInstance();
}

Node* Range::startContainer()
{
    return m_start.m_node;
}

unsigned Range::startOffset()
{
    return m_start.m_offset;
}

Node* Range::endContainer()
{
    return m_end.m_node;
}

unsigned Range::endOffset()
{
    return m_end.m_offset;
}

bool Range::collapsed()
{
    return m_start == m_end;
}

Node* Range::commonAncestorContainer()
{
    return Traverse::commonAncestor(startContainer(), endContainer());
}

void Range::setStart(Node* node, unsigned offset)
{
    if (!isValidOffset(node, offset)) {
        return;
    }

    BoundaryPoint newStart(node, offset);

    if (!compareRoots(m_start, newStart)) {
        m_document->removeRange(this);
        m_document = node->document();
        m_document->appendRange(this);
        m_end = newStart;
    } else if (compareBoundaryPoints(newStart, m_end) > 0) {
        m_end = newStart;
    }

    m_start = newStart;
}

void Range::setEnd(Node* node, unsigned offset)
{
    if (!isValidOffset(node, offset)) {
        return;
    }

    BoundaryPoint newEnd(node, offset);

    if (!compareRoots(m_start, newEnd)) {
        m_document->removeRange(this);
        m_document = node->document();
        m_document->appendRange(this);
        m_start = newEnd;
    } else if (compareBoundaryPoints(m_start, newEnd) > 0) {
        m_start = newEnd;
    }

    m_end = newEnd;
}

void Range::setStartBefore(Node* node)
{
    Node* parent = node->parentNode();
    if (!parent) {
        throw new DOMException(m_document,
                               DOMException::Code::INVALID_NODE_TYPE_ERR);
        return;
    }

    setStart(parent, node->index());
}

void Range::setStartAfter(Node* node)
{
    Node* parent = node->parentNode();
    if (!parent) {
        throw new DOMException(m_document,
                               DOMException::Code::INVALID_NODE_TYPE_ERR);
        return;
    }

    setStart(parent, node->index() + 1);
}

void Range::setEndBefore(Node* node)
{
    Node* parent = node->parentNode();
    if (!parent) {
        throw new DOMException(m_document,
                               DOMException::Code::INVALID_NODE_TYPE_ERR);
        return;
    }

    setEnd(parent, node->index());
}

void Range::setEndAfter(Node* node)
{
    Node* parent = node->parentNode();
    if (!parent) {
        throw new DOMException(m_document,
                               DOMException::Code::INVALID_NODE_TYPE_ERR);
        return;
    }

    setEnd(parent, node->index() + 1);
}

void Range::collapse(bool toStart)
{
    if (toStart) {
        m_end = m_start;
    } else {
        m_start = m_end;
    }
}

void Range::selectNode(Node* node)
{
    Node* parent = node->parentNode();
    if (!parent) {
        throw new DOMException(m_document,
                               DOMException::Code::INVALID_NODE_TYPE_ERR);
        return;
    }

    unsigned index = node->index();
    setStart(parent, index);
    setEnd(parent, index + 1);
}

void Range::selectNodeContents(Node* node)
{
    if (node->nodeType() == Node::DOCUMENT_TYPE_NODE) {
        throw new DOMException(m_document,
                               DOMException::Code::INVALID_NODE_TYPE_ERR);
        return;
    }

    unsigned length = 0;
    if (node->isCharacterData()) {
        length = node->asCharacterData()->length();
    } else {
        Node* child = node->firstChild();
        while (child) {
            child = child->nextSibling();
            length++;
        }
    }

    setStart(node, 0);
    setEnd(node, length);
}

short Range::compareBoundaryPoints(unsigned how, Range* sourceRange)
{
    if (how != START_TO_START && how != START_TO_END && how != END_TO_END &&
        how != END_TO_START) {
        throw new DOMException(m_document,
                               DOMException::Code::NOT_SUPPORTED_ERR);
        return 0;
    }

    if (root() != sourceRange->root()) {
        throw new DOMException(m_document,
                               DOMException::Code::WRONG_DOCUMENT_ERR);
        return 0;
    }

    switch (how) {
    case START_TO_START:
        return compareBoundaryPoints(m_start, sourceRange->m_start);
    case START_TO_END:
        return compareBoundaryPoints(m_end, sourceRange->m_start);
    case END_TO_END:
        return compareBoundaryPoints(m_end, sourceRange->m_end);
    case END_TO_START:
        return compareBoundaryPoints(m_start, sourceRange->m_end);
    }

    STARFISH_ASSERT_NOT_REACHED();
    return 0;
}

Range* Range::cloneRange()
{
    return Range::create(m_document, startContainer(), startOffset(),
                         endContainer(), endOffset());
}

void Range::detach()
{
    // The detach() method, when invoked, must do nothing.
    // Its functionality (disabling a Range object) was removed, but the method
    // itself is preserved for compatibility.
}

bool Range::isPointInRange(Node* node, unsigned offset)
{
    BoundaryPoint newPoint(node, offset);
    if (!compareRoots(m_start, newPoint)) {
        return false;
    }

    if (!isValidOffset(node, offset)) {
        return false;
    }

    if (compareBoundaryPoints(newPoint, m_start) < 0 ||
        compareBoundaryPoints(newPoint, m_end) > 0) {
        return false;
    }

    return true;
}

short Range::comparePoint(Node* node, unsigned offset)
{
    BoundaryPoint newPoint(node, offset);
    if (!compareRoots(m_start, newPoint)) {
        throw new DOMException(m_document,
                               DOMException::Code::WRONG_DOCUMENT_ERR);
        return 0;
    }

    if (!isValidOffset(node, offset)) {
        return 0;
    }

    if (compareBoundaryPoints(newPoint, m_start) < 0) {
        return -1;
    }
    if (compareBoundaryPoints(newPoint, m_end) > 0) {
        return 1;
    }
    return 0;
}

bool Range::intersectsNode(Node* node)
{
    if (Traverse::root(startContainer()) != Traverse::root(node)) {
        return false;
    }

    Node* parent = node->parentNode();
    if (!parent) {
        return true;
    }

    unsigned offset = node->index();
    BoundaryPoint newPoint1(parent, offset);
    BoundaryPoint newPoint2(parent, offset + 1);

    if (compareBoundaryPoints(newPoint1, m_end) < 0 &&
        compareBoundaryPoints(newPoint2, m_start) > 0) {
        return true;
    }
    return false;
}

String* Range::toString()
{
    Node* startNode = startContainer();
    Node* endNode = endContainer();
    Node::NodeType startType = startNode->nodeType();
    Node::NodeType endType = endNode->nodeType();
    StringBuilder sb;

    if (startNode == endNode) {
        if (startType == Node::TEXT_NODE ||
            startType == Node::CDATA_SECTION_NODE) {
            String* data = startNode->asCharacterData()->data();
            unsigned length = data->length();
            sb.appendSubString(data, std::min(startOffset(), length),
                               std::min(endOffset(), length));
            return sb.finalize();
        }
    }

    if (startType == Node::TEXT_NODE || startType == Node::CDATA_SECTION_NODE) {
        String* data = startNode->asCharacterData()->data();
        unsigned length = data->length();
        sb.appendSubString(data, std::min(startOffset(), length), length);
    }

    Node* node;
    if (Node* child = Traverse::childAt(startNode, startOffset())) {
        node = child;
    } else {
        node = Traverse::nextSkippingChildren(startNode, nullptr);
    }

    Node* lastNode;
    if (Node* child = Traverse::childAt(endNode, endOffset())) {
        lastNode = child;
    } else {
        lastNode = Traverse::nextSkippingChildren(endNode, nullptr);
    }

    while (node != lastNode) {
        Node::NodeType type = node->nodeType();
        if (type == Node::TEXT_NODE || type == Node::CDATA_SECTION_NODE) {
            if (isPointInRange(node, 0) &&
                isPointInRange(node, node->nodeLength())) {
                String* data = node->asCharacterData()->data();
                unsigned length = data->length();
                sb.appendSubString(data, 0, length);
            }
        }
        node = Traverse::next(node, nullptr);
    }

    if (endType == Node::TEXT_NODE || endType == Node::CDATA_SECTION_NODE) {
        String* data = endNode->asCharacterData()->data();
        unsigned length = data->length();
        sb.appendSubString(data, 0, std::min(endOffset(), length));
    }

    return sb.finalize();
}

bool Range::isValidOffset(Node* node, unsigned offset)
{
    if (node->nodeType() == Node::DOCUMENT_TYPE_NODE) {
        throw new DOMException(m_document,
                               DOMException::Code::INVALID_NODE_TYPE_ERR);
        return false;
    }

    if (offset > node->nodeLength() ||
        offset > static_cast<unsigned>(std::numeric_limits<int>::max())) {
        throw new DOMException(m_document, DOMException::Code::INDEX_SIZE_ERR);
        return false;
    }

    return true;
}

bool Range::compareRoots(const BoundaryPoint& bp1, const BoundaryPoint& bp2)
{
    Node* rootA = Traverse::root(bp1.m_node);
    Node* rootB = Traverse::root(bp2.m_node);
    return rootA == rootB;
}

short Range::compareBoundaryPoints(const BoundaryPoint& bp1,
                                   const BoundaryPoint& bp2)
{
    STARFISH_ASSERT(compareRoots(bp1, bp2));

    Node* nodeA = bp1.m_node;
    Node* nodeB = bp2.m_node;
    unsigned offsetA = bp1.m_offset;
    unsigned offsetB = bp2.m_offset;

    if (nodeA == nodeB) {
        if (offsetA == offsetB) {
            return 0;
        } else if (offsetA < offsetB) {
            return -1;
        } else {
            return 1;
        }
    }

    Node* ancestor = Traverse::commonAncestor(nodeA, nodeB);
    STARFISH_ASSERT(ancestor);
    Node* childA = nodeA;
    Node* childB = nodeB;

    while (childA && childA->parentNode() != ancestor) {
        childA = childA->parentNode();
    }
    while (childB && childB->parentNode() != ancestor) {
        childB = childB->parentNode();
    }

    if (ancestor == nodeA) {
        return offsetA <= childB->index() ? -1 : 1;
    } else if (ancestor == nodeB) {
        return childA->index() < offsetB ? -1 : 1;
    } else {
        STARFISH_ASSERT(childA && childB);
        return childA->index() < childB->index() ? -1 : 1;
    }
}

Node* Range::root()
{
    return Traverse::root(startContainer());
}
}
