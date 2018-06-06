/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishElement__
#define __StarFishElement__

#include "core/dom/Node.h"
#include "core/dom/Attribute.h"
#include "core/dom/Scrolling.h"
#include "core/style/Style.h"
#include "core/util/AttributeName.h"

namespace StarFish {

class Attr;
class CSSStyleDeclaration;
class HTMLElement;
class NamedNodeMap;
class PseudoElement;
class DOMStringMap;

class RareElementMembers : public RareNodeMembers {
public:
    RareElementMembers()
        : RareNodeMembers()
        , m_namedNodeMap(nullptr)
        , m_attrList(nullptr)
        , m_scrolling(nullptr)
        , m_dataset(nullptr)
    {
    }

    bool isRareElementMembers() const override
    {
        return true;
    }

    NamedNodeMap* m_namedNodeMap;
    GCVector<Attr*>* m_attrList;
    LayoutUnit m_scrollTop;
    LayoutUnit m_scrollLeft;
    Scrolling* m_scrolling;
    DOMStringMap* m_dataset;
};

class Element : public Node {
public:
    Element(Document* document)
        : Node(document)
        , m_inlineStyle(nullptr)
        , m_tabIndex(0)
        , m_id(AtomicString::emptyAtomicString())
    {
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
    Nullable<String*> namespaceURI();
    virtual Nullable<String*> prefix() override;
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

protected:
    size_t hasAttribute(const AttributeName& name) const;
    size_t hasAttributeNode(const AttributeName& name);
    Nullable<String*> getAttribute(const AttributeName& name) const;
    Attr* getAttributeNode(const AttributeName& name);
    void removeAttribute(const AttributeName& name);

public:
    bool hasAttribute(String* qualifiedName);
    size_t hasAttribute(const QualifiedName& qualifiedName) const;
    bool hasAttributeNS(Nullable<String*> ns, String* localName);

    Nullable<String*> getAttribute(String* qualifiedName);
    Nullable<String*> getAttribute(const QualifiedName& qualifiedName) const;
    Nullable<String*> getAttributeNS(Nullable<String*> ns, String* localName);
    Attr* getAttributeNode(String* qualifiedName);
    Attr* getAttributeNodeNS(Nullable<String*> ns, String* localName);
    String* getAttributeOrEmpty(const QualifiedName& qualifiedName) const;

    void setAttribute(const AttributeName& name, String* value);

    void setAttribute(String* qualifiedName, String* value);
    void setAttribute(const QualifiedName& qualifiedName, String* value);
    void setAttributeNS(Nullable<String*> ns, String* qualifiedName,
                        String* value);
    Attr* setAttributeNode(Attr* attrNode);
    Attr* setAttributeNodeNS(Attr* attrNode);

    void removeAttribute(size_t idx);
    void removeAttribute(String* name);
    void removeAttribute(const QualifiedName& name);
    void removeAttributeNS(Nullable<String*> ns, String* localName);
    Attr* removeAttributeNode(Attr* attr);

    bool matches(String* selectors);

    // DO NOT MODIFY ATTRIBUTE
    const Attribute& attributeData(QualifiedName name)
    {
        return m_attributes[hasAttribute(name)];
    }

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved);

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues)
    {
    }

    virtual void didComputedStyleChanged(ComputedStyle* oldStyle,
                                         ComputedStyle* newStyle) override;

#ifdef STARFISH_ENABLE_TEST
    virtual void dump() override
    {
        Node::dump();

        auto s1 = id()->toUTF8NonGCString();
        printf("id:%s, ", s1.data());
        UTF8StringDataNonGCStd className;
        for (unsigned i = 0; i < m_classNames.size(); i++) {
            auto s2 = m_classNames[i].string()->toUTF8NonGCString();
            className += s2.data();
            className += " ";
        }

        printf("className:%s", className.data());
    }
#endif

    /* Other than DOM API */
    bool hasSameAttributes(Element* otherNode)
    {
        if (getAttributes()->size() != otherNode->getAttributes()->size()) {
            return false;
        }

        for (const Attribute& otherAttr : *(otherNode->getAttributes())) {
            Nullable<String*> attr = getAttribute(otherAttr.name());
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
    void scrollIntoViewIfNeeded();
    double scrollLeftProperty(bool layoutIfNeeds = true);
    double scrollLeft(bool layoutIfNeeds = true);
    void setScrollLeftProperty(double s, bool layoutIfNeeds = true);
    void setScrollLeft(double s, bool layoutIfNeeds = true);
    double scrollTopProperty(bool layoutIfNeeds = true);
    double scrollTop(bool layoutIfNeeds = true);
    void setScrollTopProperty(double s, bool layoutIfNeeds = true);
    void setScrollTop(double s, bool layoutIfNeeds = true);
    bool canScrollVerticaly(bool layoutIfNeeds = true);
    uint32_t scrollWidth();
    uint32_t scrollHeight();

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

    AtomicString& atomicId()
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
    const GCVector<AtomicString>& classNames()
    {
        return m_classNames;
    }

    bool hasClassName(String* className)
    {
        for (unsigned i = 0; i < m_classNames.size(); i++) {
            if (className->equals(m_classNames[i].string())) {
                return true;
            }
        }
        return false;
    }

    bool hasClassName(AtomicString className)
    {
        size_t len = m_classNames.size();
        for (unsigned i = 0; i < len; i++) {
            if (className == m_classNames[i]) {
                return true;
            }
        }
        return false;
    }

    void setStyleAttr(String* style);

    CSSStyleDeclaration* inlineStyleWithoutCreation()
    {
        return m_inlineStyle;
    }

    CSSStyleDeclaration* inlineStyle();
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

    const GCVector<Attribute>& attributesVector()
    {
        return m_attributes;
    }

    virtual void onGlobalPointingEvent(float x, float y,
                                       GlobalPointingEventKind kind) override;

protected:
    void setFocused(bool flag)
    {
        setState(Node::NodeStateFocused, flag);
    }

    // clientRect is differ with clientBoundingRect.
    // this function is only for client{Left, Top, Width, Top}
    LayoutRect clientRect();

    void getClientQuads(GCVector<DOMQuad*>& quads, bool layoutIfNeeds = true);

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

    CSSStyleDeclaration* m_inlineStyle;
    int m_tabIndex;

private:
    AtomicString m_id;
    GCVector<AtomicString> m_classNames;
    GCVector<Attribute> m_attributes;
};

// For elements other than html, xhtml, and svg elements
class NamedElement : public Element {
public:
    NamedElement(Document* document, const QualifiedName& name)
        : Element(document)
        , m_name(name)
    {
    }

    virtual QualifiedName name()
    {
        return m_name;
    }

    virtual String* localName()
    {
        return m_name.localName();
    }

    virtual String* nodeName()
    {
        return m_name.localName();
    }

protected:
    QualifiedName m_name;
};
}

#endif
