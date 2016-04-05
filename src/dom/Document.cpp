#include "StarFishConfig.h"
#include "Document.h"
#include "Traverse.h"

#include "layout/FrameDocument.h"
#include "dom/Attribute.h"

namespace StarFish {

Document::Document(Window* window, ScriptBindingInstance* scriptBindingInstance)
    : Node(this, scriptBindingInstance)
    , m_styleResolver(*this)
    , m_pageVisibilityState(PageVisibilityStateVisible)
{
    m_window = window;
    m_scriptBindingInstance = scriptBindingInstance;
    setStyle(m_styleResolver.resolveDocumentStyle(window->starFish()));

    CSSStyleSheet* userAgentStyleSheet = new CSSStyleSheet(this);

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSStyleRule::Kind::TypeSelector, String::createASCIIString("html"), CSSStyleRule::PseudoClass::None, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::BlockDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSStyleRule::Kind::TypeSelector, String::createASCIIString("head"), CSSStyleRule::PseudoClass::None, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::NoneDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSStyleRule::Kind::TypeSelector, String::createASCIIString("style"), CSSStyleRule::PseudoClass::None, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::NoneDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSStyleRule::Kind::TypeSelector, String::createASCIIString("script"), CSSStyleRule::PseudoClass::None, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::NoneDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSStyleRule::Kind::TypeSelector, String::createASCIIString("meta"), CSSStyleRule::PseudoClass::None, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::NoneDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSStyleRule::Kind::TypeSelector, String::createASCIIString("body"), CSSStyleRule::PseudoClass::None, document());

        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::BlockDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginTop);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("8px");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginRight);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("8px");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginBottom);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("8px");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginLeft);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("8px");
        rule->styleDeclaration()->addValuePair(pair);

        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSStyleRule::Kind::TypeSelector, String::createASCIIString("div"), CSSStyleRule::PseudoClass::None, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::BlockDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSStyleRule::Kind::TypeSelector, String::createASCIIString("p"), CSSStyleRule::PseudoClass::None, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::BlockDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginTop);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("1em");
        rule->styleDeclaration()->addValuePair(pair);

        pair.setKeyKind(CSSStyleValuePair::MarginBottom);
        pair.setValueKind(CSSStyleValuePair::Length);
        pair.setLengthValue("1em");
        rule->styleDeclaration()->addValuePair(pair);

        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSStyleRule::Kind::TypeSelector, String::createASCIIString("span"), CSSStyleRule::PseudoClass::None, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::InlineDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    {
        CSSStyleRule* rule = new CSSStyleRule(CSSStyleRule::Kind::TypeSelector, String::createASCIIString("img"), CSSStyleRule::PseudoClass::None, document());
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::Display);
        pair.setValueKind(CSSStyleValuePair::ValueKind::DisplayValueKind);
        CSSStyleValuePair::ValueData data = {0};
        data.m_display = DisplayValue::InlineDisplayValue;
        pair.setValue(data);
        rule->styleDeclaration()->addValuePair(pair);
        userAgentStyleSheet->addRule(rule);
    }

    m_styleResolver.addSheet(userAgentStyleSheet);

    auto df = new FrameDocument(this);
    setFrame(df);
}

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
    return new Document(window(), scriptBindingInstance());
}

Element* Document::getElementById(String* id)
{
    return (Element*)Traverse::findDescendant(this, [&](Node* child) {
        if (child->isElement() && child->asElement()->id()->equals(id)) {
            return true;
        } else
            return false;

    });
}

Element* Document::createElement(QualifiedName localName)
{
    if (!QualifiedName::checkNameProductionRule(localName.string(), localName.string()->length()))
        throw new DOMException(m_document->scriptBindingInstance(), DOMException::Code::INVALID_CHARACTER_ERR, nullptr);

    if (localName == window()->starFish()->staticStrings()->m_htmlLocalName) {
        return new HTMLHtmlElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_headLocalName) {
        return new HTMLHeadElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_styleLocalName) {
        return new HTMLStyleElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_scriptLocalName) {
        return new HTMLScriptElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_linkLocalName) {
        return new HTMLLinkElement(this);
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
#ifdef STARFISH_ENABLE_AUDIO
        return new HTMLAudioElement(this);
#endif
    }

    STARFISH_LOG_INFO("got unknown element - %s\n", localName.string()->utf8Data());
    return new HTMLUnknownElement(this, localName);
}

Text* Document::createTextNode(String* data)
{
    return new Text(this, data);
}

Comment* Document::createComment(String* data)
{
    return new Comment(this, data);
}

Attr* Document::createAttribute(QualifiedName localName)
{
    if (!QualifiedName::checkNameProductionRule(localName.string(), localName.string()->length()))
        throw new DOMException(m_document->scriptBindingInstance(), DOMException::Code::INVALID_CHARACTER_ERR, nullptr);

    return new Attr(this, scriptBindingInstance(), localName);
}

HTMLHtmlElement* Document::rootElement()
{
    // root element of html document is HTMLHtmlElement
    // https://www.w3.org/TR/html-markup/html.html
    Node* n = firstChild();
    while (n) {
        if (n->isElement() && n->asElement() && n->asElement()->isHTMLElement() && n->asElement()->asHTMLElement()->isHTMLHtmlElement()) {
            return n->asElement()->asHTMLElement()->asHTMLHtmlElement();
        }
        n = n->nextSibling();
    }

    return nullptr;
}

Element* Document::documentElement()
{
    return rootElement();
}

HTMLHeadElement* Document::headElement()
{
    Node* head = childMatchedBy(this, [](Node* nd) -> bool {
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLHeadElement()) {
            return true;
        }
        return false;
    });
    if (head) {
        return head->asElement()->asHTMLElement()->asHTMLHeadElement();
    }
    return nullptr;
}

HTMLBodyElement* Document::bodyElement()
{
    Node* body = childMatchedBy(this, [](Node* nd) -> bool {
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLBodyElement()) {
            return true;
        }
        return false;
    });
    if (body) {
        return body->asElement()->asHTMLElement()->asHTMLBodyElement();
    }
    return nullptr;
}

bool Document::hidden() const
{
    return m_pageVisibilityState == PageVisibilityState::PageVisibilityStateHidden;
}

String* Document::visibilityState()
{
    switch (m_pageVisibilityState) {
    case PageVisibilityState::PageVisibilityStateHidden:
        return String::createASCIIString("hidden");
    case PageVisibilityState::PageVisibilityStatePrerender:
        return String::createASCIIString("prerender");
    case PageVisibilityState::PageVisibilityStateUnloaded:
        return String::createASCIIString("unloaded");
    case PageVisibilityState::PageVisibilityStateVisible:
        return String::createASCIIString("visible");
    }

    STARFISH_ASSERT_NOT_REACHED();
    return String::emptyString;
}

void Document::setVisibleState(PageVisibilityState visibilityState)
{
    m_pageVisibilityState = visibilityState;
}

void Document::visibilityStateChanged()
{
    QualifiedName eventType = window()->starFish()->staticStrings()->m_visibilitychange;
    Event* e = new Event(eventType, EventInit(true, false));
    EventTarget::dispatchEvent(this->asNode(), e);
}

}
