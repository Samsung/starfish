#include "StarFishConfig.h"
#include "Document.h"
#include "Traverse.h"

#include "dom/builder/html/HTMLDocumentBuilder.h"
#include "layout/FrameDocument.h"
#include "dom/Attribute.h"
#include "platform/message_loop/MessageLoop.h"

#ifdef STARFISH_EXP
#include "dom/DOMImplementation.h"
#endif

namespace StarFish {

Document::Document(Window* window, ScriptBindingInstance* scriptBindingInstance, const URL& uri)
    : Node(this, scriptBindingInstance)
    , m_inParsing(false)
    , m_compatibilityMode(Document::NoQuirksMode)
    , m_documentURI(uri)
    , m_resourceLoader(*this)
    , m_styleResolver(*this)
    , m_documentBuilder(nullptr)
    , m_pageVisibilityState(PageVisibilityStateVisible)
    , m_domVersion(0)
{
    m_window = window;
    m_scriptBindingInstance = scriptBindingInstance;
    setStyle(m_styleResolver.resolveDocumentStyle(this));

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

#ifdef STARFISH_EXP
    m_domImplementation = new DOMImplementation(m_window, m_scriptBindingInstance);
#endif

    GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
        STARFISH_LOG_INFO("Document::~Document\n");
    }, NULL, NULL, NULL);
}

void Document::open()
{
    m_resourceLoader.markDocumentOpenState();

    m_documentBuilder = new HTMLDocumentBuilder(this);
    m_documentBuilder->build(documentURI());

}

void Document::resumeDocumentParsing()
{
    window()->starFish()->messageLoop()->addIdler([](size_t handle, void* data) {
        Document* document = (Document*)data;
        STARFISH_ASSERT(document->m_documentBuilder);
        // FIXME
        if (document->m_documentBuilder)
            document->m_documentBuilder->resume();
    }, this);
}

void Document::notifyDomContentLoaded()
{
    String* eventType = window()->starFish()->staticStrings()->m_DOMContentLoaded.localName();
    Event* e = new Event(eventType, EventInit(true, true));
    EventTarget::dispatchEvent(e);

    m_resourceLoader.notifyEndParseDocument();
    m_documentBuilder = nullptr;

    STARFISH_LOG_INFO("Document::notifyDomContentLoaded\n");
}

void Document::close()
{
    if (bodyElement()) {
        String* eventType = window()->starFish()->staticStrings()->m_unload.localName();
        Event* e = new Event(eventType, EventInit(false, false));
        EventTarget::dispatchEvent(bodyElement(), e);
    }
    resourceLoader()->cancelAllOfPendingRequests();

    while (m_activeNetworkRequests.size()) {
        (*m_activeNetworkRequests.begin())->abort();
    }
}

String* Document::nodeName()
{
    return window()->starFish()->staticStrings()->m_documentLocalName.string();
}

String* Document::localName()
{
    return window()->starFish()->staticStrings()->m_documentLocalName.string();
}

Node* Document::clone()
{
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

Element* Document::getElementById(String* id)
{
    return (Element*)Traverse::findDescendant(this, [&](Node* child) {
        if (child->isElement() && child->asElement()->isHTMLElement() && child->asElement()->asHTMLElement()->id()->equals(id)) {
            return true;
        } else
            return false;
    });
}

DocumentFragment* Document::createDocumentFragment()
{
    return new DocumentFragment(this);
}

Element* Document::createElement(QualifiedName localName)
{
    if (!QualifiedName::checkNameProductionRule(localName.localName(), localName.localName()->length()))
        throw new DOMException(document()->scriptBindingInstance(), DOMException::Code::INVALID_CHARACTER_ERR, nullptr);

#ifdef STARFISH_TC_COVERAGE
    if (localName.localName()->equals("style")) {
        STARFISH_LOG_INFO("+++tag:Element&&&style\n");
    } else {
        STARFISH_LOG_INFO("+++tag:%s\n", localName.localName()->utf8Data());
    }
#endif

    if (localName == window()->starFish()->staticStrings()->m_htmlTagName) {
        return new HTMLHtmlElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_headTagName) {
        return new HTMLHeadElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_styleTagName) {
        return new HTMLStyleElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_scriptTagName) {
        return new HTMLScriptElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_linkTagName) {
        return new HTMLLinkElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_metaTagName) {
        return new HTMLMetaElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_bodyTagName) {
        return new HTMLBodyElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_divTagName) {
        return new HTMLDivElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_pTagName) {
        return new HTMLParagraphElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_spanTagName) {
        return new HTMLSpanElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_brTagName) {
        return new HTMLBRElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_imgTagName) {
        return new HTMLImageElement(this);
    } else if (localName == window()->starFish()->staticStrings()->m_audioTagName) {
#ifdef STARFISH_ENABLE_AUDIO
        return new HTMLAudioElement(this);
#endif
    }

    STARFISH_LOG_INFO("got unknown element - %s\n", localName.localName()->utf8Data());
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
    if (!QualifiedName::checkNameProductionRule(localName.localName(), localName.localName()->length()))
        throw new DOMException(document()->scriptBindingInstance(), DOMException::Code::INVALID_CHARACTER_ERR, nullptr);

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
    String* eventType = window()->starFish()->staticStrings()->m_visibilitychange.localName();
    Event* e = new Event(eventType, EventInit(true, false));
    EventTarget::dispatchEvent(this->asNode(), e);
}

void Document::didNodeInserted(Node* parent, Node* newChild)
{
    Node::didNodeInserted(parent, newChild);
    updateDOMVersion();
    invalidNamedAccessCacheIfNeeded();
}

void Document::didNodeRemoved(Node* parent, Node* oldChild)
{
    Node::didNodeRemoved(parent, oldChild);
    updateDOMVersion();
    invalidNamedAccessCacheIfNeeded();
}

HTMLCollection* Document::namedAccess(String* name)
{
    for (size_t i = 0; i < m_namedAccessActiveHTMLCollectionList.size(); i ++) {
        if (m_namedAccessActiveHTMLCollectionList.at(i).first->equals(name)) {
            return m_namedAccessActiveHTMLCollectionList[i].second;
        }
    }

    // https://html.spec.whatwg.org/multipage/browsers.html#named-access-on-the-window-object
    auto filter = [](Node* node, void* data)
    {
        QualifiedName* qualifiedName = (QualifiedName*)data;

        if (node->isElement() && node->asElement()->isHTMLElement()) {

            // a, applet, area, embed, form, frameset, img, or object elements that have a name content attribute whose value is name, or
            QualifiedName nm = node->asElement()->asHTMLElement()->name();
            StaticStrings* ss = node->document()->window()->starFish()->staticStrings();
            bool shouldConsiderNameAttribute = false;
            if (nm == ss->m_aTagName) {
                shouldConsiderNameAttribute = true;
            } else if (nm == ss->m_areaTagName) {
                shouldConsiderNameAttribute = true;
            } else if (nm == ss->m_embedTagName) {
                shouldConsiderNameAttribute = true;
            } else if (nm == ss->m_formTagName) {
                shouldConsiderNameAttribute = true;
            } else if (nm == ss->m_framesetTagName) {
                shouldConsiderNameAttribute = true;
            } else if (nm == ss->m_imgTagName) {
                shouldConsiderNameAttribute = true;
            } else if (nm == ss->m_objectTagName) {
                shouldConsiderNameAttribute = true;
            }

            if (shouldConsiderNameAttribute) {
                if (node->asElement()->getAttribute(ss->m_name)->equals(qualifiedName->localName())) {
                    return true;
                }
            }

            // HTML elements that have an id content attribute whose value is name
            if (node->asElement()->id()->equals(qualifiedName->localName())) {
                return true;
            }
        }
        return false;
    };

    // TODO
    // now, we always create html collection for every query
    // but, if len(result) == 0:
    // we should not need to make HTMLCollection
    // just return nullptr;
    void* ptr = new QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(document()->window()->starFish(), name));
    auto list = new HTMLCollection(scriptBindingInstance(), document(), filter, ptr, true);
    m_namedAccessActiveHTMLCollectionList.push_back(std::make_pair(name, list));

    return list;
}

void Document::invalidNamedAccessCacheIfNeeded()
{
    for (size_t i = 0; i < m_namedAccessActiveHTMLCollectionList.size(); i ++) {
        m_namedAccessActiveHTMLCollectionList.at(i).second->activeNodeList().invalidateCache();
    }
}

}
