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
#ifdef STARFISH_ENABLE_AUDIO
#include "HTMLAudioElement.h"
#endif


namespace StarFish {

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
        didAttributeChanged(name, String::emptyString, value);
    } else {
        String* v = m_attributes[idx].value();
        m_attributes[idx].setValue(value);
        didAttributeChanged(name, v, value);
    }
}

void Element::removeAttribute(QualifiedName name)
{
    size_t idx = hasAttribute(name);
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
    } else if (name == document()->window()->starFish()->staticStrings()->m_style) {
        // TODO implement parse comment
        inlineStyle()->clear();
        std::vector<String*, gc_allocator<String*>> tokens = value->tokenize(";", 1);

        for (size_t i = 0; i < tokens.size(); i ++) {

            std::vector<String*, gc_allocator<String*>> rule = tokens[i]->tokenize(":", 1);

            if (rule.size() > 1) {
                const char* key = rule[0]->trim()->utf8Data();
                if (strcmp(key, "opacity") == 0) { \
                    inlineStyle()->setOpacity(rule[1]->utf8Data());
                }
#define SET_VALUE(name, nameLower, nameCSSCase) \
                else if (strcmp(key, nameCSSCase) == 0) { \
                    inlineStyle()->set##name(rule[1]->utf8Data()); \
                }
                FOR_EACH_STYLE_ATTRIBUTE_TOTAL(SET_VALUE)
#undef SET_VALUE
                else {
                    STARFISH_LOG_INFO("unknown key %s in Element::didAttributeChanged::style\n", key);
                }
            }
        }
    }
}

void Element::setTextContent(String* text)
{
    Text* node = new Text(document(), text);

    while (firstChild()) {
        removeChild(firstChild());
    }

    appendChild(node);
}

Node* Element::clone()
{
    Element* newNode = nullptr;
    if (isHTMLElement()) {
        if (localName()->equals(document()->window()->starFish()->staticStrings()->m_htmlLocalName)) {
            newNode = new HTMLHtmlElement(document());
        } else if (localName()->equals(document()->window()->starFish()->staticStrings()->m_headLocalName)) {
            newNode = new HTMLHeadElement(document());
        } else if (localName()->equals(document()->window()->starFish()->staticStrings()->m_bodyLocalName)) {
            newNode = new HTMLBodyElement(document());
        } else if (localName()->equals(document()->window()->starFish()->staticStrings()->m_divLocalName)) {
            newNode = new HTMLDivElement(document());
        } else if (localName()->equals(document()->window()->starFish()->staticStrings()->m_imageLocalName)) {
            newNode = new HTMLImageElement(document());
        } else if (localName()->equals(document()->window()->starFish()->staticStrings()->m_scriptLocalName)) {
            newNode = new HTMLScriptElement(document());
        } else if (localName()->equals(document()->window()->starFish()->staticStrings()->m_styleLocalName)) {
            newNode = new HTMLStyleElement(document());
        } else {
            STARFISH_RELEASE_ASSERT(false);
        }
    } else {
        STARFISH_RELEASE_ASSERT(false);
    }

    STARFISH_ASSERT(newNode);

    for (Attribute& attr : m_attributes) {
        newNode->setAttribute(attr.name(), attr.value());
    }
    newNode->m_id = m_id;
    newNode->m_className = m_className;
    for (String* str : m_classNames) {
        newNode->m_classNames.push_back(str);
    }
    newNode->m_namespace = m_namespace;
    newNode->m_namespacePrefix = m_namespacePrefix;
    newNode->m_inlineStyle = m_inlineStyle; // FIXME: need to dup inline style

    return newNode;
}

}
