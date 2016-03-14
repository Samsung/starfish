#ifndef __StarFishElement__
#define __StarFishElement__

#include "dom/Node.h"
#include "dom/Attribute.h"

namespace StarFish {

class HTMLElement;
class CSSStyleDeclaration;

class Element : public Node {
public:

    Element(Document* document, ScriptBindingInstance* instance)
        : Node(document, instance)
        , m_id(String::emptyString)
        , m_namespace(String::emptyString)
        , m_namespacePrefix(String::emptyString)
        , m_inlineStyle(new CSSStyleDeclaration(document, this))
    {
        initScriptWrappable(this, instance);
    }

    Element(Document* document)
        : Node(document)
        , m_id(String::emptyString)
        , m_namespace(String::emptyString)
        , m_namespacePrefix(String::emptyString)
        , m_inlineStyle(new CSSStyleDeclaration(document, this))
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */
    virtual NodeType nodeType()
    {
        return ELEMENT_NODE;
    }

    virtual String* nodeName() = 0;

    virtual Element* parentElement()
    {
        Node* parent = parentNode();
        if (parent->nodeType() == ELEMENT_NODE) {
            return parent->asElement();
        } else {
            return nullptr;
        }
    }

    virtual String* nodeValue()
    {
        return nullptr;
    }

    virtual String* textContent()
    {
        if ((nodeType() == TEXT_NODE) || (nodeType() == DOCUMENT_FRAGMENT_NODE)) {
            return textContent();
        }
        String* str = String::createASCIIString("");
        for (Node* child = firstChild(); child != nullptr; child = child->nextSibling()) {
            str = str->concat(child->textContent());
        }
        return str;
    }

    virtual void setTextContent(String* text);

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

    String* id()
    {
        return m_id;
    }

    String* namespaceUri()
    {
        return m_namespace;
    }

    String* namespacePrefix()
    {
        return m_namespacePrefix;
    }

    CSSStyleDeclaration* inlineStyle()
    {
        return m_inlineStyle;
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

    size_t hasAtttibute(QualifiedName name);
    String* getAttribute(QualifiedName name)
    {
        size_t siz = hasAtttibute(name);
        if (siz == SIZE_MAX)
            return String::emptyString;
        return getAttribute(siz);
    }
    String* getAttribute(size_t pos);
    void setAttribute(QualifiedName name, String* value);
    void removeAttribute(QualifiedName name);

    virtual void didAttributeChanged(QualifiedName name, String* old, String* value);

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

    std::vector<Attribute, gc_allocator<Attribute> >* getAttributes() { return &m_attributes; }

protected:
    virtual Node* clone();

    std::vector<Attribute, gc_allocator<Attribute> > m_attributes;

    String* m_id;
    String* m_className;
    std::vector<String*, gc_allocator<String*> > m_classNames;
    String* m_namespace;
    String* m_namespacePrefix;

    CSSStyleDeclaration* m_inlineStyle;
};
}

#endif
