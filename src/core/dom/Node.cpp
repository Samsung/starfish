/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#include "StarFish.h"
#include "core/dom/Attr.h"
#include "core/dom/CharacterData.h"
#include "core/dom/Document.h"
#include "core/dom/DocumentType.h"
#include "core/dom/DOMException.h"
#include "core/dom/DOMTokenList.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/NodeList.h"
#include "core/dom/SelectorQuery.h"
#include "core/dom/Text.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameTreeBuilder.h"
#include "core/page/Window.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"

namespace StarFish {

ActiveHTMLCollectionList*
RareNodeMembers::ensureActiveHtmlCollectionListForTagName()
{
    if (m_activeHtmlCollectionListsForTagName == nullptr) {
        m_activeHtmlCollectionListsForTagName =
            new (GC) ActiveHTMLCollectionList;
    }
    return m_activeHtmlCollectionListsForTagName;
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
        AtomicString::createAtomicString(ownerNode->starFish(), query));

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

Nullable<String*> Node::nodeValue() const
{
    switch (nodeType()) {
    case ATTRIBUTE_NODE:
        return asAttr()->value();
    case TEXT_NODE:
    case COMMENT_NODE:
        return asCharacterData()->data();
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
        return str;
    }
    case ATTRIBUTE_NODE:
        return asAttr()->value();
    case TEXT_NODE:
    case COMMENT_NODE:
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
        Text* node = new Text(document(), str);

        while (firstChild()) {
            removeChild(firstChild());
        }

        if (!str->equals(String::emptyString)) {
            appendChild(node);
        }
        break;
    }
    case ATTRIBUTE_NODE:
        asAttr()->setValue(str);
        break;
    case TEXT_NODE:
    case COMMENT_NODE:
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
        if (!(thisNode->hasSameAttributes(otherNode))) {
            return false;
        }
        break;
    }
    case PROCESSING_INSTRUCTION_NODE:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        break;
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

    if (childElementCount() != other->childElementCount()) {
        return false;
    }
    Node* otherChild = other->firstChild();
    for (Node* child = firstChild(); child; child = child->nextSibling()) {
        if (!child->isEqualNode(otherChild)) {
            return false;
        }
        if (otherChild)
            otherChild = otherChild->nextSibling();
    }

    return true;
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

Node* Node::nearestParentElement()
{
    Node* t = this;
    while (t && !t->isHTMLElement() && !t->isDocument()) {
        t = t->parentNode();
    }

    return t;
}

void Node::setState(NodeState state, DynamicRestyleFlags mask, bool enable)
{
    // Node needs to recalculate its style when it is updated by user action
    // such as focus, hover and active.
    // Specially, when dynamic pseudo class selectors like :focus, :hover or
    // :active are combined with combinator selectors like descendant, child
    // or sibling, the node's child and sibling should be also recalculated
    // to update style. ex) div:hover > p { ... }
    // Finally, if dynamic pseudo class selectors are compounded to pseudo
    // element selectors, the pseudo element should be created through
    // building frame tree only when the node is updated by user action.
    // ex) div:hover:first-letter { ... }

    int oldState = m_state;

    if (state == NodeStateNormal) {
        m_state = 0;
        m_restyleFlags = 0;

        if (childrenOrSiblingsAffectedByDynamicEvent(mask)) {
            setNeedsStyleRecalcIfNeeded();
            if (isElement() && asElement()->hasPseudoElements()) {
                setNeedsFrameTreeBuild();
            }
        } else {
            setNeedsStyleRecalc();
        }

    } else if (!(m_state & state) == enable) {
        m_state ^= state;

        if (childrenOrSiblingsAffectedByDynamicEvent(mask)) {
            setNeedsStyleRecalcIfNeeded();
            if (isElement() && asElement()->hasPseudoElements()) {
                setNeedsFrameTreeBuild();
            }
        } else {
            setNeedsStyleRecalc();
        }
    }

    int newState = m_state;
    if (oldState != newState) {
        didStateChanged(oldState, newState);
    }
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

unsigned short Node::compareDocumentPosition(const Node* other)
{
    // spec does not say what to do when other is nullptr
    if (!other) {
        return DOCUMENT_POSITION_DISCONNECTED;
    }
    if (this == other) {
        return 0;
    }

    Node* root = nullptr;
    if (isDocument()) {
        root = this;
    } else if (other->isDocument()) {
        root = ownerDocument();
    } else {
        root = ownerDocument();
        if (ownerDocument() != other->ownerDocument()) {
            STARFISH_ASSERT_NOT_REACHED();
            return DOCUMENT_POSITION_DISCONNECTED +
                   DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC +
                   DOCUMENT_POSITION_PRECEDING;
        }
    }

    for (Node* p = parentNode(); p != nullptr; p = p->parentNode()) {
        if (p == other) {
            return DOCUMENT_POSITION_CONTAINS + DOCUMENT_POSITION_PRECEDING;
        }
    }

    for (Node* p = other->parentNode(); p != nullptr; p = p->parentNode()) {
        if (p == this) {
            return DOCUMENT_POSITION_CONTAINED_BY + DOCUMENT_POSITION_FOLLOWING;
        }
    }

    unsigned short result = isPreceding(root, other, this);
    if (result == 0) {
        result = DOCUMENT_POSITION_FOLLOWING;
    }
    return result;
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
    if (isElement())
        return locateNamespacePrefix(asElement(), prefix);
    else
        return parentElement() ? locateNamespacePrefix(parentElement(), prefix)
                               : Nullable<String*>();
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
    Nullable<String*> defaultNamespace;
    if (isElement())
        defaultNamespace = locateNamespacePrefix(asElement(), namespaceUri);
    else
        defaultNamespace =
            parentElement()
                ? locateNamespacePrefix(parentElement(), namespaceUri)
                : Nullable<String*>();

    // Return true if defaultNamespace is the same as namespace, and false
    // otherwise.
    if (defaultNamespace == namespaceUri) {
        return true;
    }
    return false;
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
            new DOMTokenList(asElement(), starFish()->staticStrings()->m_class);
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
            document(), DOMException::HIERARCHY_REQUEST_ERR,
            "Parent is not a Document, DocumentFragment, or Element node.");
    }

    if (node == this) {
        throw new DOMException(
            document(), DOMException::HIERARCHY_REQUEST_ERR,
            "Node is a host-including inclusive ancestor of parent.");
    } else {
        for (Node* p = this; p != nullptr; p = p->parentNode()) {
            if (p == node) {
                throw new DOMException(
                    document(), DOMException::HIERARCHY_REQUEST_ERR,
                    "Node is a host-including inclusive ancestor of parent.");
            }
        }
    }

    if (child != nullptr && child->parentNode() != this) {
        throw new DOMException(
            document(), DOMException::Code::NOT_FOUND_ERR,
            "Child is not null and its parent is not parent.");
    }
    if (!(node->isDocumentType() || node->isElement() || node->isText() ||
          node->isComment() || node->isDocumentFragment())) {
        throw new DOMException(document(), DOMException::HIERARCHY_REQUEST_ERR,
                               "Node is not a DocumentFragment, DocumentType, "
                               "Element, Text, ProcessingInstruction, or "
                               "Comment.");
    }
    if ((node->isText() && isDocument()) ||
        (node->isDocumentType() && !isDocument())) {
        throw new DOMException(document(), DOMException::HIERARCHY_REQUEST_ERR,
                               "Either node is a Text node and parent is a "
                               "document, or node is a doctype and parent is "
                               "not a document.");
    }
    if (isDocument()) {
        if (node->isElement()) {
            if ((firstElementChild() != nullptr) ||
                (child != nullptr && child->isElement()) ||
                (child != nullptr && child->nextSibling() != nullptr &&
                 child->nextSibling()->isDocumentType())) {
                throw new DOMException(document(),
                                       DOMException::HIERARCHY_REQUEST_ERR,
                                       "parent has an element child, child is "
                                       "a doctype, or child is not null and a "
                                       "doctype is following child.");
            }
        } else if (node->isDocumentType()) {
            if (getDoctypeChild() ||
                (child != nullptr && child->previousSibling() != nullptr &&
                 child->previousSibling()->isElement()) ||
                (child == nullptr && firstElementChild() != nullptr)) {
                throw new DOMException(document(),
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

    node->didNodeInsertedToDocumentTree();
    Node* child = node->firstChild();
    while (child) {
        notifyNodeInsertedToDocumentTree(head, child);
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

    if (self->isInDocumentScope() &&
        self->document()->doesParticipateInRendering()) {
        notifyNodeInsertedToDocumentTree(self, child);
        self->setNeedsStyleRecalc();
        self->setChildrenNeedsStyleRecalc();
        child->setNeedsFrameTreeBuild();
    }
}

Node* Node::appendChild(Node* child)
{
    if (!isContainerNode()) {
        throw new DOMException(document(), DOMException::HIERARCHY_REQUEST_ERR,
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
        throw new DOMException(document(), DOMException::HIERARCHY_REQUEST_ERR,
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

void Node::validateReplace(Node* child, Node* childToRemove) // node, child
{
    Node* childRef = childToRemove;
    // 4.2.1 replace validity
    if (!(isDocument() || isElement())) {
        throw new DOMException(
            document(), DOMException::HIERARCHY_REQUEST_ERR,
            "Parent is not a Document, DocumentFragment, or Element node.");
    }

    if (child == this) {
        throw new DOMException(
            document(), DOMException::HIERARCHY_REQUEST_ERR,
            "Node is a host-including inclusive ancestor of parent.");
    } else {
        for (Node* p = this; p != nullptr; p = p->parentNode()) {
            if (p == child) {
                throw new DOMException(
                    document(), DOMException::HIERARCHY_REQUEST_ERR,
                    "Node is a host-including inclusive ancestor of parent.");
            }
        }
    }

    if (childRef != nullptr && childRef->parentNode() != this) {
        throw new DOMException(
            document(), DOMException::Code::NOT_FOUND_ERR,
            "Child is not null and its parent is not parent.");
    }
    if (!(child->isDocumentType() || child->isDocumentFragment() ||
          child->isElement() || child->isText() || child->isComment())) {
        throw new DOMException(document(), DOMException::HIERARCHY_REQUEST_ERR,
                               "Node is not a DocumentFragment, DocumentType, "
                               "Element, Text, ProcessingInstruction, or "
                               "Comment.");
    }
    if ((child->isText() && isDocument()) ||
        (child->isDocumentType() && !isDocument())) {
        throw new DOMException(document(), DOMException::HIERARCHY_REQUEST_ERR,
                               "Either node is a Text node and parent is a "
                               "document, or node is a doctype and parent is "
                               "not a document.");
    }
    if (isDocument()) {
        if (child->isElement()) {
            if ((firstElementChild() != nullptr) ||
                (childRef != nullptr && childRef->isElement()) ||
                (childRef != nullptr && childRef->nextSibling() != nullptr &&
                 childRef->nextSibling()->isDocumentType())) {
                throw new DOMException(document(),
                                       DOMException::HIERARCHY_REQUEST_ERR,
                                       "parent has an element child that is "
                                       "not child or a doctype is following "
                                       "child.");
            }
        } else if (child->isDocumentType()) {
            if ((getDoctypeChild() != nullptr &&
                 getDoctypeChild() != childRef) ||
                (childRef != nullptr &&
                 childRef->previousSibling() != nullptr &&
                 childRef->previousSibling()->isElement())) {
                throw new DOMException(document(),
                                       DOMException::HIERARCHY_REQUEST_ERR,
                                       "parent has a doctype child that is not "
                                       "child, or an element is preceding "
                                       "child.");
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
        throw new DOMException(document(), DOMException::NOT_FOUND_ERR,
                               "Child's parent is not parent.");
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

    FrameTreeBuilder::clearTree(child);
    setNeedsFrameTreeBuild();

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
    child->setNeedsStyleRecalc();
    setNeedsFrameTreeBuild();

    if (isInDocumentScope()) {
        notifyNodeInsertedToDocumentTree(this, child);
    }

    Node* parent = this;
    while (parent) {
        parent->didNodeInserted(this, child);
        parent = parent->parentNode();
    }

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
    child->setNeedsStyleRecalc();
    setNeedsFrameTreeBuild();

    Node* parent = this;
    while (parent) {
        parent->didNodeInserted(this, child);
        parent = parent->parentNode();
    }

    if (isInDocumentScope()) {
        notifyNodeInsertedToDocumentTree(this, child);
    }
}

void Node::parserTakeAllChildrenFrom(Node* oldParent)
{
    while (Node* child = oldParent->firstChild()) {
        oldParent->parserRemoveChild(child);
        parserAppendChild(child);
    }
}

HTMLCollection* Node::getElementsByTagName(String* name)
{
    return getElementsByTagName(document()->createAttributeName(name));
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

    list = new HTMLCollection(this, NodeListImpl::TagNameFilter,
                              new QualifiedName(qualifiedName), true);
    rareData->putActiveHtmlCollectionListWithQuery(
        activeLists, qualifiedName.localName(), list);
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
        throw new DOMException(document(), DOMException::SYNTAX_ERR,
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
        throw new DOMException(document(), DOMException::SYNTAX_ERR,
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

void Node::setNeedsFrameTreeBuild()
{
    if (!document()->doesParticipateInRendering()) {
        return;
    }

    window()->browsingContext()->setNeedsFrameTreeBuild();

    Frame* old = frame();
    if (old) {
        Frame* parent = old->parent();
        if (!parent) {
            // STARFISH_ASSERT(old->isFrameDocument());
            parent = document()->frame();
        } else {
            while (parent) {
                if (!parent->isAnonymous() &&
                    (parent->isBlockLevel() || parent->isFrameTableCellBox())) {
                    break;
                }
                parent = parent->parent();
            }
        }

        STARFISH_ASSERT(parent);
        while (parent->firstChild()) {
            parent->removeChild(parent->firstChild());
        }

        Node* node = parent->node()->firstChild();
        while (node) {
            FrameTreeBuilder::clearTree(node);
            node = node->nextSibling();
        }

        node = parent->node();
        while (node) {
            node->markChildNeedsFrameTreeBuild();
            node = node->parentNode();
        }
    } else {
        Node* node = this;
        while (node) {
            if (node->frame()) {
                node->setNeedsFrameTreeBuild();
                break;
            }
            node = node->parentNode();
        }
    }
}

void Node::setNeedsStyleRecalc()
{
    if (!document()->doesParticipateInRendering()) {
        return;
    }

    if (!m_needsStyleRecalc) {
        m_needsStyleRecalc = true;

        Node* node = parentNode();
        while (node && !node->childNeedsStyleRecalc()) {
            node->setChildNeedsStyleRecalc();
            node = node->parentNode();
        }
    }
    window()->browsingContext()->setNeedsStyleRecalc();
}

void Node::setNeedsStyleRecalcIfNeeded()
{
    if (!document()->doesParticipateInRendering()) {
        return;
    }

    // current node
    if (!m_needsStyleRecalc) {
        m_needsStyleRecalc = true;

        Node* node = parentNode();
        while (node && !node->childNeedsStyleRecalc()) {
            node->setChildNeedsStyleRecalc();
            node = node->parentNode();
        }
    }

    // siblings
    setSiblingsNeedsStyleRecalcIfNeeded();

    // children
    setChildrenNeedsStyleRecalcIfNeeded();

    window()->browsingContext()->setNeedsStyleRecalc();
}

void Node::setSiblingsNeedsStyleRecalcIfNeeded()
{
    Node* node = nextSibling();
    while (node) {
        if (node->style() &&
            node->style()->combinatorMatchingResult() ==
                StyleResolver::CombinatorMatchingResult::
                    CombinatorMatchesPartially) {
            node->m_needsStyleRecalc = true;
        }
        node = node->nextSibling();
    }
}

void Node::setChildrenNeedsStyleRecalcIfNeeded()
{
    Node* child = firstChild();
    while (child) {
        if (child->style() &&
            child->style()->combinatorMatchingResult() ==
                StyleResolver::CombinatorMatchingResult::
                    CombinatorMatchesPartially) {
            child->m_needsStyleRecalc = true;
        }
        child->setChildrenNeedsStyleRecalcIfNeeded();
        child = child->nextSibling();
    }
}

void Node::setChildrenNeedsStyleRecalc()
{
    if (!document()->doesParticipateInRendering()) {
        return;
    }

    Node* child = firstChild();
    while (child) {
        child->m_needsStyleRecalc = true;
        child->setChildrenNeedsStyleRecalc();
        child = child->nextSibling();
    }
}

void Node::setNeedsLayout()
{
    if (!document()->doesParticipateInRendering()) {
        return;
    }

    window()->browsingContext()->setNeedsLayout();
}

void Node::setNeedsPainting()
{
    if (!document()->doesParticipateInRendering()) {
        return;
    }

    window()->browsingContext()->setNeedsPainting();
}

void Node::setNeedsComposite()
{
    if (!document()->doesParticipateInRendering()) {
        return;
    }

    window()->browsingContext()->setNeedsComposite();
}

void Node::didComputedStyleChanged(ComputedStyle* oldStyle,
                                   ComputedStyle* newStyle)
{
    if (frame()) {
        frame()->computeStyleFlags();
    }
}

void Node::didNodeInserted(Node* parent, Node* newChild)
{
    if (hasRareMembers()) {
        m_rareNodeMembers->invalidateActiveActiveNodeListCacheIfNeeded();
    }
}

void Node::didNodeRemoved(Node* parent, Node* oldChild)
{
    if (hasRareMembers()) {
        m_rareNodeMembers->invalidateActiveActiveNodeListCacheIfNeeded();
    }
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
}
