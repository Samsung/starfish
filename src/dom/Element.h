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

#ifndef __StarFishElement__
#define __StarFishElement__

#include "dom/Node.h"
#include "dom/Attribute.h"

namespace StarFish {

class HTMLElement;
class CSSStyleDeclaration;

typedef std::vector<Attr*, gc_allocator<Attr*>> AttrList;

class RareElementMembers : public RareNodeMembers {
public:
    RareElementMembers()
        : RareNodeMembers()
        , m_namedNodeMap(nullptr)
        , m_attrList(nullptr)
    {
    }
    bool isRareElementMembers()
    {
        return true;
    }
    NamedNodeMap* m_namedNodeMap;
    AttrList* m_attrList;
};

class Element : public Node {
public:

    Element(Document* document, ScriptBindingInstance* instance)
        : Node(document, instance)
        , m_inlineStyle(nullptr)
    {
        m_id = String::emptyString;
        m_className = String::emptyString;
    }

    Element(Document* document)
        : Node(document)
        , m_inlineStyle(nullptr)
    {
        m_id = String::emptyString;
        m_className = String::emptyString;
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::NodeObject;
    }

    /* 4.4 Interface Node */
    virtual NodeType nodeType()
    {
        return ELEMENT_NODE;
    }

    virtual String* textContent()
    {
        if ((nodeType() == TEXT_NODE) || (nodeType() == DOCUMENT_FRAGMENT_NODE)) {
            return textContent();
        }
        String* str = String::createASCIIString("");
        for (Node* child = firstChild(); child != nullptr; child = child->nextSibling()) {
            if (child->nodeType() == TEXT_NODE || child->nodeType() == ELEMENT_NODE) {
                str = str->concat(child->textContent());
            }
        }
        return str;
    }

    virtual void setTextContent(String* text);

#ifdef STARFISH_ENABLE_TEST
    String* innerHTML();
    void setInnerHTML(String*);
#endif

    virtual QualifiedName name() = 0;

    /* Other methods (not in Node interface) */

    virtual bool isElement() const
    {
        return true;
    }

    virtual bool isHTMLElement() const
    {
        return false;
    }

    HTMLElement* asHTMLElement()
    {
        STARFISH_ASSERT(isHTMLElement());
        return (HTMLElement*)this;
    }

    // DO NOT MODIFY ATTRIBUTES WITHOUT THESE FUNCTIONS
    size_t hasAttribute(QualifiedName name);
    size_t attributeCount() const
    {
        return m_attributes.size();
    }
    QualifiedName getAttributeName(size_t t)
    {
        return m_attributes[t].name();
    }
    String* getAttribute(QualifiedName name)
    {
        size_t siz = hasAttribute(name);
        if (siz == SIZE_MAX)
            return String::emptyString;
        return getAttribute(siz);
    }
    String* getAttribute(size_t pos);
    void setAttribute(QualifiedName name, String* value);
    void removeAttribute(QualifiedName name);

    // DO NOT MODIFY ATTRIBUTE
    const Attribute& attributeData(QualifiedName name)
    {
        return m_attributes[hasAttribute(name)];
    }

    virtual void didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved);
#ifdef STARFISH_ENABLE_TEST
    virtual void dump()
    {
        Node::dump();

        printf("id:%s, ", m_id->utf8Data());
        std::string className;
        for (unsigned i = 0; i < m_classNames.size(); i++) {
            className += m_classNames[i]->utf8Data();
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

        for (const Attribute& attr : *(otherNode->getAttributes())) {
            if (!getAttribute(attr.name())->equals(attr.value())) {
                return false;
            }
        }
        return true;
    }

    NamedNodeMap* attributes();

    // https://drafts.csswg.org/cssom-view/#extension-to-the-element-interface
    uint32_t clientLeft();
    uint32_t clientTop();
    uint32_t clientWidth();
    uint32_t clientHeight();

    RareNodeMembers* ensureRareMembers();
    RareElementMembers* ensureRareElementMembers();

    void addAttr(Attr* attr);
    Attr* attr(QualifiedName name);
    Attr* ensureAttr(QualifiedName name);

    String* id()
    {
        return m_id;
    }

    // DO NOT MODIFY THIS VECTOR
    const std::vector<String*, gc_allocator<String*> >& classNames()
    {
        return m_classNames;
    }

    bool hasClassName(String* className)
    {
        for (unsigned i = 0; i < m_classNames.size(); i++) {
            if (className->equals(m_classNames[i]))
                return true;
        }
        return false;
    }

    CSSStyleDeclaration* inlineStyleWithoutCreation()
    {
        return m_inlineStyle;
    }

    CSSStyleDeclaration* inlineStyle()
    {
        if (m_inlineStyle == nullptr) {
            m_inlineStyle = new CSSStyleDeclaration(document(), this);
        }
        return m_inlineStyle;
    }

    void notifyInlineStyleChanged()
    {
        setNeedsStyleRecalc();
        m_didInlineStyleModifiedAfterAttributeSet = true;
    }

protected:
    // clientRect is differ with clientBoundingRect.
    // this function is only for client{Left, Top, Width, Top}
    LayoutRect clientRect();

    // DO NOT MODIFY ATTRIBUTES.
    const AttributeVector* getAttributes() { return (AttributeVector*)&m_attributes; }
    virtual Node* clone();

    CSSStyleDeclaration* m_inlineStyle;
private:
    String* m_id;
    String* m_className;
    std::vector<String*, gc_allocator<String*>> m_classNames;
    AttributeVector m_attributes;
};

// used for not of html, xhtml, svg element
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
