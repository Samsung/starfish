/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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
#include "Node.h"

#include "Document.h"
#include "DocumentType.h"
#include "Element.h"
#include "Traverse.h"
#include "NodeList.h"

#include "layout/Frame.h"
#include "layout/FrameTreeBuilder.h"

namespace StarFish {

HTMLCollection* RareNodeMembers::hasQueryInActiveHtmlCollectionList(ActiveHTMLCollectionList* list, String* query)
{
    for (size_t i = 0; i < list->size(); i ++) {
        if (list->at(i).first->equals(query)) {
            return list->at(i).second;
        }
    }
    return nullptr;
}

void RareNodeMembers::putActiveHtmlCollectionListWithQuery(ActiveHTMLCollectionList* list, String* query, HTMLCollection* coll)
{
    STARFISH_ASSERT(!hasQueryInActiveHtmlCollectionList(list, query));
    STARFISH_ASSERT(query);
    STARFISH_ASSERT(coll);
    list->push_back(std::make_pair(query, coll));
}

void RareNodeMembers::invalidateActiveActiveNodeListCacheIfNeeded()
{
    if (m_children) {
        m_children->activeNodeList().invalidateCache();
    }

    if (m_activeHtmlCollectionListsForTagName) {
        for (size_t i = 0; i < m_activeHtmlCollectionListsForTagName->size(); i ++) {
            m_activeHtmlCollectionListsForTagName->at(i).second->activeNodeList().invalidateCache();
        }
    }

    if (m_activeHtmlCollectionListsForClassName) {
        for (size_t i = 0; i < m_activeHtmlCollectionListsForClassName->size(); i ++) {
            m_activeHtmlCollectionListsForClassName->at(i).second->activeNodeList().invalidateCache();
        }
    }

    if (m_childNodeList) {
        m_childNodeList->activeNodeList().invalidateCache();
    }
}

Node::Node(Document* document)
    : EventTarget()
{
    m_document = document;
    initNode();
}

NodeList* Node::childNodes()
{
    STARFISH_ASSERT(m_document);
    auto rareData = ensureRareMembers();
    if (rareData->m_childNodeList == nullptr) {
        rareData->m_childNodeList = new NodeList(m_document->scriptBindingInstance(), this, [](Node* node, void* data) {
            return node->parentNode() == (Node *)data? true: false;
        }, this, true);
    }
    return rareData->m_childNodeList;
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
        if (!(thisNode->nodeName()->equals(otherNode->nodeName())
            && thisNode->publicId()->equals(otherNode->publicId())
            && thisNode->systemId()->equals(otherNode->systemId()))) {
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
        if (!nodeValue()->equals(other->nodeValue())) {
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

Element* Node::firstElementChild()
{
    Node* ret = Traverse::firstChild(this, [](Node* child) {
        if (child->isElement())
            return true;
        else
            return false;
    });
#ifndef NDEBUG
    // if debug mode, we can run-time type check
    if (ret)
        return ret->asElement();
    else
        return nullptr;
#else
    return ret->asElement();
#endif
}

Element* Node::lastElementChild()
{
    Node* ret = Traverse::lastChild(this, [](Node* child) {
        if (child->isElement())
            return true;
        else
            return false;
    });
#ifndef NDEBUG
    // if debug mode, we can run-time type check
    if (ret)
        return ret->asElement();
    else
        return nullptr;
#else
    return ret->asElement();
#endif
}

Element* Node::nextElementSibling()
{
    Node* ret = Traverse::nextSibling(this, [](Node* sibling) {
        if (sibling->isElement())
            return true;
        else
            return false;
    });
#ifndef NDEBUG
    // if debug mode, we can run-time type check
    if (ret)
        return ret->asElement();
    else
        return nullptr;
#else
    return ret->asElement();
#endif
}

Element* Node::previousElementSibling()
{
    Node* ret = Traverse::previousSibling(this, [](Node* sibling) {
        if (sibling->isElement())
            return true;
        else
            return false;
    });
#ifndef NDEBUG
    // if debug mode, we can run-time type check
    if (ret)
        return ret->asElement();
    else
        return nullptr;
#else
    return ret->asElement();
#endif
}

unsigned long Node::childElementCount()
{
    return Traverse::childCount(this, [](Node* child) {
        if (child->isElement())
            return true;
        else
            return false;
    });
}

unsigned short isPreceding(const Node* node, const Node* isPrec, const Node* refNode)
{
    if (node == isPrec) {
        return Node::DOCUMENT_POSITION_PRECEDING;
    } else if (node == refNode) {
        return Node::DOCUMENT_POSITION_FOLLOWING;
    }

    for (Node* child = node->firstChild(); child != nullptr; child = child->nextSibling()) {
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
            return DOCUMENT_POSITION_DISCONNECTED + DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC + DOCUMENT_POSITION_PRECEDING;
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

String* Node::lookupNamespacePrefix(String* namespaceUri, Element* element)
{
    if (namespaceUri == nullptr) {
        return String::emptyString;
    }
    if (element->lookupNamespaceURI(prefix())->equals(namespaceUri)) {
        return prefix();
    }
    // Impl here
    return String::emptyString;
}

String* Node::lookupPrefix(String* namespaceUri)
{
    if (!namespaceUri) {
        return String::emptyString;
    }
    if (namespaceUri->equals(String::emptyString)) {
        return String::emptyString;
    }

    switch (nodeType()) {
    case ELEMENT_NODE:
        return lookupNamespacePrefix(namespaceUri, asElement());
    case DOCUMENT_NODE: {
        Element* documentElement = asDocument()->documentElement();
        if (documentElement) {
            return documentElement->lookupPrefix(namespaceUri);
        } else {
            return String::emptyString;
        }
    }
    case DOCUMENT_TYPE_NODE:
    case DOCUMENT_FRAGMENT_NODE:
        return String::emptyString;
    default: {
        Node* parent = parentNode();
        if (parent) {
            parent->lookupPrefix(namespaceUri);
        } else {
            return String::emptyString;
        }
    }
    }
    return String::emptyString;
}

HTMLCollection* Node::children()
{
    if (!hasRareMembers()) {
        ensureRareMembers();
    } else if (m_rareNodeMembers->m_children)
        return m_rareNodeMembers->m_children;

    auto filter = [](Node* node, void* data) -> bool
    {
        if (node->parentNode() == ((Node*)data) && node->isElement())
            return true;
        return false;
    };
    m_rareNodeMembers->m_children = new HTMLCollection(m_document->scriptBindingInstance(), this, filter, this, true);
    return m_rareNodeMembers->m_children;
}

DOMTokenList* Node::classList()
{
    if (isElement()) {
        if (!hasRareMembers()) {
            ensureRareMembers();
        } else if (!m_rareNodeMembers->m_domTokenList)
            return m_rareNodeMembers->m_domTokenList;

        m_rareNodeMembers->m_domTokenList = new DOMTokenList(m_document->scriptBindingInstance(), asElement(),
            document()->window()->starFish()->staticStrings()->m_class);
        return m_rareNodeMembers->m_domTokenList;
    }
    return nullptr;
}

NamedNodeMap* Node::attributes()
{
    return nullptr;
}

Node* Node::getDocTypeChild()
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
        throw new DOMException(m_document->scriptBindingInstance(), DOMException::HIERARCHY_REQUEST_ERR, "Parent is not a Document, DocumentFragment, or Element node.");
    }

    if (node == this) {
        throw new DOMException(m_document->scriptBindingInstance(), DOMException::HIERARCHY_REQUEST_ERR, "Node is a host-including inclusive ancestor of parent.");
    } else {
        for (Node* p = this; p != nullptr; p = p->parentNode()) {
            if (p == node) {
                throw new DOMException(m_document->scriptBindingInstance(), DOMException::HIERARCHY_REQUEST_ERR, "Node is a host-including inclusive ancestor of parent.");
            }
        }
    }

    if (child != nullptr && child->parentNode() != this) {
        throw new DOMException(m_document->scriptBindingInstance(), DOMException::Code::NOT_FOUND_ERR, "Child is not null and its parent is not parent.");
    }
    if (!(node->isDocumentType() || node->isElement() || node->isText() || node->isComment() || node->isDocumentFragment())) {
        throw new DOMException(m_document->scriptBindingInstance(), DOMException::HIERARCHY_REQUEST_ERR, "Node is not a DocumentFragment, DocumentType, Element, Text, ProcessingInstruction, or Comment.");
    }
    if ((node->isText() && isDocument()) || (node->isDocumentType() && !isDocument())) {
        throw new DOMException(m_document->scriptBindingInstance(), DOMException::HIERARCHY_REQUEST_ERR, "Either node is a Text node and parent is a document, or node is a doctype and parent is not a document.");
    }
    if (isDocument()) {
        if (node->isElement()) {
            if ((firstElementChild() != nullptr) || (child != nullptr && child->isElement()) || (child != nullptr && child->nextSibling() != nullptr && child->nextSibling()->isDocumentType())) {
                throw new DOMException(m_document->scriptBindingInstance(), DOMException::HIERARCHY_REQUEST_ERR, "parent has an element child, child is a doctype, or child is not null and a doctype is following child.");
            }
        } else if (node->isDocumentType()) {
            if (getDocTypeChild() || (child != nullptr && child->previousSibling() != nullptr && child->previousSibling()->isElement()) || (child == nullptr && firstElementChild() != nullptr)) {
                throw new DOMException(m_document->scriptBindingInstance(), DOMException::HIERARCHY_REQUEST_ERR, "parent has a doctype child, child is non-null and an element is preceding child, or child is null and parent has an element child.");
            }
        }
    }
}

bool Node::isInDocumentScope()
{
    Node* t = this;
    while (t) {
        if (t->isDocument())
            return true;
        t = t->parentNode();
    }
    return false;
}

bool Node::isInDocumentScopeAndDocumentParticipateInRendering()
{
    Node* t = this;
    while (t) {
        if (t->isDocument())
            return t->asDocument()->doesParticipateInRendering();
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

    self->notifyDOMEventToParentTree(self, [self, child](Node* parent) {
        parent->didNodeInserted(self, child);
    });

    if (self->isInDocumentScope() && self->document()->doesParticipateInRendering()) {
        notifyNodeInsertedToDocumentTree(self, child);
        child->setNeedsStyleRecalc();
        child->setNeedsFrameTreeBuild();
    }

}

Node* Node::appendChild(Node* child)
{
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
        throw new DOMException(m_document->scriptBindingInstance(), DOMException::HIERARCHY_REQUEST_ERR, "Node is null.");
    }

    validatePreinsert(child, childRef);

    if (childRef == nullptr) {
        return appendChild(child);
    }
    if (child == childRef) {
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
        throw new DOMException(m_document->scriptBindingInstance(), DOMException::HIERARCHY_REQUEST_ERR, "Parent is not a Document, DocumentFragment, or Element node.");
    }

    if (child == this) {
        throw new DOMException(m_document->scriptBindingInstance(), DOMException::HIERARCHY_REQUEST_ERR, "Node is a host-including inclusive ancestor of parent.");
    } else {
        for (Node* p = this; p != nullptr; p = p->parentNode()) {
            if (p == child) {
                throw new DOMException(m_document->scriptBindingInstance(), DOMException::HIERARCHY_REQUEST_ERR, "Node is a host-including inclusive ancestor of parent.");
            }
        }
    }

    if (childRef != nullptr && childRef->parentNode() != this) {
        throw new DOMException(m_document->scriptBindingInstance(), DOMException::Code::NOT_FOUND_ERR, "Child is not null and its parent is not parent.");
    }
    if (!(child->isDocumentType() || child->isElement() || child->isText() || child->isComment())) {
        throw new DOMException(m_document->scriptBindingInstance(), DOMException::HIERARCHY_REQUEST_ERR, "Node is not a DocumentFragment, DocumentType, Element, Text, ProcessingInstruction, or Comment.");
    }
    if ((child->isText() && isDocument()) || (child->isDocumentType() && !isDocument())) {
        throw new DOMException(m_document->scriptBindingInstance(), DOMException::HIERARCHY_REQUEST_ERR, "Either node is a Text node and parent is a document, or node is a doctype and parent is not a document.");
    }
    if (isDocument()) {
        if (child->isElement()) {
            if ((firstElementChild() != nullptr) || (childRef != nullptr && childRef->isElement()) || (childRef != nullptr && childRef->nextSibling() != nullptr && childRef->nextSibling()->isDocumentType())) {
                throw new DOMException(m_document->scriptBindingInstance(), DOMException::HIERARCHY_REQUEST_ERR, "parent has an element child that is not child or a doctype is following child.");
            }
        } else if (child->isDocumentType()) {
            if ((getDocTypeChild() != nullptr && getDocTypeChild() != childRef) || (childRef != nullptr && childRef->previousSibling() != nullptr && childRef->previousSibling()->isElement())) {
                throw new DOMException(m_document->scriptBindingInstance(), DOMException::HIERARCHY_REQUEST_ERR, "parent has a doctype child that is not child, or an element is preceding child.");
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
        throw new DOMException(m_document->scriptBindingInstance(), DOMException::NOT_FOUND_ERR, "Child's parent is not parent.");
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

    notifyDOMEventToParentTree(this, [this, child](Node* parent) {
        parent->didNodeRemoved(this, child);
    });

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

    notifyDOMEventToParentTree(this, [this, child](Node* parent) {
        parent->didNodeInserted(this, child);
    });

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

    notifyDOMEventToParentTree(this, [this, child](Node* parent) {
        parent->didNodeRemoved(this, child);
    });
}

void Node::parserInsertBefore(Node* child, Node* childRef)
{
    ASSERT(child);

    if (childRef == nullptr) {
        appendChild(child);
        return;
    }

    ASSERT(childRef->parentNode() == this);
    if (childRef->previousSibling() == child || childRef == child) // nothing to do
        return;

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

    notifyDOMEventToParentTree(this, [this, child](Node* parent) {
        parent->didNodeInserted(this, child);
    });

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

HTMLCollection* Node::getElementsByTagName(QualifiedName qualifiedName)
{
    RareNodeMembers* rareData = ensureRareMembers();
    ActiveHTMLCollectionList* activeLists = rareData->ensureActiveHtmlCollectionListForTagName();
    HTMLCollection* list = rareData->hasQueryInActiveHtmlCollectionList(activeLists, qualifiedName.localName());
    if (list)
        return list;

    auto filter = [](Node* node, void* data)
    {
        QualifiedName* qualifiedName = (QualifiedName*)data;
        if (node->isElement()) {
            if (node->asElement()->name().localNameAtomic() == qualifiedName->localNameAtomic())
                return true;
            if (qualifiedName->localName()->equals("*"))
                return true;
        }
        return false;
    };
    list = new HTMLCollection(document()->scriptBindingInstance(), this, filter, new QualifiedName(qualifiedName), true);
    rareData->putActiveHtmlCollectionListWithQuery(activeLists, qualifiedName.localName(), list);
    return list;
}

HTMLCollection* Node::getElementsByClassName(String* classNames)
{
    auto rareData = ensureRareMembers();
    auto activeLists = rareData->ensureActiveHtmlCollectionListForClassName();
    auto list = rareData->hasQueryInActiveHtmlCollectionList(activeLists, classNames);
    if (list)
        return list;

    auto filter = [](Node* node, void* data) -> bool
    {
        String* classNames = (String*)data;
        if (node->isElement() && node->asElement()->isHTMLElement() && node->asElement()->asHTMLElement()->classNames().size() > 0) {

            size_t length = classNames->length();
            bool isWhiteSpaceState = true;

            UTF32String str;
            for (size_t i = 0; i < length; i ++) {
                char32_t ch = classNames->charAt(i);
                if (isWhiteSpaceState) {
                    if (ch != ' ' && ch != '\n' && ch != '\t' && ch != '\f' && ch != '\r') {
                        isWhiteSpaceState = false;
                        str += ch;
                    }
                } else {
                    if (ch == ' ' || ch == '\n' || ch == '\t' || ch == '\f' || ch == '\r') {
                        isWhiteSpaceState = true;

                        String* tok = new StringDataUTF32(std::move(str));

                        if (!node->asElement()->asHTMLElement()->hasClassName(tok))
                        return false;

                        str.clear();
                    } else {
                        str += ch;
                    }
                }
            }

            if (str.length()) {
                String* tok = new StringDataUTF32(std::move(str));
                if (!node->asElement()->asHTMLElement()->hasClassName(tok))
                return false;
            }

            return true;
        }
        return false;
    };

    list = new HTMLCollection(document()->scriptBindingInstance(), this, filter, classNames, true);
    rareData->putActiveHtmlCollectionListWithQuery(activeLists, classNames, list);
    return list;
}

void Node::setNeedsFrameTreeBuild()
{
    if (!document()->doesParticipateInRendering())
        return;

    Frame* old = frame();
    if (old) {
        Frame* parent = old->parent();
        if (!parent) {
            STARFISH_ASSERT(old->isFrameDocument());
            parent = old;
        } else {
            while (parent) {
                if (parent->isFrameBlockBox() && parent->node()) {
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
            node->m_needsFrameTreeBuild = true;
            node = node->nextSibling();
        }

        node = parent->node();
        while (node) {
            node->setChildNeedsFrameTreeBuild();
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

    m_document->window()->setNeedsFrameTreeBuild();
}

void Node::didComputedStyleChanged(ComputedStyle* oldStyle, ComputedStyle* newStyle)
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
    if (m_rareNodeMembers == nullptr)
        m_rareNodeMembers = new RareNodeMembers();
    STARFISH_ASSERT(!m_rareNodeMembers->isRareElementMembers());
    return m_rareNodeMembers;
}

void Node::invalidateNodeListCacheDueToChangeClassNameOfDescendant()
{
    if (hasRareMembers()) {
        if (m_rareNodeMembers->m_activeHtmlCollectionListsForClassName) {
            for (size_t i = 0; i < m_rareNodeMembers->m_activeHtmlCollectionListsForClassName->size(); i ++) {
                m_rareNodeMembers->m_activeHtmlCollectionListsForClassName->at(i).second->activeNodeList().invalidateCache();
            }
        }
    }
}

#ifdef STARFISH_ENABLE_TEST

CSSStyleDeclaration* Node::getComputedStyle()
{
    CSSStyleDeclaration* d = new CSSStyleDeclaration(document());
    // force render to get computed style
    setNeedsPainting();
    document()->window()->rendering();

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
    ADD_VALUE_PAIR(VerticalAlign, VerticalAlignValueKind, verticalAlign)
    ADD_VALUE_PAIR(TextAlign, SideValueKind, textAlign)
    ADD_VALUE_PAIR(TextDecoration, TextDecorationValueKind, textDecoration)
    ADD_VALUE_PAIR(Direction, DirectionValueKind, direction)
    ADD_VALUE_PAIR(BackgroundRepeatX, BackgroundRepeatValueKind, backgroundRepeatX)
    ADD_VALUE_PAIR(BackgroundRepeatY, BackgroundRepeatValueKind, backgroundRepeatY)
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
        }                                                             \
        d->addValuePair(p);                                           \
    }

    ADD_LENGTH_PAIR(Width, width)
    ADD_LENGTH_PAIR(Height, height)
    ADD_LENGTH_PAIR(LineHeight, lineHeight)
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

    // helper function to convert Length to CSSStyleValuePair format
    auto lengthToCSSStyleValue = [](Length len)
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
            vals->append(item);
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

                CSSStyleValuePair w = lengthToCSSStyleValue(style->bgSizeValue(i).width());
                vals->append(w.valueKind(), w.value());

                CSSStyleValuePair h = lengthToCSSStyleValue(style->bgSizeValue(i).height());
                vals->append(h.valueKind(), h.value());

                item.setValue(vals);
            }
            values->append(item);
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
            CSSStyleValuePair item = lengthToCSSStyleValue(style->backgroundPositionX(i));
            values->append(item);
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
            CSSStyleValuePair item = lengthToCSSStyleValue(style->backgroundPositionY(i));
            values->append(item);
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
        vals->append(t.valueKind(), t.value());

        CSSStyleValuePair r = lengthToCSSStyleValue(box.right());
        vals->append(r.valueKind(), r.value());

        CSSStyleValuePair b = lengthToCSSStyleValue(box.bottom());
        vals->append(b.valueKind(), b.value());

        CSSStyleValuePair l = lengthToCSSStyleValue(box.left());
        vals->append(l.valueKind(), l.value());

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
            vals->append(t.valueKind(), t.value());
        } else {
            vals->append(CSSStyleValuePair::ValueKind::Number, (float)box.top().number());
        }
        if (box.right().isLength()) {
            CSSStyleValuePair r = lengthToCSSStyleValue(box.right().length());
            vals->append(r.valueKind(), r.value());
        } else {
            vals->append(CSSStyleValuePair::ValueKind::Number, (float)box.right().number());
        }
        if (box.bottom().isLength()) {
            CSSStyleValuePair b = lengthToCSSStyleValue(box.bottom().length());
            vals->append(b.valueKind(), b.value());
        } else {
            vals->append(CSSStyleValuePair::ValueKind::Number, (float)box.bottom().number());
        }
        if (box.left().isLength()) {
            CSSStyleValuePair l = lengthToCSSStyleValue(box.left().length());
            vals->append(l.valueKind(), l.value());
        } else {
            vals->append(CSSStyleValuePair::ValueKind::Number, (float)box.bottom().number());
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

            CSSStyleValuePair x = lengthToCSSStyleValue(style->transformOrigin()->originValue()->getXAxis());
            vals->append(x.valueKind(), x.value());

            CSSStyleValuePair y = lengthToCSSStyleValue(style->transformOrigin()->originValue()->getYAxis());
            vals->append(y.valueKind(), y.value());

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
    } else if (m_style->verticalAlign() == VerticalAlignValue::SuperVAlignValue) {
        printf("vertical-align: super, ");
    } else if (m_style->verticalAlign() == VerticalAlignValue::TopVAlignValue) {
        printf("vertical-align: top, ");
    } else if (m_style->verticalAlign() == VerticalAlignValue::TextTopVAlignValue) {
        printf("vertical-align: text-top, ");
    } else if (m_style->verticalAlign() == VerticalAlignValue::MiddleVAlignValue) {
        printf("vertical-align: middle, ");
    } else if (m_style->verticalAlign() == VerticalAlignValue::BottomVAlignValue) {
        printf("vertical-align: bottom, ");
    } else if (m_style->verticalAlign() == VerticalAlignValue::TextBottomVAlignValue) {
        printf("vertical-align: text-bottom, ");
    } else if (m_style->verticalAlignLength().isFixed()) {
        printf("vertical-align: %.2f, ", m_style->verticalAlignLength().fixed());
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
    if (m_style->textDecoration() == TextDecorationValue::UnderLineTextDecorationValue) {
        printf("text-decoration: underline, ");
    } else if (m_style->textDecoration() == TextDecorationValue::OverLineTextDecorationValue) {
        printf("text-decoration: overline, ");
    } else if (m_style->textDecoration() == TextDecorationValue::LineThroughTextDecorationValue) {
        printf("text-decoration: line-through, ");
    } else if (m_style->textDecoration() == TextDecorationValue::BlinkTextDecorationValue) {
        printf("text-decoration: blink, ");
    } else if (m_style->textDecoration() == TextDecorationValue::NoneTextDecorationValue) {
        printf("text-decoration: none, ");
    }

    // direction
    if (m_style->direction() == DirectionValue::LtrDirectionValue)
        printf("direction: ltr, ");
    else
        printf("direction: rtl, ");

    if (m_style->whiteSpace() == WhiteSpaceValue::NormalWhiteSpaceValue)
        printf("white-space: normal, ");
    else
        printf("white-space: nowrap, ");

    // unicode-bidi
    if (m_style->unicodeBidi() == UnicodeBidiValue::NormalUnicodeBidiValue)
        printf("unicode-bidi: normal, ");
    else
        printf("unicode-bidi: embed, ");

    // font-size
    printf("font-size: %.1f, ", m_style->fontSize().fixed());

    // font-style
    printf("font-style: ");
    if (m_style->fontStyle() == FontStyleValue::NormalFontStyleValue)
        printf("normal, ");
    else if (m_style->fontStyle() == FontStyleValue::ItalicFontStyleValue)
        printf("italic, ");
    else if (m_style->fontStyle() == FontStyleValue::ObliqueFontStyleValue)
        printf("oblique, ");

    printf("font-weight: ");
    if (m_style->fontWeight() == FontWeightValue::OneHundredFontWeightValue)
        printf("100, ");
    if (m_style->fontWeight() == FontWeightValue::TwoHundredsFontWeightValue)
        printf("200, ");
    if (m_style->fontWeight() == FontWeightValue::ThreeHundredsFontWeightValue)
        printf("300, ");
    if (m_style->fontWeight() == FontWeightValue::NormalFontWeightValue)
        printf("normal, ");
    if (m_style->fontWeight() == FontWeightValue::FiveHundredsFontWeightValue)
        printf("500, ");
    if (m_style->fontWeight() == FontWeightValue::SixHundredsFontWeightValue)
        printf("600, ");
    if (m_style->fontWeight() == FontWeightValue::BoldFontWeightValue)
        printf("bold, ");
    if (m_style->fontWeight() == FontWeightValue::EightHundredsFontWeightValue)
        printf("800, ");
    if (m_style->fontWeight() == FontWeightValue::NineHundredsFontWeightValue)
        printf("900, ");

    // letter-spacing
    printf("letter-spacing: %f, ", m_style->letterSpacing().fixed());

    // line-height
    if (m_style->lineHeight().isFixed())
        printf("line-height: %.1f, ", m_style->lineHeight().fixed());
    else
        printf("line-height: normal, ");

    // color
    printf("color: (%d,%d,%d,%d), ", m_style->color().r(), m_style->color().g(), m_style->color().b(), m_style->color().a());

    // background-color
    printf("background-color: (%d,%d,%d,%d), ", m_style->backgroundColor().r(), m_style->backgroundColor().g(), m_style->backgroundColor().b(), m_style->backgroundColor().a());

    // background-image
    if (m_style->backgroundImage()->length() == 0) {
        printf("background-image: none, ");
    } else {
        printf("background-image: %s, ", m_style->backgroundImage()->utf8Data());
    }

    // background-position
    printf("background-position: (%s, %s),", m_style->backgroundPositionX().dumpString()->utf8Data(), m_style->backgroundPositionY().dumpString()->utf8Data());

    // background-size
    if (m_style->bgSizeType() == BackgroundSizeType::Cover) {
        printf("background-size: cover, ");
    } else if (m_style->bgSizeType() == BackgroundSizeType::Contain) {
        printf("background-size: contain, ");
    } else if (m_style->bgSizeType() == BackgroundSizeType::SizeValue) {
        printf("background-size: (%s, %s),", m_style->bgSizeValue().width().dumpString()->utf8Data(),
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
    printf("border-top-color: (%d,%d,%d,%d), ", m_style->borderTopColor().r(), m_style->borderTopColor().g(), m_style->borderTopColor().b(), m_style->borderTopColor().a());
    printf("border-right-color: (%d,%d,%d,%d), ", m_style->borderRightColor().r(), m_style->borderRightColor().g(), m_style->borderRightColor().b(), m_style->borderRightColor().a());
    printf("border-bottom-color: (%d,%d,%d,%d), ", m_style->borderBottomColor().r(), m_style->borderBottomColor().g(), m_style->borderBottomColor().b(), m_style->borderBottomColor().a());
    printf("border-left-color: (%d,%d,%d,%d), ", m_style->borderLeftColor().r(), m_style->borderLeftColor().g(), m_style->borderLeftColor().b(), m_style->borderLeftColor().a());

    // border-image-slice
    LengthBox l = m_style->borderImageSlices();
    if (l.top().isPercent())
        printf("border-image-slice: (%.2fp, ", l.top().percent());
    else
        printf("border-image-slice: (%.2f, ", l.top().fixed());
    if (l.right().isPercent())
        printf("%.2fp, ", l.right().percent());
    else
        printf("%.1f, ", l.right().fixed());
    if (l.bottom().isPercent())
        printf("%.2fp, ", l.bottom().percent());
    else
        printf("%.1f, ", l.bottom().fixed());
    if (l.left().isPercent())
        printf("%.2fp, ", l.left().percent());
    else
        printf("%.1f, ", l.left().fixed());
    printf("%d), ", m_style->borderImageSliceFill());

    // border-image-source
    if (!m_style->borderImageSource()->equals(String::emptyString))
        printf("border-image-source: %s, ", m_style->borderImageSource()->utf8Data());

    // border-image-width
    BorderImageLengthBox b = m_style->borderImageWidths();
    printf("border-image-width: (%s, %s, %s, %s), ", b.top().dumpString()->utf8Data(), b.right().dumpString()->utf8Data(), b.bottom().dumpString()->utf8Data(), b.left().dumpString()->utf8Data());

    // border-style
    printf("border-style(t, r, b, l): (");
    if (m_style->borderTopStyle() == BorderStyleValue::NoneBorderStyleValue) {
        printf("none,");
    } else if (m_style->borderTopStyle() == BorderStyleValue::SolidBorderStyleValue) {
        printf("solid,");
    }
    if (m_style->borderRightStyle() == BorderStyleValue::NoneBorderStyleValue) {
        printf("none,");
    } else if (m_style->borderRightStyle() == BorderStyleValue::SolidBorderStyleValue) {
        printf("solid,");
    }
    if (m_style->borderBottomStyle() == BorderStyleValue::NoneBorderStyleValue) {
        printf("none,");
    } else if (m_style->borderBottomStyle() == BorderStyleValue::SolidBorderStyleValue) {
        printf("solid,");
    }
    if (m_style->borderLeftStyle() == BorderStyleValue::NoneBorderStyleValue) {
        printf("none), ");
    } else if (m_style->borderLeftStyle() == BorderStyleValue::SolidBorderStyleValue) {
        printf("solid), ");
    }

    // border-width
    printf("border-width(t, r, b, l): (%.0f, %.0f, %.0f, %.0f), ", m_style->borderTopWidth().fixed(), m_style->borderRightWidth().fixed(), m_style->borderBottomWidth().fixed(), m_style->borderLeftWidth().fixed());

    // background-repeat-x
    if (m_style->backgroundRepeatX() == BackgroundRepeatValue::RepeatRepeatValue)
        printf("background-repeat-x: repeat, ");
    else
        printf("background-repeat-x: no-repeat, ");

    // background-repeat-y
    if (m_style->backgroundRepeatY() == BackgroundRepeatValue::RepeatRepeatValue)
        printf("background-repeat-y: repeat, ");
    else
        printf("background-repeat-y: no-repeat, ");

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
    if (m_style->overflow() == OverflowValue::VisibleOverflow)
        printf("overflow: visible, ");
    else
        printf("overflow: hidden, ");

    // visibility
    if (m_style->visibility() == VisibilityValue::VisibleVisibilityValue)
        printf("visibility: visible, ");
    else
        printf("visibility: hidden, ");

    printf("z-index : %d, ", (int)m_style->zIndex());

    // transform
    if (m_style->uncheckedTransforms() == NULL) {
        printf("transform : none, ");
    } else {
        printf("transform : %s, ", m_style->uncheckedTransforms()->dumpString()->utf8Data());
    }

    if (m_style->transformOrigin() == NULL) {
        printf("transform-origin : '', ");
    } else {
        printf("transform-origin : %s", m_style->transformOrigin()->dumpString()->utf8Data());
    }

    printf("}");
}
#endif
}
