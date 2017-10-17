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
#include "core/dom/DOMStringMap.h"
#include "core/dom/DOMTokenList.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLDocument.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/NamedNodeMap.h"
#include "core/dom/SelectorQuery.h"
#include "core/dom/Text.h"
#include "core/dom/PseudoElementData.h"
#include "core/dom/parser/HTMLParser.h"
#include "core/dom/parser/HTMLParserIdioms.h"
#include "core/dom/xml/XMLSerializer.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBox.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/StackingContext.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/style/ComputedStyle.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"
#ifdef STARFISH_ENABLE_TTS
#include "core/modules/tts/TextAlternativeHelper.h"
#include "core/modules/tts/TTS.h"
#include "core/dom/Event.h"
#endif

namespace StarFish {

String* Element::tagName()
{
    if (document()->isXMLDocument()) {
        return localName();
    } else {
        return localName()->toASCIIUpper();
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

size_t Element::hasAttribute(const AttributeName& name) const
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

Nullable<String*> Element::getAttribute(const AttributeName& name) const
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

String* Element::getAttributeOrEmpty(const AttributeName& name) const
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

bool Element::matches(String* selectors)
{
    GCVector<CSSSelectorList*> selectorListContainer;
    parseSelector(selectorListContainer, selectors);
    SelectorQuery selectorQuery(selectorListContainer);
    return selectorQuery.matches(*this);
}

void Element::didAttributeChanged(QualifiedName name, String* old,
                                  String* value, bool attributeCreated,
                                  bool attributeRemoved)
{
#ifdef STARFISH_TC_COVERAGE
    if (name.localName()->equals("style")) {
        STARFISH_LOG_INFO("+++attr:&&&style\n");
    } else {
        auto s = name.localName()->toUTF8NonGCString();
        STARFISH_LOG_INFO("+++attr:%s\n", s.data());
    }
#endif

    StaticStrings* ss = starFish()->staticStrings();
    if (name == ss->m_id) {
        if (attributeRemoved) {
            m_id = AtomicString::emptyAtomicString();
        } else {
            m_id = AtomicString::createAtomicString(starFish(), value);
        }
        document()->invalidNamedAccessCacheIfNeeded();
    } else if (name == ss->m_class) {
        GCVector<StringView> tokens = DOMTokenList::tokenize(value);
        m_classNames.clear();
        for (size_t i = 0; i < tokens.size(); i++) {
            m_classNames.push_back(
                AtomicString::createAtomicString(starFish(), tokens[i]));
        }

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

        if (!value->isEmpty()) {
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
            m_tabIndex = tabIndex;
            m_tabIndexWasSetExplicitly = true;
        } else {
            m_tabIndexWasSetExplicitly = false;
        }
        document()->invalidFocusRingCacheIfNeeded();
    }

    if (name == ss->m_audioTagName || name == ss->m_videoTagName) {
        // Media elements can have child elements, but they are not visible
        // elements.
        setNeedsStyleRecalc();
    } else {
        // Style should be recalculated from this node to children because of
        // combinators. And this is done only if children have combinators that
        // is partially matched.
        setNeedsStyleRecalcIfNeeded();
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

void Element::onGlobalPointingEvent(float x, float y,
                                    GlobalPointingEventKind kind)
{
    rareMembers()->m_scrolling->onGlobalPointingEvent(x, y, kind);
}

bool Element::handleDefaultEvent(Event* event)
{
    if (Node::handleDefaultEvent(event)) {
        return true;
    }

    if (frame() && frame()->isFrameBlockBox() &&
        frame()->shouldApplyOverflow()) {
        if (!ensureRareElementMembers()->m_scrolling) {
            ensureRareElementMembers()->m_scrolling = new Scrolling(this);
        }
        auto ox = frame()->appliedOverflowX();
        auto oy = frame()->appliedOverflowY();

        if (rareMembers()->m_scrolling->handleDefaultEvent(
                event, window(), frame()->asFrameBlockBox(), ox, oy)) {
            return true;
        }
    }
#ifdef STARFISH_ENABLE_TTS
    StarFish* sf = document()->window()->starFish();
    if (sf->tts()->isTTSEnable()) {
        if (isHTMLElement() && isFocusable() && event->isFocusEvent() &&
            event->type()->equals("focus")) {
            TextAlternativeHelper tah(sf);
            String* altText = tah.getComputedTextAlternative(this);
            if (altText) {
                sf->tts()->speech(altText);
            }
        }
    }
#endif
    return false;
}

void Element::scrollIntoViewIfNeeded()
{
    DOMRect* rect = getBoundingClientRect();

    LayoutRect windowRect(0, 0, window()->innerWidth(),
                          window()->innerHeight());

    if (!windowRect.contains(rect->x(), rect->y()) ||
        !windowRect.contains(rect->x() + rect->width(), rect->y()) ||
        !windowRect.contains(rect->x(), rect->y() + rect->height()) ||
        !windowRect.contains(rect->x() + rect->width(),
                             rect->y() + rect->height())) {
        scrollIntoView();
    }
}

void Element::scrollIntoView(bool alignToTop)
{
    DOMRect* rect = getBoundingClientRect();

    LayoutUnit remainSpaceToScrollEnd;
    if (alignToTop) {
        remainSpaceToScrollEnd = rect->top();

        Element* e = this->parentElement();
        while (e && remainSpaceToScrollEnd) {
            if (e->canScrollVerticaly()) {
                LayoutUnit initialValue = e->scrollTop(false);
                LayoutUnit outer = e->frame()->asFrameBox()->paddingTop() +
                                   e->frame()->asFrameBox()->borderTop();
                DOMRect* eBounds = e->getBoundingClientRect();
                outer += (LayoutUnit)eBounds->top();
                e->setScrollTop(initialValue + remainSpaceToScrollEnd - outer,
                                false);
                LayoutUnit now = e->scrollTop(false);
                remainSpaceToScrollEnd -= (now - initialValue);
            }
            e = e->parentElement();
        }

        if (remainSpaceToScrollEnd) {
            window()->scrollTo(window()->scrollX(),
                               window()->scrollY() + remainSpaceToScrollEnd);
        }
    } else {
        remainSpaceToScrollEnd = rect->bottom();

        Element* e = this->parentElement();
        while (e && remainSpaceToScrollEnd) {
            if (e->canScrollVerticaly()) {
                LayoutUnit initialValue = e->scrollTop(false);
                LayoutUnit outer = -e->frame()->asFrameBox()->paddingBottom() -
                                   e->frame()->asFrameBox()->borderBottom();
                DOMRect* eBounds = e->getBoundingClientRect();
                outer += (LayoutUnit)eBounds->bottom();
                e->setScrollTop(initialValue + remainSpaceToScrollEnd - outer,
                                false);
                LayoutUnit now = e->scrollTop(false);
                remainSpaceToScrollEnd -= (now - initialValue);
            }
            e = e->parentElement();
        }

        if (remainSpaceToScrollEnd) {
            remainSpaceToScrollEnd -= window()->height();
            window()->scrollTo(window()->scrollX(),
                               window()->scrollY() + remainSpaceToScrollEnd);
        }
    }
}

double Element::scrollLeft(bool layoutIfNeeds)
{
    if (layoutIfNeeds) {
        window()->browsingContext()->webView()->layoutIfNeeds();
    }

    if (!frame() || !frame()->isFrameBlockBox()) {
        return 0;
    }

    if (!isHTMLInputElement()) {
        if (appliedOverflowX() < OverflowValue::AutoOverflow) {
            return 0;
        }
    }

    if (hasRareMembers()) {
        return rareMembers()->m_scrollLeft;
    }
    return 0;
}

void Element::setScrollLeft(double s, bool layoutIfNeeds)
{
    if (layoutIfNeeds) {
        window()->browsingContext()->webView()->layoutIfNeeds();
    }

    if (!frame() || !frame()->isFrameBlockBox()) {
        return;
    }

    if (appliedOverflowX() < OverflowValue::AutoOverflow) {
        return;
    }

    if (s > scrollWidth() - frame()->asFrameBlockBox()->width().toUnsigned()) {
        s = scrollWidth() - frame()->asFrameBlockBox()->width().toUnsigned();
    }

    if (s < 0) {
        s = 0;
    }

    ensureRareElementMembers()->m_scrollLeft = s;
    setNeedsPainting();
}

double Element::scrollTop(bool layoutIfNeeds)
{
    if (layoutIfNeeds) {
        window()->browsingContext()->webView()->layoutIfNeeds();
    }

    if (!frame() || !frame()->isFrameBlockBox()) {
        return 0;
    }

    if (appliedOverflowY() < OverflowValue::AutoOverflow) {
        return 0;
    }

    if (hasRareMembers()) {
        return rareMembers()->m_scrollTop;
    }
    return 0;
}

bool Element::canScrollVerticaly(bool layoutIfNeeds)
{
    if (layoutIfNeeds) {
        window()->browsingContext()->webView()->layoutIfNeeds();
    }

    if (!frame() || !frame()->isFrameBlockBox()) {
        return false;
    }

    if (appliedOverflowY() < OverflowValue::AutoOverflow) {
        return false;
    }

    return frame()->asFrameBlockBox()->hasBiggerContentThanFrameHeight();
}

void Element::setScrollTop(double s, bool layoutIfNeeds)
{
    if (layoutIfNeeds) {
        window()->browsingContext()->webView()->layoutIfNeeds();
    }

    if (!frame() || !frame()->isFrameBlockBox()) {
        return;
    }

    if (appliedOverflowY() < OverflowValue::AutoOverflow) {
        return;
    }

    if (s >
        scrollHeight() - frame()->asFrameBlockBox()->height().toUnsigned()) {
        s = scrollHeight() - frame()->asFrameBlockBox()->height().toUnsigned();
    }

    if (s < 0) {
        s = 0;
    }

    ensureRareElementMembers()->m_scrollTop = s;
    setNeedsPainting();
}

uint32_t Element::scrollWidth()
{
    window()->browsingContext()->webView()->layoutIfNeeds();
    if (!frame()) {
        return 0;
    }
    if (!frame()->isFrameBlockBox()) {
        return 0;
    }
    return frame()->asFrameBlockBox()->scrollWidth();
}

uint32_t Element::scrollHeight()
{
    window()->browsingContext()->webView()->layoutIfNeeds();
    if (!frame()) {
        return 0;
    }
    if (!frame()->isFrameBlockBox()) {
        return 0;
    }
    return frame()->asFrameBlockBox()->scrollHeight();
}

static void applyMatrixToPoint(DOMPoint* p, const SkMatrix& mat)
{
    // TODO transform-3d
    float x, y;
    x = p->x();
    y = p->y();
    SkPoint skP = SkPoint::Make(SkFloatToScalar(x), SkFloatToScalar(y));
    mat.mapPoints(&skP, 1);
    p->setX(SkScalarToFloat(skP.x()));
    p->setY(SkScalarToFloat(skP.y()));
}

static void applyTransform(DOMQuad* q, FrameBox* box)
{
    SkMatrix mat;
    mat.reset();
    Frame* f = box;
    std::vector<SkMatrix> m;
    while (f) {
        StackingContext* sc = f->asFrameBox()->stackingContext();
        if (sc) {
            m.push_back(sc->transformMatrix());
        }
        f = f->layoutParent();
    }

    auto iter = m.rbegin();

    while (iter != m.rend()) {
        mat.preConcat(*iter);
        iter++;
    }

    applyMatrixToPoint(q->p1(), mat);
    applyMatrixToPoint(q->p2(), mat);
    applyMatrixToPoint(q->p3(), mat);
    applyMatrixToPoint(q->p4(), mat);
}

void Element::getClientQuads(GCVector<DOMQuad*>& quads, bool layoutIfNeeds)
{
    if (layoutIfNeeds) {
        // FIXME
        // now computing matrix of stacking needs painting/ compositing
        // move computing into other step
        window()->webView()->renderingIfNeeds();
    }

    Frame* frameObject = this->frame();
    if (!frameObject) {
        return;
    }
    // TODO : support SVG model
    // there is Getting bounding rectangle from the SVG model in the spec, but
    // SVG model is not supported

    if (frameObject->isFrameBox()) {
        LayoutRect rect =
            frameObject->asFrameBox()->absoluteRectIncludingScroll(
                document()->frame()->asFrameBox());
        rect.setX(rect.x() - (LayoutUnit)window()->scrollX());
        rect.setY(rect.y() - (LayoutUnit)window()->scrollY());

        DOMQuad* q = new DOMQuad(
            document(), DOMPointInit(rect.location().x(), rect.location().y()),
            DOMPointInit(rect.location().x() + rect.size().width(),
                         rect.location().y()),
            DOMPointInit(rect.location().x() + rect.size().width(),
                         rect.location().y() + rect.size().height()),
            DOMPointInit(rect.location().x(),
                         rect.location().y() + rect.size().height()));

        applyTransform(q, frameObject->asFrameBox());
        quads.push_back(q);
    } else if (frameObject->isFrameInline()) {
        Frame* nearestFrameBox = frameObject->parent();
        while (!nearestFrameBox->isFrameBox()) {
            nearestFrameBox = nearestFrameBox->parent();
        }

        if (nearestFrameBox) {
            FrameBox* box = nearestFrameBox->asFrameBox();
            box->iterateChildFrameBox([&](FrameBox* childBox) {
                if (childBox->isInlineNonReplacedBox()) {
                    if (childBox->asInlineNonReplacedBox()->origin()->node() ==
                        this) {
                        LayoutRect rect = childBox->absoluteRectIncludingScroll(
                            document()->frame()->asFrameBox());
                        rect.setX(rect.x() - (LayoutUnit)window()->scrollX());
                        rect.setY(rect.y() - (LayoutUnit)window()->scrollY());

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

                        applyTransform(q, childBox->asFrameBox());
                        quads.push_back(q);
                    }
                }
            });
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

    return DOMRectList::create(document(), quads);
}

DOMRect* Element::getBoundingClientRect(bool layoutIfNeeds)
{
    GCVector<DOMQuad*> quads;
    getClientQuads(quads, layoutIfNeeds);
    if (quads.empty()) {
        return new DOMRect(document());
    }

    DOMRect* rect = quads[0]->getBounds();

    for (size_t i = 1; i < quads.size(); ++i) {
        rect->unite(quads[i]->getBounds());
    }

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
    if (position->equalsIgnoreCase("beforebegin") ||
        position->equalsIgnoreCase("afterend")) {
        context = parentElement();
        // If context is null or a Document, throw a
        // "NoModificationAllowedError" DOMException.
        if (context == nullptr || context->isDocument()) {
            throw new DOMException(document(),
                                   DOMException::NO_MODIFICATION_ALLOWED_ERR,
                                   "Can not execute `insertAdjacentHTML`.");
        }
    } else if (position->equalsIgnoreCase("afterbegin") ||
               position->equalsIgnoreCase("beforeend")) {
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
    if (position->equalsIgnoreCase("beforebegin")) {
        // If position is an ASCII case-insensitive match for the string
        // "beforebegin"
        // Insert fragment into the context object's parent before the context
        // object.
        contextObject->parentNode()->insertBefore(df, contextObject);
    } else if (position->equalsIgnoreCase("afterbegin")) {
        // If position is an ASCII case-insensitive match for the string
        // "afterbegin"
        // Insert fragment into the context object before its first child.
        contextObject->insertBefore(df, firstChild());
    } else if (position->equalsIgnoreCase("beforeend")) {
        // If position is an ASCII case-insensitive match for the string
        // "beforeend"
        // Append fragment to the context object.
        contextObject->appendChild(df);
    } else {
        STARFISH_ASSERT(position->equalsIgnoreCase("afterend"));
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
    if (where->equalsIgnoreCase("beforebegin")) {
        // If element’s parent is null, return null.
        if (element->parentNode() == nullptr)
            return nullptr;
        // Return the result of pre-inserting node into element’s parent before
        // element.
        return element->parentNode()->insertBefore(node, element);
    } else if (where->equalsIgnoreCase("afterbegin")) {
        // Return the result of pre-inserting node into element before element’s
        // first child.
        return element->insertBefore(node, element->firstChild());
    } else if (where->equalsIgnoreCase("beforeend")) {
        // Return the result of pre-inserting node into element before null.
        return element->insertBefore(node, nullptr);
    } else if (where->equalsIgnoreCase("afterend")) {
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
    } else if (isSVGElement()) {
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

DOMStringMap* Element::dataset()
{
    RareElementMembers* rareMembers = ensureRareElementMembers();
    if (!rareMembers->m_dataset) {
        rareMembers->m_dataset = new DOMStringMap(this);
    }
    return rareMembers->m_dataset;
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
    } else if (len.isFontPercent()) {
        p.setValueKind(CSSStyleValuePair::ValueKind::Length);
        Length::Type t = len.type();
        CSSLength::Kind k;
        if (t == Length::Em) {
            k = CSSLength::EM;
        } else if (t == Length::Ex) {
            k = CSSLength::EX;
        } else {
            k = CSSLength::REM;
        }
        p.setValue(CSSLength(k, len.fontPercent()));
    } else if (len.isViewportPercent()) {
        p.setValueKind(CSSStyleValuePair::ValueKind::Length);
        Length::Type t = len.type();
        CSSLength::Kind k;
        if (t == Length::Vw) {
            k = CSSLength::VW;
        } else if (t == Length::Vh) {
            k = CSSLength::VH;
        } else if (t == Length::Vmin) {
            k = CSSLength::VMIN;
        } else {
            k = CSSLength::VMAX;
        }
        p.setValue(CSSLength(k, len.viewportPercent()));
    } else if (len.isAuto()) {
        p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
    } else if (len.isCalc()) {
        p.setCalcValue(len.calcData());
    } else if (len.isInheritableNumber()) {
        p.setNumberValue(len.inheritableNumber());
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    return p;
};

CSSStyleDeclaration* Element::getComputedStyle()
{
    CSSStyleDeclaration* d = new ComputedStyleCSSStyleDeclaration(this);

    window()->browsingContext()->webView()->layoutIfNeeds();

    ComputedStyle* style = this->style();
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
    ADD_VALUE_PAIR(TextTransform, TextTransformValueKind, textTransform)
    ADD_VALUE_PAIR(Direction, DirectionValueKind, direction)
    ADD_VALUE_PAIR(BorderTopStyle, BorderStyleValueKind, borderTopStyle)
    ADD_VALUE_PAIR(BorderRightStyle, BorderStyleValueKind, borderRightStyle)
    ADD_VALUE_PAIR(BorderBottomStyle, BorderStyleValueKind, borderBottomStyle)
    ADD_VALUE_PAIR(BorderLeftStyle, BorderStyleValueKind, borderLeftStyle)
    ADD_VALUE_PAIR(Visibility, VisibilityValueKind, visibility)
    ADD_VALUE_PAIR(FontStyle, FontStyleValueKind, fontStyle)
    ADD_VALUE_PAIR(FontWeight, FontWeightValueKind, fontWeight)
    ADD_VALUE_PAIR(WordWrap, WordWrapValueKind, wordWrap)
    ADD_VALUE_PAIR(OverflowWrap, WordWrapValueKind, wordWrap)
    ADD_VALUE_PAIR(OverflowX, OverflowValueKind, overflowX)
    ADD_VALUE_PAIR(OverflowY, OverflowValueKind, overflowY)
    ADD_VALUE_PAIR(UnicodeBidi, UnicodeBidiValueKind, unicodeBidi)
    ADD_VALUE_PAIR(Opacity, Number, opacity)
    ADD_VALUE_PAIR(BoxSizing, BoxSizingValueKind, boxSizing)
    ADD_VALUE_PAIR(FlexDirection, FlexDirectionValueKind, flexDirection)
    ADD_VALUE_PAIR(FlexWrap, FlexWrapValueKind, flexWrap)
    ADD_VALUE_PAIR(Order, Int32, order)
    ADD_VALUE_PAIR(JustifyContent, JustifyContentValueKind, justifyContent)
    ADD_VALUE_PAIR(AlignItems, AlignItemValueKind, alignItems)
    ADD_VALUE_PAIR(AlignSelf, AlignItemValueKind, alignSelf)
    ADD_VALUE_PAIR(AlignContent, AlignContentValueKind, alignContent)
    ADD_VALUE_PAIR(FlexGrow, Number, flexGrow)
    ADD_VALUE_PAIR(FlexShrink, Number, flexShrink)
#undef ADD_VALUE_PAIR

    {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::ZIndex);
        if (style->isSpecifiedZIndex()) {
            p.setValueKind(CSSStyleValuePair::ValueKind::Int32);
            p.setValue(style->zIndex());
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
        }
        d->addValuePair(p);
    }

    {
        CSSStyleValuePair p;
        FlexBasisData flexBasis = style->flexBasis();
        p.setKeyKind(CSSStyleValuePair::KeyKind::FlexBasis);
        if (flexBasis.isContent()) {
            p.setValueKind(CSSStyleValuePair::ValueKind::FlexBasisValueKind);
            p.setValue(FlexBasisValue::ContentFlexBasisValue);
        } else {
            Length len = flexBasis.width();
            lengthToCSSStyleValue(len);
        }
        d->addValuePair(p);
    }

// length properties
#define ADD_ABSOLUTE_LENGTH_PAIR(keyKind, getter)                   \
    {                                                               \
        CSSStyleValuePair p;                                        \
        p.setKeyKind(CSSStyleValuePair::KeyKind::keyKind);          \
        if (frame() && frame()->isFrameBox()) {                     \
            p.setValueKind(CSSStyleValuePair::ValueKind::Length);   \
            p.setValue(CSSLength(frame()->asFrameBox()->getter())); \
        } else {                                                    \
            p.setValueKind(CSSStyleValuePair::ValueKind::Auto);     \
        }                                                           \
        d->addValuePair(p);                                         \
    }
    ADD_ABSOLUTE_LENGTH_PAIR(MarginTop, marginTop)
    ADD_ABSOLUTE_LENGTH_PAIR(MarginRight, marginRight)
    ADD_ABSOLUTE_LENGTH_PAIR(MarginBottom, marginBottom)
    ADD_ABSOLUTE_LENGTH_PAIR(MarginLeft, marginLeft)
    ADD_ABSOLUTE_LENGTH_PAIR(PaddingTop, paddingTop)
    ADD_ABSOLUTE_LENGTH_PAIR(PaddingRight, paddingRight)
    ADD_ABSOLUTE_LENGTH_PAIR(PaddingBottom, paddingBottom)
    ADD_ABSOLUTE_LENGTH_PAIR(PaddingLeft, paddingLeft)
    ADD_ABSOLUTE_LENGTH_PAIR(BorderTopWidth, borderTop)
    ADD_ABSOLUTE_LENGTH_PAIR(BorderRightWidth, borderRight)
    ADD_ABSOLUTE_LENGTH_PAIR(BorderBottomWidth, borderBottom)
    ADD_ABSOLUTE_LENGTH_PAIR(BorderLeftWidth, borderLeft)

#define ADD_LENGTH_PAIR(keyKind, getter)                              \
    {                                                                 \
        CSSStyleValuePair p = lengthToCSSStyleValue(style->getter()); \
        p.setKeyKind(CSSStyleValuePair::KeyKind::keyKind);            \
        d->addValuePair(p);                                           \
    }

    ADD_LENGTH_PAIR(TextIndent, textIndent)

    {
        CSSStyleValuePair w, h;
        w.setKeyKind(CSSStyleValuePair::KeyKind::Width);
        h.setKeyKind(CSSStyleValuePair::KeyKind::Height);

        LayoutContext ctx(starFish(), document()->frame()->asFrameDocument());

        if (frame() && style->width().isDefinite(true)) {
            w.setLengthValue(CSSLength(style->width().specifiedValue(
                ctx.parentContentWidth(frame()), this)));
        } else {
            if (frame() && frame()->isFrameBox()) {
                w.setLengthValue(
                    CSSLength(frame()->asFrameBox()->contentWidth()));
            } else {
                w.setValueKind(CSSStyleValuePair::ValueKind::Auto);
            }
        }

        if (frame() && frame()->isFrameBox()) {
            bool parentHasFixedHeight = ctx.parentHasFixedHeight(frame());
            if (style->height().isDefinite(parentHasFixedHeight)) {
                LayoutUnit parentContentHeight;
                if (parentHasFixedHeight) {
                    parentContentHeight = ctx.parentFixedHeight(frame());
                }
                h.setLengthValue(CSSLength(
                    style->height().specifiedValue(parentContentHeight, this)));
            } else {
                h.setLengthValue(
                    CSSLength(frame()->asFrameBox()->contentHeight()));
            }
        } else {
            h.setValueKind(CSSStyleValuePair::ValueKind::Auto);
        }

        d->addValuePair(w);
        d->addValuePair(h);
    }

    if (frame() && frame()->isFrameBox() && frame()->isPositioned()) {
        CSSStyleValuePair t, b, l, r;
        t.setKeyKind(CSSStyleValuePair::KeyKind::Top);
        b.setKeyKind(CSSStyleValuePair::KeyKind::Bottom);
        l.setKeyKind(CSSStyleValuePair::KeyKind::Left);
        r.setKeyKind(CSSStyleValuePair::KeyKind::Right);

        LayoutContext ctx(starFish(), document()->frame()->asFrameDocument());
        FrameBox* cb = containingBlock(frame());
        FrameBox* parent = frame()->layoutParent()->asFrameBox();
        FrameBox* self = frame()->asFrameBox();

        LayoutLocation l1, l2;
        if (cb->isAncestorOf(parent)) {
            l2 = parent->absolutePoint(cb);
        } else {
            l1 = cb->absolutePoint(ctx.frameDocument());
            l2 = parent->absolutePoint(ctx.frameDocument());
        }
        LayoutUnit absX = self->x() + l2.x() - l1.x() - cb->borderLeft();
        LayoutUnit absY = self->y() + l2.y() - l1.y() - cb->borderTop();
        LayoutUnit left = absX - self->marginLeft();
        LayoutUnit top = absY - self->marginTop();
        LayoutUnit parentContentWidth = cb->contentWidth();
        LayoutUnit parentContentHeight = cb->contentHeight();
        if (frame()->isAbsolutePositioned()) {
            parentContentWidth += cb->paddingWidth();
            parentContentHeight += cb->paddingHeight();
        }

        t.setLengthValue(CSSLength(top));
        b.setLengthValue(
            CSSLength(parentContentHeight - top - self->outerHeight()));
        l.setLengthValue(CSSLength(left));
        r.setLengthValue(
            CSSLength(parentContentWidth - left - self->outerWidth()));

        d->addValuePair(t);
        d->addValuePair(b);
        d->addValuePair(l);
        d->addValuePair(r);
    } else {
        ADD_LENGTH_PAIR(Top, top)
        ADD_LENGTH_PAIR(Right, right)
        ADD_LENGTH_PAIR(Bottom, bottom)
        ADD_LENGTH_PAIR(Left, left)
    }

    {
        CSSStyleValuePair lh;
        lh.setKeyKind(CSSStyleValuePair::KeyKind::LineHeight);

        if (style->hasNormalLineHeight() || !frame()) {
            lh.setValueKind(CSSStyleValuePair::ValueKind::Normal);
        } else {
            lh.setLengthValue(CSSLength(CSSLength::PX, frame()->lineHeight()));
        }

        d->addValuePair(lh);
    }

    {
        CSSStyleValuePair fs;
        fs.setKeyKind(CSSStyleValuePair::KeyKind::FontSize);

        fs.setLengthValue(CSSLength(CSSLength::PX, style->fixedFontSize()));

        d->addValuePair(fs);
    }

    {
        CSSStyleValuePair minW, minH, maxW, maxH;
        minW.setKeyKind(CSSStyleValuePair::KeyKind::MinWidth);
        minH.setKeyKind(CSSStyleValuePair::KeyKind::MinHeight);
        maxW.setKeyKind(CSSStyleValuePair::KeyKind::MaxWidth);
        maxH.setKeyKind(CSSStyleValuePair::KeyKind::MaxHeight);
        Length minWidth = style->minWidth();
        Length minHeight = style->minHeight();
        Length maxWidth = style->maxWidth();
        Length maxHeight = style->maxHeight();

        if (minWidth.isAuto()) {
            if (frame() && frame()->isFlexItem()) {
                minW.setValueKind(CSSStyleValuePair::ValueKind::Auto);
            } else {
                minW.setLengthValue(CSSLength(CSSLength::PX, 0));
            }
        } else if (minWidth.isDefinite(false)) {
            CSSStyleValuePair p = lengthToCSSStyleValue(minWidth);
            p.setKeyKind(CSSStyleValuePair::KeyKind::MinWidth);
            minW = p;
        } else if (minWidth.isPercent()) {
            minW.setPercentageValue(minWidth.percent());
        } else if (minWidth.isCalc()) {
            minW.setCalcValue(minWidth.calcData());
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }

        if (minHeight.isAuto()) {
            if (frame() && frame()->isFlexItem()) {
                minH.setValueKind(CSSStyleValuePair::ValueKind::Auto);
            } else {
                minH.setLengthValue(CSSLength(CSSLength::PX, 0));
            }
        } else if (minHeight.isDefinite(false)) {
            CSSStyleValuePair p = lengthToCSSStyleValue(minHeight);
            p.setKeyKind(CSSStyleValuePair::KeyKind::MinHeight);
            minH = p;
        } else if (minHeight.isPercent()) {
            minH.setPercentageValue(minHeight.percent());
        } else if (minHeight.isCalc()) {
            minH.setCalcValue(minHeight.calcData());
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }

        if (maxWidth.isAuto()) {
            maxW.setValueKind(CSSStyleValuePair::ValueKind::None);
        } else if (maxWidth.isDefinite(false)) {
            CSSStyleValuePair p = lengthToCSSStyleValue(maxWidth);
            p.setKeyKind(CSSStyleValuePair::KeyKind::MaxWidth);
            maxW = p;
        } else if (maxWidth.isPercent()) {
            maxW.setPercentageValue(minHeight.percent());
        } else if (maxWidth.isCalc()) {
            maxW.setCalcValue(maxWidth.calcData());
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }

        if (maxHeight.isAuto()) {
            maxH.setValueKind(CSSStyleValuePair::ValueKind::None);
        } else if (maxHeight.isDefinite(false)) {
            CSSStyleValuePair p = lengthToCSSStyleValue(maxHeight);
            p.setKeyKind(CSSStyleValuePair::KeyKind::MaxHeight);
            maxH = p;
        } else if (minHeight.isPercent()) {
            maxH.setPercentageValue(maxHeight.percent());
        } else if (maxHeight.isCalc()) {
            maxH.setCalcValue(maxHeight.calcData());
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }

        d->addValuePair(minW);
        d->addValuePair(minH);
        d->addValuePair(maxW);
        d->addValuePair(maxH);
    }

#undef ADD_ABSOLUTE_LENGTH_PAIR
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

    // other properties that cannot be generated by macros
    // background
    {
        CSSStyleValuePair bgImage, bgSize, bgRepeatX, bgRepeatY, bgPositionX,
            bgPositionY, bgAttachment, bgClip, bgOrigin;
        bgImage.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundImage);
        bgImage.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        bgSize.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundSize);
        bgSize.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        bgRepeatX.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundRepeatX);
        bgRepeatX.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        bgRepeatY.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundRepeatY);
        bgRepeatY.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        bgPositionX.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundPositionX);
        bgPositionX.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        bgPositionY.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundPositionY);
        bgPositionY.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        bgAttachment.setKeyKind(
            CSSStyleValuePair::KeyKind::BackgroundAttachment);
        bgAttachment.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        bgClip.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundClip);
        bgClip.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        bgOrigin.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundOrigin);
        bgOrigin.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);

        ValueList *bgImageValues, *bgSizeValues, *bgRepeatXValues,
            *bgRepeatYValues, *bgPositionXValues, *bgPositionYValues,
            *bgAttachmentValues, *bgClipValues, *bgOriginValues;
        bgImageValues = new ValueList(ValueList::Separator::CommaSeparator);
        bgSizeValues = new ValueList(ValueList::Separator::CommaSeparator);
        bgRepeatXValues = new ValueList(ValueList::Separator::CommaSeparator);
        bgRepeatYValues = new ValueList(ValueList::Separator::CommaSeparator);
        bgPositionXValues = new ValueList(ValueList::Separator::CommaSeparator);
        bgPositionYValues = new ValueList(ValueList::Separator::CommaSeparator);
        bgAttachmentValues =
            new ValueList(ValueList::Separator::CommaSeparator);
        bgClipValues = new ValueList(ValueList::Separator::CommaSeparator);
        bgOriginValues = new ValueList(ValueList::Separator::CommaSeparator);

        for (unsigned int i = 0; i < style->backgroundLayerSize(); i++) {
            CSSStyleValuePair item;
            if (style->backgroundImage(i)->length() == 0) {
                item.setValueKind(CSSStyleValuePair::ValueKind::None);
            } else {
                item.setUrlValue(style->backgroundImage(i));
            }
            bgImageValues->push_back(item);

            if (style->backgroundSizeIsLength(i)) {
                item.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
                ValueList* vals =
                    new ValueList(ValueList::Separator::SpaceSeparator);
                LengthSize lengthSize = style->backgroundSizeLengthValue(i);

                CSSStyleValuePair w = lengthToCSSStyleValue(lengthSize.width());
                vals->emplace_back(w.valueKind(), w.value());

                CSSStyleValuePair h =
                    lengthToCSSStyleValue(lengthSize.height());
                vals->emplace_back(h.valueKind(), h.value());

                item.setValue(vals);
            } else {
                item.setBackgroundSizeValue(style->backgroundSizeTypeValue(i));
                item.setValueKind(
                    CSSStyleValuePair::ValueKind::BackgroundSizeValueKind);
                item.setValue(style->backgroundSizeTypeValue(i));
            }
            bgSizeValues->push_back(item);

            item.setBackgroundRepeatValue(style->backgroundRepeatX(i));
            bgRepeatXValues->push_back(item);

            item.setBackgroundRepeatValue(style->backgroundRepeatY(i));
            bgRepeatYValues->push_back(item);

            item = lengthToCSSStyleValue(style->backgroundPositionX(i));
            bgPositionXValues->push_back(item);

            item = lengthToCSSStyleValue(style->backgroundPositionY(i));
            bgPositionYValues->push_back(item);

            item.setBackgroundAttachmentValue(style->backgroundAttachment(i));
            bgAttachmentValues->push_back(item);

            item.setBoxValue(style->backgroundClip(i));
            bgClipValues->push_back(item);

            item.setBoxValue(style->backgroundOrigin(i));
            bgOriginValues->push_back(item);
        }

        bgImage.setValueList(bgImageValues);
        bgSize.setValueList(bgSizeValues);
        bgRepeatX.setValueList(bgRepeatXValues);
        bgRepeatY.setValueList(bgRepeatYValues);
        bgPositionX.setValueList(bgPositionXValues);
        bgPositionY.setValueList(bgPositionYValues);
        bgAttachment.setValueList(bgAttachmentValues);
        bgClip.setValueList(bgClipValues);
        bgOrigin.setValueList(bgOriginValues);

        d->addValuePair(bgImage);
        d->addValuePair(bgSize);
        d->addValuePair(bgRepeatX);
        d->addValuePair(bgRepeatY);
        d->addValuePair(bgPositionX);
        d->addValuePair(bgPositionY);
        d->addValuePair(bgAttachment);
        d->addValuePair(bgClip);
        d->addValuePair(bgOrigin);
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

        if (!style->hasTransformOrigin()) {
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

    // transform
    {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::Transform);
        if (!style->hasTransforms()) {
            p.setValueKind(CSSStyleValuePair::ValueKind::None);
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::TransformFunctions);
            FrameDocument* doc =
                window()->document()->frame()->asFrameDocument();
            FrameBox* box = frame()->findNearestAssociateBox();
            SkMatrix m =
                style->transformsToMatrix(box->width(), box->height(), box,
                                          style->hasTransforms(frame()));

            CSSTransformFunctions* transforms = new CSSTransformFunctions();

            ValueList* values =
                new ValueList(ValueList::Separator::CommaSeparator);

            values->emplace_back(CSSStyleValuePair::ValueKind::Number,
                                 m.getScaleX());
            values->emplace_back(CSSStyleValuePair::ValueKind::Number,
                                 m.getSkewY());
            values->emplace_back(CSSStyleValuePair::ValueKind::Number,
                                 m.getSkewX());
            values->emplace_back(CSSStyleValuePair::ValueKind::Number,
                                 m.getScaleY());
            values->emplace_back(CSSStyleValuePair::ValueKind::Number,
                                 m.getTranslateX());
            values->emplace_back(CSSStyleValuePair::ValueKind::Number,
                                 m.getTranslateY());

            transforms->emplace_back(CSSTransformFunction::Matrix, values);
            p.setValue(transforms);
        }
        d->addValuePair(p);
    }

    return d;
}

void Element::dumpStyle()
{
    dump();
    printf(", style: { ");
    auto s = getComputedStyle()->generateCSSText()->toUTF8NonGCString();
    printf("%s", s.data());
    printf(" }");
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
            value = document()->contentLanguage();
        }

        n = n->parentNode();
    } while (n && value->equals(String::emptyString));

    return value;
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

bool Element::tabIndexSetExplicitly() const
{
    return m_tabIndexWasSetExplicitly;
};

void Element::focus()
{
    // TODO: Consider nested browsing contexts.
    starFish()->messageLoop()->addIdler(
        window()->browsingContext(),
        [](size_t, void* data) {
            Element* element = (Element*)data;
            element->window()->browsingContext()->setFocusedNode(element);
        },
        this);
}

void Element::blur()
{
    starFish()->messageLoop()->addIdler(
        window()->browsingContext(),
        [](size_t, void* data) {
            Element* element = (Element*)data;
            element->window()->browsingContext()->releaseFocusedNode(element);
        },
        this);
}
}
