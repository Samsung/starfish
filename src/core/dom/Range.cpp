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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/dom/DOMPoint.h"
#include "core/dom/DOMQuad.h"
#include "core/dom/DOMRect.h"
#include "core/dom/DOMRectList.h"
#include "core/dom/Range.h"
#include "core/dom/Text.h"
#include "core/dom/Traverse.h"
#include "core/dom/DOMException.h"
#include "core/dom/CharacterData.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBox.h"
#include "core/layout/FrameBlockBox.h"
#include "core/dom/Text.h"
#include "core/dom/DocumentFragment.h"

namespace Starfish {

Range::Range(Document* document)
    : ScriptWrappable(this)
    , m_document(document)
    , m_start(document)
    , m_end(document)
{
}

Range::Range(Document* document, Node* startContainer, unsigned startOffset,
             Node* endContainer, unsigned endOffset)
    : ScriptWrappable(this)
    , m_document(document)
    , m_start(document)
    , m_end(document)
{
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
        m_document = node->document();
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
        m_document = node->document();
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
        throw new DOMException(m_document->executionContext(),
                               DOMException::Code::INVALID_NODE_TYPE_ERR);
        return;
    }

    setStart(parent, node->index());
}

void Range::setStartAfter(Node* node)
{
    Node* parent = node->parentNode();
    if (!parent) {
        throw new DOMException(m_document->executionContext(),
                               DOMException::Code::INVALID_NODE_TYPE_ERR);
        return;
    }

    setStart(parent, node->index() + 1);
}

void Range::setEndBefore(Node* node)
{
    Node* parent = node->parentNode();
    if (!parent) {
        throw new DOMException(m_document->executionContext(),
                               DOMException::Code::INVALID_NODE_TYPE_ERR);
        return;
    }

    setEnd(parent, node->index());
}

void Range::setEndAfter(Node* node)
{
    Node* parent = node->parentNode();
    if (!parent) {
        throw new DOMException(m_document->executionContext(),
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
        throw new DOMException(m_document->executionContext(),
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
        throw new DOMException(m_document->executionContext(),
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
        throw new DOMException(m_document->executionContext(),
                               DOMException::Code::NOT_SUPPORTED_ERR);
        return 0;
    }

    if (root() != sourceRange->root()) {
        throw new DOMException(m_document->executionContext(),
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

void Range::insertNode(Node* node)
{
    Node* startNode = startContainer();
    if (startNode->isProcessingInstruction() || startNode->isComment()) {
        throw new DOMException(m_document->executionContext(),
                               DOMException::HIERARCHY_REQUEST_ERR);
    }
    bool isStartText = startNode->isText();
    if (isStartText && !startNode->parentNode()) {
        throw new DOMException(m_document->executionContext(),
                               DOMException::HIERARCHY_REQUEST_ERR);
    }
    if (startNode == node) {
        throw new DOMException(m_document->executionContext(),
                               DOMException::HIERARCHY_REQUEST_ERR);
    }

    Node* referenceNode =
        isStartText ? startNode
                    : Traverse::childAtOrNull(startNode, startOffset());
    Node* parent = referenceNode ? referenceNode->parentNode() : startNode;
    parent->validatePreinsert(node, referenceNode);

    if (isStartText) {
        referenceNode = startNode->asText()->splitText(startOffset());
    }
    if (node == referenceNode) {
        referenceNode = referenceNode->nextSibling();
    }

    node->remove();

    unsigned newOffset =
        referenceNode ? referenceNode->index() : parent->nodeLength();
    newOffset += node->isDocumentFragment() ? node->nodeLength() : 1;

    parent->insertBefore(node, referenceNode);

    if (collapsed()) {
        setEnd(parent, newOffset);
    }
}

DOMRectList* Range::getClientRects()
{
    GCVector<DOMQuad*> quads;
    borderAndTextQuads(quads);
    if (quads.empty()) {
        return DOMRectList::create(m_document->executionContext());
    }
    return DOMRectList::create(m_document->executionContext(), quads);
}

DOMRect* Range::getBoundingClientRect(bool layoutIfNeeds /* = true */)
{
    GCVector<DOMQuad*> quads;
    borderAndTextQuads(quads, layoutIfNeeds);
    if (quads.empty()) {
        return new DOMRect(m_document->executionContext());
    }

    DOMRect* rect = quads[0]->getBounds();

    for (size_t i = 1; i < quads.size(); ++i) {
        rect->unite(quads[i]->getBounds());
    }

    return rect;
}

void Range::borderAndTextQuads(GCVector<DOMQuad*>& quads,
                               bool layoutIfNeeds /* = true */)
{
    if (layoutIfNeeds) {
        m_document->window()->webView()->layoutIfNeeded(false);
    }

    GCUnorderedSet<Node*> selectedElements;
    Node* stop = pastLastNode();
    for (Node* n = firstNode(); n != stop; n = Traverse::next(n, nullptr)) {
        if (n->isElement()) {
            if ((selectedElements.find(n) == selectedElements.end()) ||
                (!n->contains(startContainer()) &&
                 !n->contains(endContainer()))) {
                selectedElements.insert(n);
            }
        }
    }
    GCUnorderedSet<Frame*> checkedFrameBlockBox;
    for (Node* n = firstNode(); n != stop; n = Traverse::next(n, nullptr)) {
        if (n->isElement()) {
            if (selectedElements.find(n) == selectedElements.end() ||
                selectedElements.find(n->parentNode()) !=
                    selectedElements.end()) {
                continue;
            }
            n->asElement()->getClientQuads(quads, layoutIfNeeds);
        } else if (n->isText()) {
            Frame* f = n->frame();
            if (f->isFrameText()) {
                Frame* nearestFrameBlockBox = f->parent();
                while (nearestFrameBlockBox &&
                       !nearestFrameBlockBox->isFrameBlockBox()) {
                    nearestFrameBlockBox = nearestFrameBlockBox->parent();
                }
                if (nearestFrameBlockBox &&
                    selectedElements.find(n) == selectedElements.end() &&
                    checkedFrameBlockBox.find(nearestFrameBlockBox) ==
                        checkedFrameBlockBox.end()) {
                    checkedFrameBlockBox.insert(nearestFrameBlockBox);

                    auto blockBox = nearestFrameBlockBox->asFrameBlockBox();
                    blockBox->iterateChildFrameBox([&](FrameBox* childBox) {
                        if (childBox->node() &&
                            isPointInRange(childBox->node(), 0)) {
                            SkMatrix m =
                                childBox->asFrameBox()->computeScreenMatrix();
                            LayoutRect rect;
                            rect.setWidth(childBox->asFrameBox()->width());
                            rect.setHeight(childBox->asFrameBox()->height());
                            rect = computeBoxExtent(rect, m);

                            DOMQuad* q = new DOMQuad(
                                m_document->executionContext(),
                                DOMPointInit(rect.location().x(),
                                             rect.location().y()),
                                DOMPointInit(rect.location().x() +
                                                 rect.size().width(),
                                             rect.location().y()),
                                DOMPointInit(
                                    rect.location().x() + rect.size().width(),
                                    rect.location().y() + rect.size().height()),
                                DOMPointInit(rect.location().x(),
                                             rect.location().y() +
                                                 rect.size().height()));
                            quads.push_back(q);
                        }
                    });
                }
            }
        }
    }
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
        throw new DOMException(m_document->executionContext(),
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

Node* Range::firstNode()
{
    Node* node = startContainer();
    if (node->isCharacterData()) {
        return node;
    }
    if (Node* child = Traverse::childAt(node, m_start.m_offset)) {
        return child;
    }
    if (!m_start.m_offset) {
        return node;
    }
    return Traverse::nextSkippingChildren(node, nullptr);
}

Node* Range::pastLastNode()
{
    Node* node = endContainer();
    if (node->isCharacterData()) {
        return Traverse::nextSkippingChildren(node, nullptr);
    }
    if (Node* child = Traverse::childAt(node, m_end.m_offset)) {
        return child;
    }
    return Traverse::nextSkippingChildren(node, nullptr);
}

bool Range::isValidOffset(Node* node, unsigned offset)
{
    if (node->nodeType() == Node::DOCUMENT_TYPE_NODE) {
        throw new DOMException(m_document->executionContext(),
                               DOMException::Code::INVALID_NODE_TYPE_ERR);
        return false;
    }

    if (offset > node->nodeLength() ||
        offset > static_cast<unsigned>(std::numeric_limits<int>::max())) {
        throw new DOMException(m_document->executionContext(),
                               DOMException::Code::INDEX_SIZE_ERR);
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

void Range::deleteContents()
{
    processContents(Delete);
}

DocumentFragment* Range::extractContents()
{
    return processContents(Extract);
}

DocumentFragment* Range::cloneContents()
{
    return processContents(Clone);
}

void Range::surroundContents(Node* newParent)
{
    Node* start = startContainer();
    if (start->nodeType() == Node::TEXT_NODE) {
        start = start->parentNode();
    }
    Node* end = endContainer();
    if (end->nodeType() == Node::TEXT_NODE) {
        end = end->parentNode();
    }
    if (start != end) {
        throw new DOMException(m_document->executionContext(),
                               DOMException::Code::INVALID_NODE_TYPE_ERR);
    }

    Node::NodeType type = newParent->nodeType();
    if (type == Node::DOCUMENT_NODE || type == Node::DOCUMENT_TYPE_NODE ||
        type == Node::DOCUMENT_FRAGMENT_NODE) {
        throw new DOMException(m_document->executionContext(),
                               DOMException::Code::INVALID_NODE_TYPE_ERR);
    }

    DocumentFragment* fragment = extractContents();
    if (newParent->hasChildNodes()) {
        while (Node* first = newParent->firstChild()) {
            newParent->removeChild(first);
        }
    }

    insertNode(newParent);
    newParent->appendChild(fragment);
    selectNode(newParent);
    return;
}

// The part of below is taken from Webkit Project.
// (Source/WebCore/dom/Range.cpp)

static inline Node* highestAncestorUnderCommonRoot(Node* node, Node* rootNode)
{
    if (node == rootNode)
        return nullptr;
    while (node->parentNode() != rootNode)
        node = node->parentNode();
    return node;
}

static inline void removeCharacterData(CharacterData& data,
                                       unsigned startOffset, unsigned endOffset)
{
    if (data.length() - endOffset) {
        data.deleteData(endOffset, data.length() - endOffset);
    }
    if (startOffset) {
        data.deleteData(0, startOffset);
    }
}

static void processNodes(Range::ProcessingType type, GCVector<Node*>& nodes,
                         Node* oldContainer, Node* newContainer)
{
    for (auto& node : nodes) {
        switch (type) {
        case Range::Delete: {
            oldContainer->removeChild(node);
            break;
        }
        case Range::Extract: {
            newContainer->appendChild(node);
            break;
        }
        case Range::Clone: {
            newContainer->appendChild(node->cloneNode(true));
            break;
        }
        }
    }
}

static Node* processContentsBetweenOffsets(Range::ProcessingType type,
                                           DocumentFragment* documentFragment,
                                           Node* container,
                                           unsigned startOffset,
                                           unsigned endOffset)
{
    Node* result = nullptr;
    switch (container->nodeType()) {
    case Node::TEXT_NODE:
    case Node::CDATA_SECTION_NODE:
    case Node::COMMENT_NODE: {
        endOffset = std::min(endOffset, ((CharacterData*)container)->length());
        startOffset = std::min(startOffset, endOffset);
        if (type == Range::Extract || type == Range::Clone) {
            CharacterData* characters =
                (CharacterData*)(container->cloneNode(true));
            removeCharacterData(*characters, startOffset, endOffset);
            if (documentFragment) {
                result = documentFragment;
                result->appendChild(characters);
            } else
                result = std::move(characters);
        }
        if (type == Range::Extract || type == Range::Delete) {
            ((CharacterData*)container)
                ->deleteData(startOffset, endOffset - startOffset);
        }
        break;
    }
    case Node::PROCESSING_INSTRUCTION_NODE:
        break;
    case Node::ELEMENT_NODE:
    case Node::ATTRIBUTE_NODE:
    case Node::DOCUMENT_NODE:
    case Node::DOCUMENT_TYPE_NODE:
    case Node::DOCUMENT_FRAGMENT_NODE: {
        if (type == Range::Extract || type == Range::Clone) {
            if (documentFragment)
                result = documentFragment;
            else
                result = container->cloneNode(false);
        }
        GCVector<Node*> nodes;
        Node* n = container->firstChild();
        for (unsigned i = startOffset; n && i; i--)
            n = n->nextSibling();
        for (unsigned i = startOffset; n && i < endOffset;
             i++, n = n->nextSibling()) {
            if (type != Range::Delete && n->isDocumentType()) {
                return nullptr;
            }
            nodes.push_back(n);
        }
        processNodes(type, nodes, container, result);
        break;
    }
    default:
        break;
    }
    return result;
}

static Node* processAncestorsAndTheirSiblings(
    Range::ProcessingType type, Node* container,
    Range::ContentsProcessDirection direction, Node* passedContainer,
    Node* rootNode)
{
    Node* copyedContainer = passedContainer;

    GCVector<Node*> ancestors;
    for (Node* ancestor = container->parentNode();
         ancestor && ancestor != rootNode; ancestor = ancestor->parentNode())
        ancestors.push_back(ancestor);

    Node* firstChildInAncestorToProcess = direction == Range::ProcessForward
                                              ? container->nextSibling()
                                              : container->previousSibling();
    for (auto& ancestor : ancestors) {
        if (type == Range::Extract || type == Range::Clone) {
            auto copyedAncestor = ancestor->cloneNode(false);
            if (copyedContainer) {
                copyedAncestor->appendChild(copyedContainer);
            }
            copyedContainer = std::move(copyedAncestor);
        }

        GCVector<Node*> nodes;
        for (Node* child = firstChildInAncestorToProcess; child;
             child = (direction == Range::ProcessForward)
                         ? child->nextSibling()
                         : child->previousSibling())
            nodes.push_back(child);

        for (auto& child : nodes) {
            switch (type) {
            case Range::Delete: {
                ancestor->removeChild(child);
                break;
            }
            case Range::Extract:
                if (direction == Range::ProcessForward) {
                    copyedContainer->appendChild(child);
                } else {
                    copyedContainer->insertBefore(
                        child, copyedContainer->firstChild());
                }
                break;
            case Range::Clone:
                if (direction == Range::ProcessForward) {
                    copyedContainer->appendChild(child->cloneNode(true));
                } else {
                    copyedContainer->insertBefore(
                        child->cloneNode(true), copyedContainer->firstChild());
                }
                break;
            }
        }
        firstChildInAncestorToProcess = direction == Range::ProcessForward
                                            ? ancestor->nextSibling()
                                            : ancestor->previousSibling();
    }
    return copyedContainer;
}

static inline unsigned lengthOfContentsInNode(Node* node)
{
    switch (node->nodeType()) {
    case Node::DOCUMENT_TYPE_NODE:
    case Node::ATTRIBUTE_NODE:
        return 0;
    case Node::TEXT_NODE:
    case Node::CDATA_SECTION_NODE:
    case Node::COMMENT_NODE:
    case Node::PROCESSING_INSTRUCTION_NODE:
        return ((CharacterData*)(node))->length();
    case Node::ELEMENT_NODE:
    case Node::DOCUMENT_NODE:
    case Node::DOCUMENT_FRAGMENT_NODE:
        return node->nodeLength();
    default:
        break;
    }
    STARFISH_ASSERT_NOT_REACHED();
    return 0;
}

static inline Node* childOfCommonRootBeforeOffset(Node* container,
                                                  unsigned offset,
                                                  Node* rootNode)
{
    if (!rootNode->contains(container))
        return 0;

    if (container == rootNode) {
        container = container->firstChild();
        for (unsigned i = 0; container && i < offset; i++)
            container = container->nextSibling();
    } else {
        while (container->parentNode() != rootNode)
            container = container->parentNode();
    }
    return container;
}

static unsigned computeNodeIdx(Node* node)
{
    unsigned count = 0;
    for (Node* sibling = node->previousSibling(); sibling;
         sibling = sibling->previousSibling())
        ++count;
    return count;
}

DocumentFragment* Range::processContents(ProcessingType type)
{
    DocumentFragment* fragment = nullptr;
    if (type == Extract || type == Clone) {
        fragment = m_document->createDocumentFragment();
    }

    if (collapsed()) {
        return fragment;
    }

    Node* rootNode = Traverse::commonAncestor(startContainer(), endContainer());
    Node* partialStart =
        highestAncestorUnderCommonRoot(m_start.m_node, rootNode);
    Node* partialEnd = highestAncestorUnderCommonRoot(m_end.m_node, rootNode);
    Node* leftContents = nullptr;
    if (m_start.m_node != rootNode && rootNode->contains(m_start.m_node)) {
        auto firstResult = processContentsBetweenOffsets(
            type, nullptr, m_start.m_node, m_start.m_offset,
            lengthOfContentsInNode(m_start.m_node));
        leftContents = processAncestorsAndTheirSiblings(
            type, m_start.m_node, ProcessForward, std::move(firstResult),
            rootNode);
    }

    Node* rightContents = nullptr;
    if (endContainer() != rootNode && rootNode->contains(m_end.m_node)) {
        auto firstResult = processContentsBetweenOffsets(
            type, nullptr, m_end.m_node, 0, m_end.m_offset);
        rightContents = processAncestorsAndTheirSiblings(
            type, m_end.m_node, ProcessBackward, std::move(firstResult),
            rootNode);
    }

    Node* processStart = childOfCommonRootBeforeOffset(
        m_start.m_node, m_start.m_offset, rootNode);
    if (processStart && m_start.m_node != rootNode)
        processStart = processStart->nextSibling();
    Node* processEnd =
        childOfCommonRootBeforeOffset(m_end.m_node, m_end.m_offset, rootNode);

    if (type == Extract || type == Delete) {
        if (partialStart && rootNode->contains(partialStart)) {
            setStart(partialStart->parentNode(),
                     computeNodeIdx(partialStart) + 1);
        } else if (partialEnd && rootNode->contains(partialEnd)) {
            setStart(partialEnd->parentNode(), computeNodeIdx(partialEnd));
        }
        m_end = m_start;
    }

    if ((type == Extract || type == Clone) && leftContents) {
        fragment->appendChild(leftContents);
    }

    if (processStart) {
        GCVector<Node*> nodes;
        for (Node* node = processStart; node && node != processEnd;
             node = node->nextSibling())
            nodes.push_back(node);
        processNodes(type, nodes, rootNode, fragment);
    }

    if ((type == Extract || type == Clone) && rightContents) {
        fragment->appendChild(rightContents);
    }
    return fragment;
}

} // namespace Starfish
