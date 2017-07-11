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
#include "core/style/ComputedStyle.h"
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
    if (state == NodeStateNormal) {
        m_state = 0;
        m_restyleFlags = 0;

        setNeedsStyleRecalc();
        if (childrenOrSiblingsAffectedByDynamicEvent(mask)) {
            setChildrenNeedsStyleRecalc();
            setSiblingsNeedsStyleRecalc();
            if (isElement() && asElement()->hasPseudoElements()) {
                setNeedsFrameTreeBuild();
            }
        }

    } else if (!(m_state & state) == enable) {
        m_state ^= state;

        setNeedsStyleRecalc();
        if (childrenOrSiblingsAffectedByDynamicEvent(mask)) {
            setChildrenNeedsStyleRecalc();
            setSiblingsNeedsStyleRecalc();
            if (isElement() && asElement()->hasPseudoElements()) {
                setNeedsFrameTreeBuild();
            }
        }
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

    node->didNodeInsertedToDocumenTree();
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
    node->didNodeRemovedFromDocumenTree();
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

void Node::setSiblingsNeedsStyleRecalc()
{
    if (!document()->doesParticipateInRendering()) {
        return;
    }

    Node* node = nextSibling();
    while (node) {
        node->m_needsStyleRecalc = true;
        node = node->nextSibling();
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

#ifdef STARFISH_ENABLE_TEST

// helper function to convert Length to CSSStyleValuePair format
static CSSStyleValuePair lengthToCSSStyleValue(Length len)
{
    CSSStyleValuePair p;
    if (len.isFixed()) {
        p.setValueKind(CSSStyleValuePair::ValueKind::Length);
        p.setValue(CSSLength(len.fixed()));
    } else if (len.isPercent()) {
        p.setValueKind(CSSStyleValuePair::ValueKind::Percentage);
        p.setValue(len.percent());
    } else if (len.isAuto()) {
        p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
    } else {
        STARFISH_ASSERT(false);
    }
    return p;
};

CSSStyleDeclaration* Node::getComputedStyle()
{
    Element* e = nullptr;
    if (isElement()) {
        e = asElement();
    }
    CSSStyleDeclaration* d = new ComputedStyleCSSStyleDeclaration(e);

    window()->browsingContext()->webView()->layoutIfNeeds();

    ComputedStyle* style = m_style;
    if (style == nullptr) {
        style = new ComputedStyle();
    }

// general properties
#define ADD_VALUE_PAIR(keyKind, valueKind, getter)               \
    {                                                            \
        CSSStyleValuePair p;                                     \
        p.setKeyKind(CSSStyleValuePair::KeyKind::keyKind);       \
        p.setValueKind(CSSStyleValuePair::ValueKind::valueKind); \
        p.setValue(style->getter());                             \
        d->addValuePair(p);                                      \
    }

    ADD_VALUE_PAIR(Display, DisplayValueKind, display)
    ADD_VALUE_PAIR(Position, PositionValueKind, position)
    ADD_VALUE_PAIR(Float, FloatValueKind, floating)
    ADD_VALUE_PAIR(Clear, ClearValueKind, clear)
    ADD_VALUE_PAIR(VerticalAlign, VerticalAlignValueKind, verticalAlign)
    ADD_VALUE_PAIR(TextAlign, SideValueKind, textAlign)
    ADD_VALUE_PAIR(TextDecoration, TextDecorationValueKind, textDecoration)
    ADD_VALUE_PAIR(Direction, DirectionValueKind, direction)
    ADD_VALUE_PAIR(BackgroundRepeatX, BackgroundRepeatValueKind,
                   backgroundRepeatX)
    ADD_VALUE_PAIR(BackgroundRepeatY, BackgroundRepeatValueKind,
                   backgroundRepeatY)
    ADD_VALUE_PAIR(BorderTopStyle, BorderStyleValueKind, borderTopStyle)
    ADD_VALUE_PAIR(BorderRightStyle, BorderStyleValueKind, borderRightStyle)
    ADD_VALUE_PAIR(BorderBottomStyle, BorderStyleValueKind, borderBottomStyle)
    ADD_VALUE_PAIR(BorderLeftStyle, BorderStyleValueKind, borderLeftStyle)
    ADD_VALUE_PAIR(Visibility, VisibilityValueKind, visibility)
    ADD_VALUE_PAIR(FontStyle, FontStyleValueKind, fontStyle)
    ADD_VALUE_PAIR(FontWeight, FontWeightValueKind, fontWeight)
    ADD_VALUE_PAIR(Overflow, OverflowValueKind, overflow)
    ADD_VALUE_PAIR(UnicodeBidi, UnicodeBidiValueKind, unicodeBidi)
    ADD_VALUE_PAIR(Opacity, Number, opacity)
    ADD_VALUE_PAIR(ZIndex, Int32, zIndex)
#undef ADD_VALUE_PAIR

// length properties
#define ADD_LENGTH_PAIR(keyKind, getter)                              \
    {                                                                 \
        CSSStyleValuePair p;                                          \
        p.setKeyKind(CSSStyleValuePair::KeyKind::keyKind);            \
        if (style->getter().isFixed()) {                              \
            p.setValueKind(CSSStyleValuePair::ValueKind::Length);     \
            p.setValue(CSSLength(style->getter().fixed()));           \
        } else if (style->getter().isPercent()) {                     \
            p.setValueKind(CSSStyleValuePair::ValueKind::Percentage); \
            p.setValue(style->getter().percent());                    \
        } else if (style->getter().isAuto()) {                        \
            p.setValueKind(CSSStyleValuePair::ValueKind::Auto);       \
        } else {                                                      \
            p.setValueKind(CSSStyleValuePair::ValueKind::None);       \
        }                                                             \
        d->addValuePair(p);                                           \
    }

    ADD_LENGTH_PAIR(Width, width)
    ADD_LENGTH_PAIR(MaxWidth, maxWidth)
    ADD_LENGTH_PAIR(MinWidth, minWidth)
    ADD_LENGTH_PAIR(Height, height)
    ADD_LENGTH_PAIR(MaxHeight, maxHeight)
    ADD_LENGTH_PAIR(MinHeight, minHeight)
    ADD_LENGTH_PAIR(LineHeight, lineHeight)
    ADD_LENGTH_PAIR(TextIndent, textIndent)
    ADD_LENGTH_PAIR(Top, top)
    ADD_LENGTH_PAIR(Right, right)
    ADD_LENGTH_PAIR(Bottom, bottom)
    ADD_LENGTH_PAIR(Left, left)
    ADD_LENGTH_PAIR(MarginTop, marginTop)
    ADD_LENGTH_PAIR(MarginRight, marginRight)
    ADD_LENGTH_PAIR(MarginBottom, marginBottom)
    ADD_LENGTH_PAIR(MarginLeft, marginLeft)
    ADD_LENGTH_PAIR(PaddingTop, paddingTop)
    ADD_LENGTH_PAIR(PaddingRight, paddingRight)
    ADD_LENGTH_PAIR(PaddingBottom, paddingBottom)
    ADD_LENGTH_PAIR(PaddingLeft, paddingLeft)
#undef ADD_LENGTH_PAIR

// color properties
#define ADD_COLOR_PAIR(keyKind, getter)                                \
    {                                                                  \
        CSSStyleValuePair p;                                           \
        p.setKeyKind(CSSStyleValuePair::KeyKind::keyKind);             \
        p.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind); \
        p.setValue(style->getter().toString());                        \
        d->addValuePair(p);                                            \
    }

    ADD_COLOR_PAIR(Color, color)
    ADD_COLOR_PAIR(BackgroundColor, backgroundColor)
    ADD_COLOR_PAIR(BorderTopColor, borderTopColor)
    ADD_COLOR_PAIR(BorderRightColor, borderRightColor)
    ADD_COLOR_PAIR(BorderBottomColor, borderBottomColor)
    ADD_COLOR_PAIR(BorderLeftColor, borderLeftColor)
#undef ADD_COLOR_PAIR

// fontSize
// border-width
#define LENGTH_RELATED(keyKind, getter)                       \
    {                                                         \
        CSSStyleValuePair p;                                  \
        p.setKeyKind(CSSStyleValuePair::KeyKind::keyKind);    \
        p.setValueKind(CSSStyleValuePair::ValueKind::Length); \
        p.setValue(CSSLength(style->getter().fixed()));       \
        d->addValuePair(p);                                   \
    }

    LENGTH_RELATED(FontSize, fontSize)
    LENGTH_RELATED(BorderTopWidth, borderTopWidth)
    LENGTH_RELATED(BorderRightWidth, borderRightWidth)
    LENGTH_RELATED(BorderBottomWidth, borderBottomWidth)
    LENGTH_RELATED(BorderLeftWidth, borderLeftWidth)
#undef LENGTH_RELATED

    // other properties that cannot be generated by macros

    // backgroundImage
    {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundImage);
        p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        ValueList* vals = new ValueList(ValueList::Separator::CommaSeparator);
        for (unsigned int i = 0; i < style->backgroundLayerSize(); i++) {
            CSSStyleValuePair item;
            if (style->backgroundImage(i)->length() == 0) {
                item.setValueKind(CSSStyleValuePair::ValueKind::None);
            } else {
                item.setValueKind(CSSStyleValuePair::ValueKind::UrlValueKind);
                item.setValue(style->backgroundImage(i));
            }
            vals->push_back(item);
        }
        p.setValueList(vals);
        d->addValuePair(p);
    }

    // borderImageSource
    {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderImageSource);
        if (style->borderImageSource()->length() == 0) {
            p.setValueKind(CSSStyleValuePair::ValueKind::None);
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::UrlValueKind);
            p.setValue(style->borderImageSource());
        }
        d->addValuePair(p);
    }

    // backgroundSize
    {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundSize);
        p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        ValueList* values = new ValueList(ValueList::Separator::CommaSeparator);
        for (unsigned int i = 0; i < style->backgroundLayerSize(); i++) {
            CSSStyleValuePair item;
            if (style->bgSizeType(i) == BackgroundSizeType::Cover) {
                item.setValueKind(CSSStyleValuePair::ValueKind::Cover);
            } else if (style->bgSizeType(i) == BackgroundSizeType::Contain) {
                item.setValueKind(CSSStyleValuePair::ValueKind::Contain);
            } else if (style->bgSizeType(i) == BackgroundSizeType::SizeValue) {
                item.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
                ValueList* vals = new ValueList();

                CSSStyleValuePair w =
                    lengthToCSSStyleValue(style->bgSizeValue(i).width());
                vals->emplace_back(w.valueKind(), w.value());

                CSSStyleValuePair h =
                    lengthToCSSStyleValue(style->bgSizeValue(i).height());
                vals->emplace_back(h.valueKind(), h.value());

                item.setValue(vals);
            }
            values->push_back(item);
        }
        p.setValueList(values);
        d->addValuePair(p);
    }

    // backgroundPositionX
    {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundPositionX);
        p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        ValueList* values = new ValueList(ValueList::Separator::CommaSeparator);
        for (unsigned int i = 0; i < style->backgroundLayerSize(); i++) {
            CSSStyleValuePair item =
                lengthToCSSStyleValue(style->backgroundPositionX(i));
            values->push_back(item);
        }
        p.setValueList(values);
        d->addValuePair(p);
    }

    // backgroundPositionY
    {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundPositionY);
        p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        ValueList* values = new ValueList(ValueList::Separator::CommaSeparator);
        for (unsigned int i = 0; i < style->backgroundLayerSize(); i++) {
            CSSStyleValuePair item =
                lengthToCSSStyleValue(style->backgroundPositionY(i));
            values->push_back(item);
        }
        p.setValueList(values);
        d->addValuePair(p);
    }

    // LineHeight
    {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::LineHeight);
        if (style->lineHeight().isFixed()) {
            p.setValueKind(CSSStyleValuePair::ValueKind::Length);
            p.setValue(CSSLength(style->lineHeight().fixed()));
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::Normal);
        }
        d->addValuePair(p);
    }

    // borderImageSlice
    {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderImageSlice);
        p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        ValueList* vals = new ValueList();
        LengthBox box = style->borderImageSlices();

        CSSStyleValuePair t = lengthToCSSStyleValue(box.top());
        vals->emplace_back(t.valueKind(), t.value());

        CSSStyleValuePair r = lengthToCSSStyleValue(box.right());
        vals->emplace_back(r.valueKind(), r.value());

        CSSStyleValuePair b = lengthToCSSStyleValue(box.bottom());
        vals->emplace_back(b.valueKind(), b.value());

        CSSStyleValuePair l = lengthToCSSStyleValue(box.left());
        vals->emplace_back(l.valueKind(), l.value());

        p.setValue(vals);
        d->addValuePair(p);
    }

    // borderImageWidth
    // FIXME: Need to refactor BorderImageLength.h, and update the lines below
    {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderImageWidth);
        p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        ValueList* vals = new ValueList();
        BorderImageLengthBox box = style->borderImageWidths();

        if (box.top().isLength()) {
            CSSStyleValuePair t = lengthToCSSStyleValue(box.top().length());
            vals->emplace_back(t.valueKind(), t.value());
        } else {
            vals->emplace_back(CSSStyleValuePair::ValueKind::Number,
                               (float)box.top().number());
        }
        if (box.right().isLength()) {
            CSSStyleValuePair r = lengthToCSSStyleValue(box.right().length());
            vals->emplace_back(r.valueKind(), r.value());
        } else {
            vals->emplace_back(CSSStyleValuePair::ValueKind::Number,
                               (float)box.right().number());
        }
        if (box.bottom().isLength()) {
            CSSStyleValuePair b = lengthToCSSStyleValue(box.bottom().length());
            vals->emplace_back(b.valueKind(), b.value());
        } else {
            vals->emplace_back(CSSStyleValuePair::ValueKind::Number,
                               (float)box.bottom().number());
        }
        if (box.left().isLength()) {
            CSSStyleValuePair l = lengthToCSSStyleValue(box.left().length());
            vals->emplace_back(l.valueKind(), l.value());
        } else {
            vals->emplace_back(CSSStyleValuePair::ValueKind::Number,
                               (float)box.bottom().number());
        }

        p.setValue(vals);
        d->addValuePair(p);
    }

    // transform-origin
    {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::TransformOrigin);

        if (style->transformOrigin() == NULL) {
            p.setValueKind(CSSStyleValuePair::ValueKind::None);
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
            ValueList* vals = new ValueList();

            CSSStyleValuePair x = lengthToCSSStyleValue(
                style->transformOrigin()->originValue()->getXAxis());
            vals->emplace_back(x.valueKind(), x.value());

            CSSStyleValuePair y = lengthToCSSStyleValue(
                style->transformOrigin()->originValue()->getYAxis());
            vals->emplace_back(y.valueKind(), y.value());

            p.setValue(vals);
        }
        d->addValuePair(p);
    }

    // TODO: transform

    return d;
}

void Node::dumpStyle()
{
    dump();
    printf(", style: { ");

    // display
    if (m_style->display() == InlineDisplayValue) {
        printf("display: inline, ");
    } else if (m_style->display() == BlockDisplayValue) {
        printf("display: block, ");
    } else if (m_style->display() == InlineBlockDisplayValue) {
        printf("display: inline-block, ");
    } else if (m_style->display() == TableDisplayValue) {
        printf("display: table, ");
    } else if (m_style->display() == InlineTableDisplayValue) {
        printf("display: inline-table, ");
    } else if (m_style->display() == TableRowGroupDisplayValue) {
        printf("display: table-row-group, ");
    } else if (m_style->display() == TableHeaderGroupDisplayValue) {
        printf("display: table-header-group, ");
    } else if (m_style->display() == TableFooterGroupDisplayValue) {
        printf("display: table-footer-group, ");
    } else if (m_style->display() == TableRowDisplayValue) {
        printf("display: table-row, ");
    } else if (m_style->display() == TableColumnGroupDisplayValue) {
        printf("display: table-column-group, ");
    } else if (m_style->display() == TableColumnDisplayValue) {
        printf("display: table-column, ");
    } else if (m_style->display() == TableCellDisplayValue) {
        printf("display: table-cell, ");
    } else if (m_style->display() == TableCaptionDisplayValue) {
        printf("display: table-caption, ");
    } else if (m_style->display() == NoneDisplayValue) {
        printf("display: none, ");
    }

    // position
    if (m_style->position() == StaticPositionValue) {
        printf("position: static, ");
    } else if (m_style->position() == RelativePositionValue) {
        printf("position: relative, ");
    } else if (m_style->position() == AbsolutePositionValue) {
        printf("position: absolute, ");
    }

    // width
    if (m_style->width().isFixed()) {
        printf("width: %f, ", m_style->width().fixed());
    } else if (m_style->width().isPercent()) {
        printf("width: %f, ", m_style->width().percent());
    } else if (m_style->width().isAuto()) {
        printf("width: auto, ");
    }

    // height
    if (m_style->height().isFixed()) {
        printf("height: %.2f, ", m_style->height().fixed());
    } else if (m_style->height().isPercent()) {
        printf("height: %.2fp, ", m_style->height().percent());
    } else if (m_style->height().isAuto()) {
        printf("height: auto, ");
    }

    // vertical-align
    if (m_style->verticalAlign() == VerticalAlignValue::BaselineVAlignValue) {
        printf("vertical-align: baseline, ");
    } else if (m_style->verticalAlign() == VerticalAlignValue::SubVAlignValue) {
        printf("vertical-align: sub, ");
    } else if (m_style->verticalAlign() ==
               VerticalAlignValue::SuperVAlignValue) {
        printf("vertical-align: super, ");
    } else if (m_style->verticalAlign() == VerticalAlignValue::TopVAlignValue) {
        printf("vertical-align: top, ");
    } else if (m_style->verticalAlign() ==
               VerticalAlignValue::TextTopVAlignValue) {
        printf("vertical-align: text-top, ");
    } else if (m_style->verticalAlign() ==
               VerticalAlignValue::MiddleVAlignValue) {
        printf("vertical-align: middle, ");
    } else if (m_style->verticalAlign() ==
               VerticalAlignValue::BottomVAlignValue) {
        printf("vertical-align: bottom, ");
    } else if (m_style->verticalAlign() ==
               VerticalAlignValue::TextBottomVAlignValue) {
        printf("vertical-align: text-bottom, ");
    } else if (m_style->verticalAlignLength().isFixed()) {
        printf("vertical-align: %.2f, ",
               m_style->verticalAlignLength().fixed());
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    // text-align
    if (m_style->textAlign() == SideValue::LeftSideValue) {
        printf("text-align: left, ");
    } else if (m_style->textAlign() == SideValue::RightSideValue) {
        printf("text-align: right, ");
    } else if (m_style->textAlign() == SideValue::CenterSideValue) {
        printf("text-align: center, ");
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    // text-decoration
    if (m_style->textDecoration() ==
        TextDecorationValue::UnderLineTextDecorationValue) {
        printf("text-decoration: underline, ");
    } else if (m_style->textDecoration() ==
               TextDecorationValue::OverLineTextDecorationValue) {
        printf("text-decoration: overline, ");
    } else if (m_style->textDecoration() ==
               TextDecorationValue::LineThroughTextDecorationValue) {
        printf("text-decoration: line-through, ");
    } else if (m_style->textDecoration() ==
               TextDecorationValue::BlinkTextDecorationValue) {
        printf("text-decoration: blink, ");
    } else if (m_style->textDecoration() ==
               TextDecorationValue::NoneTextDecorationValue) {
        printf("text-decoration: none, ");
    }

    // direction
    if (m_style->direction() == DirectionValue::LtrDirectionValue) {
        printf("direction: ltr, ");
    } else {
        printf("direction: rtl, ");
    }

    if (m_style->whiteSpace() == WhiteSpaceValue::NormalWhiteSpaceValue) {
        printf("white-space: normal, ");
    } else {
        printf("white-space: nowrap, ");
    }

    // unicode-bidi
    if (m_style->unicodeBidi() == UnicodeBidiValue::NormalUnicodeBidiValue) {
        printf("unicode-bidi: normal, ");
    } else {
        printf("unicode-bidi: embed, ");
    }

    // font-size
    printf("font-size: %.1f, ", m_style->fontSize().fixed());

    // font-style
    printf("font-style: ");
    if (m_style->fontStyle() == FontStyleValue::NormalFontStyleValue) {
        printf("normal, ");
    } else if (m_style->fontStyle() == FontStyleValue::ItalicFontStyleValue) {
        printf("italic, ");
    } else if (m_style->fontStyle() == FontStyleValue::ObliqueFontStyleValue) {
        printf("oblique, ");
    }

    printf("font-weight: ");
    if (m_style->fontWeight() == FontWeightValue::OneHundredFontWeightValue) {
        printf("100, ");
    }
    if (m_style->fontWeight() == FontWeightValue::TwoHundredsFontWeightValue) {
        printf("200, ");
    }
    if (m_style->fontWeight() ==
        FontWeightValue::ThreeHundredsFontWeightValue) {
        printf("300, ");
    }
    if (m_style->fontWeight() == FontWeightValue::NormalFontWeightValue) {
        printf("normal, ");
    }
    if (m_style->fontWeight() == FontWeightValue::FiveHundredsFontWeightValue) {
        printf("500, ");
    }
    if (m_style->fontWeight() == FontWeightValue::SixHundredsFontWeightValue) {
        printf("600, ");
    }
    if (m_style->fontWeight() == FontWeightValue::BoldFontWeightValue) {
        printf("bold, ");
    }
    if (m_style->fontWeight() ==
        FontWeightValue::EightHundredsFontWeightValue) {
        printf("800, ");
    }
    if (m_style->fontWeight() == FontWeightValue::NineHundredsFontWeightValue) {
        printf("900, ");
    }

    // letter-spacing
    printf("letter-spacing: %f, ", m_style->letterSpacing().fixed());

    // line-height
    if (m_style->lineHeight().isFixed()) {
        printf("line-height: %.1f, ", m_style->lineHeight().fixed());
    } else {
        printf("line-height: normal, ");
    }

    // color
    printf("color: (%d,%d,%d,%d), ", m_style->color().r(), m_style->color().g(),
           m_style->color().b(), m_style->color().a());

    // background-color
    printf("background-color: (%d,%d,%d,%d), ", m_style->backgroundColor().r(),
           m_style->backgroundColor().g(), m_style->backgroundColor().b(),
           m_style->backgroundColor().a());

    // background-image
    if (m_style->backgroundImage()->length() == 0) {
        printf("background-image: none, ");
    } else {
        printf("background-image: %s, ",
               m_style->backgroundImage()->utf8Data());
    }

    // background-position
    printf("background-position: (%s, %s),",
           m_style->backgroundPositionX().dumpString()->utf8Data(),
           m_style->backgroundPositionY().dumpString()->utf8Data());

    // background-size
    if (m_style->bgSizeType() == BackgroundSizeType::Cover) {
        printf("background-size: cover, ");
    } else if (m_style->bgSizeType() == BackgroundSizeType::Contain) {
        printf("background-size: contain, ");
    } else if (m_style->bgSizeType() == BackgroundSizeType::SizeValue) {
        printf("background-size: (%s, %s),",
               m_style->bgSizeValue().width().dumpString()->utf8Data(),
               m_style->bgSizeValue().height().dumpString()->utf8Data());
    }

    // box offsets: top
    if (m_style->top().isFixed()) {
        printf("top: %f, ", m_style->top().fixed());
    } else if (m_style->top().isPercent()) {
        printf("top: %f, ", m_style->top().percent());
    } else if (m_style->top().isAuto()) {
        printf("top: auto, ");
    }

    // box offsets: right
    if (m_style->right().isFixed()) {
        printf("right: %f, ", m_style->right().fixed());
    } else if (m_style->right().isPercent()) {
        printf("right: %f, ", m_style->right().percent());
    } else if (m_style->right().isAuto()) {
        printf("right: auto, ");
    }

    // box offsets: bottom
    if (m_style->bottom().isFixed()) {
        printf("bottom: %f, ", m_style->bottom().fixed());
    } else if (m_style->bottom().isPercent()) {
        printf("bottom: %f, ", m_style->bottom().percent());
    } else if (m_style->bottom().isAuto()) {
        printf("bottom: auto, ");
    }

    // box offsets: left
    if (m_style->left().isFixed()) {
        printf("left: %f, ", m_style->left().fixed());
    } else if (m_style->left().isPercent()) {
        printf("left: %f, ", m_style->left().percent());
    } else if (m_style->left().isAuto()) {
        printf("left: auto, ");
    }

    // border-color
    printf("border-top-color: (%d,%d,%d,%d), ", m_style->borderTopColor().r(),
           m_style->borderTopColor().g(), m_style->borderTopColor().b(),
           m_style->borderTopColor().a());
    printf("border-right-color: (%d,%d,%d,%d), ",
           m_style->borderRightColor().r(), m_style->borderRightColor().g(),
           m_style->borderRightColor().b(), m_style->borderRightColor().a());
    printf("border-bottom-color: (%d,%d,%d,%d), ",
           m_style->borderBottomColor().r(), m_style->borderBottomColor().g(),
           m_style->borderBottomColor().b(), m_style->borderBottomColor().a());
    printf("border-left-color: (%d,%d,%d,%d), ", m_style->borderLeftColor().r(),
           m_style->borderLeftColor().g(), m_style->borderLeftColor().b(),
           m_style->borderLeftColor().a());

    // border-image-slice
    LengthBox l = m_style->borderImageSlices();
    if (l.top().isPercent()) {
        printf("border-image-slice: (%.2fp, ", l.top().percent());
    } else {
        printf("border-image-slice: (%.2f, ", l.top().fixed());
    }
    if (l.right().isPercent()) {
        printf("%.2fp, ", l.right().percent());
    } else {
        printf("%.1f, ", l.right().fixed());
    }
    if (l.bottom().isPercent()) {
        printf("%.2fp, ", l.bottom().percent());
    } else {
        printf("%.1f, ", l.bottom().fixed());
    }
    if (l.left().isPercent()) {
        printf("%.2fp, ", l.left().percent());
    } else {
        printf("%.1f, ", l.left().fixed());
    }
    printf("%d), ", m_style->borderImageSliceFill());

    // border-image-source
    if (!m_style->borderImageSource()->equals(String::emptyString)) {
        printf("border-image-source: %s, ",
               m_style->borderImageSource()->utf8Data());
    }

    // border-image-width
    BorderImageLengthBox b = m_style->borderImageWidths();
    printf("border-image-width: (%s, %s, %s, %s), ",
           b.top().dumpString()->utf8Data(), b.right().dumpString()->utf8Data(),
           b.bottom().dumpString()->utf8Data(),
           b.left().dumpString()->utf8Data());

    // border-style
    printf("border-style(t, r, b, l): (");
    if (m_style->borderTopStyle() == BorderStyleValue::NoneBorderStyleValue) {
        printf("none,");
    } else if (m_style->borderTopStyle() ==
               BorderStyleValue::SolidBorderStyleValue) {
        printf("solid,");
    }
    if (m_style->borderRightStyle() == BorderStyleValue::NoneBorderStyleValue) {
        printf("none,");
    } else if (m_style->borderRightStyle() ==
               BorderStyleValue::SolidBorderStyleValue) {
        printf("solid,");
    }
    if (m_style->borderBottomStyle() ==
        BorderStyleValue::NoneBorderStyleValue) {
        printf("none,");
    } else if (m_style->borderBottomStyle() ==
               BorderStyleValue::SolidBorderStyleValue) {
        printf("solid,");
    }
    if (m_style->borderLeftStyle() == BorderStyleValue::NoneBorderStyleValue) {
        printf("none), ");
    } else if (m_style->borderLeftStyle() ==
               BorderStyleValue::SolidBorderStyleValue) {
        printf("solid), ");
    }

    // border-width
    printf("border-width(t, r, b, l): (%.0f, %.0f, %.0f, %.0f), ",
           m_style->borderTopWidth().fixed(),
           m_style->borderRightWidth().fixed(),
           m_style->borderBottomWidth().fixed(),
           m_style->borderLeftWidth().fixed());

    // background-repeat-x
    if (m_style->backgroundRepeatX() ==
        BackgroundRepeatValue::RepeatRepeatValue) {
        printf("background-repeat-x: repeat, ");
    } else {
        printf("background-repeat-x: no-repeat, ");
    }

    // background-repeat-y
    if (m_style->backgroundRepeatY() ==
        BackgroundRepeatValue::RepeatRepeatValue) {
        printf("background-repeat-y: repeat, ");
    } else {
        printf("background-repeat-y: no-repeat, ");
    }

    // margin-top
    if (m_style->marginTop().isFixed()) {
        printf("margin-top: %f, ", m_style->marginTop().fixed());
    } else if (m_style->marginTop().isPercent()) {
        printf("margin-top: %f, ", m_style->marginTop().percent());
    } else if (m_style->marginTop().isAuto()) {
        printf("margin-top: auto, ");
    }

    // margin-bottom
    if (m_style->marginBottom().isFixed()) {
        printf("margin-bottom: %.2f, ", m_style->marginBottom().fixed());
    } else if (m_style->marginBottom().isPercent()) {
        printf("margin-bottom: %.2fp, ", m_style->marginBottom().percent());
    } else if (m_style->marginBottom().isAuto()) {
        printf("margin-bottom: auto, ");
    }

    // margin-left
    if (m_style->marginLeft().isFixed()) {
        printf("margin-left: %f, ", m_style->marginLeft().fixed());
    } else if (m_style->marginLeft().isPercent()) {
        printf("margin-left: %f, ", m_style->marginLeft().percent());
    } else if (m_style->marginLeft().isAuto()) {
        printf("margin-left: auto, ");
    }

    // margin-right
    if (m_style->marginRight().isFixed()) {
        printf("margin-right: %f, ", m_style->marginRight().fixed());
    } else if (m_style->marginRight().isPercent()) {
        printf("margin-right: %f, ", m_style->marginRight().percent());
    } else if (m_style->marginRight().isAuto()) {
        printf("margin-right: auto, ");
    }

    // padding-top
    if (m_style->paddingTop().isFixed()) {
        printf("padding-top: %f, ", m_style->paddingTop().fixed());
    } else if (m_style->paddingTop().isPercent()) {
        printf("padding-top: %f, ", m_style->paddingTop().percent());
    } else {
        printf("padding-top: not computed yet,");
    }

    // padding-right
    if (m_style->paddingRight().isFixed()) {
        printf("padding-right: %.2f, ", m_style->paddingRight().fixed());
    } else if (m_style->paddingRight().isPercent()) {
        printf("padding-right: %.2fp, ", m_style->paddingRight().percent());
    } else {
        printf("padding-right: not computed yet,");
    }

    // padding-bottom
    if (m_style->paddingBottom().isFixed()) {
        printf("padding-bottom: %f, ", m_style->paddingBottom().fixed());
    } else if (m_style->paddingBottom().isPercent()) {
        printf("padding-bottom: %f, ", m_style->paddingBottom().percent());
    } else {
        printf("padding-bottom: not computed yet,");
    }

    // padding-left
    if (m_style->paddingLeft().isFixed()) {
        printf("padding-left: %f, ", m_style->paddingLeft().fixed());
    } else if (m_style->paddingLeft().isPercent()) {
        printf("padding-left: %f, ", m_style->paddingLeft().percent());
    } else {
        printf("padding-left: not computed yet,");
    }

    // opacity
    printf("opacity: %.1f, ", m_style->opacity());

    // overflow-x
    if (m_style->overflow() == OverflowValue::VisibleOverflow) {
        printf("overflow: visible, ");
    } else {
        printf("overflow: hidden, ");
    }

    // visibility
    if (m_style->visibility() == VisibilityValue::VisibleVisibilityValue) {
        printf("visibility: visible, ");
    } else {
        printf("visibility: hidden, ");
    }

    printf("z-index : %d, ", (int)m_style->zIndex());

    // transform
    if (m_style->uncheckedTransforms() == NULL) {
        printf("transform : none, ");
    } else {
        printf("transform : %s, ",
               m_style->uncheckedTransforms()->dumpString()->utf8Data());
    }

    if (m_style->transformOrigin() == NULL) {
        printf("transform-origin : '', ");
    } else {
        printf("transform-origin : %s",
               m_style->transformOrigin()->dumpString()->utf8Data());
    }

    printf("}");
}
#endif
}
