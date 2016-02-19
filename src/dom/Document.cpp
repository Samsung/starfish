#include "StarFishConfig.h"
#include "Document.h"
#include "Traverse.h"

#include "layout/FrameDocument.h"
#include "dom/Attribute.h"

namespace StarFish {

String* Document::nodeName()
{
    return window()->starFish()->staticStrings()->m_documentLocalName;
}

String* Document::localName()
{
    return window()->starFish()->staticStrings()->m_documentLocalName;
}

Node* Document::clone()
{
    return new Document(window(), scriptBindingInstance(), style());
}

Element* Document::getElementById(String* id)
{
    return (Element*) Traverse::findDescendant(this, [&](Node* child) {
        if (child->isElement() && child->asElement()->id()->equals(id)) {
            return true;
        } else
            return false;

    });
}

HTMLCollection* Document::getElementsByTagName(String* qualifiedName)
{
    auto filter = [=](Node* node) {
        if ( node->isElement() && node->localName()->equals(qualifiedName))
        return true;
        return false;
    };
    return new HTMLCollection(scriptBindingInstance(), this, filter);
}

HTMLCollection* Document::getElementsByClassName(String* classNames)
{
    auto filter = [=](Node* node) {
        if (node->isElement()&&node->asElement()->classNames().size()>0) {

            const char* data = classNames->utf8Data();
            size_t length = classNames->length();
            bool isWhiteSpaceState = true;

            std::string str;
            for (size_t i = 0; i < length; i ++) {
                if (isWhiteSpaceState) {
                    if (data[i] != ' ') {
                        isWhiteSpaceState = false;
                        str += data[i];
                    }
                } else {
                    if (data[i] == ' ') {
                        isWhiteSpaceState = true;

                        String* tok = String::fromUTF8(str.data(), str.length());
                        if(!node->asElement()->hasClassName(tok))
                        return false;

                        str.clear();
                    } else {
                        str += data[i];
                    }
                }
            }

            if (str.length()) {
                String* tok = String::fromUTF8(str.data(), str.length());
                if(!node->asElement()->hasClassName(tok))
                return false;
            }

            return true;
        }
        return false;
    };
    return new HTMLCollection(scriptBindingInstance(), this, filter);
}

Element* Document::createElement(QualifiedName localName)
{
    if(!QualifiedName::checkNameProductionRule(localName.string()->utf8Data(),localName.string()->length()))
        throw new DOMException(m_document->scriptBindingInstance(), DOMException::Code::INVALID_CHARACTER_ERR, nullptr);

    if (localName == window()->starFish()->staticStrings()->m_htmlLocalName) {
        return new HTMLHtmlElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_headLocalName) {
        return new HTMLHeadElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_styleLocalName) {
        return new HTMLStyleElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_scriptLocalName) {
        return new HTMLScriptElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_metaLocalName) {
        return new HTMLMetaElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_bodyLocalName) {
        return new HTMLBodyElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_divLocalName) {
        return new HTMLDivElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_pLocalName) {
        return new HTMLParagraphElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_spanLocalName) {
        return new HTMLSpanElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_brLocalName) {
        return new HTMLBRElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_imageLocalName) {
        return new HTMLImageElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_audioLocalName) {
        return new HTMLAudioElement(this);
    }

    STARFISH_LOG_INFO("got unknown element - %s\n", localName.string()->utf8Data());
    return new HTMLUnknownElement(this, localName);
}

Text* Document::createTextNode(String* data)
{
    return new Text(this,data);
}

Comment* Document::createComment(String* data)
{
    return new Comment(this,data);
}

Attr* Document::createAttribute(QualifiedName localName)
{
    if(!QualifiedName::checkNameProductionRule(localName.string()->utf8Data(),localName.string()->length()))
        throw new DOMException(m_document->scriptBindingInstance(), DOMException::Code::INVALID_CHARACTER_ERR, nullptr);

    return new Attr(scriptBindingInstance(),localName);
}

}
