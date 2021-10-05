/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#include "core/dom/Node.h"

#include "Starfish.h"
#include "core/dom/Attr.h"
#include "core/dom/CharacterData.h"
#include "core/dom/Document.h"
#include "core/dom/DocumentFragment.h"
#include "core/dom/DocumentType.h"
#include "core/dom/ProcessingInstruction.h"
#include "core/dom/DOMException.h"
#include "core/dom/DOMTokenList.h"
#include "core/dom/Traverse.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLElement.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/NodeList.h"
#include "core/dom/SelectorQuery.h"
#include "core/dom/Text.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBox.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameTreeBuilder.h"
#include "core/layout/StackingContext.h"
#include "core/page/Window.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"
#include "binding/NodeOrDOMStringUnion.h"

namespace Starfish {

ActiveHTMLCollectionList*
RareNodeMembers::ensureActiveHtmlCollectionListForTagName()
{
    if (m_activeHtmlCollectionListsForTagName == nullptr) {
        m_activeHtmlCollectionListsForTagName =
            new (GC) ActiveHTMLCollectionList;
    }
    return m_activeHtmlCollectionListsForTagName;
}

ActiveStringPairHTMLCollectionList*
RareNodeMembers::ensureActiveHtmlCollectionListForTagNameNS()
{
    if (m_activeHtmlCollectionListsForTagNameNS == nullptr) {
        m_activeHtmlCollectionListsForTagNameNS =
            new (GC) ActiveStringPairHTMLCollectionList;
    }
    return m_activeHtmlCollectionListsForTagNameNS;
}

ActiveHTMLCollectionList*
RareNodeMembers::ensureActiveHtmlCollectionListForClassName()
{
    if (m_activeHtmlCollectionListsForClassName == nullptr) {
        m_activeHtmlCollectionListsForClassName =
            new (GC) ActiveHTMLCollectionList;
    }
    return m_activeHtmlCollectionListsForClassName;
}

ActiveNodeListVector* RareNodeMembers::ensureActiveNodeListVectorForName()
{
    if (m_activeNodeListVectorForName == nullptr) {
        m_activeNodeListVectorForName = new (GC) ActiveNodeListVector;
    }
    return m_activeNodeListVectorForName;
}

HTMLCollection* RareNodeMembers::hasQueryInActiveHtmlCollectionList(
    ActiveHTMLCollectionList* list, String* query)
{
    for (size_t i = 0; i < list->size(); i++) {
        if ((*list)[i].first->equals(query)) {
            return (*list)[i].second;
        }
    }
    return nullptr;
}

HTMLCollection* RareNodeMembers::hasQueryInActiveHtmlCollectionList(
    ActiveStringPairHTMLCollectionList* list, std::pair<String*, String*> query)
{
    for (size_t i = 0; i < list->size(); i++) {
        if ((*list)[i].first == query) {
            return (*list)[i].second;
        }
    }
    return nullptr;
}

NodeList* RareNodeMembers::ensureQueryInActiveNodeListVectorForName(
    Node* ownerNode, String* query)
{
    ensureActiveNodeListVectorForName();
    for (size_t i = 0; i < m_activeNodeListVectorForName->size(); i++) {
        if ((*m_activeNodeListVectorForName)[i].first->equals(query)) {
            return (*m_activeNodeListVectorForName)[i].second;
        }
    }

    QualifiedName* ptr = new QualifiedName(
        AtomicString::emptyAtomicString(),
        AtomicString::createAtomicString(ownerNode->starfish(), query));

    m_activeNodeListVectorForName->emplace_back(std::make_pair(
        query,
        new NodeList(ownerNode, NodeListImpl::NamedAccessFilter, ptr, false)));
    return m_activeNodeListVectorForName->back().second;
}

void RareNodeMembers::putActiveHtmlCollectionListWithQuery(
    ActiveHTMLCollectionList* list, String* query, HTMLCollection* coll)
{
    STARFISH_ASSERT(!hasQueryInActiveHtmlCollectionList(list, query));
    STARFISH_ASSERT(query);
    STARFISH_ASSERT(coll);
    list->push_back(std::make_pair(query, coll));
}

void RareNodeMembers::putActiveHtmlCollectionListWithQuery(
    ActiveStringPairHTMLCollectionList* list, std::pair<String*, String*> query,
    HTMLCollection* coll)
{
    STARFISH_ASSERT(!hasQueryInActiveHtmlCollectionList(list, query));
    STARFISH_ASSERT(query.second);
    STARFISH_ASSERT(coll);
    list->push_back(std::make_pair(query, coll));
}

void RareNodeMembers::invalidateActiveActiveNodeListCacheIfNeeded()
{
    if (m_children) {
        m_children->getNodeListImpl().invalidateCache();
    }

    if (m_activeHtmlCollectionListsForTagName) {
        for (size_t i = 0; i < m_activeHtmlCollectionListsForTagName->size();
             i++) {
            (*m_activeHtmlCollectionListsForTagName)[i]
                .second->getNodeListImpl()
                .invalidateCache();
        }
    }

    if (m_activeHtmlCollectionListsForTagNameNS) {
        for (size_t i = 0; i < m_activeHtmlCollectionListsForTagNameNS->size();
             i++) {
            (*m_activeHtmlCollectionListsForTagNameNS)[i]
                .second->getNodeListImpl()
                .invalidateCache();
        }
    }

    if (m_activeHtmlCollectionListsForClassName) {
        for (size_t i = 0; i < m_activeHtmlCollectionListsForClassName->size();
             i++) {
            (*m_activeHtmlCollectionListsForClassName)[i]
                .second->getNodeListImpl()
                .invalidateCache();
        }
    }

    if (m_childNodeList) {
        m_childNodeList->getNodeListImpl().invalidateCache();
    }
}

GetRootNodeOptions::GetRootNodeOptions()
    : GetRootNodeOptions(false)
{
}

GetRootNodeOptions::GetRootNodeOptions(bool composed)
    : m_composed(composed)
{
}

bool GetRootNodeOptions::composed() const
{
    return m_composed;
}

void GetRootNodeOptions::setComposed(bool composed)
{
    m_composed = composed;
}

NodeList* Node::childNodes()
{
    STARFISH_ASSERT(m_document);
    auto rareData = ensureRareMembers();
    if (rareData->m_childNodeList == nullptr) {
        rareData->m_childNodeList =
            new NodeList(this, NodeListImpl::ChildNodeFilter, this, true);
    }
    return rareData->m_childNodeList;
}

static bool isNodeInNodes(Node* node, const GCVector<NodeOrDOMString>& nodes)
{
    size_t size = nodes.size();
    for (size_t i = 0; i < size; ++i) {
        if (nodes[i].isNodeValue() &&
            nodes[i].getNodeValue()->isEqualNode(node)) {
            return true;
        }
    }
    return false;
}

static Node* findViablePreviousSibling(Node* node,
                                       const GCVector<NodeOrDOMString>& nodes)
{
    Node* sibling = node->previousSibling();
    while (sibling) {
        if (!isNodeInNodes(sibling, nodes)) {
            return sibling;
        }
        sibling = sibling->previousSibling();
    }
    return nullptr;
}

static Node* findViableNextSibling(Node* node,
                                   const GCVector<NodeOrDOMString>& nodes)
{
    Node* sibling = node->nextSibling();
    while (sibling) {
        if (!isNodeInNodes(sibling, nodes)) {
            return sibling;
        }
        sibling = sibling->nextSibling();
    }
    return nullptr;
}

static Node* nodeOrStringToNode(const NodeOrDOMString& nodeOrString,
                                Document* document)
{
    if (nodeOrString.isNodeValue()) {
        return nodeOrString.getNodeValue();
    }
    return new Text(document, nodeOrString.getDOMStringValue());
}

static Node* convertNodesIntoNode(const GCVector<NodeOrDOMString>& nodes,
                                  Document* document)
{
    size_t size = nodes.size();
    if (size == 1) {
        return nodeOrStringToNode(nodes[0], document);
    }

    Node* fragment = new DocumentFragment(document);
    for (size_t i = 0; i < size; ++i) {
        fragment->appendChild(nodeOrStringToNode(nodes[i], document));
    }
    return fragment;
}

void Node::before(const GCVector<NodeOrDOMString>& nodes)
{
    Node* parent = parentNode();
    if (!parent) {
        return;
    }

    Node* viablePreviousSibling = findViablePreviousSibling(this, nodes);
    Node* node = convertNodesIntoNode(nodes, document());
    if (node) {
        parent->insertBefore(node, viablePreviousSibling
                                       ? viablePreviousSibling->nextSibling()
                                       : parent->firstChild());
    }
}

void Node::after(const GCVector<NodeOrDOMString>& nodes)
{
    Node* parent = parentNode();
    if (!parent) {
        return;
    }
    Node* viableNextSibling = findViableNextSibling(this, nodes);
    Node* node = convertNodesIntoNode(nodes, document());
    if (node) {
        parent->insertBefore(node, viableNextSibling);
    }
}

void Node::replaceWith(const GCVector<NodeOrDOMString>& nodes)
{
    Node* parent = parentNode();
    if (!parent) {
        return;
    }
    Node* viableNextSibling = findViableNextSibling(this, nodes);
    Node* node = convertNodesIntoNode(nodes, document());

    if (parent == parentNode()) {
        parent->replaceChild(node, this);
    } else {
        parent->insertBefore(node, viableNextSibling);
    }
}

String* Node::baseURI() const
{
    return document()->baseURL()->urlString();
}

Nullable<String*> Node::nodeValue() const
{
    switch (nodeType()) {
    case ATTRIBUTE_NODE:
        return asAttr()->value();
    case TEXT_NODE:
    case COMMENT_NODE:
        return asCharacterData()->data();
    case PROCESSING_INSTRUCTION_NODE:
        return asProcessingInstruction()->data();
    default:
        return nullptr;
    }
}

void Node::setNodeValue(Nullable<String*> val)
{
    String* str = String::emptyString;
    if (val.hasValue()) {
        str = val.getValue();
    }

    switch (nodeType()) {
    case ATTRIBUTE_NODE:
        asAttr()->setValue(str);
        break;
    case TEXT_NODE:
    case COMMENT_NODE:
        asCharacterData()->setData(str);
        break;
    case PROCESSING_INSTRUCTION_NODE:
        asProcessingInstruction()->setData(str);
        break;
    default:
        break;
    }
}

Nullable<String*> Node::textContent() const
{
    switch (nodeType()) {
    case DOCUMENT_FRAGMENT_NODE:
    case ELEMENT_NODE: {
        String* str = String::emptyString;
        for (Node* child = firstChild(); child != nullptr;
             child = child->nextSibling()) {
            if (child->isText() || child->isElement()) {
                STARFISH_ASSERT(child->textContent().hasValue());
                str = str->concat(child->textContent().getValue());
            }
        }

        StringBuilder sb;
        if (!nextSibling() && str->length()) {
            size_t last = str->length() - 1;
            if (str->charAt(last) == '\n') {
                str = str->substring(0, last);
            }
        }
        sb.appendString(str);

        return sb.finalize();
    }
    case ATTRIBUTE_NODE:
        return asAttr()->value();
    case TEXT_NODE:
    case COMMENT_NODE:
    case PROCESSING_INSTRUCTION_NODE:
        return asCharacterData()->data();
    default:
        return nullptr;
    }
}

void Node::setTextContent(Nullable<String*> val)
{
    String* str = String::emptyString;
    if (val.hasValue()) {
        str = val.getValue();
    }

    switch (nodeType()) {
    case DOCUMENT_FRAGMENT_NODE:
    case ELEMENT_NODE: {
        while (firstChild()) {
            removeChild(firstChild());
        }

        if (!str->equals(String::emptyString)) {
            appendChild(new Text(document(), str));
        }
        break;
    }
    case ATTRIBUTE_NODE:
        asAttr()->setValue(str);
        break;
    case TEXT_NODE:
    case COMMENT_NODE:
    case PROCESSING_INSTRUCTION_NODE:
        asCharacterData()->setData(str);
        break;
    default:
        break;
    }
}

Node* Node::cloneNode(bool deep)
{
    Node* newNode = clone();
    STARFISH_ASSERT(newNode);

    if (deep) {
        for (Node* child = firstChild(); child; child = child->nextSibling()) {
            Node* newChild = child->cloneNode(true);
            STARFISH_ASSERT(newChild);
            newNode->appendChild(newChild);
        }
    }
    return newNode;
}

Node* Node::getRootNode(GetRootNodeOptions options)
{
    bool composed = options.composed();
    if (composed) {
        return getRootNode();
    } else {
        Node* n = this;
        while (n) {
            if (!n->parentNode() || n->isShadowRoot()) {
                break;
            }
            n = n->parentNode();
        }
        return n;
    }
}

Node* Node::makeShadowClone()
{
    if (isSVGUseElement()) {
        return nullptr;
    }

    Node* newNode = clone();
    if (newNode) {
        for (Node* child = firstChild(); child; child = child->nextSibling()) {
            Node* newChild = child->makeShadowClone();
            if (newChild) {
                newNode->appendChild(newChild);
            }
        }
    }
    return newNode;
}

bool Node::isEqualNode(Node* other)
{
    if (other == nullptr) {
        return false;
    }
    if (this == other) {
        return true;
    }
    if (nodeType() != other->nodeType()) {
        return false;
    }

    switch (nodeType()) {
    case DOCUMENT_TYPE_NODE: {
        DocumentType* thisNode = asDocumentType();
        DocumentType* otherNode = other->asDocumentType();
        if (!(thisNode->nodeName()->equals(otherNode->nodeName()) &&
              thisNode->publicId()->equals(otherNode->publicId()) &&
              thisNode->systemId()->equals(otherNode->systemId()))) {
            return false;
        }
        break;
    }
    case ELEMENT_NODE: {
        Element* thisNode = asElement();
        Element* otherNode = other->asElement();
        if (!(thisNode->localName()->equals(otherNode->localName())) ||
            !(thisNode->prefix() == otherNode->prefix()) ||
            !(thisNode->namespaceURI() == otherNode->namespaceURI()) ||
            !(thisNode->hasSameAttributes(otherNode))) {
            return false;
        }
        break;
    }
    case ATTRIBUTE_NODE: {
        Attr* thisAttr = asAttr();
        Attr* otherAttr = other->asAttr();
        if (!(thisAttr->localName()->equals(otherAttr->localName())) ||
            !(thisAttr->namespaceURI() == otherAttr->namespaceURI()) ||
            !(thisAttr->value()->equals(otherAttr->value()))) {
            return false;
        }
        break;
    }
    case PROCESSING_INSTRUCTION_NODE: {
        ProcessingInstruction* thisNode = asProcessingInstruction();
        ProcessingInstruction* otherNode = other->asProcessingInstruction();
        if (!(thisNode->nodeName()->equals(otherNode->nodeName()) &&
              thisNode->target()->equals(otherNode->target()) &&
              thisNode->data()->equals(otherNode->data()))) {
            return false;
        }
        break;
    }
    case TEXT_NODE:
    case COMMENT_NODE:
        STARFISH_ASSERT(nodeValue().hasValue());
        STARFISH_ASSERT(other->nodeValue().hasValue());
        if (!nodeValue().getValue()->equals(other->nodeValue().getValue())) {
            return false;
        }
        break;
    default: {
        // for any other node, do nothing
        break;
    }
    }

    Node* child = firstChild();
    Node* otherChild = other->firstChild();
    while (child) {
        if (!child->isEqualNode(otherChild)) {
            return false;
        }
        child = child->nextSibling();
        otherChild = otherChild->nextSibling();
    }
    if (otherChild) {
        return false;
    }

    return true;
}

bool Node::isSameNode(Node* other)
{
    return other == this;
}

void Node::normalize()
{
    Node* node = this;
    while (Node* firstChild = node->firstChild()) {
        node = firstChild;
    }

    while (node) {
        if (node == this) {
            break;
        }

        if (node->nodeType() == TEXT_NODE && !node->isCDATASection()) {
            node = node->asText()->mergeWithTextSiblings();
        } else {
            node = Traverse::nextPostOrder(node, nullptr);
        }
    }
}

bool Node::isDescendantOf(const Node* other)
{
    // Return true if other is an ancestor of this, otherwise false
    if (!other || !other->hasChildNodes()) {
        return false;
    }
    for (const Node* n = parentNode(); n; n = n->parentNode()) {
        if (n == other) {
            return true;
        }
    }
    return false;
}

unsigned Node::index()
{
    STARFISH_ASSERT(parentNode());
    Node* parent = parentNode();
    Node* child = parent->firstChild();
    unsigned index = 0;

    while (child != this) {
        index++;

        STARFISH_ASSERT(child != nullptr);
        child = child->nextSibling();
    }

    return index;
}

OverflowValue Node::appliedOverflowX()
{
    if (isDocument()) {
        HTMLElement* htmlElement = document()->rootElement();
        HTMLElement* bodyElement = document()->body();
        OverflowValue htmlOverflowX = VisibleOverflow;
        OverflowValue bodyOverflowX = VisibleOverflow;
        OverflowValue appliedOverflowX = AutoOverflow;

        if (htmlElement && htmlElement->style()) {
            htmlOverflowX = htmlElement->style()->overflowX();
        }

        if (bodyElement && bodyElement->style()) {
            bodyOverflowX = bodyElement->style()->overflowX();
        }

        if (htmlOverflowX == HiddenOverflow) {
            if (bodyOverflowX == VisibleOverflow ||
                bodyOverflowX == HiddenOverflow) {
                appliedOverflowX = HiddenOverflow;
            }
        } else if (bodyOverflowX == HiddenOverflow) {
            if (htmlOverflowX == VisibleOverflow) {
                appliedOverflowX = HiddenOverflow;
            }
        }

        return appliedOverflowX;
    }

    if (isHTMLHtmlElement()) {
        return VisibleOverflow;
    }

    return style()->overflowX();
}

OverflowValue Node::appliedOverflowY()
{
    if (isDocument()) {
        HTMLElement* htmlElement = document()->rootElement();
        HTMLElement* bodyElement = document()->body();
        OverflowValue htmlOverflowY = VisibleOverflow;
        OverflowValue bodyOverflowY = VisibleOverflow;
        OverflowValue appliedOverflowY = AutoOverflow;

        if (htmlElement && htmlElement->style()) {
            htmlOverflowY = htmlElement->style()->overflowY();
        }

        if (bodyElement && bodyElement->style()) {
            bodyOverflowY = bodyElement->style()->overflowY();
        }

        if (htmlOverflowY == HiddenOverflow) {
            appliedOverflowY = HiddenOverflow;
        } else if (bodyOverflowY == HiddenOverflow) {
            if (htmlOverflowY == VisibleOverflow) {
                appliedOverflowY = HiddenOverflow;
            }
        }

        return appliedOverflowY;
    }

    if (isHTMLHtmlElement()) {
        return VisibleOverflow;
    }

    return style()->overflowY();
}

Element* Node::firstElementChild()
{
    Node* child = firstChild();
    while (child) {
        if (child->isElement()) {
            break;
        }
        child = child->nextSibling();
    }

    if (child) {
        return child->asElement();
    } else {
        return nullptr;
    }
}

Element* Node::lastElementChild()
{
    Node* child = lastChild();
    while (child) {
        if (child->isElement()) {
            break;
        }
        child = child->previousSibling();
    }

    if (child) {
        return child->asElement();
    } else {
        return nullptr;
    }
}

Element* Node::nextElementSibling()
{
    Node* sibling = nextSibling();
    while (sibling) {
        if (sibling->isElement()) {
            break;
        }
        sibling = sibling->nextSibling();
    }

    if (sibling) {
        return sibling->asElement();
    } else {
        return nullptr;
    }
}

Element* Node::previousElementSibling()
{
    Node* sibling = previousSibling();
    while (sibling) {
        if (sibling->isElement()) {
            break;
        }
        sibling = sibling->previousSibling();
    }

    if (sibling) {
        return sibling->asElement();
    } else {
        return nullptr;
    }
}

unsigned long Node::childElementCount()
{
    unsigned long count = 0;
    Node* child = firstChild();
    while (child) {
        if (child->isElement()) {
            count++;
        }
        child = child->nextSibling();
    }
    return count;
}

void Node::prepend(const GCVector<NodeOrDOMString>& nodes)
{
    Node* node = convertNodesIntoNode(nodes, document());
    insertBefore(node, firstChild());
}

void Node::append(const GCVector<NodeOrDOMString>& nodes)
{
    Node* node = convertNodesIntoNode(nodes, document());
    appendChild(node);
}

Node* Node::nearestParentElement()
{
    Node* t = this;
    while (t && !t->isHTMLElement() && !t->isDocument() && !t->isSVGElement()) {
        t = t->parentNode();
    }

    return t;
}

static void setSiblingsNeedsStyleRecalcIfNeededWithStateChange(Node* startNode,
                                                               int oldState,
                                                               int newState)
{
    Node* node = startNode->nextSibling();
    while (node) {
        if (node->isElement()) {
            int stateDamageMap = 0;
            if (node->style()) {
                stateDamageMap =
                    node->style()->styleDamageSourceNodeStateDOMTreeMap();
            }

            if ((stateDamageMap & oldState) | (stateDamageMap & newState)) {
                node->setNeedsStyleRecalc(
                    Node::StyleChangeReason::ElementStateChangeDomTree);
            }
        }

        node = node->nextSibling();
    }
}

static void setChildrenNeedsStyleRecalcIfNeededWithStateChange(Node* startNode,
                                                               int oldState,
                                                               int newState)
{
    Node* child = startNode->firstChild();
    while (child) {
        if (child->isElement()) {
            int stateDamageMap = 0;
            if (child->style()) {
                stateDamageMap =
                    child->style()->styleDamageSourceNodeStateDOMTreeMap();
            }

            if ((stateDamageMap & oldState) | (stateDamageMap & newState)) {
                child->setNeedsStyleRecalc(
                    Node::StyleChangeReason::ElementStateChangeDomTree);
            }
        }
        child = child->nextSibling();
    }
}

void Node::setState(NodeState state, bool enable)
{
    int newState = m_state;

    if (state == NodeStateNormal) {
        newState = 0;
    } else {
        if (enable) {
            newState = newState | state;
        } else {
            newState = newState & ~state;
        }
    }

    if (m_state != newState) {
        int oldState = m_state;
        m_state = newState;

        if (document()->doesParticipateInRendering()) {
            StyleResolver::StyleDamageSource cmr;
            int stateDamageMap = 0;
            if (style() && style()->styleDamageSource()) {
                cmr = style()->styleDamageSource();
                stateDamageMap = style()->styleDamageSourceNodeStateMap();
            } else {
                cmr = StyleResolver::StyleDamageSource::NoDamage;
            }

            if (cmr &
                StyleResolver::StyleDamageSource::StyleDamageFromElementState) {
                if ((stateDamageMap & oldState) | (stateDamageMap & newState)) {
                    m_needsStyleRecalc = true;
                    if (parentNode()) {
                        parentNode()->setChildNeedsStyleRecalc();
                    }
                }
            }

            setSiblingsNeedsStyleRecalcIfNeededWithStateChange(this, oldState,
                                                               newState);
            setChildrenNeedsStyleRecalcIfNeededWithStateChange(this, oldState,
                                                               newState);
        }

        window()->browsingContext()->setNeedsStyleRecalc();

        didStateChanged(oldState, newState);
    }
}

unsigned Node::nodeLength() const
{
    switch (nodeType()) {
    case DOCUMENT_TYPE_NODE:
        return 0;
    case TEXT_NODE:
    case CDATA_SECTION_NODE:
    case PROCESSING_INSTRUCTION_NODE:
    case COMMENT_NODE:
        return this->asCharacterData()->length();
    default:
        unsigned childCount = 0;
        Node* child = firstChild();
        while (child) {
            childCount++;
            child = child->nextSibling();
        }
        return childCount;
    }
    STARFISH_ASSERT_NOT_REACHED();
    return 0;
}

bool Node::isSpecificTypeNodeFollowing(NodeType type) const
{
    Node* next = Traverse::next(this, nullptr);
    while (next) {
        if (next->nodeType() == type) {
            return true;
        }
        next = Traverse::next(next, nullptr);
    }
    return false;
}

bool Node::isSpecificTypeNodePreceding(NodeType type) const
{
    Node* prev = Traverse::previous(this, nullptr);
    while (prev) {
        if (prev->nodeType() == type) {
            return true;
        }
        prev = Traverse::previous(prev, nullptr);
    }
    return false;
}

unsigned short isPreceding(const Node* node, const Node* isPrec,
                           const Node* refNode)
{
    if (node == isPrec) {
        return Node::DOCUMENT_POSITION_PRECEDING;
    } else if (node == refNode) {
        return Node::DOCUMENT_POSITION_FOLLOWING;
    }

    for (Node* child = node->firstChild(); child != nullptr;
         child = child->nextSibling()) {
        unsigned short result = isPreceding(child, isPrec, refNode);
        if (result != 0) {
            return result;
        }
    }
    return 0;
}

unsigned short Node::compareDocumentPosition(Node* other)
{
    // spec does not say what to do when other is nullptr
    if (!other) {
        return DOCUMENT_POSITION_DISCONNECTED;
    }

    if (this == other) {
        return 0;
    }

    Attr* attr1 = nodeType() == ATTRIBUTE_NODE ? asAttr() : nullptr;
    Attr* attr2 =
        other->nodeType() == ATTRIBUTE_NODE ? other->asAttr() : nullptr;

    Node* node1 = attr1 ? attr1->ownerElement() : this;
    Node* node2 = attr2 ? attr2->ownerElement() : other;
    const Node* root = isDocument() ? this : getRootNode();

    if (!node1 || !node2) {
        return DOCUMENT_POSITION_DISCONNECTED +
               DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC +
               (this > other ? DOCUMENT_POSITION_PRECEDING
                             : DOCUMENT_POSITION_FOLLOWING);
    }

    if (attr1 && attr2 && node1 == node2 && node1) {
        auto& v = attr1->ownerElement()->attributesVector();
        for (size_t i = 0; i < v.size(); i++) {
            const QualifiedName& attrName = v[i].name();
            if (attr1 && attrName == attr1->qname()) {
                return DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC +
                       DOCUMENT_POSITION_FOLLOWING;
            }

            if (attr2 && attrName == attr2->qname()) {
                return DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC +
                       DOCUMENT_POSITION_PRECEDING;
            }
        }
    }

    // spec does not say what to do for node's connection
    // follow other browsers
    if (node1->isConnected() != node2->isConnected() ||
        node1->getRootNode() != node2->getRootNode()) {
        unsigned short result = isPreceding(root, other, this);
        if (result == 0) {
            result = DOCUMENT_POSITION_FOLLOWING;
        }

        return DOCUMENT_POSITION_DISCONNECTED +
               DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC + result;
    }

    if ((attr1 == nullptr && node1->contains(node2)) ||
        (attr2 && node1 == node2)) {
        return DOCUMENT_POSITION_CONTAINED_BY + DOCUMENT_POSITION_FOLLOWING;
    }

    if ((attr2 == nullptr && node2->contains(node1)) ||
        (attr1 && node1 == node2)) {
        return DOCUMENT_POSITION_CONTAINS + DOCUMENT_POSITION_PRECEDING;
    }

    unsigned short result = isPreceding(root, node2, node1);
    if (result == 0) {
        result = DOCUMENT_POSITION_FOLLOWING;
    }

    return result;
}

// https://dom.spec.whatwg.org/#locate-a-namespace
static Nullable<String*> locateNamespace(Node* node, Nullable<String*> prefix)
{
    switch (node->nodeType()) {
    case Node::NodeType::ELEMENT_NODE: {
        Element* element = node->asElement();
        // If its namespace is not null and its namespace prefix is prefix,
        // then return namespace.
        const QualifiedName& name = element->name();
        if (name.namespaceURI().hasValue() && name.hasSamePrefix(prefix)) {
            return name.namespaceURI().getValue().string();
        }

        // If it has an attribute whose namespace is the XMLNS namespace,
        // namespace prefix is "xmlns", and local name is prefix, or if
        // prefix is null and it has an attribute whose namespace is the
        // XMLNS namespace, namespace prefix is null, and local name is
        // "xmlns", then return its value if it is not the empty string,
        // and null otherwise.
        auto& v = element->attributesVector();
        for (size_t i = 0; i < v.size(); i++) {
            const QualifiedName& attrName = v[i].name();
            if (attrName.hasSameNamespaceURI(XMLNS_NAMESPACE) &&
                ((attrName.hasSamePrefix("xmlns") &&
                  attrName.hasSameLocalName(prefix)) ||
                 (!prefix.hasValue() && !attrName.hasPrefix() &&
                  attrName.hasSameLocalName("xmlns")))) {
                return v[i].value();
            }
        }

        // If its parent element is null, then return null.
        // Return the result of running locate a namespace on its parent
        // element using prefix.
        return node->parentElement()
                   ? locateNamespace(node->parentElement(), prefix)
                   : Nullable<String*>();
    }
    case Node::NodeType::DOCUMENT_NODE:
        // If its document element is null, then return null.
        // Return the result of running locate a namespace on its document
        // element using prefix.
        return node->asDocument()->documentElement()
                   ? locateNamespace(node->asDocument()->documentElement(),
                                     prefix)
                   : Nullable<String*>();
    case Node::NodeType::DOCUMENT_TYPE_NODE:
    case Node::NodeType::DOCUMENT_FRAGMENT_NODE:
        // Return null.
        return Nullable<String*>();
    case Node::NodeType::ATTRIBUTE_NODE:
        // If its element is null, then return null.
        // Return the result of running locate a namespace on its element
        // using prefix.
        return node->asAttr()->ownerElement()
                   ? locateNamespace(node->asAttr()->ownerElement(), prefix)
                   : Nullable<String*>();
    default:
        // If its parent element is null, then return null.
        // Return the result of running locate a namespace on its parent
        // element using prefix.
        return node->parentElement()
                   ? locateNamespace(node->parentElement(), prefix)
                   : Nullable<String*>();
    }
}

// https://dom.spec.whatwg.org/#locate-a-namespace-prefix
static Nullable<String*> locateNamespacePrefix(Element* element,
                                               Nullable<String*> namespaceUri)
{
    // If element’s namespace is namespace and its namespace prefix is not null,
    // then return its namespace prefix.
    QualifiedName name = element->name();
    if (name.hasSameNamespaceURI(namespaceUri) && name.prefix().hasValue()) {
        return name.prefixString();
    }

    // If element has an attribute whose namespace prefix is "xmlns" and value
    // is namespace, then return element’s first such attribute’s local name.
    auto& v = element->attributesVector();
    for (size_t i = 0; i < v.size(); i++) {
        if (v[i].name().prefix().hasValue()) {
            if (v[i].name().prefix().getValue().string()->equals("xmlns")) {
                if (v[i].name().namespaceURI().hasValue()) {
                    if (v[i].name().hasSameNamespaceURI(namespaceUri)) {
                        return v[i].name().localName();
                    }
                }
            }
        }
    }

    // If element’s parent element is not null, then return the result of
    // running locate a namespace prefix on that element using namespace.
    if (element->parentElement() != nullptr) {
        return locateNamespacePrefix(element->parentElement(), namespaceUri);
    }

    // Return null.
    return Nullable<String*>();
}

// https://dom.spec.whatwg.org/#dom-node-lookupprefix
Nullable<String*> Node::lookupPrefix(Nullable<String*> namespaceUri)
{
    // If namespace is null or the empty string, then return null.
    if (!namespaceUri.hasValue() || namespaceUri.getValue()->equals("")) {
        return Nullable<String*>();
    }

    switch (nodeType()) {
    case ELEMENT_NODE:
        // Return the result of locating a namespace prefix for it using
        // namespace.
        return locateNamespacePrefix(asElement(), namespaceUri);
    case DOCUMENT_NODE: {
        // Return the result of locating a namespace prefix for its document
        // element, if its document element is non-null, and null otherwise.
        Element* documentElement = asDocument()->documentElement();
        if (documentElement) {
            return locateNamespacePrefix(documentElement, namespaceUri);
        } else {
            return Nullable<String*>();
        }
    }
    case DOCUMENT_TYPE_NODE:
    case DOCUMENT_FRAGMENT_NODE:
        return Nullable<String*>();
    case ATTRIBUTE_NODE:
        // Return the result of locating a namespace prefix for its element, if
        // its element is non-null, and null otherwise.
        if (asAttr()->ownerElement()) {
            return locateNamespacePrefix(asAttr()->ownerElement(),
                                         namespaceUri);
        }
        return Nullable<String*>();
    default: {
        // Return the result of locating a namespace prefix for its parent
        // element, if its parent element is non-null, and null otherwise.
        Element* parent = parentElement();
        if (parent) {
            return locateNamespacePrefix(parent, namespaceUri);
        } else {
            return Nullable<String*>();
        }
    }
    }
    return String::emptyString;
}

// https://dom.spec.whatwg.org/#dom-node-lookupnamespaceuri
Nullable<String*> Node::lookupNamespaceURI(Nullable<String*> prefix)
{
    // If prefix is the empty string, then set it to null.
    if (prefix.hasValue() && prefix.getValue()->equals(String::emptyString)) {
        prefix = Nullable<String*>();
    }
    // Return the result of running locate a namespace for the context object
    // using prefix.
    return locateNamespace(this, prefix);
}

// https://dom.spec.whatwg.org/#dom-node-isdefaultnamespace
bool Node::isDefaultNamespace(Nullable<String*> namespaceUri)
{
    // If namespace is the empty string, then set it to null.
    if (namespaceUri.hasValue() &&
        namespaceUri.getValue()->equals(String::emptyString)) {
        namespaceUri = Nullable<String*>();
    }

    // Let defaultNamespace be the result of running locate a namespace for
    // context object using null.
    Nullable<String*> defaultNamespace = locateNamespace(this, nullptr);
    if (defaultNamespace.hasValue() != namespaceUri.hasValue()) {
        return false;
    }

    // Return true if defaultNamespace is the same as namespace, and false
    // otherwise.
    return defaultNamespace.hasValue()
               ? defaultNamespace.getValue()->equals(namespaceUri.getValue())
               : true;
}

HTMLCollection* Node::children()
{
    if (!hasRareMembers()) {
        ensureRareMembers();
    } else if (m_rareNodeMembers->m_children) {
        return m_rareNodeMembers->m_children;
    }

    m_rareNodeMembers->m_children =
        new HTMLCollection(this, NodeListImpl::ChildElementFilter, this, true);
    return m_rareNodeMembers->m_children;
}

DOMTokenList* Node::classList()
{
    if (isElement()) {
        if (!hasRareMembers()) {
            ensureRareMembers();
        } else if (m_rareNodeMembers->m_domTokenList) {
            return m_rareNodeMembers->m_domTokenList;
        }

        m_rareNodeMembers->m_domTokenList =
            new DOMTokenList(asElement(), starfish()->staticStrings()->m_class);
        return m_rareNodeMembers->m_domTokenList;
    }
    return nullptr;
}

NamedNodeMap* Node::attributes()
{
    return nullptr;
}

Node* Node::getDoctypeChild()
{
    for (Node* c = firstChild(); c != nullptr; c = c->nextSibling()) {
        if (c->isDocumentType()) {
            return c;
        }
    }
    return nullptr;
}

void Node::validatePreinsert(Node* node, Node* child) // (node, child)
{
    // 4.2.1 pre-insertion validity
    if (!(isDocument() || isElement() || isDocumentFragment())) {
        throw new DOMException(
            executionContext(), DOMException::HIERARCHY_REQUEST_ERR,
            "Parent is not a Document, DocumentFragment, or Element node.");
    }

    for (Node* p = this; p != nullptr; p = p->parentNode()) {
        if (p == node) {
            throw new DOMException(
                executionContext(), DOMException::HIERARCHY_REQUEST_ERR,
                "Node is a host-including inclusive ancestor of parent.");
        }
    }

    if (child != nullptr && child->parentNode() != this) {
        throw new DOMException(
            executionContext(), DOMException::Code::NOT_FOUND_ERR,
            "Child is not null and its parent is not parent.");
    }
    if (!(node->isDocumentType() || node->isElement() || node->isText() ||
          node->isProcessingInstruction() || node->isComment() ||
          node->isDocumentFragment())) {
        throw new DOMException(executionContext(),
                               DOMException::HIERARCHY_REQUEST_ERR,
                               "Node is not a DocumentFragment, DocumentType, "
                               "Element, Text, ProcessingInstruction, or "
                               "Comment.");
    }
    if ((node->isText() && isDocument()) ||
        (node->isDocumentType() && !isDocument())) {
        throw new DOMException(executionContext(),
                               DOMException::HIERARCHY_REQUEST_ERR,
                               "Either node is a Text node and parent is a "
                               "document, or node is a doctype and parent is "
                               "not a document.");
    }
    if (isDocument()) {
        if (node->isDocumentFragment()) {
            Node* nodeChild = node->firstChild();
            unsigned childElementCount = 0;
            while (nodeChild) {
                if (nodeChild->isElement()) {
                    childElementCount++;
                } else if (nodeChild->isText()) {
                    throw new DOMException(
                        executionContext(), DOMException::HIERARCHY_REQUEST_ERR,
                        "If node has more than one element child or has a Text "
                        "node child. Otherwise, if node has one element child "
                        "and "
                        "either parent has an element child, child is a "
                        "doctype, "
                        "or child is not null and a doctype is following "
                        "child.");
                }
                nodeChild = nodeChild->nextSibling();
            }
            if (childElementCount > 1) {
                throw new DOMException(
                    executionContext(), DOMException::HIERARCHY_REQUEST_ERR,
                    "If node has more than one element child or has a Text "
                    "node child. Otherwise, if node has one element child and "
                    "either parent has an element child, child is a doctype, "
                    "or child is not null and a doctype is following child.");
            }
            if (childElementCount == 1) {
                Node* c = firstChild();
                while (c) {
                    if (c->isElement()) {
                        throw new DOMException(
                            executionContext(),
                            DOMException::HIERARCHY_REQUEST_ERR,
                            "If node has more than one element child or has a "
                            "Text "
                            "node child. Otherwise, if node has one element "
                            "child and "
                            "either parent has an element child, child is a "
                            "doctype, "
                            "or child is not null and a doctype is following "
                            "child.");
                    }
                    c = c->nextSibling();
                }
                if (child && (child->isDocumentType() ||
                              child->isSpecificTypeNodeFollowing(
                                  NodeType::DOCUMENT_TYPE_NODE))) {
                    throw new DOMException(
                        executionContext(), DOMException::HIERARCHY_REQUEST_ERR,
                        "If node has more than one element child or has a Text "
                        "node child. Otherwise, if node has one element child "
                        "and "
                        "either parent has an element child, child is a "
                        "doctype, "
                        "or child is not null and a doctype is following "
                        "child.");
                }
            }
        } else if (node->isElement()) {
            Node* c = firstChild();
            while (c) {
                if (c->isElement()) {
                    throw new DOMException(
                        executionContext(), DOMException::HIERARCHY_REQUEST_ERR,
                        "parent has an element child, child is "
                        "a doctype, or child is not null and a "
                        "doctype is following child.");
                }
                c = c->nextSibling();
            }
            if (child && (child->isDocumentType() ||
                          child->isSpecificTypeNodeFollowing(
                              NodeType::DOCUMENT_TYPE_NODE))) {
                throw new DOMException(executionContext(),
                                       DOMException::HIERARCHY_REQUEST_ERR,
                                       "parent has an element child, child is "
                                       "a doctype, or child is not null and a "
                                       "doctype is following child.");
            }
        } else if (node->isDocumentType()) {
            Node* c = firstChild();
            while (c) {
                if (c->isDocumentType() || (!child && c->isElement())) {
                    throw new DOMException(
                        executionContext(), DOMException::HIERARCHY_REQUEST_ERR,
                        "parent has a doctype child, child is "
                        "non-null and an element is preceding "
                        "child, or child is null and parent has "
                        "an element child.");
                }
                c = c->nextSibling();
            }
            if (child &&
                child->isSpecificTypeNodePreceding(NodeType::ELEMENT_NODE)) {
                throw new DOMException(executionContext(),
                                       DOMException::HIERARCHY_REQUEST_ERR,
                                       "parent has a doctype child, child is "
                                       "non-null and an element is preceding "
                                       "child, or child is null and parent has "
                                       "an element child.");
            }
        }
    }
}

bool Node::isInDocumentScope()
{
    Node* t = this;
    while (t) {
        if (t->isDocument()) {
            return true;
        }
        t = t->parentNode();
    }
    return false;
}

bool Node::isInDocumentScopeAndDocumentParticipateInRendering()
{
    Node* t = this;
    while (t) {
        if (t->isDocument()) {
            return t->asDocument()->doesParticipateInRendering();
        }
        t = t->parentNode();
    }
    return false;
}

static void notifyNodeInsertedToDocumentTree(Node* head, Node* node)
{
    // adopt node
    if (node->document() != head->document()) {
        node->setDocument(head->document());
        node->didNodeAdopted();
    }

    if (head->isConnected()) {
        node->setConnected();
    }

    node->didNodeInsertedToDocumentTree();
    Node* child = node->firstChild();
    while (child) {
        notifyNodeInsertedToDocumentTree(head, child);
        child = child->nextSibling();
    }
}

static void setChildrenNeedsStyleRecalc(Node* node)
{
    node->setNeedsStyleRecalc(Node::JustNeedsRecalcSelf);

    Node* child = node->firstChild();
    while (child) {
        setChildrenNeedsStyleRecalc(child);
        child = child->nextSibling();
    }
}

static void didInsertNode(Node* self, Node* child)
{
    child->setParentNode(self);

    Node* parent = self;
    while (parent) {
        parent->didNodeInserted(self, child);
        parent = parent->parentNode();
    }
    if (self->isInDocumentScope()) {
        notifyNodeInsertedToDocumentTree(self, child);
        if (self->document()->doesParticipateInRendering()) {
            self->setNeedsStyleRecalc(Node::StyleChangeReason::DOMTreeChange);
            setChildrenNeedsStyleRecalc(child);
        }
    }
}

Node* Node::appendChild(Node* child)
{
    if (!isContainerNode()) {
        throw new DOMException(executionContext(),
                               DOMException::HIERARCHY_REQUEST_ERR,
                               "This node type does not support this method.");
    }

    // spec does not say what to do when child is null
    STARFISH_ASSERT(child);

    validatePreinsert(child, nullptr);

    if (child->isDocumentFragment()) {
        while (Node* nd = child->firstChild()) {
            child->removeChild(nd);
            appendChild(nd);
        }
        return child;
    }

    if (child->parentNode()) {
        Node* p = child->parentNode();
        child = p->removeChild(child);
    }

    STARFISH_ASSERT(child->parentNode() == nullptr);
    STARFISH_ASSERT(child->nextSibling() == nullptr);
    STARFISH_ASSERT(child->previousSibling() == nullptr);

    if (m_lastChild) {
        child->setPreviousSibling(m_lastChild);
        m_lastChild->setNextSibling(child);
    } else {
        m_firstChild = child;
    }
    m_lastChild = child;

    didInsertNode(this, child);

    return child;
}

Node* Node::insertBefore(Node* child, Node* childRef)
{
    // Spec does not say what to do when node is null
    if (child == nullptr) {
        throw new DOMException(executionContext(),
                               DOMException::HIERARCHY_REQUEST_ERR,
                               "Node is null.");
    }

    validatePreinsert(child, childRef);

    if (childRef == nullptr) {
        return appendChild(child);
    }
    if (child == childRef) {
        return child;
    }

    if (child->isDocumentFragment()) {
        while (Node* nd = child->firstChild()) {
            child->removeChild(nd);
            insertBefore(nd, childRef);
        }
        return child;
    }

    if (child->parentNode()) {
        child->parentNode()->removeChild(child);
    }

    STARFISH_ASSERT(child->parentNode() == nullptr);
    STARFISH_ASSERT(child->nextSibling() == nullptr);
    STARFISH_ASSERT(child->previousSibling() == nullptr);

    Node* prev = childRef->previousSibling();
    childRef->setPreviousSibling(child);
    STARFISH_ASSERT(m_lastChild != prev);
    if (prev) {
        STARFISH_ASSERT(m_firstChild != childRef);
        prev->setNextSibling(child);
    } else {
        STARFISH_ASSERT(m_firstChild == childRef);
        m_firstChild = child;
    }

    child->setPreviousSibling(prev);
    child->setNextSibling(childRef);

    didInsertNode(this, child);

    return child;
}

void Node::validateReplace(Node* node, Node* child) // node, child
{
    // 4.2.1 replace validity
    if (!(isDocument() || isDocumentFragment() || isElement())) {
        throw new DOMException(
            executionContext(), DOMException::HIERARCHY_REQUEST_ERR,
            "Parent is not a Document, DocumentFragment, or Element node.");
    }

    for (Node* p = this; p != nullptr; p = p->parentNode()) {
        if (p == node) {
            throw new DOMException(
                executionContext(), DOMException::HIERARCHY_REQUEST_ERR,
                "Node is a host-including inclusive ancestor of parent.");
        }
    }

    if (child != nullptr && child->parentNode() != this) {
        throw new DOMException(
            executionContext(), DOMException::Code::NOT_FOUND_ERR,
            "Child is not null and its parent is not parent.");
    }
    if (!(node->isDocumentType() || node->isDocumentFragment() ||
          node->isElement() || node->isText() ||
          node->isProcessingInstruction() || node->isComment())) {
        throw new DOMException(executionContext(),
                               DOMException::HIERARCHY_REQUEST_ERR,
                               "Node is not a DocumentFragment, DocumentType, "
                               "Element, Text, ProcessingInstruction, or "
                               "Comment.");
    }
    if ((node->isText() && isDocument()) ||
        (node->isDocumentType() && !isDocument())) {
        throw new DOMException(executionContext(),
                               DOMException::HIERARCHY_REQUEST_ERR,
                               "Either node is a Text node and parent is a "
                               "document, or node is a doctype and parent is "
                               "not a document.");
    }
    if (isDocument()) {
        if (node->isDocumentFragment()) {
            Node* nodeChild = node->firstChild();
            unsigned childElementCount = 0;
            while (nodeChild) {
                if (nodeChild->isElement()) {
                    childElementCount++;
                } else if (nodeChild->isText()) {
                    throw new DOMException(executionContext(),
                                           DOMException::HIERARCHY_REQUEST_ERR,
                                           "node has a Text node child.");
                }
                nodeChild = nodeChild->nextSibling();
            }
            if (childElementCount > 1) {
                throw new DOMException(executionContext(),
                                       DOMException::HIERARCHY_REQUEST_ERR,
                                       "node has more than one element child.");
            }
            if (childElementCount == 1) {
                Node* c = firstChild();
                while (c) {
                    if (c->isElement() && c != child) {
                        throw new DOMException(
                            executionContext(),
                            DOMException::HIERARCHY_REQUEST_ERR,
                            "node has one element child and parent has"
                            "an element child that is not child.");
                    }
                    c = c->nextSibling();
                }

                if (child && child->isSpecificTypeNodeFollowing(
                                 NodeType::DOCUMENT_TYPE_NODE)) {
                    throw new DOMException(
                        executionContext(), DOMException::HIERARCHY_REQUEST_ERR,
                        "node has one element child and doctype"
                        "is following child.");
                }
            }
        } else if (node->isElement()) {
            Node* c = firstChild();
            while (c) {
                if (c->isElement() && c != child) {
                    throw new DOMException(
                        executionContext(), DOMException::HIERARCHY_REQUEST_ERR,
                        "parent has an element child that is "
                        "not child or a doctype is following "
                        "child.");
                }
                c = c->nextSibling();
            }
            if (child && child->isSpecificTypeNodeFollowing(
                             NodeType::DOCUMENT_TYPE_NODE)) {
                throw new DOMException(executionContext(),
                                       DOMException::HIERARCHY_REQUEST_ERR,
                                       "doctype is following child.");
            }
        } else if (node->isDocumentType()) {
            Node* c = firstChild();
            while (c) {
                if (c->isDocumentType() && c != child) {
                    throw new DOMException(
                        executionContext(), DOMException::HIERARCHY_REQUEST_ERR,
                        "parent has an element child that is "
                        "not child or a doctype is following "
                        "child.");
                }
                c = c->nextSibling();
            }
            if (child &&
                child->isSpecificTypeNodePreceding(NodeType::ELEMENT_NODE)) {
                throw new DOMException(executionContext(),
                                       DOMException::HIERARCHY_REQUEST_ERR,
                                       "doctype is following child.");
            }
        }
    }
}

Node* Node::replaceChild(Node* child, Node* childToRemove)
{
    STARFISH_ASSERT(child);

    validateReplace(child, childToRemove);

    STARFISH_ASSERT(childToRemove);
    STARFISH_ASSERT(childToRemove->parentNode() == this);

    if (child == childToRemove) {
        return childToRemove;
    }
    if (child->parentNode()) {
        child->parentNode()->removeChild(child);
    }
    insertBefore(child, childToRemove);
    Node* removed = removeChild(childToRemove);
    return removed;
}

void notifyNodeRemoveFromDocumentTree(Node* node)
{
    node->clearConnected();
    node->didNodeRemovedFromDocumentTree();
    Node* child = node->firstChild();
    while (child) {
        notifyNodeRemoveFromDocumentTree(child);
        child = child->nextSibling();
    }
}

Node* Node::removeChild(Node* child)
{
    STARFISH_ASSERT(child);

    if (child->parentNode() != this) {
        throw new DOMException(executionContext(), DOMException::NOT_FOUND_ERR,
                               "Child's parent is not parent.");
    }

    Frame* old = child->frame();
    if (old) {
        child->setNeedsFrameTreeBuild();
    }

    // TODO: Should be removed later!
    if (child->isSVGElement()) {
        child->setNeedsFrameTreeBuild();
    }

    if (document()) {
        document()->willNodeBeRemoved(this, child);
    }

    Node* prevChild = child->previousSibling();
    Node* nextChild = child->nextSibling();

    if (nextChild) {
        nextChild->setPreviousSibling(prevChild);
    }
    if (prevChild) {
        prevChild->setNextSibling(nextChild);
    }
    if (m_firstChild == child) {
        m_firstChild = nextChild;
    }
    if (m_lastChild == child) {
        m_lastChild = prevChild;
    }

    child->setPreviousSibling(nullptr);
    child->setNextSibling(nullptr);
    child->setParentNode(nullptr);

    setNeedsStyleRecalc(Node::StyleChangeReason::DOMTreeChange);
    if (isInDocumentScope() && document()->doesParticipateInRendering()) {
        notifyNodeRemoveFromDocumentTree(child);
    }

    Node* parent = this;
    while (parent) {
        parent->didNodeRemoved(this, child);
        parent = parent->parentNode();
    }

    return child;
}

Node* Node::parserAppendChild(Node* child)
{
    STARFISH_ASSERT(child);
    STARFISH_ASSERT(child->parentNode() == nullptr);
    STARFISH_ASSERT(child->nextSibling() == nullptr);
    STARFISH_ASSERT(child->previousSibling() == nullptr);

    if (m_lastChild) {
        child->setPreviousSibling(m_lastChild);
        m_lastChild->setNextSibling(child);
    } else {
        m_firstChild = child;
    }
    m_lastChild = child;

    child->setParentNode(this);

    if (isInDocumentScope()) {
        notifyNodeInsertedToDocumentTree(this, child);
    }

    Node* parent = this;
    while (parent) {
        parent->didNodeInserted(this, child);
        parent = parent->parentNode();
    }

    setChildrenNeedsStyleRecalc(child);

    return child;
}

void Node::parserRemoveChild(Node* child)
{
    Node* prevChild = child->previousSibling();
    Node* nextChild = child->nextSibling();

    if (nextChild) {
        nextChild->setPreviousSibling(prevChild);
    }
    if (prevChild) {
        prevChild->setNextSibling(nextChild);
    }
    if (m_firstChild == child) {
        m_firstChild = nextChild;
    }
    if (m_lastChild == child) {
        m_lastChild = prevChild;
    }

    child->setPreviousSibling(nullptr);
    child->setNextSibling(nullptr);
    child->setParentNode(nullptr);

    if (isInDocumentScope()) {
        notifyNodeRemoveFromDocumentTree(child);
    }

    Node* parent = this;
    while (parent) {
        parent->didNodeRemoved(this, child);
        parent = parent->parentNode();
    }
}

void Node::parserInsertBefore(Node* child, Node* childRef)
{
    STARFISH_ASSERT(child);

    if (childRef == nullptr) {
        appendChild(child);
        return;
    }

    STARFISH_ASSERT(childRef->parentNode() == this);
    if (childRef->previousSibling() == child || childRef == child) {
        // nothing to do
        return;
    }

    if (child == childRef) {
        return;
    }
    if (child->parentNode()) {
        child->parentNode()->removeChild(child);
    }

    STARFISH_ASSERT(child->parentNode() == nullptr);
    STARFISH_ASSERT(child->nextSibling() == nullptr);
    STARFISH_ASSERT(child->previousSibling() == nullptr);

    Node* prev = childRef->previousSibling();
    childRef->setPreviousSibling(child);
    STARFISH_ASSERT(m_lastChild != prev);
    if (prev) {
        STARFISH_ASSERT(m_firstChild != childRef);
        prev->setNextSibling(child);
    } else {
        STARFISH_ASSERT(m_firstChild == childRef);
        m_firstChild = child;
    }

    child->setParentNode(this);
    child->setPreviousSibling(prev);
    child->setNextSibling(childRef);

    Node* parent = this;
    while (parent) {
        parent->didNodeInserted(this, child);
        parent = parent->parentNode();
    }

    if (isInDocumentScope()) {
        notifyNodeInsertedToDocumentTree(this, child);
    }

    setChildrenNeedsStyleRecalc(child);
}

void Node::parserTakeAllChildrenFrom(Node* oldParent)
{
    while (Node* child = oldParent->firstChild()) {
        oldParent->parserRemoveChild(child);
        parserAppendChild(child);
    }
}

struct HTMLTagCollectionData : public gc {
    QualifiedName* name;
    QualifiedName* lowerName;
};

HTMLCollection* Node::getElementsByTagName(String* name)
{
    QualifiedName qname(
        AtomicString::emptyAtomicString(),
        AtomicString::createAtomicString(window()->starfish(), name));
    return getElementsByTagName(qname);
}

HTMLCollection* Node::getElementsByTagName(QualifiedName qualifiedName)
{
    RareNodeMembers* rareData = ensureRareMembers();
    ActiveHTMLCollectionList* activeLists =
        rareData->ensureActiveHtmlCollectionListForTagName();
    HTMLCollection* list = rareData->hasQueryInActiveHtmlCollectionList(
        activeLists, qualifiedName.localName());
    if (list) {
        return list;
    }
    if (document()->isXMLDocument()) {
        list = new HTMLCollection(this, NodeListImpl::XMLTagNameFilter,
                                  new QualifiedName(qualifiedName), true);
    } else {
        HTMLTagCollectionData* data = new HTMLTagCollectionData;
        data->name = new QualifiedName(qualifiedName);
        data->lowerName = new QualifiedName(
            AtomicString::emptyAtomicString(),
            AtomicString::createAttrAtomicString(window()->starfish(),
                                                 qualifiedName.localName()));
        list = new HTMLCollection(this, NodeListImpl::HTMLTagNameFilter, data,
                                  true);
    }
    rareData->putActiveHtmlCollectionListWithQuery(
        activeLists, qualifiedName.localName(), list);
    return list;
}

HTMLCollection* Node::getElementsByTagNameNS(Nullable<String*> ns, String* name)
{
    if (ns.hasValue() && ns.getValue()->equals(String::emptyString)) {
        ns = Nullable<String*>();
    }

    QualifiedName qName(document()->createAttributeNameNS(ns, name));
    RareNodeMembers* rareData = ensureRareMembers();
    ActiveStringPairHTMLCollectionList* activeLists =
        rareData->ensureActiveHtmlCollectionListForTagNameNS();
    HTMLCollection* list = rareData->hasQueryInActiveHtmlCollectionList(
        activeLists,
        std::make_pair(qName.hasNamespaceURI()
                           ? qName.namespaceURI().getValue().string()
                           : nullptr,
                       qName.localName()));
    if (list) {
        return list;
    }

    if (isElement()) {
        list = new HTMLCollection(this, NodeListImpl::TagNameNSFilter,
                                  new QualifiedName(qName), true, false);
    } else {
        STARFISH_ASSERT(isDocument());
        list = new HTMLCollection(this, NodeListImpl::TagNameNSFilter,
                                  new QualifiedName(qName), true, true);
    }

    rareData->putActiveHtmlCollectionListWithQuery(
        activeLists,
        std::make_pair(qName.hasNamespaceURI()
                           ? qName.namespaceURI().getValue().string()
                           : nullptr,
                       qName.localName()),
        list);
    return list;
}

HTMLCollection* Node::getElementsByClassName(String* classNames)
{
    RareNodeMembers* rareData = ensureRareMembers();
    ActiveHTMLCollectionList* activeLists =
        rareData->ensureActiveHtmlCollectionListForClassName();
    HTMLCollection* list =
        rareData->hasQueryInActiveHtmlCollectionList(activeLists, classNames);
    if (list) {
        return list;
    }

    list = new HTMLCollection(this, NodeListImpl::ClassNamesFilter, classNames,
                              true);
    rareData->putActiveHtmlCollectionListWithQuery(activeLists, classNames,
                                                   list);
    return list;
}

void Node::parseSelector(GCVector<CSSSelectorList*>& selectorListContainer,
                         String* selectors)
{
    if (selectors->equals(String::emptyString)) {
        throw new DOMException(executionContext(), DOMException::SYNTAX_ERR,
                               "Failed to execute 'querySelector' on "
                               "'Document': The provided selector is empty.");
    }

    CSSParser parser(document());
    RefPtr<CSSToken> token = parser.makeToken(selectors);

    GCVector<StyleRuleBase*> nullVec;
    parser.parseStyleRule(token, nullVec,
                          CSSParser::AllowedRulesType::RegularRules,
                          &selectorListContainer, true);

    if (selectorListContainer.size() < 1) {
        throw new DOMException(executionContext(), DOMException::SYNTAX_ERR,
                               "Failed to execute 'querySelector' on "
                               "'Document': The provided selector is invalid.");
    }
}

Element* Node::querySelector(String* selectors)
{
    GCVector<CSSSelectorList*> selectorListContainer;
    parseSelector(selectorListContainer, selectors);

    SelectorQuery selectorQuery(selectorListContainer);
    return selectorQuery.queryFirst(*this);
}

NodeList* Node::querySelectorAll(String* selectors)
{
    GCVector<CSSSelectorList*> selectorListContainer;
    parseSelector(selectorListContainer, selectors);

    SelectorQuery selectorQuery(selectorListContainer);
    return selectorQuery.queryAll(*this);
}

void Node::propagateMarkChildNeedsFrameTreeBuild()
{
    Node* n = this;
    while (n) {
        n->markChildNeedsFrameTreeBuild();
        n = n->parentNode();
    }
}

void Node::setNeedsFrameTreeBuild()
{
    if (!document()->doesParticipateInRendering()) {
        return;
    }

    window()->browsingContext()->setNeedsFrameTreeBuild();

    Frame* old = frame();
    if (old) {
        Frame* blockParent = FrameTreeBuilder::
            findNearestBlockStartPositionOfFrameTreeBuildCandidate(
                old->parent());
        if (!blockParent) {
            blockParent = document()->frame();
        }

        STARFISH_ASSERT(blockParent);
        FrameTreeBuilder::needsFrameTreeBuildFromChildrenOfThisFrame(
            blockParent);
    } else {
        if (isElement() && !needsFrameTreeBuild()) {
            if (parentElement() && parentElement()->frame()) {
                Frame* blockParent = FrameTreeBuilder::
                    findNearestBlockStartPositionOfFrameTreeBuildCandidate(
                        parentElement()->frame());
                if (blockParent) {
                    FrameTreeBuilder::
                        needsFrameTreeBuildFromChildrenOfThisFrame(blockParent);
                    return;
                }
            }

            markNeedsFrameTreeBuild();
            Node* node = parentNode();
            while (node) {
                if (node->childNeedsFrameTreeBuild()) {
                    break;
                }
                node->markChildNeedsFrameTreeBuild();
                node = node->parentNode();
            }
        }
    }
}

void Node::setNeedsFrameTreeBuildWithoutSelf()
{
    if (!document()->doesParticipateInRendering()) {
        return;
    }

    window()->browsingContext()->setNeedsFrameTreeBuild();

    if (needsFrameTreeBuild()) {
        return;
    }

    Frame* old = frame();
    if (old) {
        Frame* blockParent = FrameTreeBuilder::
            findNearestBlockStartPositionOfFrameTreeBuildCandidate(old);
        if (!blockParent) {
            blockParent = document()->frame();
        }

        STARFISH_ASSERT(blockParent);
        FrameTreeBuilder::needsFrameTreeBuildFromChildrenOfThisFrame(
            blockParent);
    } else {
        if (isElement()) {
            if (parentElement() && parentElement()->frame()) {
                Frame* blockParent = FrameTreeBuilder::
                    findNearestBlockStartPositionOfFrameTreeBuildCandidate(
                        parentElement()->frame());
                if (blockParent) {
                    FrameTreeBuilder::
                        needsFrameTreeBuildFromChildrenOfThisFrame(blockParent);
                    return;
                }
            }

            markNeedsFrameTreeBuild();
            Node* node = parentNode();
            while (node) {
                if (node->childNeedsFrameTreeBuild()) {
                    break;
                }
                node->markChildNeedsFrameTreeBuild();
                node = node->parentNode();
            }
        }
    }
}

void Node::setNeedsStyleRecalcForAnimation()
{
    if (!m_needsStyleRecalc) {
        m_needsStyleRecalc = true;
        m_needsStyleRecalcOnlyForAnimation = true;
    }

    if (parentNode()) {
        parentNode()->setChildNeedsStyleRecalc();
    }

    window()->browsingContext()->setNeedsStyleRecalc();
}

void Node::setNeedsStyleRecalc(StyleChangeReason reason)
{
    if (!isInDocumentScopeAndDocumentParticipateInRendering()) {
        return;
    }

    m_needsStyleRecalcOnlyForAnimation = false;

    if (reason <= StyleChangeReason::AttributeChange) {
        if (!m_needsStyleRecalc) {
            m_needsStyleRecalc = true;
        }

        if (parentNode()) {
            parentNode()->setChildNeedsStyleRecalc();
        }
    } else {
        StyleResolver::StyleDamageSource cmr;
        if (style() && style()->styleDamageSource()) {
            cmr = style()->styleDamageSource();
        } else {
            cmr = StyleResolver::StyleDamageSource::NoDamage;
        }

        if (cmr & reason) {
            m_needsStyleRecalc = true;
            if (parentNode()) {
                parentNode()->setChildNeedsStyleRecalc();
            }
        }
    }

    if (reason) {
        // siblings
        setSiblingsNeedsStyleRecalcIfNeeded(reason);

        // children
        setChildrenNeedsStyleRecalcIfNeeded(reason);
    }

    window()->browsingContext()->setNeedsStyleRecalc();
}

void Node::setSiblingsNeedsStyleRecalcIfNeeded(StyleChangeReason reason)
{
    Node* node = nextSibling();
    while (node) {
        if (node->isElement()) {
            StyleResolver::StyleDamageSource cmr;
            if (node->style() && node->style()->styleDamageSource()) {
                cmr = node->style()->styleDamageSource();
            } else {
                cmr = StyleResolver::StyleDamageSource::NoDamage;
            }

            if (cmr & reason) {
                node->m_needsStyleRecalc = true;
                if (node->parentNode()) {
                    node->parentNode()->setChildNeedsStyleRecalc();
                }
            }
        }

        node = node->nextSibling();
    }
}

void Node::setChildrenNeedsStyleRecalcIfNeeded(StyleChangeReason reason)
{
    Node* child = firstChild();
    while (child) {
        if (child->isElement()) {
            StyleResolver::StyleDamageSource cmr;
            if (child->style() && child->style()->styleDamageSource()) {
                cmr = child->style()->styleDamageSource();
            } else {
                cmr = StyleResolver::StyleDamageSource::NoDamage;
            }
            if (cmr & reason) {
                child->m_needsStyleRecalc = true;
                if (child->parentNode()) {
                    child->parentNode()->setChildNeedsStyleRecalc();
                }
            }
            child->setChildrenNeedsStyleRecalcIfNeeded(reason);
        }
        child = child->nextSibling();
    }
}

void Node::setNeedsLayout()
{
    if (!isInDocumentScopeAndDocumentParticipateInRendering()) {
        return;
    }

    window()->browsingContext()->setNeedsLayout();
    Frame* frame = this->frame();
    if (frame) {
        frame->propagateMarkNeedsLayout();
    }
}

void Node::setNeedsPainting()
{
    if (!isInDocumentScopeAndDocumentParticipateInRendering()) {
        return;
    }

    webView()->markNeedsPaintingConsiderInRendering();

    Frame* frame = this->frame();
    if (frame) {
        frame->markNeedsPainting();
    }
}

void Node::setNeedsComposite()
{
    if (!isInDocumentScopeAndDocumentParticipateInRendering()) {
        return;
    }

    window()->browsingContext()->setNeedsComposite();
}

void Node::didComputedStyleChanged(ComputedStyle* oldStyle,
                                   ComputedStyle* newStyle,
                                   Nullable<StyleResolveContext*> ctx)
{
    if (newStyle && frame()) {
        frame()->computeStyleFlags();
    }
    if (newStyle) {
        m_canBeCountingRoot =
            newStyle->counterIncrement() || newStyle->counterReset();
        m_canBeQuoteRoot = newStyle->hasQuote();
    } else {
        m_canBeCountingRoot = false;
        m_canBeQuoteRoot = false;
    }
}

void Node::didNodeInserted(Node* parent, Node* newChild)
{
    if (hasRareMembers()) {
        m_rareNodeMembers->invalidateActiveActiveNodeListCacheIfNeeded();
    }
    if (m_canBeCountingRoot) {
        document()->notifyCountingOutdated();
    }
    if (m_canBeQuoteRoot) {
        document()->notifyQuoteOutdated();
    }
}

void Node::didNodeRemoved(Node* parent, Node* oldChild)
{
    if (hasRareMembers()) {
        m_rareNodeMembers->invalidateActiveActiveNodeListCacheIfNeeded();
    }
    if (m_canBeCountingRoot) {
        document()->notifyCountingOutdated();
    }
    if (m_canBeQuoteRoot) {
        document()->notifyQuoteOutdated();
    }
}

static void clearStyle(Element* element)
{
    STARFISH_ASSERT(element != nullptr);

    Node* child = element->firstChild();
    while (child != nullptr) {
        if (child->isElement() == true) {
            child->clearNeedsStyleRecalc();
            if (child->style() != nullptr) {
                child->setStyle(nullptr);
                clearStyle(child->asElement());
            }
        } else {
            child->setStyle(nullptr);
        }
        child = child->nextSibling();
    }
}

void Node::didNodeRemovedFromDocumentTree()
{
    if (document()->activeElement() == this) {
        document()->browsingContext()->releaseFocusedNode(this);
    }
    clearDidPrepareAnimation();
    setState(NodeStateNormal, false);
    setStyle(nullptr);
    if (isElement()) {
        clearStyle(asElement());
    }
    FrameTreeBuilder::clearTree(this);
}

RareNodeMembers* Node::ensureRareMembers()
{
    STARFISH_ASSERT(!isElement());
    if (m_rareNodeMembers == nullptr) {
        m_rareNodeMembers = new RareNodeMembers();
    }
    STARFISH_ASSERT(!m_rareNodeMembers->isRareElementMembers());
    return m_rareNodeMembers;
}

void Node::invalidateNodeListCacheDueToChangeClassNameOfDescendant()
{
    if (hasRareMembers()) {
        if (m_rareNodeMembers->m_activeHtmlCollectionListsForClassName) {
            for (size_t i = 0;
                 i < m_rareNodeMembers->m_activeHtmlCollectionListsForClassName
                         ->size();
                 i++) {
                (*m_rareNodeMembers->m_activeHtmlCollectionListsForClassName)[i]
                    .second->getNodeListImpl()
                    .invalidateCache();
            }
        }
    }
}

ExecutionContext* Node::executionContext() const
{
    return document()->executionContext();
}
} // namespace Starfish
