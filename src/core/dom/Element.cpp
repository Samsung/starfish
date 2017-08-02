/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "core/dom/Attr.h"
#include "core/dom/Document.h"
#include "core/dom/DocumentFragment.h"
#include "core/dom/DOMException.h"
#include "core/dom/DOMPoint.h"
#include "core/dom/DOMQuad.h"
#include "core/dom/DOMRect.h"
#include "core/dom/DOMRectList.h"
#include "core/dom/DOMTokenList.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLDocument.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/NamedNodeMap.h"
#include "core/dom/Text.h"
#include "core/dom/PseudoElementData.h"
#include "core/dom/parser/HTMLParser.h"
#include "core/dom/parser/HTMLParserIdioms.h"
#include "core/dom/xml/XMLSerializer.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBox.h"
#include "core/layout/FrameBlockBox.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"

namespace StarFish {

String* Element::tagName()
{
    if (document()->isXMLDocument()) {
        return localName();
    } else {
        return localName()->toUpper();
    }
}

Nullable<String*> Element::namespaceURI()
{
    auto v = name().namespaceURI();
    if (v.hasValue()) {
        return v.getValue().string();
    } else {
        return Nullable<String*>();
    }
}

String* Element::nodeName()
{
    return tagName();
}

Nullable<String*> Element::prefix()
{
    auto v = name().prefix();
    if (v.hasValue()) {
        return v.getValue().string();
    } else {
        return Nullable<String*>();
    }
}

String* Element::localName()
{
    return name().localName();
}

size_t Element::hasAttribute(const AttributeName& name)
{
    for (size_t i = 0; i < m_attributes.size(); i++) {
        if (name.equals(m_attributes[i].name())) {
            return i;
        }
    }
    return SIZE_MAX;
}

bool Element::hasAttribute(String* name)
{
    AttributeName attrName(document(), name);
    return hasAttribute(attrName) != SIZE_MAX;
}

bool Element::hasAttributeNS(Nullable<String*> ns, String* localName)
{
    AttributeName attrName(document(), ns, localName, AttributeName::MatchNS);
    return hasAttribute(attrName) != SIZE_MAX;
}

size_t Element::hasAttributeNode(const AttributeName& name)
{
    if (hasRareMembers() && rareMembers()->isRareElementMembers() &&
        rareMembers()->asRareElementMembers()->m_attrList) {
        GCVector<Attr*>* l = rareMembers()->asRareElementMembers()->m_attrList;
        size_t len = l->size();
        for (size_t i = 0; i < len; i++) {
            if (name.equals((*l)[i]->qname())) {
                return i;
            }
        }
    }
    return SIZE_MAX;
}

Nullable<String*> Element::getAttribute(const AttributeName& name)
{
    size_t idx = hasAttribute(name);
    if (idx == SIZE_MAX) {
        return Nullable<String*>();
    }
    return Nullable<String*>(m_attributes[idx].value());
}

Nullable<String*> Element::getAttribute(String* name)
{
    AttributeName attrName(document(), name);
    return getAttribute(attrName);
}

Nullable<String*> Element::getAttributeNS(Nullable<String*> ns,
                                          String* localName)
{
    AttributeName attrName(document(), ns, localName, AttributeName::MatchNS);
    return getAttribute(attrName);
}

Attr* Element::getAttributeNode(const AttributeName& name)
{
    size_t idx = hasAttribute(name);
    if (idx == SIZE_MAX) {
        return nullptr;
    }
    // NOTE Should use attribute's QualifiedName here
    const Attribute& attribute = m_attributes[idx];
    return ensureAttr(attribute.name());
}

Attr* Element::getAttributeNode(String* qualifiedName)
{
    AttributeName attrName(document(), qualifiedName);
    return getAttributeNode(attrName);
}

Attr* Element::getAttributeNodeNS(Nullable<String*> ns, String* localName)
{
    AttributeName attrName(document(), ns, localName, AttributeName::MatchNS);
    return getAttributeNode(attrName);
}

String* Element::getAttributeOrEmpty(const AttributeName& name)
{
    Nullable<String*> result = getAttribute(name);
    if (result.hasValue()) {
        return result.getValue();
    }
    return String::emptyString;
}

void Element::setAttribute(const AttributeName& name, String* value)
{
    STARFISH_ASSERT(name.qname().localName()->length());
    size_t idx = hasAttribute(name);
    if (idx == SIZE_MAX) {
        m_attributes.push_back(Attribute(name.qname(), value));
        didAttributeChanged(name.qname(), String::emptyString, value, true,
                            false);
    } else {
        if (name.isNamespaceAware()) {
            // If an attribute with the same local name and namespace URI is
            // already present on the element, its prefix is changed to be
            // the prefix part of the qualifiedName.
            m_attributes[idx].name().copyPrefixFrom(name.qname());
        }
        String* v = m_attributes[idx].value();
        m_attributes[idx].setValue(value);
        didAttributeChanged(name.qname(), v, value, false, false);
    }
}

void Element::setAttribute(String* name, String* value)
{
    if (!QualifiedName::checkNameProductionRule(name)) {
        throw new DOMException(document(),
                               DOMException::Code::INVALID_CHARACTER_ERR);
    }
    setAttribute(AttributeName(document(), name), value);
}

void Element::setAttributeNS(Nullable<String*> ns, String* qualifiedName,
                             String* value)
{
    QualifiedName qname =
        document()->validateAndExtractQualifiedName(ns, qualifiedName);
    setAttribute(AttributeName(qname, AttributeName::MatchNS), value);
}

Attr* Element::setAttributeNode(Attr* newAttr)
{
    RareElementMembers* rareMembers = ensureRareElementMembers();
    STARFISH_ASSERT(rareMembers->isRareElementMembers());
    if (!rareMembers->m_attrList) {
        rareMembers->m_attrList = new (GC) GCVector<Attr*>();
    }

    size_t attrIdx = hasAttributeNode(newAttr->qname());
    Attr* oldAttr =
        attrIdx == SIZE_MAX ? nullptr : (*rareMembers->m_attrList)[attrIdx];
    if (oldAttr == newAttr) {
        return newAttr;
    }

    if (newAttr->ownerElement()) {
        throw new DOMException(document(), DOMException::INUSE_ATTRIBUTE_ERR,
                               "The node provided is an attribute node that is "
                               "already an attribute of another Element; "
                               "attribute nodes must be explicitly cloned.");
    }

    size_t idx = hasAttribute(newAttr->qname());
    String* oldValue = String::emptyString;
    String* newValue = newAttr->value();
    if (idx != SIZE_MAX) {
        Attribute& attribute = m_attributes[idx];
        if (oldAttr) {
            STARFISH_ASSERT(oldAttr->qname() == newAttr->qname());
            oldAttr->detachFromElement(attribute.value());
            (*rareMembers->m_attrList)[attrIdx] = newAttr;
        } else {
            oldAttr = new Attr(document(), newAttr->qname(), attribute.value());
            rareMembers->m_attrList->push_back(newAttr);
        }
        oldValue = attribute.value();
        attribute.setValue(newValue);
    } else {
        m_attributes.push_back(Attribute(newAttr->qname(), newValue));
        rareMembers->m_attrList->push_back(newAttr);
    }
    didAttributeChanged(newAttr->qname(), oldValue, newValue, idx == SIZE_MAX,
                        false);
    newAttr->attachToElement(this, String::emptyString);
    return oldAttr;
}

Attr* Element::setAttributeNodeNS(Attr* attrNode)
{
    // Seem to have no difference.
    return setAttributeNode(attrNode);
}

void Element::removeAttribute(const AttributeName& name)
{
    size_t idx = hasAttribute(name);
    if (idx != SIZE_MAX) {
        String* v = m_attributes[idx].value();
        m_attributes.erase(m_attributes.begin() + idx);
        // Remove Attr if exist
        size_t attrIdx = hasAttributeNode(name);
        if (attrIdx != SIZE_MAX) {
            STARFISH_ASSERT(hasRareMembers());
            STARFISH_ASSERT(rareMembers()->isRareElementMembers());
            STARFISH_ASSERT(rareMembers()->asRareElementMembers()->m_attrList);
            auto l = rareMembers()->asRareElementMembers()->m_attrList;
            Attr* attrNode = (*l)[attrIdx];
            attrNode->detachFromElement(v);
            l->erase(l->begin() + attrIdx);
        }
        didAttributeChanged(name.qname(), v, String::emptyString, false, true);
    }
}

void Element::removeAttribute(String* name)
{
    removeAttribute(AttributeName(document(), name));
}

void Element::removeAttributeNS(Nullable<String*> ns, String* localName)
{
    removeAttribute(AttributeName(document(), ns, localName));
}

Attr* Element::removeAttributeNode(Attr* attr)
{
    STARFISH_ASSERT(attr);
    if (attr->ownerElement() != this) {
        throw new DOMException(
            document(), DOMException::NOT_FOUND_ERR,
            "The node provided is owned by another element.");
    }
    STARFISH_ASSERT(hasAttribute(attr->qname()) != SIZE_MAX);
    removeAttribute(attr->qname());

    return attr;
}

void Element::didAttributeChanged(QualifiedName name, String* old,
                                  String* value, bool attributeCreated,
                                  bool attributeRemoved)
{
#ifdef STARFISH_TC_COVERAGE
    if (name.localName()->equals("style")) {
        STARFISH_LOG_INFO("+++attr:&&&style\n");
    } else {
        STARFISH_LOG_INFO("+++attr:%s\n", name.localName()->utf8Data());
    }
#endif

    StaticStrings* ss = starFish()->staticStrings();
    if (name == ss->m_id) {
        if (attributeRemoved) {
            m_id = AtomicString::emptyAtomicString();
        } else {
            m_id = AtomicString::createAtomicString(starFish(), value);
        }

        // Style should be recalculated from this node to decendants because of
        // combinators.
        setNeedsStyleRecalc();

        document()->invalidNamedAccessCacheIfNeeded();
    } else if (name == ss->m_class) {
        GCVector<StringView> tokens = DOMTokenList::tokenize(value);
        m_classNames.clear();
        for (size_t i = 0; i < tokens.size(); i++) {
            m_classNames.push_back(
                AtomicString::createAtomicString(starFish(), tokens[i]));
        }

        // Style should be recalculated from this node to decendants because of
        // combinators.
        setNeedsStyleRecalc();

        // propagate invalidate nodeList cache(getElementsByClassName) damage to
        // parent tree
        Node* parent = parentNode();
        while (parent) {
            parent->invalidateNodeListCacheDueToChangeClassNameOfDescendant();
            parent = parent->parentNode();
        }
    } else if (name == ss->m_style) {
        if (attributeCreated) {
            registerInlineStyleCallback();
        }
        inlineStyle()->clear();

        if (value->isEmpty()) {
            setNeedsStyleRecalc();
        } else {
            CSSParser parser(document());
            parser.parseStyleDeclaration(value, inlineStyle());
        }
        m_didInlineStyleModifiedAfterAttributeSet = false;
    } else if (name == ss->m_name) {
        // TODO we should not always invalidate cache
        // according spec,
        // https://html.spec.whatwg.org/multipage/browsers.html#named-access-on-the-window-object
        // only few html elements are affected by name attribute changing
        document()->invalidNamedAccessCacheIfNeeded();
    } else if (name == ss->m_tabindex) {
        int tabIndex = 0;
        if (!value->isEmpty() && parseHTMLInteger(value, tabIndex)) {
            setTabIndex(tabIndex, true);
        }
    }

    // The 'content' property is used with ::before and ::after pseudo-elements
    // to generate content in a document. This property supports attr(X)
    // function and this function returns as a string the value of attribute X.
    // Since these pseudo-elements are generated during the creation of the
    // frame tree, if the attribute is changed, the frame tree of the
    // corresponding node should be rebuilt.
    if (hasPseudoElement(
            StyleResolver::PseudoElementType::PseudoElementBefore) ||
        hasPseudoElement(
            StyleResolver::PseudoElementType::PseudoElementAfter)) {
        setNeedsFrameTreeBuild();
    }
}

LayoutRect Element::clientRect()
{
    window()->browsingContext()->webView()->layoutIfNeeds();
    if (frame()) {
        if (frame()->isFrameBox()) {
            FrameBox* box = frame()->asFrameBox();
            return LayoutRect(box->borderLeft(), box->borderTop(),
                              box->contentWidth() + box->paddingWidth(),
                              box->contentHeight() + box->paddingHeight());
        }
    }
    return LayoutRect(0, 0, 0, 0);
}

uint32_t Element::clientLeft()
{
    return (float)clientRect().x() + .5f;
}

uint32_t Element::clientTop()
{
    return (float)clientRect().y() + .5f;
}

uint32_t Element::clientWidth()
{
    return (float)clientRect().width() + .5f;
}

uint32_t Element::clientHeight()
{
    return (float)clientRect().height() + .5f;
}

void Element::getClientQuads(GCVector<DOMQuad*>& quads)
{
    Frame* frameObject = this->frame();
    if (!frameObject) {
        return;
    }
    // TODO : support SVG model
    // there is Getting bounding rectangle from the SVG model in the spec, but
    // SVG model is not supported

    if (frameObject->isFrameBox() &&
        frameObject->style()->display() == DisplayValue::BlockDisplayValue) {
        LayoutRect rect = frameObject->asFrameBox()->absoluteRect(
            document()->frame()->asFrameBox());

        DOMQuad* q = new DOMQuad(
            document(), DOMPointInit(rect.location().x(), rect.location().y()),
            DOMPointInit(rect.location().x() + rect.size().width(),
                         rect.location().y()),
            DOMPointInit(rect.location().x() + rect.size().width(),
                         rect.location().y() + rect.size().height()),
            DOMPointInit(rect.location().x(),
                         rect.location().y() + rect.size().height()));

        quads.push_back(q);
    } else {
        if (frameObject->isFrameInline()) {
            Frame* nearestFrameBox = frameObject->parent();
            while (!nearestFrameBox->isFrameBox()) {
                nearestFrameBox = nearestFrameBox->parent();
            }

            if (nearestFrameBox) {
                FrameBox* box = nearestFrameBox->asFrameBox();
                box->iterateChildFrameBox([&](FrameBox* childBox) {
                    if (childBox->isInlineNonReplacedBox()) {
                        if (childBox->asInlineNonReplacedBox()
                                ->origin()
                                ->node() == this) {
                            LayoutRect rect = childBox->absoluteRect(
                                document()->frame()->asFrameBox());

                            DOMQuad* q = new DOMQuad(
                                document(), DOMPointInit(rect.location().x(),
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
                    }
                });
            }
        } else {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }
    }
    return;
}

DOMRectList* Element::getClientRects()
{
    GCVector<DOMQuad*> quads;
    getClientQuads(quads);

    if (quads.empty()) {
        return DOMRectList::create(document());
    }

    // TODO : Apply the transforms
    return DOMRectList::create(document(), quads);
}

DOMRect* Element::getBoundingClientRect()
{
    GCVector<DOMQuad*> quads;
    getClientQuads(quads);
    if (quads.empty()) {
        return new DOMRect(document());
    }

    DOMRect* rect = quads[0]->getBounds();

    for (size_t i = 1; i < quads.size(); ++i) {
        rect->unite(quads[i]->getBounds());
    }

    // TODO : Apply the transforms
    return rect;
}

String* Element::innerHTML()
{
    return XMLSerializer::serializeToXML(this, false);
}

// https://w3c.github.io/DOM-Parsing/#dfn-fragment-parsing-algorithm
static DocumentFragment* fragmentParsingAlgorithm(Document* document,
                                                  String* src,
                                                  Element* contextElement)
{
    DocumentFragment* df = document->createDocumentFragment();
    HTMLParser parser(document->starFish(), df, contextElement, src);
    parser.startParse();
    parser.parseStep();
    parser.endParse();
    return df;
}

void Element::setInnerHTML(String* html)
{
    while (firstChild()) {
        removeChild(firstChild());
    }

    DocumentFragment* df = fragmentParsingAlgorithm(document(), html, this);
    appendChild(df);
}

String* Element::outerHTML()
{
    return XMLSerializer::serializeToXML(this, true);
}

void Element::setOuterHTML(String* text)
{
    // Let parent be the context object's parent.
    Node* parent = parentNode();
    // If parent is null, terminate these steps. There would be no way to obtain
    // a reference to the nodes created even if the remaining steps were run.
    if (parent == nullptr) {
        return;
    }
    // If parent is a Document, throw a "NoModificationAllowedError"
    // DOMException.
    if (parent == document()) {
        throw new DOMException(document(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "Parent can not be document");
    }
    // If parent is a DocumentFragment, let parent be a new Element with:
    if (parent->isDocumentFragment()) {
        // body as its local name,
        // The HTML namespace as its namespace, and
        // The context object's node document as its node document.
        parent = new HTMLBodyElement(document());
    }
    // Let fragment be the result of invoking the fragment parsing algorithm
    // with the new value as markup, and parent as the context element.
    DocumentFragment* fragment =
        fragmentParsingAlgorithm(document(), text, parent->asElement());
    // Replace the context object with fragment within the context object's
    // parent.
    parentNode()->replaceChild(fragment, this);
}

// https://w3c.github.io/DOM-Parsing/#dom-element-insertadjacenthtml
void Element::insertAdjacentHTML(String* position, String* text)
{
    Element* context = nullptr;
    if (position->equalsWithoutCase("beforebegin") ||
        position->equalsWithoutCase("afterend")) {
        context = parentElement();
        // If context is null or a Document, throw a
        // "NoModificationAllowedError" DOMException.
        if (context == nullptr || context->isDocument()) {
            throw new DOMException(document(),
                                   DOMException::NO_MODIFICATION_ALLOWED_ERR,
                                   "Can not execute `insertAdjacentHTML`.");
        }
    } else if (position->equalsWithoutCase("afterbegin") ||
               position->equalsWithoutCase("beforeend")) {
        context = this;
    } else {
        throw new DOMException(document(), DOMException::SYNTAX_ERR,
                               "The first parameter is not one of "
                               "'beforeBegin', 'afterBegin', 'beforeEnd', or "
                               "'afterEnd'.");
    }

    // If context is not an Element or the following are all true:
    if (!context->isElement() ||
        (
            // context's node document is an HTML document,
            context->document()->isHTMLDocument() &&
            // context's local name is "html", and
            context->localName()->equals("html") &&
            // context's namespace is the HTML namespace;
            context->name().hasSameNamespaceURI(HTML_NAMESPACE))) {
        // let context be a new Element with
        // body as its local name,
        // The HTML namespace as its namespace, and
        // The context object's node document as its node document.
        context = new HTMLBodyElement(document());
    }

    DocumentFragment* df = fragmentParsingAlgorithm(document(), text, context);

    Element* contextObject = this;
    if (position->equalsWithoutCase("beforebegin")) {
        // If position is an ASCII case-insensitive match for the string
        // "beforebegin"
        // Insert fragment into the context object's parent before the context
        // object.
        contextObject->parentNode()->insertBefore(df, contextObject);
    } else if (position->equalsWithoutCase("afterbegin")) {
        // If position is an ASCII case-insensitive match for the string
        // "afterbegin"
        // Insert fragment into the context object before its first child.
        contextObject->insertBefore(df, firstChild());
    } else if (position->equalsWithoutCase("beforeend")) {
        // If position is an ASCII case-insensitive match for the string
        // "beforeend"
        // Append fragment to the context object.
        contextObject->appendChild(df);
    } else {
        STARFISH_ASSERT(position->equalsWithoutCase("afterend"));
        // If position is an ASCII case-insensitive match for the string
        // "afterend"
        // Insert fragment into the context object's parent before the context
        // object's next sibling.
        contextObject->parentNode()->insertBefore(df,
                                                  contextObject->nextSibling());
    }
}

// https://dom.spec.whatwg.org/#dom-element-insertadjacentelement
// To insert adjacent, given an element element, string where, and a node node
static Node* insertAdjacent(Element* element, String* where, Node* node)
{
    // run the steps associated with the first ASCII case-insensitive match for
    // where:
    if (where->equalsWithoutCase("beforebegin")) {
        // If element’s parent is null, return null.
        if (element->parentNode() == nullptr)
            return nullptr;
        // Return the result of pre-inserting node into element’s parent before
        // element.
        return element->parentNode()->insertBefore(node, element);
    } else if (where->equalsWithoutCase("afterbegin")) {
        // Return the result of pre-inserting node into element before element’s
        // first child.
        return element->insertBefore(node, element->firstChild());
    } else if (where->equalsWithoutCase("beforeend")) {
        // Return the result of pre-inserting node into element before null.
        return element->insertBefore(node, nullptr);
    } else if (where->equalsWithoutCase("afterend")) {
        // If element’s parent is null, return null.
        if (element->parentNode() == nullptr)
            return nullptr;
        // Return the result of pre-inserting node into element’s parent before
        // element’s next sibling.
        return element->parentNode()->insertBefore(node,
                                                   element->nextSibling());
    } else {
        throw new DOMException(element->document(), DOMException::SYNTAX_ERR,
                               "The first parameter is not one of "
                               "'beforeBegin', 'afterBegin', 'beforeEnd', or "
                               "'afterEnd'.");
    }
}

Node* Element::insertAdjacentElement(String* where, Element* element)
{
    // The insertAdjacentElement(where, element) method, when invoked, must
    // return the result of running insert adjacent, given context object,
    // where, and element.
    return insertAdjacent(this, where, element);
}

void Element::insertAdjacentText(String* where, String* data)
{
    // Let text be a new Text node whose data is data and node document is
    // context object’s node document.
    Text* text = new Text(document(), data);
    // Run insert adjacent, given context object, where, and text.
    insertAdjacent(this, where, text);
}

Node* Element::clone()
{
    Element* newNode = nullptr;
    if (isHTMLElement()) {
        newNode = HTMLDocument::createHTMLElement(document(),
                                                  name().localNameAtomic());
    } else {
        newNode = new NamedElement(document(), name());
    }

    STARFISH_ASSERT(newNode);

    for (const Attribute& attr : m_attributes) {
        newNode->setAttribute(attr.name(), attr.value());
    }

    return newNode;
}

NamedNodeMap* Element::attributes()
{
    RareElementMembers* rareMembers = ensureRareElementMembers();
    if (!rareMembers->m_namedNodeMap) {
        rareMembers->m_namedNodeMap = new NamedNodeMap(this);
    }
    return rareMembers->m_namedNodeMap;
}

RareNodeMembers* Element::ensureRareMembers()
{
    if (!hasRareMembers()) {
        m_rareNodeMembers = new RareElementMembers();
    }
    STARFISH_ASSERT(m_rareNodeMembers->isRareElementMembers());
    return m_rareNodeMembers;
}

RareElementMembers* Element::ensureRareElementMembers()
{
    RareNodeMembers* rareMembers = ensureRareMembers();
    STARFISH_ASSERT(rareMembers->isRareElementMembers());
    return rareMembers->asRareElementMembers();
}

Attr* Element::attr(QualifiedName name)
{
    STARFISH_ASSERT(
        (hasRareMembers() && rareMembers()->isRareElementMembers()) ||
        !hasRareMembers());
    if (hasRareMembers() && rareMembers()->asRareElementMembers()->m_attrList) {
        auto attrList = rareMembers()->asRareElementMembers()->m_attrList;
        for (Attr* item : *attrList) {
            STARFISH_ASSERT(item);
            if (item->qname() == name) {
                return item;
            }
        }
    }
    return nullptr;
}

Attr* Element::ensureAttr(QualifiedName name)
{
    STARFISH_ASSERT(hasAttribute(name) != SIZE_MAX);
    Attr* returnAttr = attr(name);
    if (!returnAttr) {
        RareElementMembers* rareMembers = ensureRareElementMembers();
        STARFISH_ASSERT(rareMembers->isRareElementMembers());
        if (!rareMembers->m_attrList) {
            rareMembers->m_attrList = new (GC) GCVector<Attr*>();
        }
        returnAttr = new Attr(document(), this, name);
        rareMembers->m_attrList->push_back(returnAttr);
    }
    return returnAttr;
}

bool Element::hasPseudoElements()
{
    RareElementMembers* rareMembers = ensureRareElementMembers();
    return (rareMembers->m_pseudoElementData &&
            rareMembers->m_pseudoElementData->hasPseudoElements());
}

bool Element::hasPseudoElement(StyleResolver::PseudoElementType type)
{
    RareElementMembers* rareMembers = ensureRareElementMembers();
    return (rareMembers->m_pseudoElementData &&
            rareMembers->m_pseudoElementData->hasPseudoElement(type));
}

PseudoElement* Element::pseudoElement(StyleResolver::PseudoElementType type)
{
    RareElementMembers* rareMembers = ensureRareElementMembers();
    return rareMembers->m_pseudoElementData
               ? rareMembers->m_pseudoElementData->pseudoElement(type)
               : nullptr;
}

void Element::setPseudoElement(StyleResolver::PseudoElementType type,
                               PseudoElement* pseudoElement)
{
    RareElementMembers* rareMembers = ensureRareElementMembers();
    if (!rareMembers->m_pseudoElementData) {
        rareMembers->m_pseudoElementData = new PseudoElementData();
    }
    rareMembers->m_pseudoElementData->setPseudoElement(type, pseudoElement);
}

void Element::clearPseudoElements()
{
    RareElementMembers* rareMembers = ensureRareElementMembers();
    if (rareMembers->m_pseudoElementData) {
        rareMembers->m_pseudoElementData->clearPseudoElements();
    }
}

void Element::setId(String* id)
{
    setAttribute(starFish()->staticStrings()->m_id, id);
}

String* Element::className()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_class);
}

void Element::setClassName(String* className)
{
    setAttribute(starFish()->staticStrings()->m_class, className);
}

void Element::setStyleAttr(String* style)
{
    setAttribute(starFish()->staticStrings()->m_style, style);
}

void Element::registerInlineStyleCallback()
{
    attributeData(starFish()->staticStrings()->m_style)
        .registerGetterCallback(
            this, [](Element* element, const Attribute* const attr) -> String* {
                if (element->m_didInlineStyleModifiedAfterAttributeSet) {
                    return element->inlineStyle()->generateCSSText();
                } else {
                    return attr->valueWithoutCheckGetter();
                }
                return String::emptyString;
            });
}

void Element::notifyInlineStyleChanged()
{
    setNeedsStyleRecalc();
    m_didInlineStyleModifiedAfterAttributeSet = true;
    if (hasAttribute(starFish()->staticStrings()->m_style) == SIZE_MAX) {
        m_attributes.push_back(Attribute(starFish()->staticStrings()->m_style,
                                         String::emptyString));
        registerInlineStyleCallback();
    }
}

CSSStyleDeclaration* Element::inlineStyle()
{
    if (m_inlineStyle == nullptr) {
        m_inlineStyle = new InlineCSSStyleDeclaration(
            this, CSSStyleDeclaration::InlineStyle);
    }
    return m_inlineStyle;
}

String* Element::getLaunguage()
{
    Node* n = this;
    String* value = String::emptyString;

    do {
        if (n->isElement()) {
            value = n->asElement()->getAttributeOrEmpty(
                n->starFish()->staticStrings()->m_lang);
        } else if (n->isDocument()) {
            // TODO: checking the MIME content-language
            // value = document()->contentLanguage();
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }

        n = n->parentNode();
    } while (n && value->equals(String::emptyString));

    return value;
}

void Element::setFocus(bool flag)
{
    if (flag == focused()) {
        return;
    }

    setFocused(flag);
    // TODO: Style should be recalculated when we implement :focus selector or
    // apply visual effects for focusable elements.
    // setNeedsStyleRecalc();
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

bool Element::supportsFocus() const
{
    if (!tabIndexSetExplicitly()) {
        return false;
    }
    return true;
}

bool Element::isFocusable()
{
    // TODO: https://www.w3.org/TR/html5/editing.html#focus-management
    if (!supportsFocus()) {
        return false;
    }
    return true;
}

int Element::tabIndex() const
{
    return m_tabIndex;
}

void Element::setTabIndex(int index, bool setExplicitly)
{
    m_tabIndex = index;
    m_tabIndexWasSetExplicitly = setExplicitly;
}

bool Element::tabIndexSetExplicitly() const
{
    return m_tabIndexWasSetExplicitly;
};

void Element::focus()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void Element::blur()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}
}
