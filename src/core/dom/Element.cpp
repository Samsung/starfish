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

#include "StarfishConfig.h"
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
#include "core/dom/Event.h"
#include "core/dom/HTMLDocument.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/NamedNodeMap.h"
#include "core/dom/SelectorQuery.h"
#include "core/dom/Text.h"
#include "core/dom/PseudoElement.h"
#include "core/dom/parser/HTMLParser.h"
#include "core/dom/parser/HTMLParserIdioms.h"
#include "core/dom/xml/XMLSerializer.h"
#include "core/dom/UIEvent.h"
#include "core/dom/Scrolling.h"
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
#include "core/csp/ContentSecurityPolicy.h"
#ifdef STARFISH_ENABLE_TTS
#include "core/modules/tts/TextAlternativeHelper.h"
#include "core/modules/tts/TTS.h"
#endif

namespace Starfish {

static bool isInHTMLNamespaceAndHTMLDocument(Element* e)
{
    if (e->namespaceURI().hasValue() &&
        e->namespaceURI().getValue()->equals(HTML_NAMESPACE) &&
        e->document()->isHTMLDocument()) {
        return true;
    }
    return false;
}

static AttributeName properAttributeName(Element* e, String* name)
{
    if (isInHTMLNamespaceAndHTMLDocument(e)) {
        return AttributeName(QualifiedName(AtomicString::createAttrAtomicString(
                                 e->starfish(), name)),
                             AttributeName::MatchName);
    }
    return AttributeName(
        QualifiedName(AtomicString::createAtomicString(e->starfish(), name)),
        AttributeName::MatchName);
}

static AttributeName properAttributeNameNS(Element* e, Nullable<String*> ns,
                                           String* name)
{
    if (ns.hasValue() && ns.getValue()->equals(String::emptyString)) {
        ns = Nullable<String*>();
    }

    return AttributeName((ns.hasValue()
                              ? QualifiedName(AtomicString::createAtomicString(
                                                  e->starfish(), ns.getValue()),
                                              AtomicString::createAtomicString(
                                                  e->starfish(), name))
                              : QualifiedName(AtomicString::createAtomicString(
                                    e->starfish(), name))),
                         AttributeName::MatchNS);
}

Scrolling* RareElementMembers::ensureScrolling(Element* self)
{
    if (m_scrolling == nullptr) {
        m_scrolling = new Scrolling(self);
    }
    return m_scrolling;
}

String* Element::tagName()
{
    // https://www.w3.org/TR/dom/#dom-element-tagname
    String* tagName = localName();
    if (prefix().hasValue()) {
        StringBuilder sb;
        sb.appendString(prefix().getValue());
        sb.appendChar(':');
        sb.appendString(localName());
        tagName = sb.finalize();
    }
    if (isInHTMLNamespaceAndHTMLDocument(this)) {
        return tagName->toASCIIUpper();
    }
    return tagName;
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

QualifiedName Element::name()
{
    return m_name;
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
        if (name.isMatch(m_attributes[i].name())) {
            return i;
        }
    }
    return SIZE_MAX;
}

size_t Element::hasAttribute(const QualifiedName& name) const
{
    return hasAttribute(AttributeName(name, AttributeName::MatchName));
}

bool Element::hasAttribute(String* name)
{
    return hasAttribute(properAttributeName(this, name)) != SIZE_MAX;
}

bool Element::hasAttributeNS(Nullable<String*> ns, String* name)
{
    return hasAttribute(properAttributeNameNS(this, ns, name)) != SIZE_MAX;
}

size_t Element::hasAttributeNode(const AttributeName& name)
{
    if (hasRareMembers() && rareMembers()->isRareElementMembers() &&
        rareMembers()->asRareElementMembers()->m_attrList) {
        GCVector<Attr*>* l = rareMembers()->asRareElementMembers()->m_attrList;
        size_t len = l->size();
        for (size_t i = 0; i < len; i++) {
            if (name.isMatch((*l)[i]->qname())) {
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

Nullable<String*> Element::getAttribute(
    const QualifiedName& qualifiedName) const
{
    return getAttribute(AttributeName(qualifiedName, AttributeName::MatchName));
}

Nullable<String*> Element::getAttribute(String* name)
{
    return getAttribute(properAttributeName(this, name));
}

Nullable<String*> Element::getAttributeNS(Nullable<String*> ns,
                                          String* localName)
{
    return getAttribute(properAttributeNameNS(this, ns, localName));
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

Attr* Element::getAttributeNode(String* name)
{
    return getAttributeNode(properAttributeName(this, name));
}

Attr* Element::getAttributeNodeNS(Nullable<String*> ns, String* name)
{
    return getAttributeNode(properAttributeNameNS(this, ns, name));
}

String* Element::getAttributeOrEmpty(const QualifiedName& qualifiedName) const
{
    Nullable<String*> result = getAttribute(qualifiedName);
    if (result.hasValue()) {
        return result.getValue();
    }
    return String::emptyString;
}

void Element::invokeDidAttributeChanged(QualifiedName name, String* old,
                                        String* value, bool attributeCreated,
                                        bool attributeRemoved)
{
    STARFISH_ASSERT(old != nullptr);
    STARFISH_ASSERT(value != nullptr);

#if !defined(NDEBUG)
    m_didAttributeChangedCorrectlyInvoked = false;
#endif
    didAttributeChanged(name, old, value, attributeCreated, attributeRemoved);
#if !defined(NDEBUG)
    STARFISH_ASSERT(m_didAttributeChangedCorrectlyInvoked);
#endif
}

void Element::setAttribute(const AttributeName& name, String* value)
{
    STARFISH_ASSERT(value != nullptr);
    STARFISH_ASSERT(name.qname().localName()->length());

    size_t idx = hasAttribute(name);
    if (idx == SIZE_MAX) {
        m_attributes.push_back(Attribute(name.qname(), value));
        invokeDidAttributeChanged(name.qname(), String::emptyString, value,
                                  true, false);
    } else {
        if (name.isNamespaceAware()) {
            // If an attribute with the same local name and namespace URI is
            // already present on the element, its prefix is changed to be
            // the prefix part of the qualifiedName.
            m_attributes[idx].name().copyPrefixFrom(name.qname());
        }
        String* v = m_attributes[idx].value();
        m_attributes[idx].setValue(value);
        invokeDidAttributeChanged(name.qname(), v, value, false, false);
    }
}

void Element::setAttribute(const QualifiedName& name, String* value)
{
    STARFISH_ASSERT(value != nullptr);

    setAttribute(AttributeName(name, AttributeName::MatchName), value);
}

void Element::setAttribute(String* name, String* value)
{
    STARFISH_ASSERT(value != nullptr);

    if (!QualifiedName::checkNameProductionRule(name)) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INVALID_CHARACTER_ERR);
    }
    setAttribute(properAttributeName(this, name), value);
}

void Element::setAttributeNS(Nullable<String*> ns, String* qualifiedName,
                             String* value)
{
    STARFISH_ASSERT(qualifiedName != nullptr);
    STARFISH_ASSERT(value != nullptr);

    QualifiedName qname =
        document()->validateAndExtractQualifiedName(ns, qualifiedName);
    setAttribute(AttributeName(qname, AttributeName::MatchNS), value);
}

Attr* Element::setAttributeNode(Attr* newAttr)
{
    STARFISH_ASSERT(newAttr != nullptr);

    RareElementMembers* rareMembers = ensureRareElementMembers();
    STARFISH_ASSERT(rareMembers->isRareElementMembers());
    if (!rareMembers->m_attrList) {
        rareMembers->m_attrList = new (GC) GCVector<Attr*>();
    }
    AttributeName attrName =
        AttributeName(newAttr->qname(), AttributeName::MatchAll);

    size_t attrIdx = hasAttributeNode(attrName);
    Attr* oldAttr =
        attrIdx == SIZE_MAX ? nullptr : (*rareMembers->m_attrList)[attrIdx];
    if (oldAttr == newAttr) {
        return newAttr;
    }

    if (newAttr->ownerElement()) {
        throw new DOMException(executionContext(),
                               DOMException::INUSE_ATTRIBUTE_ERR,
                               "The node provided is an attribute node that is "
                               "already an attribute of another Element; "
                               "attribute nodes must be explicitly cloned.");
    }

    size_t idx = hasAttribute(attrName);
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
    invokeDidAttributeChanged(newAttr->qname(), oldValue, newValue,
                              idx == SIZE_MAX, false);
    newAttr->attachToElement(this, String::emptyString);
    return oldAttr;
}

Attr* Element::setAttributeNodeNS(Attr* attrNode)
{
    STARFISH_ASSERT(attrNode != nullptr);

    // Seem to have no difference.
    return setAttributeNode(attrNode);
}

void Element::removeAttribute(size_t idx)
{
    String* v = m_attributes[idx].value();
    QualifiedName name = m_attributes[idx].name();

    m_attributes.erase(m_attributes.begin() + idx);
    // Remove Attr if exist
    size_t attrIdx =
        hasAttributeNode(AttributeName(name, AttributeName::MatchAll));
    if (attrIdx != SIZE_MAX) {
        STARFISH_ASSERT(hasRareMembers());
        STARFISH_ASSERT(rareMembers()->isRareElementMembers());
        STARFISH_ASSERT(rareMembers()->asRareElementMembers()->m_attrList);
        auto l = rareMembers()->asRareElementMembers()->m_attrList;
        Attr* attrNode = (*l)[attrIdx];
        attrNode->detachFromElement(v);
        l->erase(l->begin() + attrIdx);
    }
    invokeDidAttributeChanged(name, v, String::emptyString, false, true);
}

void Element::removeAttribute(const AttributeName& name)
{
    size_t idx = hasAttribute(name);
    if (idx != SIZE_MAX) {
        removeAttribute(idx);
    }
}

void Element::removeAttribute(const QualifiedName& name)
{
    removeAttribute(AttributeName(name, AttributeName::MatchName));
}

void Element::removeAttribute(String* name)
{
    STARFISH_ASSERT(name != nullptr);

    removeAttribute(properAttributeName(this, name));
}

void Element::removeAttributeNS(Nullable<String*> ns, String* localName)
{
    STARFISH_ASSERT(localName != nullptr);

    removeAttribute(properAttributeNameNS(this, ns, localName));
}

Attr* Element::removeAttributeNode(Attr* attr)
{
    STARFISH_ASSERT(attr != nullptr);

    if (attr->ownerElement() != this) {
        throw new DOMException(
            executionContext(), DOMException::NOT_FOUND_ERR,
            "The node provided is owned by another element.");
    }
    AttributeName attrName(attr->qname(), AttributeName::MatchAll);
    STARFISH_ASSERT(hasAttribute(attrName) != SIZE_MAX);
    removeAttribute(attrName);

    return attr;
}

GCVector<String*> Element::getAttributeNames() const
{
    GCVector<String*> ret;

    ret.reserve(m_attributes.size());

    auto siz = m_attributes.size();
    for (size_t i = 0; i < siz; i++) {
        ret.push_back(m_attributes[i].name().toString());
    }

    return ret;
}

Element* Element::closest(String* selectors)
{
    STARFISH_ASSERT(selectors != nullptr);

    GCVector<CSSSelectorList*> selectorListContainer;
    parseSelector(selectorListContainer, selectors);
    SelectorQuery selectorQuery(selectorListContainer);
    Node* node = this;
    while (node) {
        if (node->isElement()) {
            Element* element = node->asElement();
            if (selectorQuery.matches(*element)) {
                return element;
            }
        }
        node = node->parentNode();
    }
    return nullptr;
}

bool Element::matches(String* selectors)
{
    STARFISH_ASSERT(selectors != nullptr);

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

    STARFISH_ASSERT(old != nullptr);
    STARFISH_ASSERT(value != nullptr);
#if !defined(NDEBUG)
    STARFISH_ASSERT(!m_didAttributeChangedCorrectlyInvoked);
    m_didAttributeChangedCorrectlyInvoked = true;
#endif

    StaticStrings* ss = starfish()->staticStrings();
    if (name == ss->m_id) {
        if (attributeRemoved) {
            m_id = AtomicString::emptyAtomicString();
        } else {
            m_id = AtomicString::createAtomicString(starfish(), value);
        }
        if (attributeCreated) {
            if (value->length()) {
                document()->invalidNamedAccessCacheIfNeeded(value, true, false);
            }
        } else if (attributeRemoved) {
            if (old->length()) {
                document()->invalidNamedAccessCacheIfNeeded(old, false, true);
            }
        } else {
            if (old->length()) {
                document()->invalidNamedAccessCacheIfNeeded(old, false, true);
            }
            if (value->length()) {
                document()->invalidNamedAccessCacheIfNeeded(value, true, false);
            }
        }
        setNeedsStyleRecalc(StyleChangeReason::IdChange);
    } else if (name == ss->m_class) {
        GCVector<StringView> tokens;
        DOMTokenList::tokenize(value, tokens);
        m_classNames.clear();
        for (size_t i = 0; i < tokens.size(); i++) {
            m_classNames.push_back(
                AtomicString::createAtomicString(starfish(), tokens[i]));
        }

        // propagate invalidate nodeList cache(getElementsByClassName) damage to
        // parent tree
        Node* parent = parentNode();
        while (parent) {
            parent->invalidateNodeListCacheDueToChangeClassNameOfDescendant();
            parent = parent->parentNode();
        }

        setNeedsStyleRecalc(StyleChangeReason::ClassChange);
    } else if (name == ss->m_style) {
        if (!value->isEmpty() &&
            !document()->contentSecurityPolicy()->allowInline(
                CSPDirectives::StyleSrc, value)) {
            String* eventType =
                starfish()->staticStrings()->m_error.localName();
            Event* e = new Event(executionContext(), eventType,
                                 EventInit(false, false));
            dispatchEventIdleTimeByUA(e);
            return;
        }

        if (attributeCreated) {
            registerInlineStyleCallback();
        }
        inlineStyle()->clear();

        if (!value->isEmpty()) {
            CSSParser parser(document());
            parser.parseStyleDeclaration(value, inlineStyle());
        }
        m_didInlineStyleModifiedAfterAttributeSet = false;
        setNeedsStyleRecalc(StyleChangeReason::InlineStyleChange);
    } else if (name == ss->m_name) {
        // TODO we should not always invalidate cache
        // according spec,
        // https://html.spec.whatwg.org/multipage/browsers.html#named-access-on-the-window-object
        // only few html elements are affected by name attribute changing
        if (attributeCreated) {
            if (value->length()) {
                document()->invalidNamedAccessCacheIfNeeded(value, true, false);
            }
        } else if (attributeRemoved) {
            if (old->length()) {
                document()->invalidNamedAccessCacheIfNeeded(old, false, true);
            }
        } else {
            if (old->length()) {
                document()->invalidNamedAccessCacheIfNeeded(old, false, true);
            }
            if (value->length()) {
                document()->invalidNamedAccessCacheIfNeeded(value, true, false);
            }
        }
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

    if (document()->styleResolver().mayHaveAttrSelectorWithName(
            name.localNameAtomic())) {
        setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
    }
}

static ComputedStyleDamage comparePseudoElementStyle(ComputedStyle* oldStyle,
                                                     ComputedStyle* newStyle,
                                                     bool* damagedKeys)
{
    STARFISH_ASSERT(oldStyle != nullptr);
    STARFISH_ASSERT(newStyle != nullptr);
    STARFISH_ASSERT(damagedKeys != nullptr);

    ComputedStyleDamage damage = ComputedStyleDamage::ComputedStyleDamageNone;
    // The style for the 'content' property is computed when we build the frame
    // tree if it is needed.

    ContentDataGroup* oldContent =
        oldStyle->hasRareComputeStyleData()
            ? oldStyle->rareComputedStyleData()->content()
            : nullptr;
    ContentDataGroup* newContent =
        newStyle->hasRareComputeStyleData()
            ? newStyle->rareComputedStyleData()->content()
            : nullptr;

    if (newContent == nullptr && oldContent == nullptr) {
    } else if (newContent == nullptr || oldContent == nullptr) {
        damagedKeys[CSSStyleValuePair::KeyKind::Content] = true;
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
    } else {
        if (*oldContent != *newContent) {
            damagedKeys[CSSStyleValuePair::KeyKind::Content] = true;
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);
        }
    }

    return damage;
}

void Element::didComputedStyleChanged(ComputedStyle* oldStyle,
                                      ComputedStyle* newStyle)
{
    Node::didComputedStyleChanged(oldStyle, newStyle);

    Frame* frame = Element::frame();
    if (newStyle == nullptr) {
        if (hasRareMembers()) {
            if (rareMembers()->m_pseudoElementMap) {
                rareMembers()->m_pseudoElementMap->clear();
            }
        }
    } else if (!isPseudoElement()) {
        // ensure pseudo elements
        for (int i = PseudoElementType::PseudoElementGeneralTypeStart;
             i <= PseudoElementType::PseudoElementGeneralTypeEnd; i++) {
            PseudoElementType type = (PseudoElementType)i;
            bool o = oldStyle ? oldStyle->seenPseudoElement(type) : false;
            bool n = newStyle->seenPseudoElement(type);

            if (o != n) {
                setNeedsFrameTreeBuild();
            }

            if (type == PseudoElementBefore || type == PseudoElementAfter) {
                PseudoElementMap* pseudoElementMap =
                    ensureRareElementMembers()->ensurePseudoElementMap();
                ComputedStyle* ocs =
                    oldStyle ? oldStyle->pseudoStyle(this, type) : nullptr;
                o = ocs && pseudoElementFrameIsNeeded(ocs) && ocs->content();
                ComputedStyle* ncs =
                    n ? newStyle->pseudoStyle(this, type) : nullptr;
                n = ncs && pseudoElementFrameIsNeeded(ncs) && ncs->content();

                if (n) {
                    if (pseudoElementMap->pseudoElement(type) == nullptr) {
                        PseudoElement* pseudoElement =
                            new PseudoElement(document(), this, type);
                        pseudoElementMap->setPseudoElement(type, pseudoElement);
                        pseudoElement->setParentNode(this);
                        pseudoElement->setStyle(ncs);
                    } else {
                        pseudoElementMap->pseudoElement(type)->setStyle(ncs);
                    }
                } else {
                    pseudoElementMap->setPseudoElement(type, nullptr);
                }

                if (o && n) {
                    Element* pseudoNode = pseudoElementMap->pseudoElement(type);

                    bool damagedKeys[CSSStyleValuePair::KeyKindSize] = {
                        false,
                    };

                    ComputedStyleDamage damage = (ComputedStyleDamage)(
                        compareStyle(ocs, ncs, damagedKeys) |
                        comparePseudoElementStyle(ocs, ncs, damagedKeys));

                    if (damage !=
                        ComputedStyleDamage::ComputedStyleDamageNone) {
                        computeTransition(pseudoNode, ocs, pseudoNode->frame(),
                                          ncs, damage, damagedKeys);
#if defined(STARFISH_ENABLE_ANIMATION)
                        if (ocs->animationNameSize() > 0) {
                            computeAnimationKeyframes(
                                pseudoNode->document()->styleResolver(),
                                pseudoNode, oldStyle, pseudoNode->frame(), ocs,
                                damage);
                            if (ocs->animation()->allKeyframeListSize() > 0) {
                                computeAnimation(pseudoNode, ocs,
                                                 pseudoNode->frame(), ncs,
                                                 damage);
                            }
                        }
#endif
                        if ((damage & ComputedStyleDamage::
                                          ComputedStyleDamageInherited) ||
                            (damage & ComputedStyleDamage::
                                          ComputedStyleDamageRebuildFrame)) {
                            setNeedsFrameTreeBuild();
                        }

                        if (damage &
                            ComputedStyleDamage::ComputedStyleDamageLayout) {
                            if (pseudoNode) {
                                pseudoNode->setNeedsLayout();
                            }
                        }

                        if (damage &
                            ComputedStyleDamage::
                                ComputedStyleDamageEstablishesStackingContext) {
                            webView()->setNeedsEstablishesStackingContext();
                        }

                        if (damage &
                            ComputedStyleDamage::
                                ComputedStyleDamageComputeStackingContextProperties) {
                            webView()
                                ->setNeedsComputeStackingContextProperties();
                        }

                        if (damage &
                            ComputedStyleDamage::ComputedStyleDamagePainting) {
                            if (pseudoNode) {
                                pseudoNode->setNeedsPainting();
                            }
                        }

                        if (damage &
                            ComputedStyleDamage::ComputedStyleDamageComposite) {
                            setNeedsComposite();
                        }
                    }
                }
            } else {
                if (!needsFrameTreeBuild() && frame) {
                    if (o && n) {
                        ComputedStyle* ocs = oldStyle->pseudoStyle(this, type);
                        ComputedStyle* ncs =
                            newStyle->pseudoStyle(this, type, nullptr, ocs);
                        bool damagedKeys[CSSStyleValuePair::KeyKindSize] = {
                            false,
                        };
                        if (compareStyle(ocs, ncs, damagedKeys) !=
                            ComputedStyleDamageNone) {
                            setNeedsFrameTreeBuild();
                        }
                    }
                }
            }
        }
    }

    if (newStyle) {
        if (frame && frame->isFrameBlockBox()) {
            frame = frame->firstChild();
            while (frame) {
                if (frame->isAnonymous()) {
                    frame->updateComputedStyle(this);
                }
                frame = frame->next();
            }
        }
    }
}

LayoutRect Element::clientRect()
{
    window()->browsingContext()->webView()->layoutIfNeeded(false);
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
        bool isDownEvent =
            (event->isMouseEvent() && event->type()->equals("mousedown")) ||
            (event->isTouchEvent() && event->type()->equals("touchstart"));
        auto ox = frame()->appliedOverflowX();
        auto oy = frame()->appliedOverflowY();

        if (isDownEvent &&
            ensureRareElementMembers()
                ->ensureScrolling(this)
                ->handleDefaultEvent(event, window(),
                                     frame()->asFrameBlockBox(), ox, oy)) {
            return true;
        }
    }
#ifdef STARFISH_ENABLE_TTS
    WebView* wv = document()->window()->webView();
    if (wv->tts()->isAccessibilityMode() ||
        wv->tts()->mode() == LWE::TTSMode::Forced) {
        if (isHTMLElement() && isFocusable() && event->isFocusEvent() &&
            event->type()->equals("focus")) {
            TextAlternativeHelper tah(wv);
            String* altText = tah.getComputedTextAlternative(this);
            if (altText->length()) {
                wv->tts()->speech(this, altText);
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
    LayoutUnit remainSpaceToScrollEndHorizontal;
    if (alignToTop) {
        remainSpaceToScrollEnd = rect->top();
        remainSpaceToScrollEndHorizontal = rect->left();
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
        e = this->parentElement();
        while (e && remainSpaceToScrollEnd) {
            if (e->canScrollHorizontally()) {
                LayoutUnit initialValue = e->scrollLeft(false);
                LayoutUnit outer = e->frame()->asFrameBox()->paddingLeft() +
                                   e->frame()->asFrameBox()->borderLeft();
                DOMRect* eBounds = e->getBoundingClientRect();
                outer += (LayoutUnit)eBounds->left();
                e->setScrollLeft(initialValue +
                                     remainSpaceToScrollEndHorizontal - outer,
                                 false);
                LayoutUnit now = e->scrollLeft(false);
                remainSpaceToScrollEndHorizontal -= (now - initialValue);
            }
            e = e->parentElement();
        }

        if (remainSpaceToScrollEnd) {
            window()->scrollTo(window()->scrollX() +
                                   remainSpaceToScrollEndHorizontal,
                               window()->scrollY() + remainSpaceToScrollEnd);
        }
    } else {
        remainSpaceToScrollEnd = rect->bottom();
        remainSpaceToScrollEndHorizontal = rect->left();
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
        e = this->parentElement();
        while (e && remainSpaceToScrollEnd) {
            if (e->canScrollHorizontally()) {
                LayoutUnit initialValue = e->scrollLeft(false);
                LayoutUnit outer = e->frame()->asFrameBox()->paddingLeft() +
                                   e->frame()->asFrameBox()->borderLeft();
                DOMRect* eBounds = e->getBoundingClientRect();
                outer += (LayoutUnit)eBounds->left();
                e->setScrollLeft(initialValue +
                                     remainSpaceToScrollEndHorizontal - outer,
                                 false);
                LayoutUnit now = e->scrollLeft(false);
                remainSpaceToScrollEndHorizontal -= (now - initialValue);
            }
            e = e->parentElement();
        }

        if (remainSpaceToScrollEnd) {
            remainSpaceToScrollEnd -= window()->innerHeight();
            window()->scrollTo(window()->scrollX() +
                                   remainSpaceToScrollEndHorizontal,
                               window()->scrollY() + remainSpaceToScrollEnd);
        }
    }
}

double Element::scrollLeftProperty(bool layoutIfNeeds)
{
    // NOTE DOM interface only
    if (layoutIfNeeds) {
        window()->browsingContext()->webView()->layoutIfNeeded(false);
    }

    if (document()->rootElement() == this) {
        if (!document()->inQuirksMode()) {
            return document()->frame()->asFrameDocument()->scrollLeft();
        }
        return 0;
    }

    if (isHTMLBodyElement() && document()->inQuirksMode() &&
        !asHTMLBodyElement()->isPotentiallyScrollable()) {
        return document()->frame()->asFrameDocument()->scrollLeft();
    }

    if (!frame() || !frame()->isFrameBlockBox()) {
        return 0;
    }

    if (!isHTMLInputElement()) {
        if (appliedOverflowX() < OverflowValue::HiddenOverflow) {
            return 0;
        }
    }

    if (hasRareMembers()) {
        return rareMembers()->m_scrollLeft;
    }
    return 0;
}

double Element::scrollLeft(bool layoutIfNeeds)
{
    if (layoutIfNeeds) {
        window()->browsingContext()->webView()->layoutIfNeeded(false);
    }

    if (!frame() || !frame()->isFrameBlockBox()) {
        return 0;
    }

    if (!isHTMLInputElement()) {
        if (appliedOverflowX() < OverflowValue::HiddenOverflow) {
            return 0;
        }
    }

    if (hasRareMembers()) {
        return rareMembers()->m_scrollLeft;
    }
    return 0;
}

void Element::setScrollLeftProperty(double s, bool layoutIfNeeds)
{
    // NOTE DOM interface only
    setScrollLeft(s, layoutIfNeeds);
}

static void elementScrollPropertyChanged(Element* element)
{
    element->ensureRareElementMembers()
        ->ensureScrolling(element)
        ->markAsActive();
    element->ensureRareElementMembers()
        ->ensureScrolling(element)
        ->giveDamageToTarget();

    String* eventType =
        element->starfish()->staticStrings()->m_scroll.localName();
    UIEvent* e = new UIEvent(element->executionContext(), eventType);
    e->setTarget(element);
    e->setView(element->window());
    if (element->document()->browsingContext()->isTopLevelBrowsingContext()) {
        element->dispatchEventByUA(e);
    } else {
        element->dispatchEventIdleTimeByUA(e);
    }
}

bool Element::setScrollLeft(double s, bool layoutIfNeeds)
{
    // https://drafts.csswg.org/cssom-view/#dom-element-scrollleft
    if (layoutIfNeeds) {
        window()->browsingContext()->webView()->layoutIfNeeded(false);
    }

    if (!window()) {
        return false;
    }

    if (document()->rootElement() == this) {
        if (!document()->inQuirksMode()) {
            return window()->scrollTo(s, window()->scrollY());
        }
        return false;
    }

    if (isHTMLBodyElement() && document()->inQuirksMode() &&
        !asHTMLBodyElement()->isPotentiallyScrollable()) {
        return window()->scrollTo(s, window()->scrollY());
    }

    if (!frame() || !frame()->isFrameBlockBox() ||
        appliedOverflowX() < OverflowValue::HiddenOverflow) {
        return false;
    }

    auto scrollMax = (frame()->asFrameBlockBox()->width() -
                      frame()->asFrameBlockBox()->borderWidth())
                         .toUnsigned();
    if (s > scrollWidth() - scrollMax) {
        s = scrollWidth() - scrollMax;
    }

    if (s < 0) {
        s = 0;
    }

    if (ensureRareElementMembers()->m_scrollLeft != (LayoutUnit)s) {
        ensureRareElementMembers()->m_scrollLeft = s;
        elementScrollPropertyChanged(this);
        return true;
    }

    return false;
}

double Element::scrollTopProperty(bool layoutIfNeeds)
{
    if (layoutIfNeeds) {
        window()->browsingContext()->webView()->layoutIfNeeded(false);
    }

    if (document()->rootElement() == this) {
        if (!document()->inQuirksMode()) {
            return document()->frame()->asFrameDocument()->scrollTop();
        }
        return 0;
    }

    if (isHTMLBodyElement() && document()->inQuirksMode() &&
        !asHTMLBodyElement()->isPotentiallyScrollable()) {
        return document()->frame()->asFrameDocument()->scrollTop();
    }

    if (!frame() || !frame()->isFrameBlockBox()) {
        return 0;
    }

    if (appliedOverflowY() < OverflowValue::HiddenOverflow) {
        return 0;
    }

    if (hasRareMembers()) {
        return rareMembers()->m_scrollTop;
    }
    return 0;
}

double Element::scrollTop(bool layoutIfNeeds)
{
    if (layoutIfNeeds) {
        window()->browsingContext()->webView()->layoutIfNeeded(false);
    }

    if (!frame() || !frame()->isFrameBlockBox()) {
        return 0;
    }

    if (appliedOverflowY() < OverflowValue::HiddenOverflow) {
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
        window()->browsingContext()->webView()->layoutIfNeeded(false);
    }

    if (!frame() || !frame()->isFrameBlockBox()) {
        return false;
    }

    if (appliedOverflowY() < OverflowValue::HiddenOverflow) {
        return false;
    }

    return frame()->asFrameBlockBox()->hasBiggerContentThanFrameHeight();
}

bool Element::canScrollHorizontally(bool layoutIfNeeds)
{
    if (layoutIfNeeds) {
        window()->browsingContext()->webView()->layoutIfNeeded(false);
    }

    if (!frame() || !frame()->isFrameBlockBox()) {
        return false;
    }

    if (appliedOverflowX() < OverflowValue::HiddenOverflow) {
        return false;
    }

    return frame()->asFrameBlockBox()->hasBiggerContentThanFrameWidth();
}

void Element::setScrollTopProperty(double s, bool layoutIfNeeds)
{
    // NOTE DOM interface only
    setScrollTop(s, layoutIfNeeds);
}

bool Element::setScrollTop(double s, bool layoutIfNeeds)
{
    // https://drafts.csswg.org/cssom-view/#dom-element-scrolltop
    if (layoutIfNeeds) {
        window()->browsingContext()->webView()->layoutIfNeeded(false);
    }

    if (!window()) {
        return false;
    }

    if (document()->rootElement() == this) {
        if (!document()->inQuirksMode()) {
            return window()->scrollTo(window()->scrollX(), s);
        }
        return false;
    }

    if (isHTMLBodyElement() && document()->inQuirksMode() &&
        !asHTMLBodyElement()->isPotentiallyScrollable()) {
        return window()->scrollTo(window()->scrollX(), s);
    }

    if (!frame() || !frame()->isFrameBlockBox() ||
        appliedOverflowY() < OverflowValue::HiddenOverflow) {
        return false;
    }

    auto scrollMax = (frame()->asFrameBlockBox()->height() -
                      frame()->asFrameBlockBox()->borderHeight())
                         .toUnsigned();
    if (s > scrollHeight() - scrollMax) {
        s = scrollHeight() - scrollMax;
    }

    if (s < 0) {
        s = 0;
    }

    if (ensureRareElementMembers()->m_scrollTop != (LayoutUnit)s) {
        ensureRareElementMembers()->m_scrollTop = s;
        elementScrollPropertyChanged(this);
        return true;
    }

    return false;
}

uint32_t Element::scrollWidth()
{
    window()->browsingContext()->webView()->layoutIfNeeded(false);
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
    window()->browsingContext()->webView()->layoutIfNeeded(false);
    if (!frame()) {
        return 0;
    }
    if (!frame()->isFrameBlockBox()) {
        return 0;
    }
    return frame()->asFrameBlockBox()->scrollHeight();
}

void Element::scroll(double x, double y)
{
    scrollTo(x, y);
}

void Element::scrollTo(double x, double y)
{
    window()->browsingContext()->webView()->layoutIfNeeded(false);

    if (!window()) {
        return;
    }

    if (document()->rootElement() == this) {
        if (!document()->inQuirksMode()) {
            window()->scrollTo(x, y);
        }
        return;
    }

    if (isHTMLBodyElement() && document()->inQuirksMode() &&
        !asHTMLBodyElement()->isPotentiallyScrollable()) {
        window()->scrollTo(x, y);
        return;
    }

    if (!frame() || !frame()->isFrameBlockBox()) {
        return;
    }

    bool scrolled = false;
    if (appliedOverflowX() >= OverflowValue::HiddenOverflow) {
        auto scrollMaxW = (frame()->asFrameBlockBox()->width() -
                           frame()->asFrameBlockBox()->borderWidth())
                              .toUnsigned();
        if (x > scrollWidth() - scrollMaxW) {
            x = scrollWidth() - scrollMaxW;
        }

        if (x < 0) {
            x = 0;
        }

        if (ensureRareElementMembers()->m_scrollLeft != (LayoutUnit)x) {
            ensureRareElementMembers()->m_scrollLeft = x;
            scrolled = true;
        }
    }

    if (appliedOverflowY() >= OverflowValue::HiddenOverflow) {
        auto scrollMaxH = (frame()->asFrameBlockBox()->height() -
                           frame()->asFrameBlockBox()->borderHeight())
                              .toUnsigned();
        if (y > scrollHeight() - scrollMaxH) {
            y = scrollHeight() - scrollMaxH;
        }

        if (y < 0) {
            y = 0;
        }

        if (ensureRareElementMembers()->m_scrollTop != (LayoutUnit)y) {
            ensureRareElementMembers()->m_scrollTop = y;
            scrolled = true;
        }
    }

    if (scrolled) {
        elementScrollPropertyChanged(this);
    }
}

void Element::getClientQuads(GCVector<DOMQuad*>& quads, bool layoutIfNeeds)
{
    if (layoutIfNeeds) {
        window()->webView()->layoutIfNeeded(false);
    }

    Frame* frameObject = this->frame();
    if (!frameObject) {
        return;
    }
    // TODO : support SVG model
    // there is Getting bounding rectangle from the SVG model in the spec, but
    // SVG model is not supported

    if (frameObject->isFrameBox()) {
        SkMatrix m = frameObject->asFrameBox()->computeScreenMatrix();
        LayoutRect rect;
        rect.setWidth(frameObject->asFrameBox()->width());
        rect.setHeight(frameObject->asFrameBox()->height());
        rect = computeBoxExtent(rect, m);

        DOMQuad* q = new DOMQuad(
            executionContext(),
            DOMPointInit(rect.location().x(), rect.location().y()),
            DOMPointInit(rect.location().x() + rect.size().width(),
                         rect.location().y()),
            DOMPointInit(rect.location().x() + rect.size().width(),
                         rect.location().y() + rect.size().height()),
            DOMPointInit(rect.location().x(),
                         rect.location().y() + rect.size().height()));

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
                        SkMatrix m =
                            childBox->asFrameBox()->computeScreenMatrix();
                        LayoutRect rect;
                        rect.setWidth(childBox->asFrameBox()->width());
                        rect.setHeight(childBox->asFrameBox()->height());
                        rect = computeBoxExtent(rect, m);

                        DOMQuad* q = new DOMQuad(
                            executionContext(),
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
        return DOMRectList::create(executionContext());
    }

    return DOMRectList::create(executionContext(), quads);
}

DOMRect* Element::getBoundingClientRect(bool layoutIfNeeds)
{
    GCVector<DOMQuad*> quads;
    getClientQuads(quads, layoutIfNeeds);
    if (quads.empty()) {
        return new DOMRect(executionContext());
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
    HTMLParser parser(document->starfish(), df, contextElement, src);
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
        throw new DOMException(executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "Parent can not be document");
    }
    // If parent is a DocumentFragment, let parent be a new Element with:
    if (parent->isDocumentFragment()) {
        // body as its local name,
        // The HTML namespace as its namespace, and
        // The context object's node document as its node document.
        parent = new HTMLBodyElement(
            document(), starfish()->staticStrings()->m_bodyTagName);
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
            throw new DOMException(executionContext(),
                                   DOMException::NO_MODIFICATION_ALLOWED_ERR,
                                   "Can not execute `insertAdjacentHTML`.");
        }
    } else if (position->equalsIgnoreCase("afterbegin") ||
               position->equalsIgnoreCase("beforeend")) {
        context = this;
    } else {
        throw new DOMException(executionContext(), DOMException::SYNTAX_ERR,
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
        context = new HTMLBodyElement(
            document(), starfish()->staticStrings()->m_bodyTagName);
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
        throw new DOMException(element->executionContext(),
                               DOMException::SYNTAX_ERR,
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
        newNode = HTMLDocument::createHTMLElement(document(), name());
    } else if (isSVGElement()) {
        newNode = HTMLDocument::createHTMLElement(document(), name());
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

void Element::setId(String* id)
{
    setAttribute(starfish()->staticStrings()->m_id, id);
}

String* Element::className()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_class);
}

void Element::setClassName(String* className)
{
    setAttribute(starfish()->staticStrings()->m_class, className);
}

bool Element::hasClassName(String* className)
{
    bool containsOnlyASCIIChars = className->containsOnlyASCIIChars();
    for (unsigned i = 0; i < m_classNames.size(); i++) {
        if (document()->inQuirksMode() && containsOnlyASCIIChars) {
            if (className->equalsIgnoreCase(m_classNames[i].string())) {
                return true;
            }
        } else if (className->equals(m_classNames[i].string())) {
            return true;
        }
    }
    return false;
}

bool Element::hasClassName(AtomicString className)
{
    size_t len = m_classNames.size();
    String* classNameString = className.string();
    bool containsOnlyASCIIChars = classNameString->containsOnlyASCIIChars();
    for (unsigned i = 0; i < len; i++) {
        if (document()->inQuirksMode() && containsOnlyASCIIChars) {
            if (classNameString->equalsIgnoreCase(m_classNames[i].string())) {
                return true;
            }
        } else if (className == m_classNames[i]) {
            return true;
        }
    }
    return false;
}

void Element::setStyleAttr(String* style)
{
    setAttribute(starfish()->staticStrings()->m_style, style);
}

void Element::registerInlineStyleCallback()
{
    attributeData(starfish()->staticStrings()->m_style)
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
    setNeedsStyleRecalc(StyleChangeReason::InlineStyleChange);
    m_didInlineStyleModifiedAfterAttributeSet = true;
    if (hasAttribute(starfish()->staticStrings()->m_style) == SIZE_MAX) {
        m_attributes.push_back(Attribute(starfish()->staticStrings()->m_style,
                                         String::emptyString));
        registerInlineStyleCallback();
    }
}

CSSStyleDeclaration* Element::inlineStyle()
{
    if (m_inlineStyle == nullptr) {
        m_inlineStyle = new InlineCSSStyleDeclaration(this);
    }
    return m_inlineStyle;
}

CSSStyleDeclaration* Element::getComputedStyle()
{
    return new ComputedStyleCSSStyleDeclaration(this);
}

#ifdef STARFISH_ENABLE_TEST
void Element::dumpStyle()
{
    dump();
    printf(", style: { ");
    auto s = getComputedStyle()->generateCSSText()->toUTF8NonGCString();
    printf("%s", s.data());
    printf("}");
}
#endif

String* Element::getDir()
{
    Node* n = this;
    String* value = String::emptyString;

    do {
        if (n->isElement()) {
            value = n->asElement()->getAttributeOrEmpty(
                n->starfish()->staticStrings()->m_dir);
        }
        n = n->parentNode();
    } while (n && value->equals(String::emptyString));

    return value;
}

String* Element::getLaunguage()
{
    Node* n = this;
    String* value = String::emptyString;

    do {
        if (n->isElement()) {
            value = n->asElement()->getAttributeOrEmpty(
                n->starfish()->staticStrings()->m_lang);
        } else if (n->isDocument()) {
            value = document()->contentLanguage();
        }

        n = n->parentNode();
    } while (n && value->equals(String::emptyString));

    return value;
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

int Element::tabIndex()
{
    return m_tabIndex;
}

bool Element::tabIndexSetExplicitly() const
{
    return m_tabIndexWasSetExplicitly;
}

void Element::focus()
{
    // TODO: Consider nested browsing contexts.
    window()->browsingContext()->setFocusedNode(this, false);
}

void Element::blur()
{
    window()->browsingContext()->releaseFocusedNode(this);
}
}
