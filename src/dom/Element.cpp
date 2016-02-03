#include "StarFishConfig.h"
#include "Element.h"
#include "Text.h"

#include "Document.h"

#include "style/Style.h"

#include "HTMLHtmlElement.h"
#include "HTMLHeadElement.h"
#include "HTMLBodyElement.h"
#include "HTMLDivElement.h"
#include "HTMLImageElement.h"
#include "HTMLScriptElement.h"
#include "HTMLStyleElement.h"


namespace StarFish {

size_t Element::hasAtttibute(QualifiedName name)
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
    size_t idx = hasAtttibute(name);
    if (idx == SIZE_MAX) {
        m_attributes.push_back(Attribute(name, value));
        didAttributeChanged(name, String::emptyString, value);
    } else {
        String* v = m_attributes[idx].value();
        m_attributes[idx].setValue(value);
        didAttributeChanged(name, v, value);
    }
}

void Element::removeAttribute(QualifiedName name)
{
    size_t idx = hasAtttibute(name);
    if (idx == SIZE_MAX) {
    } else {
        String* v = m_attributes[idx].value();
        m_attributes.erase(m_attributes.begin() + idx);
        didAttributeChanged(name, v, String::emptyString);
    }
}

void Element::didAttributeChanged(QualifiedName name, String* old, String* value)
{
    if (name == document()->window()->starFish()->staticStrings()->m_id) {
        m_id = value;
        setNeedsStyleRecalc();
    } else if (name == document()->window()->starFish()->staticStrings()->m_class) {
        DOMTokenList::tokenize(&m_classNames, value);
        setNeedsStyleRecalc();
    }
}

void Element::setTextContent(String* text)
{
    Text* newNode = new Text(document(), text);
    std::vector<Node*> children;
    for(Node* child=firstChild(); child; child = child->nextSibling()) {
        children.push_back(child);
    }
    std::for_each(children.begin(), children.end(), [&](Node* child) {
        removeChild(child);
        // how to remove child from document?
    });
    appendChild(newNode);
}

Node* Element::clone()
{
    Node* newNode = nullptr;
    if(isHTMLElement()) {
        if(localName()->equals(document()->window()->starFish()->staticStrings()->m_htmlLocalName)) {
            newNode = new HTMLHtmlElement(document());
        } else if(localName()->equals(document()->window()->starFish()->staticStrings()->m_headLocalName)) {
            newNode = new HTMLHeadElement(document());
        } else if(localName()->equals(document()->window()->starFish()->staticStrings()->m_bodyLocalName)) {
            newNode = new HTMLBodyElement(document());
        } else if(localName()->equals(document()->window()->starFish()->staticStrings()->m_divLocalName)) {
            newNode = new HTMLDivElement(document());
        } else if(localName()->equals(document()->window()->starFish()->staticStrings()->m_imageLocalName)) {
            newNode = new HTMLImageElement(document());
        } else if(localName()->equals(document()->window()->starFish()->staticStrings()->m_scriptLocalName)) {
            newNode = new HTMLScriptElement(document());
        } else if(localName()->equals(document()->window()->starFish()->staticStrings()->m_styleLocalName)) {
            newNode = new HTMLStyleElement(document());
        } else {
            STARFISH_RELEASE_ASSERT(false);
        }
    } else {
        STARFISH_RELEASE_ASSERT(false);
    }

    // FIXME: impl 4.8.1 NamedNodeMap to iterate attr

    STARFISH_ASSERT(newNode);
    return newNode;
}

}
