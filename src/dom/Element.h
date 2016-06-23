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

    /* 4.4 Interface Node */
    virtual NodeType nodeType()
    {
        return ELEMENT_NODE;
    }

    virtual String* nodeName() = 0;
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
}

#endif
