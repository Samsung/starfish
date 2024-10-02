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

#ifndef __StarfishElement__
#define __StarfishElement__

#include "core/animation/Animation.h"
#include "core/dom/Node.h"
#include "core/dom/Attribute.h"
#include "core/dom/ShadowRoot.h"
#include "core/style/Style.h"
#include "core/util/AttributeName.h"
#include "core/page/ScrollOptions.h"

namespace Starfish {

class Attr;
class CSSStyleDeclaration;
class HTMLElement;
class NamedNodeMap;
class PseudoElement;
class DOMStringMap;
class Scrolling;
class ShadowRoot;
class IntersectionObserver;
struct IntersectionObserverRegistration;
struct ResizeObserverRegistration;

class PseudoElementMap : public gc {
public:
    PseudoElementMap()
    {
        memset(m_pseudoElements, 0, sizeof(m_pseudoElements));
    }

    void setPseudoElement(PseudoElementType type, PseudoElement* e)
    {
        STARFISH_ASSERT(type >= PseudoElementGeneralTypeStart &&
                        type <= PseudoElementGeneralTypeEnd);
        m_pseudoElements[type - PseudoElementGeneralTypeStart] = e;
    }

    PseudoElement* pseudoElement(PseudoElementType type)
    {
        STARFISH_ASSERT(type >= PseudoElementGeneralTypeStart &&
                        type <= PseudoElementGeneralTypeEnd);
        return m_pseudoElements[type - PseudoElementGeneralTypeStart];
    }

    void clear()
    {
        memset(m_pseudoElements, 0, sizeof(m_pseudoElements));
    }

protected:
    PseudoElement* m_pseudoElements[PseudoElementGeneralTypeEnd -
                                    PseudoElementGeneralTypeStart + 1];
};

class RareElementMembers : public RareNodeMembers {
public:
    RareElementMembers()
        : RareNodeMembers()
        , m_namedNodeMap(nullptr)
        , m_attrList(nullptr)
        , m_scrolling(nullptr)
        , m_dataset(nullptr)
        , m_pseudoElementMap(nullptr)
        , m_shadowRoot(nullptr)
        , m_registeredIntersectionObservers(nullptr)
        , m_registeredResizeObservers(nullptr)
    {
    }

    bool isRareElementMembers() const override
    {
        return true;
    }

    PseudoElementMap* ensurePseudoElementMap()
    {
        if (m_pseudoElementMap == nullptr) {
            m_pseudoElementMap = new PseudoElementMap();
        }
        return m_pseudoElementMap;
    }

    Scrolling* ensureScrolling(Element* self);

    GCVector<IntersectionObserverRegistration*>*
    ensureRegisteredIntersectionObservers();
    GCVector<ResizeObserverRegistration*>* ensureRegisteredResizeObservers();

    NamedNodeMap* m_namedNodeMap;
    GCVector<Attr*>* m_attrList;
    LayoutUnit m_scrollTop;
    LayoutUnit m_scrollLeft;
    Scrolling* m_scrolling;
    DOMStringMap* m_dataset;
    PseudoElementMap* m_pseudoElementMap;
    Optional<ShadowRoot*> m_shadowRoot;
    GCVector<IntersectionObserverRegistration*>*
        m_registeredIntersectionObservers;
    GCVector<ResizeObserverRegistration*>* m_registeredResizeObservers;
};

class Element : public Node {
public:
    Element(Document* document, const QualifiedName& qname)
        : Node(document)
        , m_inlineStyle(nullptr)
        , m_tabIndex(0)
        , m_name(qname)
#if !defined(NDEBUG)
        , m_didAttributeChangedCorrectlyInvoked(true)
#endif
        , m_id(AtomicString::emptyAtomicString())
    {
        STARFISH_ASSERT(document != nullptr);
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isElement() const override;

    /* 4.4 Interface Node */
    virtual NodeType nodeType() const override
    {
        return ELEMENT_NODE;
    }

    virtual Node* clone() override;

    String* innerHTML();
    void setInnerHTML(String*);
    String* outerHTML();
    void setOuterHTML(String*);
    // https://w3c.github.io/DOM-Parsing/#dom-element-insertadjacenthtml
    void insertAdjacentHTML(String* position, String* text);

    // https://dom.spec.whatwg.org/#dom-element-insertadjacentelement
    // spec defines return type of insertAdjacentElement as Element?
    // but our implementation returns Node?
    Node* insertAdjacentElement(String* where, Element* element); // historical
    void insertAdjacentText(String* where, String* data);         // historical

    virtual QualifiedName name() = 0;
    virtual String* nodeName() override;
    Optional<String*> namespaceURI();
    virtual Optional<String*> prefix() override;
    virtual String* localName() override;
    virtual bool handleDefaultEvent(Event* event) override;
    String* tagName();

    // DO NOT MODIFY ATTRIBUTES WITHOUT THESE FUNCTIONS
    size_t attributeCount() const
    {
        return m_attributes.size();
    }

    QualifiedName getAssuredAttributeName(size_t t)
    {
        return m_attributes[t].name();
    }

    String* getAssuredAttribute(size_t t)
    {
        return m_attributes[t].value();
    }

    bool hasAttributes() const
    {
        return m_attributes.size();
    }

    GCVector<String*> getAttributeNames() const;

    // https://dom.spec.whatwg.org/#dom-element-shadowroot
    Optional<ShadowRoot*> shadowRoot(bool returnNullWhenMeetClosed = true);
    Optional<ShadowRoot*> internalShadowRoot()
    {
        return shadowRoot(false);
    }
    ShadowRoot* internalEnsureShadowRoot();

    Node* createNodeWithHTML(String*);

protected:
    size_t hasAttribute(const AttributeName& name) const;
    size_t hasAttributeNode(const AttributeName& name);
    Optional<String*> getAttribute(const AttributeName& name) const;
    Attr* getAttributeNode(const AttributeName& name);
    void removeAttribute(const AttributeName& name);

public:
    bool hasAttribute(String* qualifiedName);
    size_t hasAttribute(const QualifiedName& qualifiedName) const;
    bool hasAttributeNS(Optional<String*> ns, String* localName);

    Optional<String*> getAttribute(String* qualifiedName);
    Optional<String*> getAttribute(const QualifiedName& qualifiedName) const;
    Optional<String*> getAttributeNS(Optional<String*> ns, String* localName);
    Attr* getAttributeNode(String* qualifiedName);
    Attr* getAttributeNodeNS(Optional<String*> ns, String* localName);
    String* getAttributeOrEmpty(const QualifiedName& qualifiedName) const;
    String* getAttributeOrVarReferencedValue(
        const QualifiedName& attributeName,
        Optional<const MutablePropertyValueList*> cssCustomValues);

    void setAttribute(const AttributeName& name, String* value);

    void setAttribute(String* qualifiedName, String* value);
    void setAttribute(const QualifiedName& qualifiedName, String* value);
    void setAttributeNS(Optional<String*> ns, String* qualifiedName,
                        String* value);
    Attr* setAttributeNode(Attr* attrNode);
    Attr* setAttributeNodeNS(Attr* attrNode);

    void removeAttribute(size_t idx);
    void removeAttribute(String* name);
    void removeAttribute(const QualifiedName& name);
    void removeAttributeNS(Optional<String*> ns, String* localName);
    Attr* removeAttributeNode(Attr* attr);

    Element* closest(String* selectors);
    bool matches(String* selectors);

    // DO NOT MODIFY ATTRIBUTE
    const Attribute& attributeData(QualifiedName name)
    {
        return m_attributes[hasAttribute(name)];
    }

    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved);

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues)
    {
    }

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues,
        Optional<const MutablePropertyValueList*> cssCustomValues)
    {
    }

    virtual void didComputedStyleChanged(
        ComputedStyle* oldStyle, ComputedStyle* newStyle,
        Optional<StyleResolveContext*> ctx) override;

#ifdef STARFISH_ENABLE_TEST
    virtual void dump() override
    {
        Node::dump();

        auto s1 = id()->toUTF8NonGCString();
        printf("Id:[%s] ", s1.data());
        UTF8StringDataNonGCStd className;
        for (unsigned i = 0; i < m_classNames.size(); i++) {
            auto s2 = m_classNames[i].string()->toUTF8NonGCString();
            className += s2.data();
            className += " ";
        }

        printf("Class:[%s]", className.data());
    }
#endif

    /* Other than DOM API */
    bool hasSameAttributes(Element* otherNode)
    {
        if (getAttributes()->size() != otherNode->getAttributes()->size()) {
            return false;
        }

        for (const Attribute& otherAttr : *(otherNode->getAttributes())) {
            AttributeName attrName(otherAttr.name(), AttributeName::MatchNS);
            Optional<String*> attr = getAttribute(attrName);
            if (!attr.hasValue() ||
                !attr.getValue()->equals(otherAttr.value())) {
                return false;
            }
        }
        return true;
    }

    virtual NamedNodeMap* attributes() override;
    DOMStringMap* dataset();

    // https://drafts.csswg.org/cssom-view/#extension-to-the-element-interface
    uint32_t clientLeft();
    uint32_t clientTop();
    uint32_t clientWidth();
    uint32_t clientHeight();

    void scrollIntoView()
    {
        scrollIntoView(true);
    }
    void scrollIntoView(bool alignToTop);
    void scrollIntoView(ScrollIntoViewOptions options);
    void scrollIntoViewIfNeeded();
    double scrollLeftProperty(bool layoutIfNeeds = true);
    double scrollLeft(bool layoutIfNeeds = true);
    void setScrollLeftProperty(double s, bool layoutIfNeeds = true);
    bool setScrollLeft(
        double s,
        bool layoutIfNeeds = true); // returns scrolling is actually happened
    double scrollTopProperty(bool layoutIfNeeds = true);
    double scrollTop(bool layoutIfNeeds = true);
    void setScrollTopProperty(double s, bool layoutIfNeeds = true);
    bool setScrollTop(
        double s,
        bool layoutIfNeeds = true); // returns scrolling is actually happened
    bool canScrollVerticaly(bool layoutIfNeeds = true);
    bool canScrollHorizontally(bool layoutIfNeeds = true);
    uint32_t scrollWidth();
    uint32_t scrollHeight();

    void scroll(double x, double y);
    void scrollTo(double x, double y);
    void scroll(ScrollToOptions options = {});
    void scrollTo(ScrollToOptions options = {});
    void scrollBy(double x, double y);
    void scrollBy(ScrollToOptions options = {});

    // https://www.w3.org/TR/cssom-view-1/#dom-element-getclientrects
    DOMRectList* getClientRects();
    DOMRect* getBoundingClientRect(bool layoutIfNeeds = true);

    virtual RareNodeMembers* ensureRareMembers() override;
    RareElementMembers* ensureRareElementMembers();
    RareElementMembers* rareMembers()
    {
        return (RareElementMembers*)m_rareNodeMembers;
    }

    Attr* attr(QualifiedName name);
    Attr* ensureAttr(QualifiedName name);

    const AtomicString& atomicId()
    {
        return m_id;
    }

    String* id()
    {
        return m_id.string();
    }

    void setId(String* id);

    String* className();
    void setClassName(String* className);

    // DO NOT MODIFY THIS VECTOR
    const GCAtomicTightVector<AtomicString>& classNames()
    {
        return m_classNames;
    }

    bool hasClassName(String* className);
    bool hasClassName(AtomicString className);

    void setStyleAttr(String* style);

    InlineCSSStyleDeclaration* inlineStyleWithoutCreation()
    {
        return m_inlineStyle;
    }

    InlineCSSStyleDeclaration* inlineStyle();
    CSSStyleDeclaration* getComputedStyle();
#ifdef STARFISH_ENABLE_TEST
    void dumpStyle();
#endif
    void notifyInlineStyleChanged();
    void registerInlineStyleCallback();

    bool focused() const
    {
        return m_state & NodeStateFocused;
    }

    virtual bool supportsFocus();
    virtual bool isFocusable();
    bool hasFocusableStyle();
    virtual bool isDisabledFormControl()
    {
        return false;
    }

    virtual int tabIndex();
    void setTabIndex(int32_t t)
    {
        m_tabIndexWasSetExplicitly = true;
        m_tabIndex = t;
    }
    bool tabIndexSetExplicitly() const;

    /* Element-level focus APIs */
    virtual void focus();
    virtual void blur();

    inline bool hasClass()
    {
        return m_classNames.size() > 0;
    }
    inline bool hasId()
    {
        return !m_id.isEmptyAtomicString();
    }

    String* getDir();
    String* getLaunguage();

    const GCTightVector<Attribute>& attributesVector()
    {
        return m_attributes;
    }

    virtual void onGlobalPointingEvent(float x, float y, DOMTimeStamp timeStamp,
                                       GlobalPointingEventKind kind) override;

    void makeKeyframesFromObject(ScriptObject object,
                                 std::vector<StyleRuleBase*>& keyframeRules);
    Animation* animate(ExecutionContext* executionContext,
                       Optional<GCVector<ScriptValue>>& keyframes,
                       KeyframeAnimationOptions& options);
    Animation* animate(ExecutionContext* executionContext,
                       Optional<GCVector<ScriptValue>>& keyframes);
    void getClientQuads(GCVector<DOMQuad*>& quads, bool layoutIfNeeds = true);

    void appendIntersectionObserverRegistration(
        IntersectionObserverRegistration* intersectionObserverRegistration);
    void removeIntersectionObserverRegistration(IntersectionObserver* observer);
    IntersectionObserverRegistration* findIntersectionObserverRegistration(
        IntersectionObserver* observer);

    void appendResizeObserverRegistration(
        ResizeObserverRegistration* resizeObserverRegistration);
    void removeResizeObserverRegistration(ResizeObserver* observer);
    ResizeObserverRegistration* findResizeObserverRegistration(
        ResizeObserver* observer);

protected:
    void setFocused(bool flag)
    {
        setState(Node::NodeStateFocused, flag);
    }

    // clientRect is differ with clientBoundingRect.
    // this function is only for client{Left, Top, Width, Top}
    virtual LayoutRect clientRect();

    // DO NOT MODIFY ATTRIBUTES.
    const GCVector<Attribute>* getAttributes()
    {
        return (GCVector<Attribute>*)&m_attributes;
    }

    static inline void fillGCDescriptor(GC_word* desc)
    {
        Node::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(Element, m_inlineStyle));
        GC_set_bit(desc, GC_WORD_OFFSET(Element, m_classNames));
        GC_set_bit(desc, GC_WORD_OFFSET(Element, m_attributes));
    }

    InlineCSSStyleDeclaration* m_inlineStyle;
    int m_tabIndex;
    QualifiedName m_name;

private:
    LayoutUnit scrollBlockAlign(ScrollLogicalPosition position);
    LayoutUnit scrollInlineAlign(ScrollLogicalPosition position);
    void invokeDidAttributeChanged(QualifiedName name, Optional<String*> old,
                                   String* value, bool attributeCreated,
                                   bool attributeRemoved);
#if !defined(NDEBUG)
    bool m_didAttributeChangedCorrectlyInvoked;
#endif
    AtomicString m_id;
    GCAtomicTightVector<AtomicString> m_classNames;
    GCTightVector<Attribute> m_attributes;
};

// For elements other than html, xhtml, and svg elements
class NamedElement : public Element {
public:
    NamedElement(Document* document, const QualifiedName& qname)
        : Element(document, qname)
    {
    }

    virtual QualifiedName name()
    {
        return m_name;
    }
};
} // namespace Starfish

#endif
