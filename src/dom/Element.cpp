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
#include "Element.h"
#include "Text.h"

#include "Document.h"
#include "DocumentFragment.h"

#include "dom/parser/HTMLParser.h"

#include "layout/Frame.h"
#include "layout/FrameBox.h"

#include "style/Style.h"
#include "style/CSSParser.h"

#include "HTMLHtmlElement.h"
#include "HTMLHeadElement.h"
#include "HTMLHeadingElement.h"
#include "HTMLLIElement.h"
#include "HTMLUListElement.h"
#include "HTMLStrongElement.h"
#include "HTMLBodyElement.h"
#include "HTMLDivElement.h"
#include "HTMLImageElement.h"
#include "HTMLScriptElement.h"
#include "HTMLStyleElement.h"
#include "HTMLBRElement.h"
#include "HTMLLinkElement.h"
#include "HTMLMetaElement.h"
#include "HTMLParagraphElement.h"
#include "HTMLSpanElement.h"
#include "HTMLUnknownElement.h"
#ifdef STARFISH_ENABLE_MULTIMEDIA
#include "HTMLMediaElement.h"
#include "HTMLVideoElement.h"
#include "HTMLAudioElement.h"
#include "HTMLTrackElement.h"
#endif
#include "dom/Attr.h"
#include "dom/xml/XMLSerializer.h"
#include "dom/DOMRect.h"
#include "dom/DOMRectList.h"

namespace StarFish {

String* Element::tagName()
{
    if (document()->isXMLDocument()) {
        return localName();
    } else {
        return localName()->toUpper();
    }
}

size_t Element::hasAttribute(QualifiedName name)
{
    for (size_t i = 0; i < m_attributes.size(); i ++) {
        if (m_attributes[i].name() == name) {
            return i;
        }
    }
    return SIZE_MAX;
}

String* Element::getAttribute(size_t pos)
{
    return m_attributes[pos].value();
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
            AttrList* attrList = rareMembers()->asRareElementMembers()->m_attrList;
            for (unsigned i = 0, size = attrList->size(); i < size; i++) {
                if (attrList->at(i) == attrNode) {
                    attrList->erase(attrList->begin() + i);
                    break;
                }
            }
        }
        didAttributeChanged(name, v, String::emptyString, false, true);
    }
}

void Element::didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved)
{
#ifdef STARFISH_TC_COVERAGE
    if (name.localName()->equals("style")) {
        STARFISH_LOG_INFO("+++attr:&&&style\n");
    } else {
        STARFISH_LOG_INFO("+++attr:%s\n", name.localName()->utf8Data());
    }
#endif

    StaticStrings* ss = document()->window()->starFish()->staticStrings();
    if (name == ss->m_id) {
        m_id = value;
        // Style should be recalculated from root node because of combinators.
        document()->window()->setWholeDocumentNeedsStyleRecalc();

        document()->invalidNamedAccessCacheIfNeeded();
    } else if (name == ss->m_class) {
        DOMTokenList::tokenize(&m_classNames, value);
        // Style should be recalculated from root node because of combinators.
        document()->window()->setWholeDocumentNeedsStyleRecalc();

        // propagate invalidate nodeList cache(getElementsByClassName) damage to parent tree
        Node* parent = parentNode();
        while (parent) {
            parent->invalidateNodeListCacheDueToChangeClassNameOfDescendant();
            parent = parent->parentNode();
        }
    } else if (name == ss->m_style) {
        if (old->equals(String::emptyString)) {
            attributeData(name).registerGetterCallback(this, [](Element* element, const Attribute * const attr) -> String* {
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
        // according spec, https://html.spec.whatwg.org/multipage/browsers.html#named-access-on-the-window-object
        // only few html elements are affected by name attribute changing
        document()->invalidNamedAccessCacheIfNeeded();
    }
}

LayoutRect Element::clientRect()
{
    document()->window()->layoutIfNeeds();
    if (frame()) {
        if (frame()->isFrameBox()) {
            FrameBox* box = frame()->asFrameBox();
            return LayoutRect(box->borderLeft(), box->borderTop(), box->contentWidth() + box->paddingWidth(), box->contentHeight() + box->paddingHeight());
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

Element* Element::offsetParent()
{
    Frame* frameObject = this->frame();
    return frameObject ? frameObject->offsetParent() : nullptr;
}

void Element::getClientQuads(std::vector<DOMQuad>& quads)
{
    Frame* frameObject = this->frame();
    if (!frameObject)
        return;
    // todo : support SVG model
    // there is Getting bounding rectangle from the SVG model in the spec, but SVG model is not supported

    // initial version : implement for display:block
    if (frameObject->isFrameBox() && frameObject->style()->display() == DisplayValue::BlockDisplayValue) {
        LayoutRect rect = ((FrameBox*)frameObject)->absoluteRect((FrameBox*)document()->rootElement()->frame());

        DOMQuad* q = DOMQuad::create
        (DOMPointInit(rect.location().x(), rect.location().y())
        , DOMPointInit(rect.location().x() + rect.size().width(), rect.location().y())
        , DOMPointInit(rect.location().x() + rect.size().width(), rect.location().y() + rect.size().height())
        , DOMPointInit(rect.location().x() , rect.location().y()+ rect.size().height()));

        quads.push_back(*q);
    } else {
        // todo assert
        STARFISH_LOG_ERROR("%s %d\n : implement not yet", __FUNCTION__, __LINE__);
        STARFISH_ASSERT(false);
    }
    return;
}

DOMRectList* Element::getClientRects()
{
    std::vector<DOMQuad> quads;
    getClientQuads(quads);

    if (quads.empty())
        return DOMRectList::create();

    // todo : Apply the transforms
    return DOMRectList::create(quads);
}

DOMRect* Element::getBoundingClientRect()
{
    std::vector<DOMQuad> quads;
    getClientQuads(quads);
    if (quads.empty())
        return DOMRect::create();

    DOMRect* rect = DOMRect::create(quads[0].bounds());

    for (size_t i= 1; i < quads.size(); ++i)
        rect->unite(quads[i].bounds());

    // todo : Apply the transforms
    return rect;
}

void Element::setTextContent(String* text)
{
    Text* node = new Text(document(), text);

    while (firstChild()) {
        removeChild(firstChild());
    }
    if (!text->equals(String::emptyString)) {
        appendChild(node);
    }
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

    HTMLParser parser(document()->window()->starFish(), df, this, html);
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
        newNode = HTMLDocument::createHTMLElement(document(), name().localNameAtomic());
    } else {
        newNode = new NamedElement(document(), name());
    }

    STARFISH_ASSERT(newNode);

    for (const Attribute& attr : m_attributes) {
        newNode->setAttribute(attr.name(), attr.value());
    }
    newNode->m_inlineStyle = inlineStyle()->clone(document(), newNode);

    return newNode;
}

NamedNodeMap* Element::attributes()
{
    if (hasRareMembers()) {
        STARFISH_ASSERT(rareMembers()->isRareElementMembers());
        if (!rareMembers()->asRareElementMembers()->m_namedNodeMap)
            return nullptr;
    }
    RareElementMembers* rareMembers = ensureRareElementMembers();
    if (!rareMembers->m_namedNodeMap)
        rareMembers->m_namedNodeMap = new NamedNodeMap(document()->scriptBindingInstance(), this);
    return rareMembers->m_namedNodeMap;
}

RareNodeMembers* Element::ensureRareMembers()
{
    if (!hasRareMembers())
        m_rareNodeMembers = new RareElementMembers();
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
    if (!rareMembers->m_attrList)
        rareMembers->m_attrList = new (GC) AttrList();
    STARFISH_ASSERT(this->attr(attr->name()) == nullptr);
    rareMembers->m_attrList->push_back(attr);
}

Attr* Element::attr(QualifiedName name)
{
    STARFISH_ASSERT((hasRareMembers() && rareMembers()->isRareElementMembers()) || !hasRareMembers());
    if (hasRareMembers() && rareMembers()->asRareElementMembers()->m_attrList) {
        AttrList* attrList = rareMembers()->asRareElementMembers()->m_attrList;
        for (Attr* item : *attrList) {
            STARFISH_ASSERT(item);
            if (item->name() == name) {
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
        if (!rareMembers->m_attrList)
            rareMembers->m_attrList = new (GC) AttrList();
        returnAttr = new Attr(document(), document()->scriptBindingInstance(), this, name);
        rareMembers->m_attrList->push_back(returnAttr);
    }
    return returnAttr;
}

void Element::setFocus(bool flag)
{
    if (flag == focused())
        return;

    setFocused(flag);
    // TODO: Style should be recalculated when we implement :focus selector or apply visual effects for focusable elements.
    // setNeedsStyleRecalc();
}

bool Element::supportsFocus()
{
    if (!tabIndexSetExplicitly())
        return false;
    return true;
}

bool Element::isFocusable()
{
    // TODO: https://www.w3.org/TR/html5/editing.html#focus-management
    if (!supportsFocus())
        return false;
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
