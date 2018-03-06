/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "StarFishConfig.h"
#include "StarFish.h"

#include "core/dom/Attr.h"
#include "core/dom/Attribute.h"
#include "core/dom/CDATASection.h"
#include "core/dom/Comment.h"
#include "core/dom/CustomEvent.h"
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
#include "core/dom/HTMLBaseElement.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLDocument.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLFormElement.h"
#include "core/dom/HTMLTitleElement.h"
#include "core/dom/HTMLAnchorElement.h"
#ifdef STARFISH_ENABLE_MULTIMEDIA
#include "core/dom/HTMLMediaElement.h"
#endif
#include "core/dom/svg/SVGDocument.h"
#include "core/dom/Text.h"
#include "core/dom/Traverse.h"
#include "core/dom/builder/html/HTMLDocumentBuilder.h"
#include "core/dom/parser/HTMLParser.h"
#include "core/dom/WebOrigin.h"
#include "core/extra/Console.h"
#include "core/layout/FrameDocument.h"
#include "platform/loader/ImageResource.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSStyleSheet.h"
#include "core/style/StyleSheetList.h"
#include "core/style/StyleRule.h"
#include "platform/file/File.h"
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
    , m_readyState(DocumentReadyStateLoading)
    , m_throwOnDynamicMarkupInsertion(false)
    , m_ignoreOpensDuringUnloadCounter(false)
    , m_salvageable(true)
    , m_domContentLoadedFired(false)
    , m_onLoadFired(false)
    , m_isFocusRingCacheValid(false)
    , m_window(window)
    , m_documentURI(uri)
    , m_baseURL(fallbackBaseURL())
    , m_baseElementURL(nullptr)
    , m_baseTarget(String::emptyString)
    , m_referrer(nullptr)
    , m_webOrigin(WebOrigin::createDocumentOrigin(uri))
    , m_characterSet(charSet)
    , m_contentType(String::createASCIIString("application/xml"))
    , m_resourceLoader(new ResourceLoader(this))
    , m_fontSelector(
          FontSelector::create(this, window->starFish()->platformFontSelector(),
                               window->starFish()->platformFontCache()))
#if defined(PORT_CANVAS_BACKEND_EFL)
    , m_fontSelectorGeneric(FontSelector::createGenericFontSelector(
          this, window->starFish()->platformFontSelector(),
          window->starFish()->platformFontCache()))
#endif
    , m_styleResolver(new StyleResolver(this))
    , m_documentBuilder(nullptr)
    , m_styleSheetList(nullptr)
    , m_brokenImage(nullptr)
    , m_animationExecutor(new AnimationExecutor(window))
    , m_domVersion(0)
    , m_implementation(nullptr)
    , m_pendingDocumentParsingIdlerHandle(SIZE_MAX)
    , m_contentLanguage(String::emptyString)
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
    // we assume that there is no important rule in ua-sheet
    STARFISH_ASSERT(strstr(ua, "important") == 0);
    CSSStyleSheet* userAgentStyleSheet =
        new CSSStyleSheet(this, String::createASCIIString(ua));
    userAgentStyleSheet->parseSheetIfneeds();
    std::vector<CSSStyleDeclaration*> webFonts;
    userAgentStyleSheet->collectStyleRules(userAgentStyleSheet->childRules(),
                                           webFonts,
                                           userAgentStyleSheet->url());

    size_t rules = userAgentStyleSheet->styleRules().size();
    for (size_t j = 0; j < rules; j++) {
        userAgentStyleSheet->styleRules()[j].first->setIsUARule(true);
    }

    m_styleResolver->addSheet(userAgentStyleSheet);

    auto df = new FrameDocument(this);
    setFrame(df);
    loadBuiltinPolyfill(window->starFish()->builtinPolyfillPathString());
}

BrowsingContext* Document::browsingContext() const
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
        NetworkSharedResourceManager::getInstance()->cookeis(documentURI());
    return ret;
}

void Document::setCookie(String* cookie)
{
    NetworkSharedResourceManager::getInstance()->setCookies(this, documentURI(),
                                                            cookie);
}

void Document::init(ResourceURL* referrerURL)
{
    m_resourceLoader->markDocumentOpenState();

    m_documentBuilder = new HTMLDocumentBuilder(this);
    m_documentBuilder->build(documentURI(), referrerURL);
}

Window* Document::open(String* url, String* name, String* features)
{
    // TODO If this Document object is not an active document, then throw an
    // "InvalidStateError" DOMException exception.
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return nullptr;
}

Document* Document::open(String* type, String* replaceInput)
{
    // https://html.spec.whatwg.org/multipage/dynamic-markup-insertion.html#opening-the-input-stream

    // If document is an XML document, then throw an "InvalidStateError"
    // DOMException exception.
    if (isXMLDocument()) {
        throw new DOMException(this, DOMException::Code::INVALID_STATE_ERR);
    }
    STARFISH_ASSERT(isHTMLDocument());
    // If document's throw-on-dynamic-markup-insertion counter is greater than
    // 0, then throw an "InvalidStateError" DOMException.
    if (m_throwOnDynamicMarkupInsertion) {
        throw new DOMException(this, DOMException::Code::INVALID_STATE_ERR);
    }
    // TODO (implement WindowProxy) If document is not an active document, then
    // return document.
    // TODO (implement WindowProxy) If document's origin is not same origin to
    // the origin of the responsible document specified by the entry settings
    // object, then throw a "SecurityError" DOMException.
    // If document has an active parser whose script nesting level is greater
    // than 0, then return document.
    if (m_documentBuilder && currentScript().hasValue()) {
        return this;
    }

    // Similarly, if document's ignore-opens-during-unload counter is greater
    // than 0, then return document.
    if (m_ignoreOpensDuringUnloadCounter) {
        return this;
    }

    // Let replace be false.
    bool replace = false;
    // If replaceInput is an ASCII case-insensitive match for "replace", then
    // set replace to true.
    if (replaceInput->equalsIgnoreCase("replace")) {
        replace = true;
    } else {
        // Otherwise, if document's browsing context's session history contains
        // only one Document object,
        // and that was the about:blank Document created when document's
        // browsing context was created,
        // and that Document object has never had the unload a document
        // algorithm invoked on it
        // (e.g., by a previous call to document.open()), then set replace to
        // true.
        if (browsingContext()->historyManager()->length() == 1) {
            if (browsingContext()
                    ->historyManager()
                    ->currentEntry()
                    ->url()
                    ->isAboutURL()) {
                replace = true;
            }
        }
    }
    // Set document's salvageable state to false.
    m_salvageable = false;
    // TODO Prompt to unload document. If the user refused to allow the document
    // to be unloaded, then return document.
    // Unload document,
    // TODO with the recycle parameter set to true.
    // Abort document.
    // Unregister all event listeners registered on document and its
    // descendants.
    // Remove any tasks associated with document in any task source.
    dispose();

    // Remove all child nodes of document, without firing any mutation events.
    while (firstChild()) {
        removeChild(firstChild());
    }
    // TODO Call the JavaScript InitializeHostDefinedRealm() abstract operation
    // with the following customizations:
    // TODO For the global object, create a new Window object window.
    // TODO For the global this value, use document's browsing context's
    // associated WindowProxy.
    // TODO Let realm execution context be the created JavaScript execution
    // context.
    // TODO Set up a window environment settings object with realm execution
    // context.
    // TODO Set the active document of document's browsing context to document
    // with window.
    // TODO Replace document's singleton objects with new instances of those
    // objects, created in window's Realm. (This includes in particular the
    // History, ApplicationCache, and Navigator, objects, the various BarProp
    // objects, the two Storage objects, the various HTMLCollection objects, and
    // objects defined by other specifications, like Selection. It also includes
    // all the Web IDL prototypes in the JavaScript binding, including
    // document's prototype.)
    // Change document's character encoding to UTF-8.
    m_characterSet = String::fromUTF8("UTF-8");

    // TODO If document is ready for post-load tasks, then set document's reload
    // override flag and set document's reload override buffer to the empty
    // string.
    // Set document's salvageable state back to true.
    m_salvageable = true;

    // TODO Change document's URL to the URL of the responsible document
    // specified by the entry settings object.
    // TODO If document's iframe load in progress flag is set, then set
    // document's mute iframe load flag.

    invalidNamedAccessCacheIfNeeded();
    // Create a new HTML parser and associate it with document. This is a
    // script-created parser
    // (meaning that it can be closed by the document.open() and
    // document.close() methods,
    // and that the tokenizer will wait for an explicit call to document.close()
    // before emitting an end-of-file token). The encoding confidence is
    // irrelevant.
    m_resourceLoader->markDocumentOpenState();
    m_documentBuilder = new HTMLDocumentBuilder(this);
    m_documentBuilder->asHTMLDocumentBuilder()->openFunctionExplicitCalled();
    m_openFunctionExplicitCalled = true;

    // TODO Set the current document readiness of document to "loading".
    // If type is an ASCII case-insensitive match for the string "replace",
    // then, for historical reasons, set it to the string "text/html".
    if (type->equalsIgnoreCase("replace")) {
        type = String::createASCIIString("text/html");
    } else {
        // Otherwise:
        // If the type string contains a U+003B SEMICOLON character (;), remove
        // the first such character and all characters from it up to the end of
        // the string.
        if (type->contains(";")) {
            type = type->substring(0, type->find(';'));
        }
        // Strip leading and trailing ASCII whitespace from type.
        type = type->stripAndCollapseASCIIwhitespace();
    }

    // If type is not now an ASCII case-insensitive match for the string
    // "text/html",
    if (!type->equalsIgnoreCase("text/html")) {
        // then act as if the tokenizer had emitted a start tag token with the
        // tag name "pre" followed by a single U+000A LINE FEED (LF) character,
        // then switch the HTML parser's tokenizer to the PLAINTEXT state.
        m_documentBuilder->asHTMLDocumentBuilder()
            ->parser()
            ->input()
            ->appendToEnd(
                SegmentedString(String::createASCIIString("<pre>\n")));
        m_documentBuilder->asHTMLDocumentBuilder()->parser()->parseStep();
        m_documentBuilder->asHTMLDocumentBuilder()
            ->parser()
            ->tokenizer()
            ->setState(HTMLTokenizer::PLAINTEXTState);
    }

    // TODO Remove any tasks queued by the history traversal task source that
    // are associated with any Document objects in the top-level browsing
    // context's document family.
    // TODO Remove all the entries in the browsing context's session history
    // after the current entry. If the current entry is the last entry in the
    // session history, then no entries are removed.
    // TODO This doesn't necessarily have to affect the user agent's user
    // interface.
    // TODO Remove any earlier entries whose Document object is document.
    // TODO If replace is false, then add a new entry, just before the last
    // entry, and associate with the new entry the text that was parsed by the
    // previous parser associated with document, as well as the state of
    // document at the start of these steps. This allows the user to step
    // backwards in the session history to see the page before it was blown away
    // by the document.open() call. This new entry does not have a Document
    // object, so a new one will be created if the session history is traversed
    // to that entry.
    // TODO Set document's fired unload flag to false. (It could have been set
    // to true during the unload step above.)
    // TODO Finally, set the insertion point to point at just before the end of
    // the input stream (which at this point will be empty).
    // TODO Return document.
    return this;
}

void Document::close()
{
    // If the Document object is an XML document, then throw an
    // "InvalidStateError" DOMException and abort these steps.
    if (isXMLDocument()) {
        throw new DOMException(this, DOMException::Code::INVALID_STATE_ERR);
    }
    // If the Document object's throw-on-dynamic-markup-insertion counter is
    // greater than zero, then throw an "InvalidStateError" DOMException and
    // abort these steps.
    if (m_throwOnDynamicMarkupInsertion) {
        throw new DOMException(this, DOMException::Code::INVALID_STATE_ERR);
    }

    // If there is no script-created parser associated with the document, then
    // abort these steps.
    if (!m_openFunctionExplicitCalled || !m_documentBuilder) {
        return;
    }

    // Insert an explicit "EOF" character at the end of the parser's input
    // stream.
    m_documentBuilder->asHTMLDocumentBuilder()
        ->parser()
        ->input()
        ->markEndOfFile();
    // If there is a pending parsing-blocking script, then abort these steps.
    if (currentScript().hasValue()) {
        return;
    }
    // Run the tokenizer, processing resulting tokens as they are emitted, and
    // stopping when the tokenizer reaches the explicit "EOF" character or spins
    // the event loop.
    m_documentBuilder->asHTMLDocumentBuilder()->parser()->parseStep();
}

void Document::write(const GCVector<String*>& str)
{
    // https://html.spec.whatwg.org/multipage/dynamic-markup-insertion.html#dom-document-write
    // If document is an XML document, then throw an "InvalidStateError"
    // DOMException.
    if (isXMLDocument()) {
        throw new DOMException(this, DOMException::Code::INVALID_STATE_ERR);
    }
    // If document's throw-on-dynamic-markup-insertion counter is greater than
    // 0, then throw an "InvalidStateError" DOMException.
    if (m_throwOnDynamicMarkupInsertion) {
        throw new DOMException(this, DOMException::Code::INVALID_STATE_ERR);
    }
    // TODO(implement WindowProxy) If document is not an active document, then
    // return.

    // If the insertion point is undefined, then:
    if (!m_documentBuilder ||
        !m_documentBuilder->asHTMLDocumentBuilder()->parser()) {
        // If document's ignore-opens-during-unload counter is greater than 0 or
        // document's
        // TODO ignore-destructive-writes counter is greater than 0, then
        // return.
        if (m_ignoreOpensDuringUnloadCounter) {
            return;
        }
        // Run the document open steps with document, "text/html", and the empty
        // string.
        // TODO If the user refused to allow the document to be unloaded, then
        // abort these steps.
        // Otherwise, the insertion point will point at just before the end of
        // the (empty) input stream.
        open(String::createASCIIString("text/html"), String::emptyString);
    }

    // Insert input into the input stream just before the insertion point.
    for (size_t i = 0; i < str.size(); i++) {
        m_documentBuilder->asHTMLDocumentBuilder()
            ->parser()
            ->input()
            ->prependAtCurrentInsertionPoint(SegmentedString(str[i]));
    }

    // If document's reload override flag is set, then append input to
    // document's reload override buffer.
    // If there is no pending parsing-blocking script, have the HTML parser
    // process input, one code point at a time,
    // processing resulting tokens as they are emitted, and stopping when the
    // tokenizer reaches the insertion point or
    // when the processing of the tokenizer is aborted by the tree construction
    // stage (this can happen if a script end tag token is emitted by the
    // tokenizer).
    m_documentBuilder->asHTMLDocumentBuilder()->parser()->parseStep(false);
}

void Document::writeln(const GCVector<String*>& str)
{
    GCVector<String*> newStr = str;
    newStr.push_back(String::createASCIIString('\n'));
    write(str);
}

void Document::resumeDocumentParsing()
{
    STARFISH_ASSERT(m_pendingDocumentParsingIdlerHandle == SIZE_MAX);
    m_pendingDocumentParsingIdlerHandle =
        window()->starFish()->messageLoop()->addIdler(
            browsingContext(),
            [](size_t handle, void* data) {
                Document* document = (Document*)data;
                STARFISH_ASSERT(document->m_documentBuilder);
                document->m_pendingDocumentParsingIdlerHandle = SIZE_MAX;
                document->m_documentBuilder->resume();
            },
            this);
}

void Document::endDocumentParsing()
{
    if (m_pendingDocumentParsingIdlerHandle != SIZE_MAX) {
        window()->starFish()->messageLoop()->removeIdler(
            m_pendingDocumentParsingIdlerHandle);
        m_pendingDocumentParsingIdlerHandle = SIZE_MAX;
    }
    m_documentBuilder = nullptr;
}

void Document::notifyDomContentLoaded()
{
    if (m_deferredScriptElements.size()) {
        return;
    }

    if (!m_domContentLoadedFired) {
        m_resourceLoader->notifyEndParseDocument();
        m_domContentLoadedFired = true;
        String* eventType = window()
                                ->starFish()
                                ->staticStrings()
                                ->m_DOMContentLoaded.localName();
        Event* e = new Event(this, eventType, EventInit(true, true));
        EventTarget::dispatchEventByUA(e);

#ifdef STARFISH_ENABLE_MULTIMEDIA
        // Trigger HTMLMediaElement's preload
        // FIXME : Should consider detached HTMLMediaElements as well
        GCVector<Element*> mediaElements;
        Traverse::collectDescendants(
            mediaElements, this,
            [&](Element* element) { return element->isHTMLMediaElement(); },
            false);
        for (size_t i = 0; i < mediaElements.size(); i++) {
            HTMLMediaElement* target = mediaElements[i]->asHTMLMediaElement();
            target->onDOMContentLoaded();
        }
#endif

        STARFISH_LOG_INFO("Document::notifyDomContentLoaded\n");
        if (m_compatibilityMode != NoQuirksMode) {
            auto s = m_documentURI->urlString()->toUTF8NonGCString();
            STARFISH_LOG_ERROR(
                "WARNING: No doctype is found or quirks mode is given in %s\n"
                "WARNING: Please make sure the document starts with "
                "\"<!DOCTYPE html>\"\n"
                "WARNING: Quirks mode is not supported. Processing the document"
                " in no-quirks (i.e., standard) mode.\n",
                s.data());
        }

        // if there is a fragment identifier, set cssTarget.
        String* fragment = documentURI()->hash();
        if (!fragment->equals(String::emptyString)) {
            window()->processUrlFragment(
                fragment->substring(1, fragment->length() - 1));
        }
    }
}

void Document::dispose()
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

HTMLCollection* Document::images()
{
    return getElementsByTagName(starFish()->staticStrings()->m_imgTagName);
}

HTMLCollection* Document::forms()
{
    return getElementsByTagName(starFish()->staticStrings()->m_formTagName);
}

HTMLCollection* Document::scripts()
{
    return getElementsByTagName(starFish()->staticStrings()->m_scriptTagName);
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
    GCVector<StringView> tokens;
    StringUtils::tokenize(qualifiedName, ":", 1, tokens);
    if (tokens.size() > 2) {
        throw new DOMException(this, DOMException::Code::NAMESPACE_ERR);
    } else if (tokens.size() == 2) {
        if (tokens[0].length() == 0 || tokens[1].length() == 0) {
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
    if (!name.prefix().hasValue() && name.namespaceURI().hasValue()) {
        if (name.namespaceURI().getValue().string()->equals(HTML_NAMESPACE)) {
            return HTMLDocument::createHTMLElement(this,
                                                   name.localNameAtomic());
        } else if (name.namespaceURI().getValue().string()->equals(
                       SVG_NAMESPACE)) {
            return SVGDocument::createSVGElement(this, name.localNameAtomic());
        }
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

HTMLElement* Document::html()
{
    Node* body = childMatchedBy(
        this, [](Node* nd) -> bool { return nd->isHTMLHtmlElement(); });
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
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
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

void Document::setReadyState(DocumentReadyState newState)
{
    DocumentReadyState old = m_readyState;
    m_readyState = newState;
    if (old != newState) {
        String* eventType =
            starFish()->staticStrings()->m_readystatechange.localName();
        Event* e = new Event(this, eventType, EventInit(false, false));
        dispatchEventByUA(e);
    }
}

String* Document::urlString()
{
    return m_documentURI->urlString();
}

Document* Document::parentDocument() const
{
    BrowsingContext* parent = browsingContext()->parentBrowsingContext();
    if (!parent) {
        return nullptr;
    }
    return parent->document();
}

ResourceURL* Document::fallbackBaseURL() const
{
    // 1. If document is an iframe srcdoc document,
    // then return the document base URL of document's
    // browsing context's browsing context container's node document.
    // 2. If document's URL is about:blank, and document's browsing context
    // has a creator browsing context, then return the creator base URL.
    // 3. Return document's URL.

    // TODO : handle iframe srcdoc.

    if (documentURI()->isAboutURL()) {
        if (Document* parent = parentDocument()) {
            return parent->baseURL();
        }
    }
    return documentURI();
}

void Document::updateBaseURL()
{
    // If there are the HTML BASE elements in the tree, then the base URI is
    // computed using the value of the href attribute of the first BASE element,
    // otherwise the value of the documentURI attribute is used.
    if (m_baseElementURL) {
        m_baseURL = m_baseElementURL;
    } else {
        m_baseURL = fallbackBaseURL();
    }

    if (!m_baseURL->isValid()) {
        m_baseURL = ResourceURL::AboutBlankURL();
    }
}

ResourceURL* Document::baseURL() const
{
    // If there is no base element that has an href attribute in the Document,
    // then return the Document's fallback base URL.
    if (m_baseURL) {
        return m_baseURL;
    }
    return ResourceURL::AboutBlankURL();
}

void Document::setBaseURL(ResourceURL* newURL)
{
    m_baseURL = newURL;
}

Element* Document::nextBaseElement(Node* node, Node* root)
{
    for (Element* e = Traverse::nextElement(node, root); e;
         e = Traverse::nextElement(e, root)) {
        if (e->isHTMLBaseElement()) {
            return e;
        }
    }
    return nullptr;
}

void Document::processBaseElement()
{
    // Find the first href attribute and the first target attribute in base
    // elements
    Element* baseElement = nextBaseElement(this, this);
    String* href = nullptr;
    String* target = nullptr;
    while (baseElement && (!href || target->isEmpty())) {
        if (!href &&
            baseElement->hasAttribute(starFish()->staticStrings()->m_href) !=
                SIZE_MAX) {
            href = baseElement->getAttributeOrEmpty(
                starFish()->staticStrings()->m_href);
        }
        if (!target &&
            baseElement->hasAttribute(starFish()->staticStrings()->m_target) !=
                SIZE_MAX) {
            target = baseElement->getAttributeOrEmpty(
                starFish()->staticStrings()->m_target);
        }
        baseElement = nextBaseElement(baseElement, this);
    }

    ResourceURL* baseElementURL = nullptr;
    if (href) {
        baseElementURL = new ResourceURL(href, fallbackBaseURL()->urlString());
    }
    if (baseElementURL) {
        if (baseElementURL->isDataURL()) {
            starFish()->console()->error(String::createASCIIString(
                "'data:' URLs may not be used as base URLs for a document."));
        }
    }

    bool isSameURL = false;
    if (!baseElementURL && !m_baseElementURL) {
        isSameURL = true;
    } else if (!baseElementURL || !m_baseElementURL) {
        isSameURL = false;
    } else {
        isSameURL = (*baseElementURL == *m_baseElementURL);
    }

    if (!isSameURL) {
        m_baseElementURL = baseElementURL;
        updateBaseURL();
    }

    if (!target) {
        m_baseTarget = target;
    } else {
        m_baseTarget = String::emptyString;
    }
}

void Document::updateDOMVersion()
{
    m_domVersion++;
    invalidNamedAccessCacheIfNeeded();
    invalidFocusRingCacheIfNeeded();
}

void Document::didNodeInserted(Node* parent, Node* newChild)
{
    Node::didNodeInserted(parent, newChild);

    if (newChild->isHTMLBaseElement()) {
        processBaseElement();
    }

    updateDOMVersion();
    window()->invalidateFramesIfNeeded();
}

void Document::didNodeRemoved(Node* parent, Node* oldChild)
{
    Node::didNodeRemoved(parent, oldChild);

    if (oldChild->isHTMLBaseElement()) {
        processBaseElement();
    }

    updateDOMVersion();
    window()->invalidateFramesIfNeeded();
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

void Document::invalidFocusRingCacheIfNeeded()
{
    m_isFocusRingCacheValid = false;
}

const GCAtomicVector<Element*>& Document::focusRing()
{
    webView()->layoutIfNeeds();

    if (!m_isFocusRingCacheValid) {
        m_focusRingCache.clear();
        size_t nodeIndex = 0;

        class FocusRingItem {
        public:
            size_t m_nodeIndex;
            int m_tabIndex;
            Element* m_element;
            FocusRingItem(size_t nodeIndex = 0, int tabIndex = 0,
                          Element* element = nullptr)
                : m_nodeIndex(nodeIndex)
                , m_tabIndex(tabIndex)
                , m_element(element)
            {
            }

            bool operator<(const FocusRingItem& o) const
            {
                size_t a = m_tabIndex;
                size_t b = o.m_tabIndex;

                if (a == 0) {
                    a = std::numeric_limits<size_t>::max();
                }

                if (b == 0) {
                    b = std::numeric_limits<size_t>::max();
                }

                if (a < b) {
                    return true;
                } else if (a > b) {
                    return false;
                } else {
                    if (m_nodeIndex < o.m_nodeIndex) {
                        return true;
                    } else {
                        STARFISH_ASSERT(m_nodeIndex > o.m_nodeIndex);
                        return false;
                    }
                }
            }
        };

        std::vector<FocusRingItem> coll; // nodeindex, tabindex, element*

        // there is no meaning `passing m_focusRingCache`
        // just for compile!
        Traverse::collectDescendants(
            m_focusRingCache, this,
            [&](Element* e) -> bool {
                if (e->isHTMLElement() && e->tabIndex() >= 0 &&
                    !e->asHTMLElement()->disabled() && e->frame()) {
                    if (e->style()->visibility() !=
                        VisibilityValue::VisibleVisibilityValue) {
                        return false;
                    }
                    if (e->isHTMLAnchorElement()) {
                        if (!e->asHTMLAnchorElement()->href()->length()) {
                            return false;
                        }
                    }
                    coll.push_back(
                        FocusRingItem(nodeIndex++, e->tabIndex(), e));
                    return false;
                }
                return false;
            },
            false);

        std::sort(coll.begin(), coll.end());

        m_focusRingCache.reserve(coll.size() + 1);
        m_focusRingCache.push_back(nullptr); // for focusing body
        for (size_t i = 0; i < coll.size(); i++) {
            m_focusRingCache.push_back(coll[i].m_element);
        }
    }

    return m_focusRingCache;
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

NativeImageData* Document::brokenImage()
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
    }
}

void Document::setContentLanguage(String* value)
{
    if (m_contentLanguage == value) {
        return;
    }
    m_contentLanguage = value;
    browsingContext()->setNeedsStyleRecalc();
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

Nullable<HTMLOrSVGScriptElement> Document::currentScript()
{
    if (m_currentScripts.size() == 0) {
        return Nullable<HTMLOrSVGScriptElement>();
    }
    STARFISH_ASSERT(m_currentScripts.back());
    STARFISH_ASSERT(m_currentScripts.back()->isHTMLScriptElement() ||
                    m_currentScripts.back()->isSVGScriptElement());
    if (m_currentScripts.back()->isHTMLScriptElement()) {
        return HTMLOrSVGScriptElement::createHTMLScriptElement(
            m_currentScripts.back()->asHTMLScriptElement());
    }
    return HTMLOrSVGScriptElement::createSVGScriptElement(
        m_currentScripts.back()->asSVGScriptElement());
}

void Document::loadBuiltinPolyfill(String* localPath)
{
    if (!localPath->length()) {
        return;
    }
    STARFISH_LOG_INFO("Load built-in javascript polyfill\n");
    File* in = File::create();
    if (!in->open(localPath, File::FileMode::Read)) {
        in->close();
        STARFISH_LOG_INFO("Invalid built-in polyfill path.\n");
        return;
    }
    Nullable<String*> data = in->readAll();
    in->close();
    if (!data.hasValue()) {
        STARFISH_LOG_INFO("Invalid built-in polyfill content.\n");
        return;
    }
    evaluateString(window()->scriptBindingInstance(), data.getValue(),
                   String::createASCIIString("builtinPolyfill"));
    STARFISH_LOG_INFO("Built-in polyfill evaluated.\n");
}

// https://dom.spec.whatwg.org/#dom-document-createevent
Event* Document::createEvent(String* type)
{
    type = type->toASCIILower();
    size_t len = type->length();
    Event* e = nullptr;

    switch (len) {
    case 5:
        if (type->equals("event")) {
            e = new Event(this);
        }
        break;
    case 6:
        if (type->equals("events")) {
            e = new Event(this);
        }
        break;
    case 7:
        if (type->equals("uievent")) {
            e = new UIEvent(this);
        }
        break;
    case 8:
        if (type->equals("uievents")) {
            e = new UIEvent(this);
        }
        break;
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
        break;
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
            e = new CustomEvent(this);
        } else if (type->equals("mouseevents")) {
            e = new MouseEvent(this);
        }
        break;
    case 12:
        if (type->equals("messageevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        } else if (type->equals("storageevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        }
        break;
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
        break;
    case 14:
        if (type->equals("animationevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        } else if (type->equals("mutationevents")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        }
        break;
    case 15:
        if (type->equals("hashchangeevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        } else if (type->equals("transitionevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        }
        break;
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
        break;
    case 19:
        if (type->equals("pagetransitionevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        }
        break;
    case 21:
        if (type->equals("idbversionchangeevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        }
        break;
    case 22:
        if (type->equals("deviceorientationevent")) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            e = new Event(this);
        }
        break;
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
DEFINE_EVENT_LISTENER(Document, blur);
DEFINE_EVENT_LISTENER(Document, click);
DEFINE_EVENT_LISTENER(Document, change);
DEFINE_EVENT_LISTENER(Document, error);
DEFINE_EVENT_LISTENER(Document, focus);
DEFINE_EVENT_LISTENER(Document, input);
DEFINE_EVENT_LISTENER(Document, invalid);
DEFINE_EVENT_LISTENER(Document, keydown);
DEFINE_EVENT_LISTENER(Document, keypress);
DEFINE_EVENT_LISTENER(Document, keyup);
DEFINE_EVENT_LISTENER(Document, load);
DEFINE_EVENT_LISTENER(Document, loadstart);
DEFINE_EVENT_LISTENER(Document, mousedown);
DEFINE_EVENT_LISTENER(Document, mousemove);
DEFINE_EVENT_LISTENER(Document, mouseover);
DEFINE_EVENT_LISTENER(Document, mouseout);
DEFINE_EVENT_LISTENER(Document, mouseup);
DEFINE_EVENT_LISTENER(Document, progress);
DEFINE_EVENT_LISTENER(Document, resize);
DEFINE_EVENT_LISTENER(Document, submit);
DEFINE_EVENT_LISTENER(Document, readystatechange);
#ifdef STARFISH_ENABLE_MULTIMEDIA
DEFINE_EVENT_LISTENER(Document, suspend);
DEFINE_EVENT_LISTENER(Document, emptied);
DEFINE_EVENT_LISTENER(Document, stalled);
DEFINE_EVENT_LISTENER(Document, loadedmetadata);
DEFINE_EVENT_LISTENER(Document, loadeddata);
DEFINE_EVENT_LISTENER(Document, canplay);
DEFINE_EVENT_LISTENER(Document, canplaythrough);
DEFINE_EVENT_LISTENER(Document, playing);
DEFINE_EVENT_LISTENER(Document, waiting);
DEFINE_EVENT_LISTENER(Document, seeking);
DEFINE_EVENT_LISTENER(Document, seeked);
DEFINE_EVENT_LISTENER(Document, ended);
DEFINE_EVENT_LISTENER(Document, durationchange);
DEFINE_EVENT_LISTENER(Document, timeupdate);
DEFINE_EVENT_LISTENER(Document, play);
DEFINE_EVENT_LISTENER(Document, pause);
DEFINE_EVENT_LISTENER(Document, ratechange);
DEFINE_EVENT_LISTENER(Document, volumechange);
#endif
}
