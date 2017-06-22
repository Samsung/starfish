/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Attr.h"
#include "core/dom/Attribute.h"
#include "core/dom/CDATASection.h"
#include "core/dom/Comment.h"
#include "core/dom/Document.h"
#include "core/dom/DocumentFragment.h"
#include "core/dom/DOMException.h"
#ifdef STARFISH_ENABLE_EXP
#include "core/dom/DOMImplementation.h"
#endif
#include "core/dom/Event.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLHtmlElement.h"
#ifdef STARFISH_ENABLE_MULTIMEDIA
#include "core/dom/HTMLMediaElement.h"
#endif
#include "core/dom/Text.h"
#include "core/dom/Traverse.h"
#include "core/dom/builder/html/HTMLDocumentBuilder.h"
#include "core/layout/FrameDocument.h"
#include "platform/loader/ImageResource.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSStyleSheet.h"
#include "core/style/StyleSheetList.h"
#include "core/style/StyleRule.h"
#include "platform/loader/ImageResource.h"
#include "core/animation/Animation.h"

namespace StarFish {

Document::Document(Window* window, ScriptBindingInstance* scriptBindingInstance,
                   ResourceURL* uri, String* charSet,
                   bool doesParticipateInRendering)
    : Node(this)
    , m_inParsing(false)
    , m_didLoadBrokenImage(false)
    , m_doesParticipateInRendering(doesParticipateInRendering)
    , m_compatibilityMode(Document::NoQuirksMode)
    , m_window(window)
    , m_documentURI(uri)
    , m_characterSet(charSet)
    , m_resourceLoader(this)
    , m_styleResolver(this)
    , m_documentBuilder(nullptr)
    , m_styleSheetList(nullptr)
    , m_brokenImage(nullptr)
    , m_animationExecutor(new AnimationExecutor(window))
    , m_pageVisibilityState(VisibilityStateVisible)
    , m_domVersion(0)
#ifdef STARFISH_TIZEN
    , m_tizenWidgetTransparentBackground(0)
#endif
{
    setStyle(m_styleResolver.resolveDocumentStyle(this));
    StaticStrings* sstrs = m_window->starFish()->staticStrings();

    const char* ua =
#include "core/style/UserAgentStyleSheet.css"
        ;
    CSSStyleSheet* userAgentStyleSheet =
        new CSSStyleSheet(this, String::createASCIIString(ua));
    userAgentStyleSheet->parseSheetIfneeds();
    userAgentStyleSheet->collectStyleRules(userAgentStyleSheet->allRules(),
                                           userAgentStyleSheet->url());
    userAgentStyleSheet->sortStyleRulesBySpecificity();

    m_styleResolver.addSheet(userAgentStyleSheet);

    auto df = new FrameDocument(this);
    setFrame(df);

#ifdef STARFISH_ENABLE_EXP
    m_implementation = new DOMImplementation(m_window, m_scriptBindingInstance);
#endif

    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) { STARFISH_LOG_INFO("Document::~Document\n"); },
        NULL, NULL, NULL);
}

BrowsingContext* Document::browsingContext()
{
    return window()->browsingContext();
}

ScriptBindingInstance* Document::scriptBindingInstance()
{
    return window()->scriptBindingInstance();
}

Location* Document::location()
{
    return window()->location();
}

void Document::open()
{
    m_resourceLoader.markDocumentOpenState();

    m_documentBuilder = new HTMLDocumentBuilder(this);
    m_documentBuilder->build(documentURI());
}

void Document::resumeDocumentParsing()
{
    window()->starFish()->messageLoop()->addIdler(
        browsingContext(),
        [](size_t handle, void* data) {
            Document* document = (Document*)data;
            STARFISH_ASSERT(document->m_documentBuilder);
            document->m_documentBuilder->resume();
        },
        this);
}

void Document::notifyDomContentLoaded()
{
    String* eventType =
        window()->starFish()->staticStrings()->m_DOMContentLoaded.localName();
    Event* e = new Event(this, eventType, EventInit(true, true));
    EventTarget::dispatchEvent(e);

    m_resourceLoader.notifyEndParseDocument();
    m_documentBuilder = nullptr;

#ifdef STARFISH_ENABLE_MULTIMEDIA
    // Trigger HTMLMediaElement's preload
    // FIXME : Should consider detached HTMLMediaElements as well
    GCVector<Element*> mediaElements;
    Traverse::getherDescendant(
        mediaElements, this,
        [&](Element* element) { return element->isHTMLMediaElement(); }, false);
    for (size_t i = 0; i < mediaElements.size(); i++) {
        HTMLMediaElement* target = mediaElements[i]->asHTMLMediaElement();
        target->onDOMContentLoaded();
    }
#endif

    STARFISH_LOG_INFO("Document::notifyDomContentLoaded\n");
    if (m_compatibilityMode != NoQuirksMode) {
        STARFISH_LOG_ERROR(
            "%s is not specified standard mode doctype. currently, StarFish "
            "could not support quirks mode.\n",
            m_documentURI->urlString()->utf8Data());
        STARFISH_LOG_ERROR(
            "You could got unexpected rendering result. please use standard "
            "mode doctype[<!DOCTYPE html>]\n");
    }

    // if there is a fragment identifier, set cssTarget.
    String* fragment = documentURI()->hash();
    if (!fragment->equals(String::emptyString)) {
        window()->processUrlFragment(
            fragment->substring(1, fragment->length() - 1));
    }
}

void Document::close()
{
    HTMLElement* body = this->body();
    if (body) {
        String* eventType =
            window()->starFish()->staticStrings()->m_unload.localName();
        Event* e = new Event(this, eventType, EventInit(false, false));
        EventTarget::dispatchEvent(body, e);
    }

    resourceLoader().clear();

    while (m_activeResourceRequests.size()) {
        m_activeResourceRequests.back()->abort();
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

Element* Document::getElementById(String* id)
{
    if (id->length() == 0) {
        return nullptr;
    }
    return (Element*)Traverse::findDescendant(this, [&](Node* child) {
        if (child->isHTMLElement() && child->asHTMLElement()->hasId() &&
            child->asHTMLElement()->id()->equals(id)) {
            return true;
        } else {
            return false;
        }
    });
}

DocumentFragment* Document::createDocumentFragment()
{
    return new DocumentFragment(this);
}

Element* Document::createElement(AtomicString localName, bool shouldCheckName)
{
    if (shouldCheckName &&
        !QualifiedName::checkNameProductionRule(localName.string())) {
        throw new DOMException(this, DOMException::Code::INVALID_CHARACTER_ERR,
                               nullptr);
    }

    return new NamedElement(this, QualifiedName(AtomicString(), localName));
}

Element* Document::createElement(String* name)
{
    AtomicString atomicName =
        AtomicString::createAttrAtomicString(window()->starFish(), name);
    return createElement(atomicName, true);
}

Text* Document::createTextNode(String* data)
{
    return new Text(this, data);
}

CDATASection* Document::createCDATASectionNode(String* data)
{
    return new CDATASection(this, data);
}

Comment* Document::createComment(String* data)
{
    return new Comment(this, data);
}

Attr* Document::createAttribute(String* name)
{
    QualifiedName qname = QualifiedName(
        AtomicString::emptyAtomicString(),
        AtomicString::createAttrAtomicString(window()->starFish(), name));
    return createAttribute(qname);
}

Attr* Document::createAttribute(QualifiedName localName)
{
    if (!QualifiedName::checkNameProductionRule(localName.localName())) {
        throw new DOMException(this, DOMException::Code::INVALID_CHARACTER_ERR,
                               nullptr);
    }

    return new Attr(this, localName);
}

HTMLHtmlElement* Document::rootElement()
{
    // root element of html document is HTMLHtmlElement
    // https://www.w3.org/TR/html-markup/html.html
    Node* n = firstChild();
    while (n) {
        if (n->isHTMLHtmlElement()) {
            return n->asHTMLHtmlElement();
        }
        n = n->nextSibling();
    }
    return nullptr;
}

Element* Document::documentElement()
{
    if (isXMLDocument()) {
        Node* n = firstChild();
        while (n) {
            if (n && n->isElement() && !n->isComment()) {
                return n->asElement();
            }
            n = n->nextSibling();
        }
    }
    return rootElement();
}

HTMLHeadElement* Document::head()
{
    Node* head = childMatchedBy(this, [](Node* nd) -> bool {
        if (nd->isHTMLHeadElement()) {
            return true;
        }
        return false;
    });
    if (head) {
        return head->asHTMLHeadElement();
    }
    return nullptr;
}

HTMLElement* Document::body()
{
    Node* body = childMatchedBy(
        this, [](Node* nd) -> bool { return nd->isHTMLBodyElement(); });
    if (body) {
        return body->asHTMLElement();
    }
    return nullptr;
}

void Document::setBody(HTMLElement* element)
{
    if (!(element && element->isHTMLBodyElement())) {
        COMPOSE_MESSAGE(reason, ARG_TYPE_MISMATCH_2, "1", "body",
                        "HTMLBodyElement", "HTMLFrameSetElement");
        COMPOSE_MESSAGE(msg, FAILED_TO_SET_PROPERTY, "body", "Document",
                        reason);
        throw new DOMException(this, DOMException::HIERARCHY_REQUEST_ERR, msg);
    }

    HTMLElement* body = this->body();
    HTMLHtmlElement* html = rootElement();
    HTMLBodyElement* newBody = element->asHTMLBodyElement();

    if (body) {
        html->removeChild(body);
    }
    html->appendChild(newBody);
}

bool Document::hidden() const
{
    return m_pageVisibilityState == VisibilityState::VisibilityStateHidden;
}

void Document::setVisibilityState(VisibilityState visibilityState)
{
    if (m_pageVisibilityState != visibilityState) {
        m_pageVisibilityState = visibilityState;
        String* eventType =
            starFish()->staticStrings()->m_visibilitychange.localName();
        Event* e = new Event(this, eventType, EventInit(true, false));
        EventTarget::dispatchEvent(this->asNode(), e);
    }
}

String* Document::urlString()
{
    return m_documentURI->urlString();
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
    for (size_t i = 0; i < m_namedAccessActiveHTMLCollectionList.size(); i++) {
        if (m_namedAccessActiveHTMLCollectionList[i].first->equals(name)) {
            return m_namedAccessActiveHTMLCollectionList[i].second;
        }
    }

    // TODO
    // now, we always create html collection for every query
    // but, if len(result) == 0:
    // we should not need to make HTMLCollection
    // just return nullptr;
    QualifiedName* ptr =
        new QualifiedName(AtomicString::emptyAtomicString(),
                          AtomicString::createAtomicString(starFish(), name));
    auto list = new HTMLCollection(document(), NodeListImpl::NamedAccessFilter,
                                   (void*)ptr, true);
    m_namedAccessActiveHTMLCollectionList.push_back(std::make_pair(name, list));

    return list;
}

void Document::invalidNamedAccessCacheIfNeeded()
{
    for (size_t i = 0; i < m_namedAccessActiveHTMLCollectionList.size(); i++) {
        m_namedAccessActiveHTMLCollectionList[i]
            .second->getNodeListImpl()
            .invalidateCache();
    }
}

Element* Document::elementFromPoint(float x, float y)
{
    Node* node = window()->browsingContext()->hitTest(x, y);
    while (node) {
        if (node->isElement()) {
            return node->asElement();
        }
        node = node->parentNode();
    }

    return rootElement();
}

StyleSheetList* Document::styleSheets()
{
    if (!m_styleSheetList) {
        m_styleSheetList = new StyleSheetList(this);
    }
    return m_styleSheetList;
}

ImageData* Document::brokenImage()
{
    if (m_didLoadBrokenImage) {
        return m_brokenImage;
    } else {
        String* brokenImg = String::fromUTF8(
            "data:image/"
            "png;base64,"
            "iVBORw0KGgoAAAANSUhEUgAAABQAAAAUCAYAAACNiR0NAAAABmJLR0QA/wD/"
            "AP+"
            "gvaeTAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH4AYQCBEZPGjJdQAAABl0R"
            "Vh0Q29tbWVudABDcmVhdGVkIHdpdGggR0lNUFeBDhcAAAAVSURBVDjLY2AYBaNgFIy"
            "CUTAKqAMABlQAAUOHH5wAAAAASUVORK5CYII=");
        ImageResource* res = resourceLoader().fetchImage(
            new ResourceURL(brokenImg, String::emptyString));
        res->request(Resource::ResourceRequestSyncLevel::AlwaysSync);
        m_brokenImage = res->imageData();
        m_didLoadBrokenImage = true;
        return m_brokenImage;
    }
}

QualifiedName Document::createAttributeName(String* name)
{
    if (isXMLDocument()) {
        return QualifiedName(
            AtomicString::emptyAtomicString(),
            AtomicString::createAtomicString(window()->starFish(), name));
    } else {
        return QualifiedName(
            AtomicString::emptyAtomicString(),
            AtomicString::createAttrAtomicString(window()->starFish(), name));
    }
}

Element* Document::activeElement()
{
    // TODO
    return this->body()->asElement();
}

bool Document::hasFocus() const
{
    // TODO
    return true;
}

DEFINE_EVENT_LISTENER(Document, abort);
DEFINE_EVENT_LISTENER(Document, canplay);
DEFINE_EVENT_LISTENER(Document, canplaythrough);
DEFINE_EVENT_LISTENER(Document, click);
DEFINE_EVENT_LISTENER(Document, durationchange);
DEFINE_EVENT_LISTENER(Document, emptied);
DEFINE_EVENT_LISTENER(Document, ended);
DEFINE_EVENT_LISTENER(Document, error);
DEFINE_EVENT_LISTENER(Document, focus);
DEFINE_EVENT_LISTENER(Document, keydown);
DEFINE_EVENT_LISTENER(Document, keyup);
DEFINE_EVENT_LISTENER(Document, load);
DEFINE_EVENT_LISTENER(Document, loadeddata);
DEFINE_EVENT_LISTENER(Document, loadedmetadata);
DEFINE_EVENT_LISTENER(Document, loadstart);
DEFINE_EVENT_LISTENER(Document, mouseover);
DEFINE_EVENT_LISTENER(Document, pause);
DEFINE_EVENT_LISTENER(Document, play);
DEFINE_EVENT_LISTENER(Document, playing);
DEFINE_EVENT_LISTENER(Document, progress);
DEFINE_EVENT_LISTENER(Document, ratechange);
DEFINE_EVENT_LISTENER(Document, seeked);
DEFINE_EVENT_LISTENER(Document, seeking);
DEFINE_EVENT_LISTENER(Document, stalled);
DEFINE_EVENT_LISTENER(Document, suspend);
DEFINE_EVENT_LISTENER(Document, timeupdate);
DEFINE_EVENT_LISTENER(Document, volumechange);
DEFINE_EVENT_LISTENER(Document, waiting);
}
