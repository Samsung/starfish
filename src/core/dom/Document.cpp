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
#include "core/dom/ProcessingInstruction.h"
#include "core/dom/Document.h"
#include "core/dom/DocumentFragment.h"
#include "core/dom/DOMException.h"
#include "core/dom/DOMImplementation.h"
#include "core/dom/Event.h"
#include "core/dom/UIEvent.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/FocusEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "core/dom/TouchEvent.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLDocument.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLFormElement.h"
#include "core/dom/HTMLTitleElement.h"
#ifdef STARFISH_ENABLE_MULTIMEDIA
#include "core/dom/HTMLMediaElement.h"
#endif
#include "core/dom/Text.h"
#include "core/dom/Traverse.h"
#include "core/dom/builder/html/HTMLDocumentBuilder.h"
#include "core/dom/WebOrigin.h"
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
#include "platform/network/NetworkSharedResourceManager.h"

namespace StarFish {

Document::Document(Window* window, ScriptBindingInstance* scriptBindingInstance,
                   ResourceURL* uri, String* charSet,
                   bool doesParticipateInRendering)
    : Node(this)
    , m_inParsing(false)
    , m_didLoadBrokenImage(false)
    , m_doesParticipateInRendering(doesParticipateInRendering)
    , m_designMode(false)
    , m_compatibilityMode(Document::NoQuirksMode)
    , m_pageVisibilityState(VisibilityStateVisible)
    , m_window(window)
    , m_documentURI(uri)
    , m_referrer(nullptr)
    , m_cookieURI(uri)
    , m_webOrigin(WebOrigin::createDocumentOrigin(uri))
    , m_characterSet(charSet)
    , m_contentType(String::createASCIIString("application/xml"))
    , m_resourceLoader(new ResourceLoader(this))
    , m_styleResolver(new StyleResolver(this))
    , m_documentBuilder(nullptr)
    , m_styleSheetList(nullptr)
    , m_brokenImage(nullptr)
    , m_animationExecutor(new AnimationExecutor(window))
    , m_domVersion(0)
    , m_implementation(nullptr)
#ifdef STARFISH_TIZEN
    , m_tizenWidgetTransparentBackground(0)
#endif
{
    // TODO https://html.spec.whatwg.org/multipage/origin.html#concept-origin
    // For Document objects
    // If the Document's active sandboxing flag set has its sandboxed origin
    // browsing context flag set
    // If the Document was generated from a data: URL
    // A unique opaque origin assigned when the Document is created.

    // If the Document's URL's scheme is a network scheme
    // A copy of the Document's URL's origin assigned when the Document is
    // created.

    // The document.open() method can change the Document's URL to
    // "about:blank". Therefore the origin is assigned when the Document is
    // created.

    // If the Document is the initial "about:blank" document
    // The one it was assigned when its browsing context was created.

    // If the Document is a non-initial "about:blank" document
    // The origin of the incumbent settings object when the navigate algorithm
    // was invoked, or, if no script was involved, the origin of the node
    // document of the element that initiated the navigation to that URL.

    // If the Document was created as part of the processing for javascript:
    // URLs
    // The origin of the active document of the browsing context being navigated
    // when the navigate algorithm was invoked.

    // If the Document is an iframe srcdoc document
    // The origin of the Document's browsing context's browsing context
    // container's node document.

    // If the Document was obtained in some other manner (e.g. a Document
    // created using the createDocument() API, etc)
    // The default behavior as defined in the WHATWG DOM standard applies.
    // [DOM].

    // The origin is a unique opaque origin assigned when the Document is
    // created.

    setStyle(m_styleResolver->resolveDocumentStyle(this));
    StaticStrings* sstrs = m_window->starFish()->staticStrings();

    const char* ua =
#include "core/style/UserAgentStyleSheet.css"
        ;
    CSSStyleSheet* userAgentStyleSheet =
        new CSSStyleSheet(this, String::createASCIIString(ua));
    userAgentStyleSheet->parseSheetIfneeds();
    userAgentStyleSheet->collectStyleRules(userAgentStyleSheet->childRules(),
                                           userAgentStyleSheet->url());
    userAgentStyleSheet->sortStyleRulesBySpecificity();

    m_styleResolver->addSheet(userAgentStyleSheet);

    auto df = new FrameDocument(this);
    setFrame(df);
}

BrowsingContext* Document::browsingContext()
{
    return window()->browsingContext();
}

ScriptBindingInstance* Document::scriptBindingInstance() const
{
    return window()->scriptBindingInstance();
}

Location* Document::location()
{
    return window()->location();
}

String* Document::referrer()
{
    if (!m_referrer) {
        return String::emptyString;
    }
    return m_referrer->urlString();
}

String* Document::cookie()
{
    // TODO : Throw a "SecurityError" DOMException on getting and setting.
    // * if the Document's origin is an opaque origin
    // * If the contents are sandboxed into a unique origin (e.g. in an iframe
    //   with the sandbox attribute)
    String* ret =
        NetworkSharedResourceManager::getInstance()->cookeis(m_cookieURI);
    return ret;
}

void Document::setCookie(String* cookie)
{
    NetworkSharedResourceManager::getInstance()->setCookies(this, m_cookieURI,
                                                            cookie);
}

void Document::open(ResourceURL* referrerURL)
{
    m_resourceLoader->markDocumentOpenState();

    m_documentBuilder = new HTMLDocumentBuilder(this);
    m_documentBuilder->build(documentURI(), referrerURL);
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
    EventTarget::dispatchEventByUA(e);

    m_resourceLoader->notifyEndParseDocument();
    m_documentBuilder = nullptr;

#ifdef STARFISH_ENABLE_MULTIMEDIA
    // Trigger HTMLMediaElement's preload
    // FIXME : Should consider detached HTMLMediaElements as well
    GCVector<Element*> mediaElements;
    Traverse::collectDescendants(
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
        Event* e = new Event(this, eventType);
        EventTarget::dispatchEventByUA(body, e);
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

NodeList* Document::getElementsByName(String* elementName)
{
    return ensureRareMembers()->ensureQueryInActiveNodeListVectorForName(
        this, elementName);
}

DocumentFragment* Document::createDocumentFragment()
{
    return new DocumentFragment(this);
}

Element* Document::createElement(String* localName)
{
    if (!QualifiedName::checkNameProductionRule(localName)) {
        throw new DOMException(this, DOMException::Code::INVALID_CHARACTER_ERR,
                               nullptr);
    }

    AtomicString localNameAtomic;
    if (isHTMLDocument()) {
        localNameAtomic = AtomicString::createAttrAtomicString(
            window()->starFish(), localName);
        return HTMLDocument::createHTMLElement(this, localNameAtomic);
    } else {
        localNameAtomic =
            AtomicString::createAtomicString(window()->starFish(), localName);
    }
    return new NamedElement(this,
                            QualifiedName(AtomicString(), localNameAtomic));
}

// https://dom.spec.whatwg.org/#validate-and-extract
QualifiedName Document::validateAndExtractQualifiedName(Nullable<String*> ns,
                                                        String* qualifiedName)
{
    // If namespace is the empty string, set it to null.
    if (ns.hasValue() && !ns.getValue()->length()) {
        ns = Nullable<String*>();
    }
    // Validate qualifiedName.
    if (!QualifiedName::checkNameProductionRule(qualifiedName)) {
        throw new DOMException(this, DOMException::Code::INVALID_CHARACTER_ERR);
    }
    // Let prefix be null.
    Nullable<AtomicString> prefix;
    AtomicString localName;
    // If qualifiedName contains a ":" (U+003E), then split the string on it and
    // set prefix to the part before and localName to the part after.
    // + It is not valid if qualifiedName has multiple ":" characters.
    // + It is not valid if it has 0-length prefix or localName part.
    GCVector<String*> tokens;
    qualifiedName->split(':', tokens);
    if (tokens.size() > 2) {
        throw new DOMException(this, DOMException::Code::NAMESPACE_ERR);
    } else if (tokens.size() == 2) {
        if (tokens[0]->length() == 0 || tokens[1]->length() == 0) {
            throw new DOMException(this, DOMException::Code::NAMESPACE_ERR);
        }
        prefix = AtomicString::createAtomicString(starFish(), tokens[0]);
        localName = AtomicString::createAtomicString(starFish(), tokens[1]);
    } else {
        localName = AtomicString::createAtomicString(starFish(), qualifiedName);
    }

    // If prefix is non-null and namespace is null, then throw a NamespaceError.
    if (prefix.hasValue() && !ns.hasValue()) {
        throw new DOMException(this, DOMException::NAMESPACE_ERR,
                               "Provided namespace is wrong");
    }

    AtomicString nsURI;
    if (ns.hasValue()) {
        nsURI = AtomicString::createAtomicString(starFish(), ns.getValue());
    }
    StaticStrings* strs = starFish()->staticStrings();
    // If prefix is "xml" and namespace is not the XML namespace, then throw a
    // NamespaceError.
    if (prefix.hasValue() && prefix.getValue() == strs->m_xml &&
        nsURI != strs->m_xmlNamespaceURI) {
        throw new DOMException(this, DOMException::NAMESPACE_ERR,
                               "Provided namespace is wrong");
    }

    // If either qualifiedName or prefix is "xmlns" and namespace is not the
    // XMLNS namespace, then throw a NamespaceError.
    // If namespace is the XMLNS namespace and neither qualifiedName nor prefix
    // is "xmlns", then throw a NamespaceError.
    bool qnameXmlns = !prefix.hasValue() && (localName == strs->m_xmlns);
    bool prefixXmlns =
        prefix.hasValue() && (prefix.getValue() == strs->m_xmlns);
    bool nsXmlns = (nsURI == strs->m_xmlnsNamespaceURI);
    if ((qnameXmlns || prefixXmlns) ^ nsXmlns) {
        throw new DOMException(this, DOMException::NAMESPACE_ERR,
                               "Provided namespace is wrong");
    }

    if (prefix.hasValue()) {
        return QualifiedName(prefix.getValue(), nsURI, localName);
    } else {
        return QualifiedName(nsURI, localName);
    }
}

Element* Document::createElementNS(Nullable<String*> namespaceString,
                                   String* qualifiedName)
{
    if (!namespaceString.hasValue()) {
        return createElement(qualifiedName);
    }
    QualifiedName name =
        validateAndExtractQualifiedName(namespaceString, qualifiedName);
    if (!name.prefix().hasValue() && name.namespaceURI().hasValue() &&
        name.namespaceURI().getValue().string()->equals(HTML_NAMESPACE)) {
        return HTMLDocument::createHTMLElement(this, name.localNameAtomic());
    }
    return new NamedElement(this, name);
}

Text* Document::createTextNode(String* data)
{
    return new Text(this, data);
}

CDATASection* Document::createCDATASection(String* data)
{
    if (isHTMLDocument()) {
        throw new DOMException(
            this, DOMException::Code::NOT_SUPPORTED_ERR,
            "This operation is not supported for HTML documents.");
    }
    if (data->contains("]]>")) {
        throw new DOMException(this, DOMException::Code::INVALID_CHARACTER_ERR,
                               "String cannot contain ']]>' since that is the "
                               "end delimiter of a CData section.");
    }
    return new CDATASection(this, data);
}

Comment* Document::createComment(String* data)
{
    return new Comment(this, data);
}

ProcessingInstruction* Document::createProcessingInstruction(String* target,
                                                             String* data)
{
    // If target does not match the Name production, then throw an
    // InvalidCharacterError.
    if (!QualifiedName::checkNameProductionRule(target)) {
        throw new DOMException(this, DOMException::Code::INVALID_CHARACTER_ERR);
    }
    // If data contains the string "?>", then throw an InvalidCharacterError.
    if (data->contains("?>")) {
        throw new DOMException(this, DOMException::Code::INVALID_CHARACTER_ERR);
    }
    // Return a new ProcessingInstruction node, with target set to target, data
    // set to data, and node document set to the context object.
    return new ProcessingInstruction(this, data, target);
}

Attr* Document::createAttribute(String* name)
{
    return createAttribute(createAttributeName(name));
}

Attr* Document::createAttribute(QualifiedName localName)
{
    if (!QualifiedName::checkNameProductionRule(localName.localName())) {
        throw new DOMException(this, DOMException::Code::INVALID_CHARACTER_ERR,
                               nullptr);
    }

    return new Attr(this, localName);
}

Attr* Document::createAttributeNS(Nullable<String*> ns, String* name)
{
    if (!QualifiedName::checkNameProductionRule(name)) {
        throw new DOMException(this, DOMException::Code::INVALID_CHARACTER_ERR);
    }
    return new Attr(this, validateAndExtractQualifiedName(ns, name));
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

String* Document::title()
{
    // The title element of a document is the first title element in the
    // document (in tree order), if there is one, or null otherwise.
    Node* title = childMatchedBy(
        this, [](Node* nd) -> bool { return nd->isHTMLTitleElement(); });
    if (!title) {
        return String::emptyString;
    }

    // TODO If the document element is an SVG svg element, then let value be the
    // child text content of the first SVG title element that is a child of the
    // document element.
    // Otherwise, let value be the child text content of the title element, or
    // the empty string if the title element is null.
    Nullable<String*> value = title->textContent();
    if (!value.hasValue())
        return String::emptyString;
    // Strip and collapse ASCII whitespace in value.
    String* v = value.getValue();
    return v->stripAndCollapseASCIIwhitespace();
}

void Document::setTitle(String* titleString)
{
    // TODO If the document element is an SVG svg element
    // TODO If there is an SVG title element that is a child of the document
    // element, let element be the first such element.
    // TODO Otherwise:
    // TODO Let element be the result of creating an element given the document
    // element's node document, title, and the SVG namespace.
    // TODO Insert element as the first child of the document element.
    // TODO Act as if the textContent IDL attribute of element was set to the
    // new value being assigned.
    // If the document element is in the HTML namespace
    if (documentElement() && isHTMLDocument()) {
        Node* head = childMatchedBy(
            this, [](Node* nd) -> bool { return nd->isHTMLHeadElement(); });
        Node* title = childMatchedBy(
            this, [](Node* nd) -> bool { return nd->isHTMLTitleElement(); });
        // If the title element is null and the head element is null, then abort
        // these steps.
        if (!title && !head) {
            return;
        }
        // If the title element is non-null, let element be the title element.
        Element* element;
        if (title) {
            element = title->asElement();
        } else {
            // Otherwise:
            // Let element be the result of creating an element given the
            // document element's node document, title, and the HTML namespace.
            element = new HTMLTitleElement(document());
        }
        // Append element to the head element.
        head->appendChild(title);
        // Act as if the textContent IDL attribute of element was set to the new
        // value being assigned.
        title->setTextContent(titleString);
    } else {
        // Otherwise
        // Do nothing.
    }
}

// https://html.spec.whatwg.org/multipage/dom.html#dom-document-dir
// The dir IDL attribute on Document objects must reflect the dir content
// attribute of the html element,
// if any, limited to only known values. If there is no such element, then the
// attribute must return the empty string and do nothing on setting.
String* Document::dir()
{
    Node* html = childMatchedBy(
        this, [](Node* nd) -> bool { return nd->isHTMLHtmlElement(); });
    if (!html) {
        return String::emptyString;
    }

    return html->asHTMLElement()->dir();
}

// https://html.spec.whatwg.org/multipage/dom.html#dom-document-dir
void Document::setDir(String* dir)
{
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
        Event* e = new Event(this, eventType, EventInit(true));
        EventTarget::dispatchEventByUA(this->asNode(), e);
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

ScriptWrappable* Document::defaultNamedGetter(String* name)
{
    HTMLCollection* list = namedAccess(name);

    if (list) {
        size_t len = list->length();
        if (len == 1) {
            return list->item(0);
        } else if (len > 1) {
            return list;
        }
    }

    return nullptr;
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
        res->request(Resource::ResourceRequestSyncLevel::AlwaysSync, nullptr);
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

QualifiedName Document::createAttributeNameNS(Nullable<String*> ns,
                                              String* localName)
{
    // Case sensitive
    AtomicString nsURI =
        ns.hasValue()
            ? AtomicString::createAtomicString(starFish(), ns.getValue())
            : AtomicString::emptyAtomicString();
    return QualifiedName(nsURI, AtomicString::createAtomicString(
                                    window()->starFish(), localName));
}

DOMImplementation* Document::implementation()
{
    if (m_implementation == nullptr) {
        m_implementation = new DOMImplementation(this, scriptBindingInstance());
    }
    return m_implementation;
}

Element* Document::activeElement()
{
    if (!browsingContext()->activeElement()) {
        return body() ? body()->asElement() : nullptr;
    }
    return browsingContext()->activeElement();
}

bool Document::hasFocus() const
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return true;
}

// https://html.spec.whatwg.org/multipage/interaction.html#designMode
String* Document::designMode()
{
    if (m_designMode) {
        return String::createASCIIString("on");
    }
    return String::createASCIIString("off");
}

void Document::setDesignMode(String* value)
{
    bool newValue = m_designMode;

    if (value->equalsIgnoreCase("on")) {
        newValue = true;
    } else if (value->equalsIgnoreCase("off")) {
        newValue = false;
    }

    if (newValue == m_designMode) {
        return;
    }

    m_designMode = newValue;
    if (m_designMode) {
        // TODO : immediately reset the document's active range's start and end
        // boundary points to be at the start of the Document
        browsingContext()->setFocusedNode(this);
        browsingContext()->setNeedsStyleRecalc();
    }
}

String* Document::origin()
{
    STARFISH_ASSERT(m_webOrigin != nullptr);
    return m_webOrigin->serialize();
}

// https://html.spec.whatwg.org/multipage/origin.html#dom-document-domain
String* Document::domain()
{
    STARFISH_ASSERT(m_webOrigin != nullptr);
    if (!browsingContext()) {
        return String::emptyString;
    }

    Nullable<String*> effectiveDomain = m_webOrigin->domain();
    if (effectiveDomain.hasValue()) {
        return effectiveDomain.getValue();
    }
    return String::emptyString;
}

void Document::setDomain(String* domain)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

// https://dom.spec.whatwg.org/#dom-document-createevent
Event* Document::createEvent(String* type)
{
    type = type->toLower();
    size_t len = type->length();
    Event* e = nullptr;

    switch (len) {
    case 5:
        if (type->equals("event")) {
            e = new Event(this);
        }
    case 6:
        if (type->equals("events")) {
            e = new Event(this);
        }
    case 7:
        if (type->equals("uievent")) {
            e = new UIEvent(this);
        }
    case 8:
        if (type->equals("uievents")) {
            e = new UIEvent(this);
        }
    case 9:
        if (type->equals("dragevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        } else if (type->equals("svgevents")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        } else if (type->equals("textevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        }
    case 10:
        switch (type->charAt(0)) {
        case 'c':
            if (type->equals("closeevent")) {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
                e = new Event(this);
            }
        case 'e':
            if (type->equals("errorevent")) {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
                e = new Event(this);
            }
        case 'f':
            if (type->equals("focusevent")) {
                e = new FocusEvent(this);
            }
        case 'h':
            if (type->equals("htmlevents")) {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
                e = new Event(this);
            }
        case 'm':
            if (type->equals("mouseevent")) {
                e = new MouseEvent(this);
            }
        case 't':
            if (type->equals("touchevent")) {
                e = new TouchEvent(this);
            } else if (type->equals("trackevent")) {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
                e = new Event(this);
            }
        case 'w':
            if (type->equals("wheelevent")) {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
                e = new Event(this);
            }
        default:
            break;
        }
        break;
    case 11:
        if (type->equals("customevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        } else if (type->equals("mouseevents")) {
            e = new MouseEvent(this);
        }
    case 12:
        if (type->equals("messageevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        } else if (type->equals("storageevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        }
    case 13:
        if (type->equals("keyboardevent")) {
            e = new KeyboardEvent(this);
        } else if (type->equals("popstateevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        } else if (type->equals("mutationevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        }
    case 14:
        if (type->equals("animationevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        } else if (type->equals("mutationevents")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        }
    case 15:
        if (type->equals("hashchangeevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        } else if (type->equals("transitionevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        }
    case 17:
        if (type->equals("beforeunloadevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        } else if (type->equals("devicemotionevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        } else if (type->equals("webglcontextevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        }
    case 19:
        if (type->equals("pagetransitionevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        }
    case 21:
        if (type->equals("idbversionchangeevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        }
    case 22:
        if (type->equals("deviceorientationevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        }
    default:
        break;
    }

    if (e) {
        // TODO: set timeStamp
        e->setIsTrusted(false);
        return e;
    }

    throw new DOMException(this, DOMException::Code::NOT_SUPPORTED_ERR,
                           nullptr);
}

DEFINE_EVENT_LISTENER(Document, abort);
DEFINE_EVENT_LISTENER(Document, canplay);
DEFINE_EVENT_LISTENER(Document, canplaythrough);
DEFINE_EVENT_LISTENER(Document, click);
DEFINE_EVENT_LISTENER(Document, change);
DEFINE_EVENT_LISTENER(Document, durationchange);
DEFINE_EVENT_LISTENER(Document, emptied);
DEFINE_EVENT_LISTENER(Document, ended);
DEFINE_EVENT_LISTENER(Document, error);
DEFINE_EVENT_LISTENER(Document, focus);
DEFINE_EVENT_LISTENER(Document, input);
DEFINE_EVENT_LISTENER(Document, invalid);
DEFINE_EVENT_LISTENER(Document, keydown);
DEFINE_EVENT_LISTENER(Document, keyup);
DEFINE_EVENT_LISTENER(Document, load);
DEFINE_EVENT_LISTENER(Document, loadeddata);
DEFINE_EVENT_LISTENER(Document, loadedmetadata);
DEFINE_EVENT_LISTENER(Document, loadstart);
DEFINE_EVENT_LISTENER(Document, mousedown);
DEFINE_EVENT_LISTENER(Document, mousemove);
DEFINE_EVENT_LISTENER(Document, mouseover);
DEFINE_EVENT_LISTENER(Document, mouseout);
DEFINE_EVENT_LISTENER(Document, mouseup);
DEFINE_EVENT_LISTENER(Document, pause);
DEFINE_EVENT_LISTENER(Document, play);
DEFINE_EVENT_LISTENER(Document, playing);
DEFINE_EVENT_LISTENER(Document, progress);
DEFINE_EVENT_LISTENER(Document, ratechange);
DEFINE_EVENT_LISTENER(Document, resize);
DEFINE_EVENT_LISTENER(Document, seeked);
DEFINE_EVENT_LISTENER(Document, seeking);
DEFINE_EVENT_LISTENER(Document, stalled);
DEFINE_EVENT_LISTENER(Document, submit);
DEFINE_EVENT_LISTENER(Document, suspend);
DEFINE_EVENT_LISTENER(Document, timeupdate);
DEFINE_EVENT_LISTENER(Document, volumechange);
DEFINE_EVENT_LISTENER(Document, waiting);
}
