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
#include "core/dom/Attr.h"
#include "core/dom/CustomElementRegistry.h"
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
#include "core/dom/MutationObserver.h"
#include "core/dom/MutationObservationScope.h"
#include "core/dom/svg/SVGDocument.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/HTMLSlotElement.h"
#include "core/dom/NamedNodeMap.h"
#include "core/dom/SelectorQuery.h"
#include "core/dom/Text.h"
#include "core/dom/PseudoElement.h"
#include "core/dom/parser/HTMLParser.h"
#include "core/dom/parser/HTMLParserIdioms.h"
#include "core/dom/xml/XMLSerializer.h"
#include "core/dom/UIEvent.h"
#include "core/dom/Scrolling.h"
#include "core/dom/svg/SVGElement.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBox.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/svg/FrameSVGBox.h"
#include "core/layout/StackingContext.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/style/ComputedStyle.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/csp/ContentSecurityPolicy.h"
#include "core/animation/TimingOptions.h"
#include "core/dom/IntersectionObserver.h"
#include "core/modules/resize_observer/ResizeObserver.h"

#include "binding/ScriptBindingInstance.h"
#include "core/style/CSSStyleLookupTrie.h"
#include "core/style/StyleRule.h"
#include "core/animation/AnimationApplier.h"
#include "core/animation/AnimationExecutor.h"
#include "core/animation/AnimationTask.h"
#include "core/dom/ShadowRoot.h"

#include <EscargotPublic.h>
using namespace Escargot;

#ifdef STARFISH_ENABLE_TTS
#include "core/modules/tts/TextAlternativeHelper.h"
#include "core/modules/tts/TTS.h"
#ifdef STARFISH_ENABLE_A11Y_TOUCH_EXPLORATION
#include "core/page/A11yTouchExploration.h"
#endif
#endif
#ifdef STARFISH_ENABLE_A11Y_ATSPI
#include "core/page/A11yAtspiTreeSource.h"
#endif

namespace Starfish {

static bool hiddenElementChangeIsContained(Element* element, bool paintOnly);

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

static AttributeName properAttributeNameNS(Element* e, Optional<String*> ns,
                                           String* name)
{
    if (ns.hasValue() && ns.getValue()->equals(String::emptyString)) {
        ns = Optional<String*>();
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

GCVector<IntersectionObserverRegistration*>*
RareElementMembers::ensureRegisteredIntersectionObservers()
{
    if (!m_registeredIntersectionObservers) {
        m_registeredIntersectionObservers =
            new (GC) GCVector<IntersectionObserverRegistration*>();
    }
    return m_registeredIntersectionObservers;
}

GCVector<ResizeObserverRegistration*>*
RareElementMembers::ensureRegisteredResizeObservers()
{
    if (!m_registeredResizeObservers) {
        m_registeredResizeObservers =
            new (GC) GCVector<ResizeObserverRegistration*>();
    }
    return m_registeredResizeObservers;
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

Optional<String*> Element::namespaceURI()
{
    auto v = name().namespaceURI();
    if (v.hasValue()) {
        return v.getValue().string();
    } else {
        return Optional<String*>();
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

Optional<String*> Element::prefix()
{
    auto v = name().prefix();
    if (v.hasValue()) {
        return v.getValue().string();
    } else {
        return Optional<String*>();
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

bool Element::hasAttributeNS(Optional<String*> ns, String* name)
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

Optional<String*> Element::getAttribute(const AttributeName& name) const
{
    size_t idx = hasAttribute(name);
    if (idx == SIZE_MAX) {
        return Optional<String*>();
    }
    return Optional<String*>(m_attributes[idx].value());
}

Optional<String*> Element::getAttribute(
    const QualifiedName& qualifiedName) const
{
    return getAttribute(AttributeName(qualifiedName, AttributeName::MatchName));
}

Optional<String*> Element::getAttribute(String* name)
{
    return getAttribute(properAttributeName(this, name));
}

Optional<String*> Element::getAttributeNS(Optional<String*> ns,
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

Attr* Element::getAttributeNodeNS(Optional<String*> ns, String* name)
{
    return getAttributeNode(properAttributeNameNS(this, ns, name));
}

String* Element::getAttributeOrEmpty(const QualifiedName& qualifiedName) const
{
    Optional<String*> result = getAttribute(qualifiedName);
    if (result.hasValue()) {
        return result.getValue();
    }
    return String::emptyString;
}

String* Element::getAttributeOrVarReferencedValue(
    const QualifiedName& attributeName,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    String* attributeValue = getAttributeOrEmpty(attributeName);
    if (attributeValue->startsWith("var(")) {
        std::string newValue = StyleResolver::resolveVarReferencedValue(
            this, attributeValue->toOptionalUTF8String(), cssCustomValues);
        return String::fromUTF8(newValue.c_str(), newValue.size());
    }
    return attributeValue;
}

void Element::invokeDidAttributeChanged(QualifiedName name,
                                        Optional<String*> old, String* value,
                                        bool attributeCreated,
                                        bool attributeRemoved)
{
    STARFISH_ASSERT(value != nullptr);

    MutationObservationScope mutationScope;
    mutationScope.startAttributeMutationScope(this, name, old);
#if !defined(NDEBUG)
    m_didAttributeChangedCorrectlyInvoked = false;
#endif
    didAttributeChanged(name, old, value, attributeCreated, attributeRemoved);
#if !defined(NDEBUG)
    STARFISH_ASSERT(m_didAttributeChangedCorrectlyInvoked);
#endif
#ifdef STARFISH_ENABLE_A11Y_ATSPI
    // Attribute changes can move a11y tree membership (aria-hidden, role,
    // tabindex, ...), names, or geometry; ping the AT-SPI tree source so it
    // re-diffs (chromium marks the AXObject dirty here).
    A11yAtspiTreeSource::notifyPageChanged(document());
#endif
}

void Element::setAttribute(const AttributeName& name, String* value)
{
    STARFISH_ASSERT(value != nullptr);
    STARFISH_ASSERT(name.qname().localName()->length());

    size_t idx = hasAttribute(name);
    if (idx == SIZE_MAX) {
        m_attributes.push_back(Attribute(name.qname(), value));
        invokeDidAttributeChanged(name.qname(), nullptr, value, true, false);
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

void Element::setAttributeNS(Optional<String*> ns, String* qualifiedName,
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

void Element::removeAttributeNS(Optional<String*> ns, String* localName)
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

bool Element::toggleAttribute(String* qualifiedName, Optional<bool> force)
{
    if (!QualifiedName::checkNameProductionRule(qualifiedName)) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INVALID_CHARACTER_ERR);
    }

    AttributeName attName = properAttributeName(this, qualifiedName);
    size_t idx = hasAttribute(attName);
    if (idx == SIZE_MAX) {
        if (!force.hasValue() || force.value()) {
            QualifiedName qname = attName.qname();
            m_attributes.push_back(Attribute(qname, String::emptyString));
            invokeDidAttributeChanged(qname, nullptr, String::emptyString, true,
                                      false);
            return true;
        } else {
            return false;
        }
    } else {
        if (!force.hasValue() || !force.value()) {
            removeAttribute(idx);
            return false;
        } else {
            return true;
        }
    }
    return false;
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

void Element::didAttributeChanged(QualifiedName name, Optional<String*> old,
                                  String* value, bool attributeCreated,
                                  bool attributeRemoved)
{
#ifdef STARFISH_TC_COVERAGE
    if (name.localName()->equals("style")) {
        STARFISH_LOG_INFO("+++attr:&&&style");
    } else {
        auto s = name.localName()->toUTF8NonGCString();
        STARFISH_LOG_INFO("+++attr:%s", s.data());
    }
#endif

    STARFISH_ASSERT(value != nullptr);
#if !defined(NDEBUG)
    STARFISH_ASSERT(!m_didAttributeChangedCorrectlyInvoked);
    m_didAttributeChangedCorrectlyInvoked = true;
#endif

    StaticStrings* ss = starfish()->staticStrings();
    if (name == ss->m_id) {
        AtomicString oldName = m_id;
        if (attributeRemoved) {
            m_id = AtomicString::emptyAtomicString();
        } else {
            m_id = AtomicString::createAtomicString(starfish(), value);
        }
        if (oldName != m_id) {
            if (attributeCreated) {
                if (value->length()) {
                    document()->invalidNamedAccessCacheIfNeeded(value, true,
                                                                false);
                }
            } else if (attributeRemoved) {
                if (old && old->length()) {
                    document()->invalidNamedAccessCacheIfNeeded(old.getValue(),
                                                                false, true);
                }
            } else {
                if (old && old->length()) {
                    document()->invalidNamedAccessCacheIfNeeded(old.getValue(),
                                                                false, true);
                }
                if (value->length()) {
                    document()->invalidNamedAccessCacheIfNeeded(value, true,
                                                                false);
                }
            }
            setNeedsStyleRecalc(StyleChangeReason::IdChange);
        }
    } else if (name == ss->m_class) {
        GCVector<StringView> tokens;
        DOMTokenList::tokenize(value, tokens);

        GCAtomicTightVector<AtomicString> newClassNames;
        newClassNames.resize(tokens.size());
        for (size_t i = 0; i < tokens.size(); i++) {
            newClassNames[i] =
                AtomicString::createAtomicString(starfish(), tokens[i]);
        }

        bool hasSameContent = true;

        if (newClassNames.size() != m_classNames.size()) {
            hasSameContent = false;
        } else {
            for (size_t i = 0; i < newClassNames.size(); i++) {
                if (newClassNames[i] != m_classNames[i]) {
                    hasSameContent = false;
                    break;
                }
            }
        }

        if (!hasSameContent) {
            m_classNames = std::move(newClassNames);

            // propagate invalidate nodeList cache(getElementsByClassName)
            // damage to
            // parent tree
            Node* parent = parentNode();
            while (parent) {
                parent
                    ->invalidateNodeListCacheDueToChangeClassNameOfDescendant();
                parent = parent->parentNode();
            }

            setNeedsStyleRecalc(StyleChangeReason::ClassChange);
        }
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
        // A whole-declaration replacement may add or drop any property, so
        // the renderer may only stay asleep when neither the old nor the
        // new declaration touches a rendering-critical property and the
        // hidden element's layout effects are contained (see
        // hiddenElementChangeIsContained).
        bool hadRenderingCriticalProperties =
            inlineStyle()->hasRenderingCriticalProperties();
        inlineStyle()->clear();

        if (!value->isEmpty()) {
            CSSParser parser(this);
            parser.parseStyleDeclaration(value, inlineStyle());
        }
        m_didInlineStyleModifiedAfterAttributeSet = false;
        bool scheduleRendering =
            hadRenderingCriticalProperties ||
            inlineStyle()->hasRenderingCriticalProperties() ||
            !hiddenElementChangeIsContained(this, false);
        setNeedsStyleRecalc(StyleChangeReason::InlineStyleChange,
                            scheduleRendering);
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
            if (old && old->length()) {
                document()->invalidNamedAccessCacheIfNeeded(old.getValue(),
                                                            false, true);
            }
        } else {
            if (old && old->length()) {
                document()->invalidNamedAccessCacheIfNeeded(old.getValue(),
                                                            false, true);
            }
            if (value->length()) {
                document()->invalidNamedAccessCacheIfNeeded(value, true, false);
            }
        }
    } else if (name == ss->m_tabindex) {
        int tabIndex = 0;
        if (!value->isEmpty() && parseHTMLInteger(value, tabIndex)) {
            m_tabIndexWasSetExplicitly = true;
        } else {
            m_tabIndexWasSetExplicitly = false;
        }
        document()->invalidFocusRingCacheIfNeeded();
        document()->clearDialogsInShowModalCache();
    } else if (name == ss->m_slot) {
        // A slottable's slot= changed: re-run slot assignment in the shadow
        // tree it is distributed into (its parent host's shadow root). The
        // slot set is unchanged, so only reassignment is needed. Also covers
        // a <slot> element that is itself slotted (its own slot= attribute),
        // since HTMLSlotElement::didAttributeChanged chains here.
        Element* parent = parentElement();
        if (parent != nullptr) {
            Optional<ShadowRoot*> shadowRoot = parent->internalShadowRoot();
            if (shadowRoot) {
                shadowRoot.value()->connectSlotWithSlottables();
            }
        }
    }

    if (styleResolver().mayHaveAttrSelectorWithName(name.localNameAtomic())) {
        setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
    }
    if (UNLIKELY(isShadowRootHost())) {
        ShadowRoot* shadowRoot = internalShadowRoot().value();
        auto& resolver = shadowRoot->styleResolver();
        if (resolver.mayHaveAttrSelectorWithName(name.localNameAtomic())) {
            shadowRoot->setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
        }
    }
}

void Element::didNodeInserted(Node* parent, Node* newChild)
{
    Node::didNodeInserted(parent, newChild);

    Optional<ShadowRoot*> shadowRoot;
    if (parent == this && (shadowRoot = internalShadowRoot())) {
        shadowRoot->connectSlotWithSlottables();
    }
}
void Element::didNodeRemoved(Node* parent, Node* oldChild)
{
    Node::didNodeRemoved(parent, oldChild);

    Optional<ShadowRoot*> shadowRoot;
    if (parent == this && (shadowRoot = internalShadowRoot())) {
        shadowRoot->connectSlotWithSlottables();
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
        damage = (ComputedStyleDamage)(ComputedStyleDamage::
                                           ComputedStyleDamageRebuildFrame |
                                       damage);
    } else {
        if (*oldContent != *newContent) {
            damagedKeys[CSSStyleValuePair::KeyKind::Content] = true;
            damage = (ComputedStyleDamage)(ComputedStyleDamage::
                                               ComputedStyleDamageRebuildFrame |
                                           damage);
        }
    }

    return damage;
}

void Element::didComputedStyleChanged(ComputedStyle* oldStyle,
                                      ComputedStyle* newStyle,
                                      Optional<StyleResolveContext*> ctx)
{
    Node::didComputedStyleChanged(oldStyle, newStyle, ctx);

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

            if (type == PseudoElementBefore || type == PseudoElementAfter) {
                PseudoElementMap* pseudoElementMap =
                    ensureRareElementMembers()->ensurePseudoElementMap();
                ComputedStyle* ocs =
                    oldStyle ? oldStyle->pseudoStyle(this, type, nullptr,
                                                     nullptr, ctx)
                             : nullptr;
                o = ocs && pseudoElementFrameIsNeeded(ocs) && ocs->content();
                ComputedStyle* ncs =
                    n ? newStyle->pseudoStyle(this, type, nullptr, nullptr, ctx)
                      : nullptr;
                n = ncs && pseudoElementFrameIsNeeded(ncs) && ncs->content();
                // we should test actual visiblity on FrameTree. not style
                // existence
                if (o != n) {
                    setNeedsFrameTreeBuild();
                }

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

                    ComputedStyleDamage damage =
                        (ComputedStyleDamage)(compareStyle(
                                                  ocs, ncs, damagedKeys,
                                                  isSVGDescendantElement()) |
                                              comparePseudoElementStyle(
                                                  ocs, ncs, damagedKeys));

                    if (damage !=
                        ComputedStyleDamage::ComputedStyleDamageNone) {
                        if (pseudoNode) {
                            computeTransition(pseudoNode, ocs,
                                              pseudoNode->frame(), ncs, damage,
                                              damagedKeys);
                        }

                        // TODO
                        // implement CSS animtion for pseudo element
                        // here when we can tracking psuedo element is appear

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

                if (ctx && ocs) {
                    ctx->pushIntoComputedStylePool(ocs);
                }
            } else {
                // test just style existence is ok
                if (o != n) {
                    setNeedsFrameTreeBuild();
                }

                if (!needsFrameTreeBuild() && frame) {
                    if (o && n) {
                        ComputedStyle* ocs = oldStyle->pseudoStyle(this, type);
                        ComputedStyle* ncs =
                            newStyle->pseudoStyle(this, type, nullptr, ocs);
                        bool damagedKeys[CSSStyleValuePair::KeyKindSize] = {
                            false,
                        };
                        if (compareStyle(ocs, ncs, damagedKeys,
                                         isSVGDescendantElement()) !=
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

void Element::onGlobalPointingEvent(float x, float y, DOMTimeStamp timeStamp,
                                    GlobalPointingEventKind kind)
{
    rareMembers()->m_scrolling->onGlobalPointingEvent(x, y, timeStamp, kind);
}

bool Element::handleDefaultEvent(Event* event)
{
    if (Node::handleDefaultEvent(event)) {
        return true;
    }

    if (frame() && frame()->isFrameBlockBox() && frame()->style() &&
        frame()->shouldApplyOverflow()) {
        bool isDownOrMoveOrUpEvent =
            ((event->isMouseEvent() && event->type()->equals("mousedown")) ||
             (event->isTouchEvent() && event->type()->equals("touchstart"))) ||
            ((event->isMouseEvent() && event->type()->equals("mousemove")) ||
             (event->isTouchEvent() && event->type()->equals("touchmove"))) ||
            ((event->isMouseEvent() && event->type()->equals("mouseup")) ||
             (event->isTouchEvent() && event->type()->equals("touchend")));
        auto ao = frame()->appliedOverflow();
        auto ox = ao.first;
        auto oy = ao.second;

        if (isDownOrMoveOrUpEvent &&
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
        if (event->type()->equals("focus") && isHTMLElement() &&
            isFocusable() && event->isFocusEvent()) {
#ifdef STARFISH_ENABLE_A11Y_TOUCH_EXPLORATION
            // The touch-exploration controller drives focus + speech itself;
            // skip so the same target isn't spoken twice.
            if (wv->a11yTouchExploration()->isSettingDomFocus()) {
                return false;
            }
#endif
            TextAlternativeHelper tah(wv);
            String* altText = tah.getComputedTextAlternative(this);
            if (altText->length()) {
                wv->tts()->speech(this, altText);
                return true;
            }
        }
    }
#endif
    return false;
}

static bool domRectContainsDOMRect(DOMRect* a, DOMRect* b)
{
    LayoutRect aRect(a->x(), a->y(), a->width(), a->height());
    LayoutRect bRect(b->x(), b->y(), b->width(), b->height());
    return aRect.containsInVisual(bRect);
}

static bool isVisibleToUser(DOMRect* rect, Element* sourceElement)
{
    LayoutRect windowRect(0, 0, sourceElement->window()->innerWidth(),
                          sourceElement->window()->innerHeight());

    if (!windowRect.containsInVisual(rect->x(), rect->y()) ||
        !windowRect.containsInVisual(rect->x() + rect->width(), rect->y()) ||
        !windowRect.containsInVisual(rect->x(), rect->y() + rect->height()) ||
        !windowRect.containsInVisual(rect->x() + rect->width(),
                                     rect->y() + rect->height())) {
        return false;
    }

    auto element = sourceElement->parentElement();
    while (element) {
        if (element->frame() && element->frame()->shouldApplyOverflow()) {
            DOMRect* dm = element->getBoundingClientRect();
            if (!domRectContainsDOMRect(dm, rect)) {
                return false;
            }
        }
        element = element->parentElement();
    }

    return true;
}

static bool isVisibleToUser(Element* e)
{
    DOMRect* rect = e->getBoundingClientRect();
    return isVisibleToUser(rect, e);
}

void Element::scrollIntoViewIfNeeded()
{
    if (!isVisibleToUser(this)) {
        scrollIntoView();
    }
}

void Element::scrollIntoView(bool alignToTop)
{
    ScrollIntoViewOptions options;
    options.setInLine(ScrollLogicalPosition::Nearest);
    if (alignToTop) {
        options.setBlock(ScrollLogicalPosition::Start);
    } else {
        options.setBlock(ScrollLogicalPosition::End);
    }
    scrollIntoView(options);
}

LayoutUnit Element::scrollBlockAlign(ScrollLogicalPosition position)
{
    DOMRect* rect = getBoundingClientRect();
    LayoutUnit remainSpaceToScrollEnd;

    Element* e = parentElement();
    if (position == ScrollLogicalPosition::Start) {
        remainSpaceToScrollEnd = rect->top();
    } else if (position == ScrollLogicalPosition::Center) {
        remainSpaceToScrollEnd = (rect->bottom() + rect->top()) / 2;
    } else if (position == ScrollLogicalPosition::End) {
        remainSpaceToScrollEnd = rect->bottom();
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }

    while (e && remainSpaceToScrollEnd) {
        if (e->canScrollVerticaly()) {
            LayoutUnit initialValue = e->scrollTop(false);
            LayoutUnit outer;
            DOMRect* eBounds = e->getBoundingClientRect();

            if (position == ScrollLogicalPosition::Start) {
                outer = e->frame()->asFrameBox()->paddingTop() +
                        e->frame()->asFrameBox()->borderTop() +
                        (LayoutUnit)eBounds->top();
            } else if (position == ScrollLogicalPosition::Center) {
                outer = (LayoutUnit)((eBounds->top() + eBounds->bottom()) / 2);
            } else {
                outer = -e->frame()->asFrameBox()->paddingBottom() -
                        e->frame()->asFrameBox()->borderBottom() +
                        (LayoutUnit)eBounds->bottom();
            }
            e->setScrollTop(initialValue + remainSpaceToScrollEnd - outer,
                            false);
            LayoutUnit now = e->scrollTop(false);
            remainSpaceToScrollEnd -= (now - initialValue);
            auto rect = getBoundingClientRect(false);
            rect->setWidth(0);
            if (isVisibleToUser(rect, this)) {
                remainSpaceToScrollEnd = 0;
                break;
            }
        }
        e = e->parentElement();
    }

    return remainSpaceToScrollEnd;
}

LayoutUnit Element::scrollInlineAlign(ScrollLogicalPosition position)
{
    DOMRect* rect = getBoundingClientRect();
    LayoutUnit remainSpaceToScrollEndHorizontal;

    if (position == ScrollLogicalPosition::Start) {
        remainSpaceToScrollEndHorizontal = rect->left();
    } else if (position == ScrollLogicalPosition::Center) {
        remainSpaceToScrollEndHorizontal = (rect->left() + rect->right()) / 2;
    } else if (position == ScrollLogicalPosition::End) {
        remainSpaceToScrollEndHorizontal = rect->right();
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }

    Element* e = parentElement();
    while (e && remainSpaceToScrollEndHorizontal) {
        if (e->canScrollHorizontally()) {
            LayoutUnit initialValue = e->scrollLeft(false);
            DOMRect* eBounds = e->getBoundingClientRect();
            LayoutUnit outer;

            if (position == ScrollLogicalPosition::Start) {
                outer = e->frame()->asFrameBox()->paddingLeft() +
                        e->frame()->asFrameBox()->borderLeft() +
                        (LayoutUnit)eBounds->left();
            } else if (position == ScrollLogicalPosition::Center) {
                outer = (LayoutUnit)((eBounds->left() + eBounds->right()) / 2);
            } else {
                outer = -e->frame()->asFrameBox()->paddingRight() -
                        e->frame()->asFrameBox()->borderRight() +
                        (LayoutUnit)eBounds->right();
            }
            e->setScrollLeft(
                initialValue + remainSpaceToScrollEndHorizontal - outer, false);
            LayoutUnit now = e->scrollLeft(false);
            remainSpaceToScrollEndHorizontal -= (now - initialValue);
            auto rect = getBoundingClientRect(false);
            rect->setHeight(0);
            if (isVisibleToUser(rect, this)) {
                remainSpaceToScrollEndHorizontal = 0;
                break;
            }
        }
        e = e->parentElement();
    }

    return remainSpaceToScrollEndHorizontal;
}

void Element::scrollIntoView(ScrollIntoViewOptions options)
{
    DOMRect* rect = getBoundingClientRect();

    LayoutUnit remainSpaceToScrollEnd;
    LayoutUnit remainSpaceToScrollEndHorizontal;
    LayoutRect windowRect(0, 0, window()->innerWidth(),
                          window()->innerHeight());

    if (options.blockValue() == ScrollLogicalPosition::Nearest) {
        Element* e = parentElement();
        if (e != nullptr) {
            LayoutUnit rectHeight = (rect->bottom() - rect->top());
            DOMRect* eBounds = e->getBoundingClientRect();
            LayoutUnit eHeight = (eBounds->bottom() - eBounds->top());
            // If the upper and lower sides of "eBound" are inside "right",
            // do nothing.
            if ((rect->top() < eBounds->top() && rectHeight < eHeight) ||
                (rect->bottom() > eBounds->bottom() && rectHeight > eHeight)) {
                remainSpaceToScrollEnd =
                    scrollBlockAlign(ScrollLogicalPosition::Start);
            } else if ((rect->top() < eBounds->top() && rectHeight > eHeight) ||
                       (rect->bottom() > eBounds->bottom() &&
                        rectHeight < eHeight)) {
                remainSpaceToScrollEnd =
                    scrollBlockAlign(ScrollLogicalPosition::End);
            }
        }
    } else if (options.blockValue() < ScrollLogicalPosition::Nearest) {
        remainSpaceToScrollEnd = scrollBlockAlign(options.blockValue());
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }

    if (options.inlineValue() == ScrollLogicalPosition::Nearest) {
        Element* e = parentElement();
        if (e != nullptr) {
            LayoutUnit rectWidth = rect->right() - rect->left();
            DOMRect* eBounds = e->getBoundingClientRect();
            LayoutUnit eWidth = eBounds->right() - eBounds->left();
            // If the left and right sides of "eBound" are inside "right",
            // do nothing.
            if ((rect->left() < eBounds->left() && rectWidth < eWidth) ||
                (rect->right() > eBounds->right() && rectWidth > eWidth)) {
                remainSpaceToScrollEndHorizontal =
                    scrollInlineAlign(ScrollLogicalPosition::Start);
            } else if ((rect->left() < eBounds->left() && rectWidth > eWidth) ||
                       (rect->right() > eBounds->right() &&
                        rectWidth < eWidth)) {
                remainSpaceToScrollEndHorizontal =
                    scrollInlineAlign(ScrollLogicalPosition::End);
            }
        }
    } else if (options.inlineValue() < ScrollLogicalPosition::Nearest) {
        remainSpaceToScrollEndHorizontal =
            scrollInlineAlign(options.inlineValue());
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }

    if (remainSpaceToScrollEnd || remainSpaceToScrollEndHorizontal) {
        if (options.blockValue() == ScrollLogicalPosition::End &&
            remainSpaceToScrollEnd) {
            remainSpaceToScrollEnd -= window()->innerHeight();
        }
        if (options.inlineValue() == ScrollLogicalPosition::End &&
            remainSpaceToScrollEndHorizontal) {
            remainSpaceToScrollEndHorizontal -= window()->innerWidth();
        }
        window()->scrollTo(window()->scrollX() +
                               remainSpaceToScrollEndHorizontal,
                           window()->scrollY() + remainSpaceToScrollEnd);
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
    Scrolling* scrolling =
        element->ensureRareElementMembers()->ensureScrolling(element);
    scrolling->markAsActive();
    scrolling->giveDamageToTarget();

    // CSSOM-View: scroll events fire asynchronously, not inline with each
    // offset change. Queue the target on the WebView and let
    // WebView::rendering() fire it once per rendering pass, coalescing to
    // one pending event per target, and skip the queue entirely when nothing
    // listens (see the same pattern in Window::scrollToWithoutLayout).
    String* eventType =
        element->starfish()->staticStrings()->m_scroll.localName();
    if (!scrolling->hasPendingScrollEvent() &&
        element->hasListenerForTypeOnPath(eventType)) {
        scrolling->setPendingScrollEvent(true);
        element->window()->webView()->pendingScrollEventSet().insert(scrolling);
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

    uint32_t scrollMax = (frame()->asFrameBlockBox()->width() -
                          frame()->asFrameBlockBox()->borderWidth())
                             .toUnsigned();
    uint32_t elementScrollWidth = scrollWidth();
    if (s > elementScrollWidth - scrollMax) {
        s = elementScrollWidth - scrollMax;
    }

    if (s < 0 || std::isnan(s)) {
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

    uint32_t scrollMax = (frame()->asFrameBlockBox()->height() -
                          frame()->asFrameBlockBox()->borderHeight())
                             .toUnsigned();
    uint32_t elementscrollHeight = scrollHeight();
    if (s > elementscrollHeight - scrollMax) {
        s = elementscrollHeight - scrollMax;
    }

    if (s < 0 || std::isnan(s)) {
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
    auto ao = appliedOverflow();
    if (ao.first >= OverflowValue::HiddenOverflow) {
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

    if (ao.second >= OverflowValue::HiddenOverflow) {
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

void Element::scroll(ScrollToOptions options)
{
    scroll(options.hasLeft() ? options.left() : scrollLeftProperty(),
           options.hasTop() ? options.top() : scrollTopProperty());
}

void Element::scrollTo(ScrollToOptions options)
{
    scrollTo(options.hasLeft() ? options.left() : scrollLeftProperty(),
             options.hasTop() ? options.top() : scrollTopProperty());
}

void Element::scrollBy(double x, double y)
{
    scrollTo(scrollLeftProperty() + x, scrollTopProperty() + y);
}

void Element::scrollBy(ScrollToOptions options)
{
    scrollBy(options.hasLeft() ? options.left() : 0,
             options.hasTop() ? options.top() : 0);
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

    // getBoundingClientRect()/getClientRects() are defined relative to the
    // element's OWN document viewport. computeScreenMatrix() maps into the top
    // (screen) space, which for an element inside an iframe also includes the
    // iframe's offset within the embedding page. That is inconsistent with
    // pointer-event clientX, which IS iframe-viewport-relative, so a page that
    // computes a hit ratio as (event.clientX - rect.left) / rect.width -- e.g.
    // the YouTube embedded player's seek bar -- lands at the wrong position.
    // Subtract the document's own viewport origin so in-iframe rects are
    // iframe-viewport-relative. No-op for the top-level document (origin 0,0).
    float docOriginX = 0, docOriginY = 0;
    if (document()->browsingContext() &&
        !document()->browsingContext()->isTopLevelBrowsingContext() &&
        document()->frame() && document()->frame()->isFrameBox()) {
        SkMatrix dm = document()->frame()->asFrameBox()->computeScreenMatrix();
        SkPoint o[1] = { { 0, 0 } };
        dm.mapPoints(o, 1);
        docOriginX = o[0].x();
        docOriginY = o[0].y();
    }

    if (frameObject->isFrameBox()) {
        auto frameBox = frameObject->asFrameBox();
        auto frameRect = frameBox->frameRect();
        SkMatrix m = frameBox->computeScreenMatrix();
        m.postTranslate(-docOriginX, -docOriginY);
        LayoutRect rect;
        rect.setWidth(frameRect.width());
        rect.setHeight(frameRect.height());

        // NOTE
        // frameRect of svgElement stores actual visible rect for hittesting &
        // repainting but spec want to return don't include stroke-width here :(
        if (isSVGElement() && asSVGElement()->isShapeElement()) {
            auto path = frameBox->asFrameSVGBox()->motionTransformedPath();
            if (!path) {
                path = frameBox->asFrameSVGBox()->path();
            }
            if (path) {
                SkMatrix svgMatrix = SkMatrix::I();
                if (frameBox->asFrameSVGBox()->computedSVGTransform()) {
                    svgMatrix =
                        *frameBox->asFrameSVGBox()->computedSVGTransform();
                }

                auto viewportFrame =
                    asSVGElement()->viewportElement()->frame()->asFrameBox();
                auto viewportScreenMatrix =
                    viewportFrame->computeScreenMatrix();
                viewportScreenMatrix.postTranslate(-docOriginX, -docOriginY);
                auto fillRect = path->fillBoundingRect();
                if (fillRect.isEmpty()) {
                    // fallback
                    fillRect = path->strokeBoundingRect(
                        { 1, 0, StrokeLineCap::Butt, StrokeLineJoin::Miter,
                          GCAtomicVector<double>(), 0 });
                }
                rect = LayoutRect(fillRect.x(), fillRect.y(), fillRect.width(),
                                  fillRect.height());
                rect = computeBoxExtent(rect, svgMatrix);
                LayoutRect viewportRect =
                    computeBoxExtent(LayoutRect(0, 0, viewportFrame->width(),
                                                viewportFrame->height()),
                                     viewportScreenMatrix);
                rect.setX(rect.x() + viewportRect.x());
                rect.setY(rect.y() + viewportRect.y());
            }
        } else {
            rect = computeBoxExtent(rect, m);
        }

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
        Frame* nearestFrameBox = frameObject->nearestAncestorFrameBox();
        if (nearestFrameBox) {
            FrameBox* box = nearestFrameBox->asFrameBox();
            box->iterateChildFrameBox([&](FrameBox* childBox) {
                if (childBox->isInlineNonReplacedBox()) {
                    if (childBox->asInlineNonReplacedBox()->origin()->node() ==
                        this) {
                        SkMatrix m =
                            childBox->asFrameBox()->computeScreenMatrix();
                        m.postTranslate(-docOriginX, -docOriginY);
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

void Element::appendIntersectionObserverRegistration(
    IntersectionObserverRegistration* intersectionObserverRegistration)
{
    ensureRareElementMembers()
        ->ensureRegisteredIntersectionObservers()
        ->emplace_back(intersectionObserverRegistration);
}

void Element::removeIntersectionObserverRegistration(
    IntersectionObserver* observer)
{
    if (ensureRareElementMembers()->m_registeredIntersectionObservers) {
        GCVector<IntersectionObserverRegistration*>* registrations =
            ensureRareElementMembers()->m_registeredIntersectionObservers;
        if (registrations->size()) {
            registrations->erase(
                std::remove_if(
                    registrations->begin(), registrations->end(),
                    [observer](const IntersectionObserverRegistration* item) {
                        return item->observer == observer;
                    }),
                registrations->end());
        }
    }
}

IntersectionObserverRegistration* Element::findIntersectionObserverRegistration(
    IntersectionObserver* observer)
{
    if (ensureRareElementMembers()->m_registeredIntersectionObservers) {
        GCVector<IntersectionObserverRegistration*>* registrations =
            ensureRareElementMembers()->m_registeredIntersectionObservers;
        for (auto* registration : *registrations) {
            if (registration->observer == observer) {
                return registration;
            }
        }
    }
    return nullptr;
}

void Element::appendResizeObserverRegistration(
    ResizeObserverRegistration* resizeObserverRegistration)
{
    ensureRareElementMembers()->ensureRegisteredResizeObservers()->emplace_back(
        resizeObserverRegistration);
}

void Element::removeResizeObserverRegistration(ResizeObserver* observer)
{
    if (ensureRareElementMembers()->m_registeredResizeObservers) {
        GCVector<ResizeObserverRegistration*>* registrations =
            ensureRareElementMembers()->m_registeredResizeObservers;
        if (registrations->size()) {
            registrations->erase(
                std::remove_if(
                    registrations->begin(), registrations->end(),
                    [observer](const ResizeObserverRegistration* item) {
                        return item->observer == observer;
                    }),
                registrations->end());
        }
    }
}

ResizeObserverRegistration* Element::findResizeObserverRegistration(
    ResizeObserver* observer)
{
    if (ensureRareElementMembers()->m_registeredResizeObservers) {
        GCVector<ResizeObserverRegistration*>* registrations =
            ensureRareElementMembers()->m_registeredResizeObservers;
        for (auto* registration : *registrations) {
            if (registration->observer == observer) {
                return registration;
            }
        }
    }
    return nullptr;
}

String* Element::innerHTML()
{
    return XMLSerializer::serializeToXML(this, false);
}

Node* Element::createNodeWithHTML(String* html)
{
    return fragmentParsingAlgorithm(document(), html, this);
}

void Element::setInnerHTML(String* html)
{
    ChildListMutationObservationScope scope;
    scope.startChildListMutationScope(this);
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
        newNode = SVGDocument::createSVGElement(document(), name());
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

// Containment half of the skip decision: the element's own painting is
// hidden and every box its change could move or repaint is inside a
// non-painting subtree.
static bool hiddenElementChangeIsContained(Element* element, bool paintOnly)
{
    ComputedStyle* cs = element->style();
    if (!cs || cs->visibility() != HiddenVisibilityValue) {
        return false;
    }

    // A pure paint property confines the effect to this element's subtree.
    // A layout-affecting change ripples upward through in-flow ancestors
    // (auto heights chain up), but cannot escape an out-of-flow box: its
    // position doesn't depend on flow, and its size change moves nothing
    // outside. So the containment boundary is the element itself for
    // paint-only properties, else the nearest out-of-flow self-or-ancestor.
    // Nothing under the boundary may paint (visibility is overridable down
    // the tree). Known accepted edge: a hidden box growing can still change
    // a visible ancestor scrollbar's scroll range.
    Element* boundary = nullptr;
    if (paintOnly) {
        boundary = element;
    } else {
        Element* cur = element;
        while (cur) {
            ComputedStyle* curStyle = cur->style();
            if (!curStyle) {
                return false;
            }
            if (curStyle->position() == AbsolutePositionValue ||
                curStyle->position() == FixedPositionValue) {
                boundary = cur;
                break;
            }
            cur = cur->renderingParentElement();
        }
        if (!boundary) {
            return false;
        }
    }

    Frame* boundaryFrame = boundary->frame();
    if (boundaryFrame && boundaryFrame->subtreePaintsSomething()) {
        return false;
    }
    return true;
}

// True when an inline style change cannot alter any pixel on screen, so
// marking style dirty is enough and the renderer need not be woken (the
// recalc still runs on the next forced layout or any other-triggered pass).
// Skipping never cancels a pending pass: any non-skippable change (e.g. a
// visibility flip anywhere) schedules rendering itself, and that pass
// resolves every dirty node including skipped ones.
static bool inlineStyleChangeCannotAffectVisiblePixels(
    Element* element, CSSStyleValuePair::KeyKind keyKind)
{
    switch (keyKind) {
    // can reveal the element or change its layout participation
    case CSSStyleValuePair::KeyKind::Visibility:
    case CSSStyleValuePair::KeyKind::Display:
    case CSSStyleValuePair::KeyKind::Position:
    case CSSStyleValuePair::KeyKind::Float:
    case CSSStyleValuePair::KeyKind::Content:
    case CSSStyleValuePair::KeyKind::All:
        return false;
    default:
        break;
    }

    bool paintOnly = keyKind == CSSStyleValuePair::KeyKind::Transform ||
                     keyKind == CSSStyleValuePair::KeyKind::TransformOrigin ||
                     keyKind == CSSStyleValuePair::KeyKind::Opacity;
    return hiddenElementChangeIsContained(element, paintOnly);
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

void Element::notifyInlineStyleChanged(CSSStyleValuePair::KeyKind keyKind)
{
    bool scheduleRendering =
        !inlineStyleChangeCannotAffectVisiblePixels(this, keyKind);
    setNeedsStyleRecalc(StyleChangeReason::InlineStyleChange,
                        scheduleRendering);
    m_didInlineStyleModifiedAfterAttributeSet = true;
    if (hasAttribute(starfish()->staticStrings()->m_style) == SIZE_MAX) {
        m_attributes.push_back(Attribute(starfish()->staticStrings()->m_style,
                                         String::emptyString));
        registerInlineStyleCallback();
    }
}

InlineCSSStyleDeclaration* Element::inlineStyle()
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

    if (!hasFocusableStyle()) {
        return false;
    }

    return true;
}

bool Element::hasFocusableStyle()
{
    window()->webView()->layoutIfNeeded(false);

    ComputedStyle* computedStyle = style();
    return computedStyle &&
           computedStyle->display() != DisplayValue::NoneDisplayValue &&
           computedStyle->visibility() == VisibleVisibilityValue;
}

int Element::tabIndex()
{
    Optional<String*> result =
        getAttribute(starfish()->staticStrings()->m_tabindex);
    if (result.hasValue()) {
        return String::parseInt(result.getValue());
    }
    return -1;
}

void Element::setTabIndex(int32_t t)
{
    setAttribute(starfish()->staticStrings()->m_tabindex, String::fromInt(t));
    m_tabIndexWasSetExplicitly = true;
}

bool Element::tabIndexSetExplicitly() const
{
    return m_tabIndexWasSetExplicitly;
}

// https://html.spec.whatwg.org/multipage/interaction.html#dom-focus
void Element::focus(const FocusOptions& focusOptions)
{
    // TODO If the allow focus steps given this's node document return false,
    // then return. Run the focusing steps for this.
    window()->browsingContext()->setFocusedNode(this, false);
    // TODO If options["focusVisible"] is true, or does not exist but in an
    // implementation-defined way the user agent determines it would be best to
    // do so, then indicate focus.
    // If options["preventScroll"] is false, then scroll a target into view
    // given this, "auto", "center", and "center".
    if (!focusOptions.preventScroll() && !isVisibleToUser(this)) {
        scrollIntoView(ScrollIntoViewOptions(
            ScrollOptions::ScrollBehavior::Auto, ScrollLogicalPosition::Center,
            ScrollLogicalPosition::Center));
    }
}

void Element::blur()
{
    window()->browsingContext()->releaseFocusedNode(this);
}

void Element::makeKeyframesFromObject(ScriptObject object,
                                      GCVector<StyleRuleBase*>& keyframeRules)
{
    ContextRef* ctx = scriptBindingInstance()->scriptContext();
    CSSStyleDeclaration* declarations = new CSSStyleDeclaration(document());

    Evaluator::execute(
        ctx,
        [](ExecutionStateRef* state, ScriptObject object,
           CSSStyleDeclaration* declarations) -> ValueRef* {
            ValueVectorRef* values = object->ownPropertyKeys(state);

            for (size_t i = 0; i < values->size(); i++) {
                auto key = values->at(i);
                if (key->isString() && object->hasOwnProperty(state, key)) {
                    ScriptValue scirptValue = object->get(state, key);
                    String* name = toBrowserString(state, key->toString(state));
                    // TODO: Need to check if camel case is really needed.
                    CSSStyleValuePair::KeyKind keykind =
                        CSSStyleLookupTrie::lookupCSSStyleCamelCase(
                            name->toUTF8NonGCString().data(), name->length());
                    String* value =
                        toBrowserString(state, scirptValue->toString(state));
                    size_t len = value->length();
                    declarations->setPropertyInternal(
                        keykind, value->toUTF8NonGCString().data(), len, false);
                }
            }

            return ValueRef::createUndefined();
        },
        object, declarations);

    if (declarations->length() > 0) {
        GCAtomicVector<double> selectorList;
        keyframeRules.push_back(
            new StyleRuleKeyframe(selectorList, declarations));
    }
}

Animation* Element::animate(ExecutionContext* executionContext,
                            Optional<GCVector<ScriptValue>>& keyframes,
                            KeyframeAnimationOptions& options)
{
    // FIXME: If CSS keyframes animation is already applied to this element,
    // it is applied together with the animation by animate, and both animations
    // start together after calling this method(This method causes CSS keyframes
    // animation to restart).
    // This implementation is wrong. it should be fixed so that it is not based
    // on css animation property.

    if (needsStyleRecalc() || !style()) {
        document()->browsingContext()->resolveStyleIfNeeds();
    }

    if (!keyframes.hasValue() || !style()) {
        return new Animation(executionContext);
    }

    GCVector<ScriptValue> values = keyframes.getValue();
    if (!TimingOptions::makeTimingOptions(this, options)) {
        return new Animation(executionContext);
    }

    GCVector<StyleRuleBase*> keyframeRules;
    for (size_t i = 0; i < values.size(); i++) {
        if (values[i]->isObject()) {
            auto object = values[i]->asObject();
            makeKeyframesFromObject(object, keyframeRules);
        } else {
            // TODO
        }
    }

    if (keyframeRules.size() > 0) {
        double key = 100.0 / (keyframeRules.size() - 1);
        for (size_t i = 0; i < keyframeRules.size(); i++) {
            keyframeRules[i]->asStyleRuleKeyframe()->setSelectorListText(
                document(), String::fromInt(key * i)->concat('%'));
        }
        // make style animation data for Web Animation
        computeWebAnimationKeyframes(styleResolver(), this, style(),
                                     keyframeRules);
    }

    Animation* animation = new Animation(executionContext);
    if (style()->display() != DisplayValue::NoneDisplayValue &&
        style()->animation()) {
        AnimationApplier animationApplier(this, AnimationType::WebAnimation,
                                          style());
        if (!animationApplier.apply()) {
            return animation;
        }
        AnimationExecutor* executor = document()->animationExecutor();
        animation->setEffectTarget(this, options.id());
        executor->attachWebAnimation(options.id(), this, animation);
        executor->removeReplacedWebAnimations(options.id(), this);
        webView()->updateActiveAnimationExecutorRegistration(executor);

        setNeedsStyleRecalcForAnimation();
    }
    return animation;
}

Animation* Element::animate(ExecutionContext* executionContext,
                            Optional<GCVector<ScriptValue>>& keyframes)
{
    // TODO
    return new Animation(executionContext);
}

Optional<ShadowRoot*> Element::shadowRoot(bool returnNullWhenMeetClosed)
{
    // The shadowRoot getter steps are:
    // Let shadow be this’s shadow root.
    // If shadow is null or its mode is "closed", then return null.
    // Return shadow.
    if (!hasRareMembers()) {
        return nullptr;
    }
    RareElementMembers* rareData = rareMembers();
    STARFISH_ASSERT(rareData->isRareElementMembers());
    if (!rareData->m_shadowRoot) {
        return nullptr;
    }
    if (returnNullWhenMeetClosed && rareData->m_shadowRoot->isClosed()) {
        return nullptr;
    }
    return rareData->m_shadowRoot;
}

bool Element::isShadowRootHost()
{
    return internalShadowRoot().hasValue();
}

ShadowRoot* Element::internalEnsureShadowRoot()
{
    RareElementMembers* rareMembers = ensureRareElementMembers();
    STARFISH_ASSERT(rareMembers->isRareElementMembers());
    if (!rareMembers->m_shadowRoot) {
        rareMembers->m_shadowRoot =
            new ShadowRoot(document(), ShadowRootMode::Closed, this);
        setNeedsFrameTreeBuild();
    }
    return rareMembers->m_shadowRoot.value();
}

void Element::updateShadowRoot(Optional<ShadowRoot*> sr)
{
    if (!hasRareMembers() && !sr) {
        return;
    }
    RareElementMembers* rareData = ensureRareElementMembers();
    STARFISH_ASSERT(rareData->isRareElementMembers());
    rareData->m_shadowRoot = sr;
    sr->assignSlot();
    setNeedsFrameTreeBuild();
}

// https://dom.spec.whatwg.org/#valid-shadow-host-name
static bool isValidShadowHostName(StaticStrings& ss, AtomicString name)
{
    // "article", "aside", "blockquote", "body", "div", "footer", "h1", "h2",
    // "h3", "h4", "h5", "h6", "header", "main", "nav", "p", "section", or
    // "span"
#define VAILD_NAMES(F) \
    F(article)         \
    F(aside)           \
    F(blockquote)      \
    F(body)            \
    F(div)             \
    F(footer)          \
    F(h1)              \
    F(h2)              \
    F(h3)              \
    F(h4)              \
    F(h5)              \
    F(h6)              \
    F(header)          \
    F(main)            \
    F(nav)             \
    F(p)               \
    F(section)         \
    F(span)

    if (false) {
    }
#define COMPARE(tagName)                                        \
    else if (name == ss.m_##tagName##TagName.localNameAtomic()) \
    {                                                           \
        return true;                                            \
    }
    VAILD_NAMES(COMPARE);

#undef COMPARE
#undef VAILD_NAMES

    // a valid custom element name
    if (CustomElementRegistry::isValidCustomElementName(name.string())) {
        return true;
    }

    return false;
}

Node* Element::firstRenderingChild()
{
    auto sr = internalShadowRoot();
    if (UNLIKELY(sr)) {
        return sr->firstChild();
    }
    return firstChild();
}

// https://dom.spec.whatwg.org/#dom-element-attachshadow
ShadowRoot* Element::attachShadow(ShadowRootInit init)
{
    // Run attach a shadow root with this, init["mode"], init["clonable"],
    // init["serializable"], init["delegatesFocus"], and init["slotAssignment"].
    // https://dom.spec.whatwg.org/#concept-attach-a-shadow-root
    // If element’s namespace is not the HTML namespace, then throw a
    // "NotSupportedError" DOMException.
    if (!namespaceURI() || !namespaceURI()->equals(HTML_NAMESPACE)) {
        throw new DOMException(
            executionContext(), DOMException::NOT_SUPPORTED_ERR,
            "Invalid element to attach shadow(namespace is not HTML)");
    }
    // If element’s local name is not a valid shadow host name, then throw a
    // "NotSupportedError" DOMException.
    if (!isValidShadowHostName(*starfish()->staticStrings(),
                               name().localNameAtomic())) {
        throw new DOMException(
            executionContext(), DOMException::NOT_SUPPORTED_ERR,
            "Invalid element to attach shadow(invalid localName)");
    }
    // If element’s local name is a valid custom element name, or element’s is
    // value is non-null, then:
    // TODO "element’s is value is non-null"
    if (CustomElementRegistry::isValidCustomElementName(name().localName())) {
        // Let definition be the result of looking up a custom element
        // definition given element’s node document, its namespace, its local
        // name, and its is value.
        auto definition =
            window()->customElements()->find(name().localNameAtomic());
        // If definition is not null and definition’s disable shadow is true,
        // then throw a "NotSupportedError" DOMException.
        if (definition && definition->disableShadow) {
            throw new DOMException(
                executionContext(), DOMException::NOT_SUPPORTED_ERR,
                "Invalid element to attach shadow(disableShadow is true)");
        }
    }

    // If element is a shadow host, then:
    auto currentShadowRoot = internalShadowRoot();
    if (currentShadowRoot) {
        // Let currentShadowRoot be element’s shadow root.
        // If any of the following are true:
        // currentShadowRoot’s declarative is false; or
        // currentShadowRoot’s mode is not mode,
        if (!currentShadowRoot->declarative() ||
            init.m_mode != currentShadowRoot->modeEnum()) {
            // then throw a "NotSupportedError" DOMException.
            throw new DOMException(executionContext(),
                                   DOMException::NOT_SUPPORTED_ERR,
                                   "Faild to attach shadow");
        } else {
            // Otherwise:
            // Remove all of currentShadowRoot’s children, in tree order.
            while (currentShadowRoot->firstChild()) {
                currentShadowRoot->removeChild(currentShadowRoot->firstChild());
            }
            // Set currentShadowRoot’s declarative to false.
            currentShadowRoot->setDeclarative(false);
            // Return.
        }
        setNeedsFrameTreeBuildWithoutSelf();
        return currentShadowRoot.value();
    }

    // Let shadow be a new shadow root whose node document is element’s node
    // document, host is element, and mode is mode.
    ShadowRoot* shadow = new ShadowRoot(document(), init.m_mode, this);
    // Set shadow’s delegates focus to delegatesFocus.
    shadow->setDelegatesFocus(init.delegatesFocus());
    // If element’s custom element state is "precustomized" or "custom", then
    // set shadow’s available to element internals to true.
    if (isHTMLCustomElement()) {
        shadow->setAvailableToElementInternals(true);
    }
    // Set shadow’s slot assignment to slotAssignment.
    shadow->setSlotAssignment(init.m_slotAssignment);
    // Set shadow’s declarative to false.
    shadow->setDeclarative(false);
    // Set shadow’s clonable to clonable.
    shadow->setClonable(init.clonable());
    // Set shadow’s serializable to serializable.
    shadow->setSerializable(init.serializable());
    // Set element’s shadow root to shadow.
    updateShadowRoot(shadow);
    return shadow;
}

String* Element::slot()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_slot);
}

Optional<HTMLSlotElement*> Element::assignedSlot()
{
    // assignedSlot returns the result of "find a slot" with the open flag set:
    // a slot inside a closed shadow tree must not be exposed to script, so use
    // the public shadowRoot() getter which yields null for closed roots.
    Optional<ShadowRoot*> shadowRoot;
    if (parentElement() && (shadowRoot = parentElement()->shadowRoot())) {
        String* slotName = slot();
        return shadowRoot->assignedSlot(slotName);
    }
    return nullptr;
}

void Element::setPointerCapture(int32_t param)
{
    // pointerId is ignored: a single active pointer is assumed. Capturing a
    // disconnected element is a no-op.
    if (!isConnected()) {
        return;
    }
    BrowsingContext* bc = document()->browsingContext();
    if (bc) {
        bc->setPointerCaptureTarget(this);
    }
}

void Element::releasePointerCapture(int32_t param)
{
    BrowsingContext* bc = document()->browsingContext();
    if (bc) {
        bc->releasePointerCaptureTarget(this);
    }
}

bool Element::hasPointerCapture(int32_t param)
{
    BrowsingContext* bc = document()->browsingContext();
    return bc && bc->pointerCaptureTarget() == this;
}

Promise* Element::requestFullscreen()
{
    // Spec returns Promise<void>. Players (e.g. YouTube) chain .then()/.catch()
    // on the result, so a void return would throw and abort fullscreen.
    Promise* promise = new Promise(document()->scriptBindingInstance());
    // Per the Fullscreen spec, reject with a TypeError when the element is not
    // connected to a document; only enter fullscreen and fulfill otherwise.
    if (!isConnected()) {
        auto exception = new DOMException(
            executionContext(), DOMException::SCRIPT_TYPE_ERR, "TypeError");
        promise->reject(exception->scriptValue());
        return promise;
    }
    document()->enterFullscreen(this);
    promise->fulfill(scriptUndefined());
    return promise;
}

bool Element::isSVGDescendantElement()
{
    return isSVGElement() && asSVGElement()->ownerSVGElement().hasValue();
}

} // namespace Starfish
