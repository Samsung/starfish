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
#include "core/dom/NamedNodeMap.h"
#include "core/dom/PseudoElementData.h"
#include "core/dom/parser/HTMLParser.h"
#include "core/dom/xml/XMLSerializer.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBox.h"
#include "core/modules/window/Window.h"
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

bool Element::hasAttribute(String* name)
{
    QualifiedName qName = document()->createAttributeName(name);
    return hasAttribute(qName) != SIZE_MAX;
}

size_t Element::hasAttribute(QualifiedName name)
{
    for (size_t i = 0; i < m_attributes.size(); i++) {
        if (m_attributes[i].name() == name) {
            return i;
        }
    }
    return SIZE_MAX;
}

Nullable<String*> Element::getAttribute(String* name)
{
    QualifiedName qName = document()->createAttributeName(name);
    return getAttribute(qName);
}

Nullable<String*> Element::getAttribute(QualifiedName name)
{
    size_t idx = hasAttribute(name);
    if (idx == SIZE_MAX) {
        return Nullable<String*>();
    }
    return Nullable<String*>(m_attributes[idx].value());
}

String* Element::getAttributeOrEmpty(QualifiedName name)
{
    Nullable<String*> result = getAttribute(name);
    if (result.hasValue()) {
        return result.getValue();
    }
    return String::emptyString;
}

void Element::setAttribute(String* name, String* value)
{
    if (!QualifiedName::checkNameProductionRule(name)) {
        throw new DOMException(
            document(), DOMException::Code::INVALID_CHARACTER_ERR, nullptr);
    }
    setAttribute(document()->createAttributeName(name), value);
}

void Element::setAttribute(QualifiedName name, String* value)
{
    size_t idx = hasAttribute(name);
    if (idx == SIZE_MAX) {
        m_attributes.push_back(Attribute(name, value));
        didAttributeChanged(name, String::emptyString, value, true, false);
    } else {
        String* v = m_attributes[idx].value();
        m_attributes[idx].setValue(value);
        didAttributeChanged(name, v, value, false, false);
    }
}

void Element::removeAttribute(String* name)
{
    removeAttribute(document()->createAttributeName(name));
}

void Element::removeAttribute(QualifiedName name)
{
    size_t idx = hasAttribute(name);
    if (idx == SIZE_MAX) {
    } else {
        String* v = m_attributes[idx].value();
        m_attributes.erase(m_attributes.begin() + idx);
        Attr* attrNode = attr(name);
        if (attrNode) {
            attrNode->detachFromElement(v);
            STARFISH_ASSERT(hasRareMembers());
            STARFISH_ASSERT(rareMembers()->isRareElementMembers());
            STARFISH_ASSERT(rareMembers()->asRareElementMembers()->m_attrList);
            auto attrList = rareMembers()->asRareElementMembers()->m_attrList;
            for (unsigned i = 0, size = attrList->size(); i < size; i++) {
                if ((*attrList)[i] == attrNode) {
                    attrList->erase(attrList->begin() + i);
                    break;
                }
            }
        }
        didAttributeChanged(name, v, String::emptyString, false, true);
    }
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
        if (old->equals(String::emptyString)) {
            attributeData(name).registerGetterCallback(
                this,
                [](Element* element, const Attribute* const attr) -> String* {
                    if (element->m_didInlineStyleModifiedAfterAttributeSet) {
                        return element->inlineStyle()->generateCSSText();
                    } else {
                        return attr->valueWithoutCheckGetter();
                    }
                    return String::emptyString;
                });
        }
        inlineStyle()->clear();
        CSSParser parser(document());
        parser.parseStyleDeclaration(value, inlineStyle());
        m_didInlineStyleModifiedAfterAttributeSet = false;
    } else if (name == ss->m_name) {
        // TODO we should not always invalidate cache
        // according spec,
        // https://html.spec.whatwg.org/multipage/browsers.html#named-access-on-the-window-object
        // only few html elements are affected by name attribute changing
        document()->invalidNamedAccessCacheIfNeeded();
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
    window()->layoutIfNeeds();
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

void Element::getClientQuads(std::vector<DOMQuad>& quads)
{
    Frame* frameObject = this->frame();
    if (!frameObject) {
        return;
    }
    // todo : support SVG model
    // there is Getting bounding rectangle from the SVG model in the spec, but
    // SVG model is not supported

    // initial version : implement for display:block
    if (frameObject->isFrameBox() &&
        frameObject->style()->display() == DisplayValue::BlockDisplayValue) {
        LayoutRect rect =
            ((FrameBox*)frameObject)
                ->absoluteRect((FrameBox*)document()->rootElement()->frame());

        DOMQuad* q = new DOMQuad(
            document(), DOMPointInit(rect.location().x(), rect.location().y()),
            DOMPointInit(rect.location().x() + rect.size().width(),
                         rect.location().y()),
            DOMPointInit(rect.location().x() + rect.size().width(),
                         rect.location().y() + rect.size().height()),
            DOMPointInit(rect.location().x(),
                         rect.location().y() + rect.size().height()));

        quads.push_back(*q);
    } else {
        // todo assert
        STARFISH_LOG_ERROR("%s %d\n : implement not yet", __FUNCTION__,
                           __LINE__);
        STARFISH_ASSERT(false);
    }
    return;
}

DOMRectList* Element::getClientRects()
{
    std::vector<DOMQuad> quads;
    getClientQuads(quads);

    if (quads.empty()) {
        return DOMRectList::create(document());
    }

    // todo : Apply the transforms
    return DOMRectList::create(document(), quads);
}

DOMRect* Element::getBoundingClientRect()
{
    std::vector<DOMQuad> quads;
    getClientQuads(quads);
    if (quads.empty()) {
        return new DOMRect(document());
    }

    DOMRect* rect = quads[0].getBounds();

    for (size_t i = 1; i < quads.size(); ++i) {
        rect->unite(quads[i].getBounds());
    }

    // todo : Apply the transforms
    return rect;
}

#ifdef STARFISH_ENABLE_TEST
String* Element::innerHTML()
{
    return XMLSerializer::serializeToXML(this, false);
}

void Element::setInnerHTML(String* html)
{
    while (firstChild()) {
        removeChild(firstChild());
    }

    DocumentFragment* df = document()->createDocumentFragment();

    HTMLParser parser(starFish(), df, this, html);
    parser.startParse();
    parser.parseStep();
    parser.endParse();
    appendChild(df);
}
#endif

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
    newNode->m_inlineStyle = inlineStyle()->clone(newNode);

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

void Element::addAttr(Attr* attr)
{
    RareElementMembers* rareMembers = ensureRareElementMembers();
    STARFISH_ASSERT(rareMembers->isRareElementMembers());
    if (!rareMembers->m_attrList) {
        rareMembers->m_attrList = new (GC) GCVector<Attr*>();
    }
    STARFISH_ASSERT(this->attr(attr->qname()) == nullptr);
    rareMembers->m_attrList->push_back(attr);
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

void Element::setPseudoElement(StyleResolver::PseudoElementType type)
{
    RareElementMembers* rareMembers = ensureRareElementMembers();
    if (!rareMembers->m_pseudoElementData) {
        rareMembers->m_pseudoElementData = new PseudoElementData();
    }
    rareMembers->m_pseudoElementData->setPseudoElement(type);
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

CSSStyleDeclaration* Element::inlineStyle()
{
    if (m_inlineStyle == nullptr) {
        m_inlineStyle =
            new CSSStyleDeclaration(this, CSSStyleDeclaration::InlineStyle);
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
}

bool Element::supportsFocus()
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

void Element::focus()
{
    // TODO
}

void Element::blur()
{
    // TODO
}
}
