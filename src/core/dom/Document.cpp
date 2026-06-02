/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#include <SkMatrix.h>

#include "StarfishConfig.h"
#include "Starfish.h"

#include "browser/history/HistoryManager.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptEngineInstance.h"
#include "core/dom/Attr.h"
#include "core/dom/Attribute.h"
#include "core/dom/CDATASection.h"
#include "core/dom/Comment.h"
#include "core/dom/CustomEvent.h"
#include "core/dom/CustomElementRegistry.h"
#include "core/dom/ProcessingInstruction.h"
#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/DocumentFragment.h"
#include "core/dom/DOMException.h"
#include "core/dom/DOMImplementation.h"
#include "core/dom/Event.h"
#include "core/dom/UIEvent.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/PointerEvent.h"
#include "core/dom/FocusEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "core/dom/TouchEvent.h"
#include "core/page/HashChangeEvent.h"
#include "core/dom/HTMLBaseElement.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/HTMLCollection.h"
#include "core/dom/HTMLDocument.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLFormElement.h"
#include "core/dom/HTMLTitleElement.h"
#include "core/dom/HTMLAnchorElement.h"
#include "core/dom/HTMLDialogElement.h"
#include "core/dom/HTMLImageElement.h"
#include "core/dom/HTMLMapElement.h"
#include "core/dom/HTMLScriptElement.h"
#include "core/dom/HTMLUnknownElement.h"
#ifdef STARFISH_ENABLE_MULTIMEDIA
#include "core/dom/HTMLMediaElement.h"
#endif
#include "core/dom/svg/SVGDocument.h"
#include "core/dom/svg/SVGUseElement.h"
#include "core/dom/Text.h"
#include "core/dom/Traverse.h"
#include "core/dom/builder/html/HTMLDocumentBuilder.h"
#include "core/dom/parser/HTMLParser.h"
#include "core/dom/WebOrigin.h"
#include "core/dom/Range.h"
#include "core/dom/NodeIterator.h"
#include "core/dom/MutationObserver.h"
#include "core/dom/TreeWalker.h"
#include "core/dom/NamedNodeMap.h"
#include "core/dom/NodeFilter.h"
#include "core/dom/IntersectionObserver.h"
#include "core/dom/DOMRect.h"
#include "core/dom/IntersectionObserverEntry.h"
#include "core/modules/resize_observer/ResizeObserver.h"
#include "core/modules/resize_observer/ResizeObserverEntry.h"
#include "core/extra/Console.h"
#include "core/layout/FrameDocument.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/BrowsingContext.h"
#ifdef STARFISH_ENABLE_OBSOLETE_SPEC
#include "core/page/Screen.h"
#endif
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSStyleSheet.h"
#include "core/style/MediaQueryListMatcher.h"
#include "core/style/StyleSheetList.h"
#include "core/style/AdoptedStyleSheets.h"
#include "core/style/StyleRule.h"
#include "core/style/GradientData.h"
#include "core/animation/AnimationExecutor.h"
#include "core/csp/ContentSecurityPolicy.h"
#include "core/modules/canvas/NativeGradient.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "platform/loader/ResourceLoader.h"
#include "platform/loader/ImageResource.h"
#include "platform/file/PlatformFile.h"
#include "platform/loader/ImageResource.h"
#include "platform/network/curl/NetworkSharedResourceManager.h"

namespace Starfish {
#ifdef STARFISH_ENABLE_NETWORK_PROFILING
extern uint64_t g_profilingBaseTime;
#endif
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
    , m_isDialogsInShowModalCacheValid(false)
    , m_isMutationObserverMicroTaskQueued(false)
    , m_executionContext(new ExecutionContext(window, scriptBindingInstance,
                                              uri, charSet, this, true))
    , m_window(window)
    , m_baseElementURL(nullptr)
    , m_baseTarget(String::emptyString)
    , m_contentType(String::createASCIIString("application/xml"))
    , m_resourceLoader(new ResourceLoader(this))
    , m_fontSelector(FontSelector::create(this,
                                          webView()->platformFontSelector(),
                                          webView()->platformFontCache()))
    , m_preloadScanner(nullptr)
    , m_styleResolver(new StyleResolver(this))
    , m_documentBuilder(nullptr)
    , m_styleSheetList(nullptr)
    , m_adoptedStyleSheetsProxy(nullptr)
    , m_brokenImage(nullptr)
    , m_animationExecutor(new AnimationExecutor())
    , m_domVersion(0)
    , m_implementation(nullptr)
    , m_pendingDocumentParsingIdlerHandle(MessageLoopInvalidID)
    , m_contentLanguage(String::emptyString)
    , m_mediaQueryListMatcher(nullptr)
    , m_referrerPolicy(ReferrerPolicy::Empty)
#ifdef STARFISH_TIZEN
    , m_tizenWidgetTransparentBackground(0)
#endif
    , m_nativeGradientCache(nullptr)
    , m_nativeGradientCacheTotalSize(0)
    , m_webFontResolveVersionForCanvas(0)
    , m_mutationTypes(MutationObserverOptionType::kNone)
{
    setBaseURL(fallbackBaseURL());

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
    if (uri->urlString()->equals("about:blank") &&
        browsingContext()->parentBrowsingContext()) {
        setWebOrigin(browsingContext()
                         ->parentBrowsingContext()
                         ->document()
                         ->webOrigin());
    }
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
    StaticStrings* sstrs = m_window->starfish()->staticStrings();

    const char ua[] =
#include "core/style/UserAgentStyleSheet.css"
        ;
    // we assume that there is no important rule in ua-sheet
    STARFISH_ASSERT(strstr(ua, "important") == 0);
    CSSStyleSheet* userAgentStyleSheet =
        new CSSStyleSheet(this, String::createASCIIStringWithNoCopy(ua));
    userAgentStyleSheet->parseSheetIfneeds();
    GCVector<std::pair<CSSStyleDeclaration*, ResourceURL*>> webFonts;
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
    loadBuiltinPolyfill(webView()->builtinPolyfillPathString());

    m_isConnected = true;
    m_nativeGradientCache = new GCUnorderedMap<
        GradientDrawingInfo*, std::shared_ptr<NativeGradient>,
        std::hash<GradientDrawingInfo*>, std::equal_to<GradientDrawingInfo*>>();
}

NodeIterator* Document::createNodeIterator(Node* root, unsigned whatToShow,
                                           ScriptValue filter)
{
    return new NodeIterator(this, root, whatToShow, filter);
}

TreeWalker* Document::createTreeWalker(Node* root, unsigned whatToShow,
                                       ScriptValue filter)
{
    return new TreeWalker(this, root, whatToShow, filter);
}

BrowsingContext* Document::browsingContext() const
{
    return window()->browsingContext();
}

ScriptBindingInstance* Document::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

Location* Document::location()
{
    if (window()->document() != this) {
        return nullptr;
    }
    return window()->location();
}

String* Document::referrer()
{
    return executionContext()->referrer();
}

ReferrerPolicy Document::referrerPolicy()
{
    // https://www.w3.org/TR/referrer-policy/#referrer-policy-delivery
    if (webOrigin()->isOpaque()) {
        return ReferrerPolicy::NoReferrer;
    }

    if (browsingContext()->isTopLevelBrowsingContext()) {
        return m_referrerPolicy;
    }

    if (m_referrerPolicy != ReferrerPolicy::Empty) {
        return m_referrerPolicy;
    } else {
        return browsingContext()
            ->parentBrowsingContext()
            ->document()
            ->referrerPolicy();
    }
}

String* Document::cookie()
{
    // TODO : Throw a "SecurityError" DOMException on getting and setting.
    // * If the contents are sandboxed into a unique origin (e.g. in an iframe
    //   with the sandbox attribute)

    if ((!documentURI()->isFileURL()) && webOrigin()->isOpaque()) {
        throw new DOMException(
            executionContext(), DOMException::Code::SECURITY_ERR,
            "Access is denied for this document, origin is opaque");
    }

    String* ret =
        NetworkSharedResourceManager::getInstance()->cookies(documentURI());
    return ret;
}

void Document::setCookie(String* cookie)
{
    if ((!documentURI()->isFileURL()) && webOrigin()->isOpaque()) {
        throw new DOMException(
            executionContext(), DOMException::Code::SECURITY_ERR,
            "Access is denied for this document, origin is opaque");
    }
    NetworkSharedResourceManager::getInstance()->setCookies(
        executionContext(), documentURI(), cookie);
}

void Document::init(ReferrerURL* referrerURL)
{
    m_resourceLoader->markDocumentOpenState();

    m_documentBuilder = new HTMLDocumentBuilder(this);
    m_documentBuilder->build(documentURI(), referrerURL);
}

WindowProxy* Document::open(String* url, String* name, String* features)
{
    // TODO If this Document object is not an active document, then throw an
    // "InvalidStateError" DOMException exception.
    STARFISH_UNSUPPORTED_METHOD();
    return nullptr;
}

Document* Document::open(Document* responsibleDoc, String* type,
                         String* replaceInput)
{
    // https://html.spec.whatwg.org/multipage/dynamic-markup-insertion.html#opening-the-input-stream

    // If document is an XML document, then throw an "InvalidStateError"
    // DOMException exception.
    if (isXMLDocument()) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INVALID_STATE_ERR);
    }
    STARFISH_ASSERT(isHTMLDocument());
    // If document's throw-on-dynamic-markup-insertion counter is greater than
    // 0, then throw an "InvalidStateError" DOMException.
    if (m_throwOnDynamicMarkupInsertion) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INVALID_STATE_ERR);
    }
    // TODO (implement WindowProxy) If document is not an active document, then
    // return document.
    // TODO (implement WindowProxy) If document's origin is not same origin to
    // the origin of the responsible document specified by the entry settings
    // object, then throw a "SecurityError" DOMException.
    if (!responsibleDoc->webOrigin()->isSameOrigin(webOrigin())) {
        throw new DOMException(responsibleDoc->executionContext(),
                               DOMException::Code::SECURITY_ERR);
    }
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
    setCharacterSet(String::fromUTF8("UTF-8"));

    // TODO If document is ready for post-load tasks, then set document's reload
    // override flag and set document's reload override buffer to the empty
    // string.
    // Set document's salvageable state back to true.
    m_salvageable = true;

    // TODO Change document's URL to the URL of the responsible document
    // specified by the entry settings object.
    // TODO If document's iframe load in progress flag is set, then set
    // document's mute iframe load flag.

    invalidNamedAccessCacheIfNeeded(String::emptyString, false, false);
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
        throw new DOMException(executionContext(),
                               DOMException::Code::INVALID_STATE_ERR);
    }
    // If the Document object's throw-on-dynamic-markup-insertion counter is
    // greater than zero, then throw an "InvalidStateError" DOMException and
    // abort these steps.
    if (m_throwOnDynamicMarkupInsertion) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INVALID_STATE_ERR);
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

void Document::write(Document* responsibleDoc, const GCVector<String*>& str)
{
    // https://html.spec.whatwg.org/multipage/dynamic-markup-insertion.html#dom-document-write
    // If document is an XML document, then throw an "InvalidStateError"
    // DOMException.
    if (isXMLDocument()) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INVALID_STATE_ERR);
    }
    // If document's throw-on-dynamic-markup-insertion counter is greater than
    // 0, then throw an "InvalidStateError" DOMException.
    if (m_throwOnDynamicMarkupInsertion) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INVALID_STATE_ERR);
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
        open(responsibleDoc, String::createASCIIString("text/html"),
             String::emptyString);
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

void Document::writeln(Document* responsibleDoc, const GCVector<String*>& str)
{
    GCVector<String*> newStr = str;
    newStr.push_back(String::createASCIIString('\n'));
    write(responsibleDoc, str);
}

void Document::resumeDocumentParsing()
{
    STARFISH_ASSERT(m_pendingDocumentParsingIdlerHandle ==
                    MessageLoopInvalidID);
    m_pendingDocumentParsingIdlerHandle =
        window()->webView()->messageLoop()->addIdler(
            window(),
            [](size_t handle, void* data) {
                Document* document = (Document*)data;
                MicroTaskExecutionManager m(
                    document->scriptBindingInstance()->engineInstance());
                STARFISH_ASSERT(document->m_documentBuilder);
                document->m_pendingDocumentParsingIdlerHandle =
                    MessageLoopInvalidID;
                document->m_documentBuilder->resume();
            },
            this);
}

void Document::endDocumentParsing()
{
    if (browsingContext()->isTopLevelBrowsingContext()) {
        m_resourceLoader->setLoadProgressState(
            ResourceLoader::LoadProgressState::ParsingEnd);
    }
    if (m_pendingDocumentParsingIdlerHandle != MessageLoopInvalidID) {
        window()->webView()->messageLoop()->removeIdler(
            m_pendingDocumentParsingIdlerHandle);
        m_pendingDocumentParsingIdlerHandle = MessageLoopInvalidID;
    }
    m_documentBuilder = nullptr;
}

static void executeModule(Document* document,
                          GCVector<Document::ScriptModuleData*>& moduleScripts,
                          size_t startSize, bool fromParser)
{
    // we should check moduleScripts.size() here
    // since we fire load, error events here
    for (size_t i = 0; i < startSize && i < moduleScripts.size(); i++) {
        Document::ScriptModuleData* data = moduleScripts[i];
        if (data->fromParser == fromParser) {
            // execute module
            if (!data->hasLoadingError) {
                STARFISH_ASSERT(data->module.hasValue());
                auto scriptModule = data->module.value();
                if (isExecutableModule(scriptModule)) {
                    data->wasSuccessful = executeModule(
                        document->scriptBindingInstance(), scriptModule);
                }
            }

            // dispatch load, error event of js module
            if (data->source &&
                !data->source->didModuleLoadOrErrorEventFired() &&
                data->url.hasValue()) {
                String* eventType;
                if (data->wasSuccessful) {
                    eventType = document->starfish()
                                    ->staticStrings()
                                    ->m_load.localName();
                } else {
                    eventType = document->starfish()
                                    ->staticStrings()
                                    ->m_error.localName();
                }
                data->source->dispatchEventByUA(
                    data->source.value(),
                    new Event(document->executionContext(), eventType,
                              EventInit(false, false)),
                    true);
                data->source->markModuleLoadOrErrorEventFired();
            }
            for (auto* promise : data->promiseForDynamicLoadedModule) {
                notifyDynamicLoadedModuleResult(
                    document->scriptBindingInstance(), data->module.value(),
                    promise);
            }
            data->promiseForDynamicLoadedModule.clear();
        }
    }
}

void Document::notifyDomContentLoaded()
{
    if (m_deferredScriptElements.size() || m_deferredSVGScriptElements.size() ||
        m_pendingDynamicLoadedModules.size()) {
        return;
    }

    MicroTaskExecutionManager m(scriptBindingInstance()->engineInstance());

    size_t startSize = m_moduleScripts.size();
    auto& moduleScripts = m_moduleScripts;
    executeModule(this, moduleScripts, startSize, true);

    if (!m_domContentLoadedFired) {
        m_preloadScanner = nullptr;
        m_resourceLoader->notifyEndParseDocument();
        m_domContentLoadedFired = true;

        String* eventType = window()
                                ->starfish()
                                ->staticStrings()
                                ->m_DOMContentLoaded.localName();
        Event* e =
            new Event(executionContext(), eventType, EventInit(true, true));

        if (window()->document() == this) {
            window()->performance()->timing()->m_domContentLoadedEventStart =
                timestamp();

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
                HTMLMediaElement* target =
                    mediaElements[i]->asHTMLMediaElement();
                target->onDOMContentLoaded();
            }
#endif

            window()->performance()->timing()->m_domContentLoadedEventEnd =
                timestamp();
            STARFISH_LOG_INFO("Document::notifyDomContentLoaded");
#ifdef STARFISH_ENABLE_NETWORK_PROFILING
            if (browsingContext()->isTopLevelBrowsingContext()) {
                STARFISH_LOG_INFO(
                    "[NETWORK_PROFILING] Document::notifyDomContentLoaded at "
                    "%dms",
                    (int)(timestamp() - g_profilingBaseTime));
            }
#endif

            if (m_compatibilityMode != NoQuirksMode) {
                std::string s;
                if (documentURI()->urlString()->length() > 128) {
                    s = documentURI()
                            ->urlString()
                            ->substring(0, 128)
                            ->toUTF8NonGCString();
                    s += "...";
                } else {
                    s = documentURI()->urlString()->toUTF8NonGCString();
                }

                STARFISH_LOG_INFO(
                    "No doctype is found or quirks mode is given in "
                    "%s",
                    s.data());
                STARFISH_LOG_INFO(
                    "Please make sure the document starts with "
                    "\"<!DOCTYPE html>\"");
                STARFISH_LOG_INFO("Quirks mode is not supported.");
                STARFISH_LOG_INFO("Processing the document in no-quirks mode.");
            }
        } else {
            // In case of DOMParser,
            // window.document != this
            EventTarget::dispatchEventByUA(this, e, true);
        }

        // if there is a fragment identifier, set cssTarget.
        String* fragment = documentURI()->hash();
        if (!fragment->equals(String::emptyString)) {
            window()->processUrlFragment(
                fragment->substring(1, fragment->length() - 1));
        }
    }

    if (browsingContext()->isTopLevelBrowsingContext()) {
        m_resourceLoader->setLoadProgressState(
            ResourceLoader::LoadProgressState::DomContentLoaded);
        struct Param : public gc {
            String* url;
        };
        Param* p = new Param;
        p->url = this->urlString();
        webView()->callPublicWebViewHandler(OnPageParsed, p);
    }

    executeModule(this, moduleScripts, startSize, false);
}

void Document::dispose()
{
    if (!m_onLoadFired) {
        m_resourceLoader->decreasePendingResourceCountWhileDocumentOpening();
    }

    HTMLElement* body = this->body();
    if (body) {
        String* eventType =
            window()->starfish()->staticStrings()->m_unload.localName();
        Event* e = new Event(executionContext(), eventType);
        EventTarget::dispatchEventByUA(body, e);
    }

    m_moduleScripts.clear();

    resourceLoader().clear();

    executionContext()->disposeActiveResourceRequests();
#ifdef STARFISH_ENABLE_WEBSOCKET
    executionContext()->disposeActiveWebSockets();
#endif
    executionContext()->clearPointerRootMap();

    m_fontSelector->clearWholeCache();

    clearNativeGradientCacheIfNeeds();

    if (m_animationExecutor != nullptr) {
        auto& v = window()->webView()->activeAnimationExecutor();
        for (size_t i = 0; i < v.size(); i++) {
            if (v[i] == m_animationExecutor) {
                v[i]->dispose();
                v.erase(i);
            }
        }
    }

    if (m_intersectionObservers.size()) {
        GCVector<IntersectionObserver*> observers = m_intersectionObservers;
        for (auto* observer : observers) {
            observer->disconnect();
        }
    }

    if (m_resizeObservers.size()) {
        GCVector<ResizeObserver*> observers = m_resizeObservers;
        for (auto* observer : observers) {
            observer->disconnect();
        }
    }

    m_isMutationObserverMicroTaskQueued = false;
    GCUnorderedSet<MutationObserver*>().swap(m_activeMuationObservers);
}

void Document::onIdle()
{
    const auto& v = loadedWebFontList();
    for (size_t i = 0; i < v.size(); i++) {
        if (v[i]->fontFace()) {
            v[i]->fontFace()->clearCache();
        }
    }

    clearNativeGradientCacheIfNeeds();
    m_svgPaintClientElements.clear();
}

String* Document::characterSet()
{
    return executionContext()->characterSet();
}

void Document::setCharacterSet(String* s)
{
    executionContext()->setCharacterSet(s);
}

String* Document::nodeName()
{
    return window()->starfish()->staticStrings()->m_documentLocalName.string();
}

String* Document::localName()
{
    return window()->starfish()->staticStrings()->m_documentLocalName.string();
}

Element* Document::getElementById(String* id)
{
    if (id->length() == 0) {
        return nullptr;
    }

    return getElementById(AtomicString::createAtomicString(starfish(), id));
}

Element* Document::getElementById(AtomicString id)
{
    return (Element*)Traverse::findDescendant(this, [&](Node* child) {
        if (child->isElement() && child->asElement()->atomicId() == id) {
            return true;
        } else {
            return false;
        }
    });
}

HTMLMapElement* Document::imageMapElement(String* url)
{
    if (url->isEmpty()) {
        return nullptr;
    }

    size_t hashPos = url->find("#");
    if (hashPos == SIZE_MAX) {
        return nullptr;
    }

    String* usemap = url->substring(hashPos + 1, url->length() - 1);
    NodeList* maps =
        querySelectorAll(starfish()->staticStrings()->m_mapTagName.localName());
    for (size_t i = 0; Node* node = maps->item(i); ++i) {
        HTMLMapElement* map = node->asHTMLMapElement();
        String* name = map->nameAttr();
        if (!name->isEmpty() && name->equals(usemap)) {
            return map;
        }
    }

    return nullptr;
}

NodeList* Document::getElementsByName(String* elementName)
{
    return ensureRareMembers()->ensureQueryInActiveNodeListVectorForName(
        this, elementName);
}

HTMLCollection* Document::images()
{
    return getElementsByTagName(starfish()->staticStrings()->m_imgTagName);
}

HTMLCollection* Document::links()
{
    return getElementsByTagName(starfish()->staticStrings()->m_linkTagName);
}

HTMLCollection* Document::forms()
{
    return getElementsByTagName(starfish()->staticStrings()->m_formTagName);
}

HTMLCollection* Document::scripts()
{
    return getElementsByTagName(starfish()->staticStrings()->m_scriptTagName);
}

HTMLCollection* Document::anchors()
{
    return getElementsByTagName(starfish()->staticStrings()->m_aTagName);
}

DocumentFragment* Document::createDocumentFragment()
{
    return new DocumentFragment(this);
}

Element* Document::createElement(String* localName)
{
    if (!QualifiedName::checkNameProductionRule(localName)) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INVALID_CHARACTER_ERR,
                               nullptr);
    }

    AtomicString localNameAtomic;
    if (isHTMLDocument()) {
        AtomicString namespaceURI = AtomicString::createAtomicString(
            window()->starfish(), HTML_NAMESPACE);
        localNameAtomic = AtomicString::createAttrAtomicString(
            window()->starfish(), localName);
        return HTMLDocument::createHTMLElement(
            this, QualifiedName(namespaceURI, localNameAtomic));
    } else {
        localNameAtomic =
            AtomicString::createAtomicString(window()->starfish(), localName);
        if (contentType()->equals("application/xhtml+xml")) {
            AtomicString namespaceURI = AtomicString::createAtomicString(
                window()->starfish(), HTML_NAMESPACE);
            return new NamedElement(
                this, QualifiedName(namespaceURI, localNameAtomic));
        }
    }
    return new NamedElement(this, QualifiedName(localNameAtomic));
}

// https://dom.spec.whatwg.org/#validate-and-extract
QualifiedName Document::validateAndExtractQualifiedName(Optional<String*> ns,
                                                        String* qualifiedName)
{
    // If namespace is the empty string, set it to null.
    if (ns.hasValue() && !ns.getValue()->length()) {
        ns = Optional<String*>();
    }
    // Validate qualifiedName.
    if (!QualifiedName::validateQualifiedName(qualifiedName)) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INVALID_CHARACTER_ERR);
    }
    // Let prefix be null.
    Optional<AtomicString> prefix;
    AtomicString localName;
    // If qualifiedName contains a ":" (U+003E), then split the string on it and
    // set prefix to the part before and localName to the part after.
    // + It is not valid if qualifiedName has multiple ":" characters.
    // + It is not valid if it has 0-length prefix or localName part.
    GCVector<StringView> tokens;
    StringUtils::tokenize(qualifiedName, ":", 1, tokens);
    if (tokens.size() > 2) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INVALID_CHARACTER_ERR);
    } else if (tokens.size() == 2) {
        if (tokens[0].length() == 0 || tokens[1].length() == 0) {
            throw new DOMException(executionContext(),
                                   DOMException::Code::INVALID_CHARACTER_ERR);
        }
        prefix = AtomicString::createAtomicString(starfish(), tokens[0]);
        localName = AtomicString::createAtomicString(starfish(), tokens[1]);
    } else {
        localName = AtomicString::createAtomicString(starfish(), qualifiedName);
    }

    // If prefix is non-null and namespace is null, then throw a NamespaceError.
    if (prefix.hasValue() && !ns.hasValue()) {
        throw new DOMException(executionContext(), DOMException::NAMESPACE_ERR,
                               "Provided namespace is wrong");
    }

    AtomicString nsURI;
    ;
    if (ns.hasValue()) {
        nsURI = AtomicString::createAtomicString(starfish(), ns.getValue());
    }
    StaticStrings* strs = starfish()->staticStrings();
    // If prefix is "xml" and namespace is not the XML namespace, then throw a
    // NamespaceError.
    if (prefix.hasValue() && prefix.getValue() == strs->m_xml &&
        nsURI != strs->m_xmlNamespaceURI) {
        throw new DOMException(executionContext(), DOMException::NAMESPACE_ERR,
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
        throw new DOMException(executionContext(), DOMException::NAMESPACE_ERR,
                               "Provided namespace is wrong");
    }

    if (prefix.hasValue()) {
        return QualifiedName(prefix.getValue(), nsURI, localName);
    } else if (ns.hasValue()) {
        return QualifiedName(nsURI, localName);
    } else {
        return QualifiedName(localName);
    }
}

Element* Document::createElementNS(Optional<String*> namespaceString,
                                   String* qualifiedName)
{
    QualifiedName name =
        validateAndExtractQualifiedName(namespaceString, qualifiedName);
    if (name.namespaceURI().hasValue()) {
        if (name.namespaceURI().getValue().string()->equals(HTML_NAMESPACE)) {
            return HTMLDocument::createHTMLElement(this, name);
        }

        if (name.namespaceURI().getValue().string()->equals(SVG_NAMESPACE)) {
            return SVGDocument::createSVGElement(this, name);
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
            executionContext(), DOMException::Code::NOT_SUPPORTED_ERR,
            "This operation is not supported for HTML documents.");
    }
    if (data->contains("]]>")) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INVALID_CHARACTER_ERR,
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
        throw new DOMException(executionContext(),
                               DOMException::Code::INVALID_CHARACTER_ERR);
    }
    // If data contains the string "?>", then throw an InvalidCharacterError.
    if (data->contains("?>")) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INVALID_CHARACTER_ERR);
    }
    // Return a new ProcessingInstruction node, with target set to target, data
    // set to data, and node document set to the context object.
    return new ProcessingInstruction(this, data, target);
}

Node* Document::importNode(Node* node, bool deep)
{
    // If node is a document or a shadow root,
    // throws a "NotSupportedError" DOMException.
    // TODO: check shadow root node
    if (node->isDocument()) {
        throw new DOMException(executionContext(),
                               DOMException::Code::NOT_SUPPORTED_ERR, nullptr);
    }

    Node* newNode = node->clone();
    STARFISH_ASSERT(newNode);
    newNode->setDocument(this);

    if (deep) {
        for (Node* child = node->firstChild(); child;
             child = child->nextSibling()) {
            Node* newChild = importNode(child, true);
            STARFISH_ASSERT(newChild);
            newChild->setDocument(this);
            newNode->appendChild(newChild);
        }
    }
    return newNode;
}

Node* Document::adoptNode(Node* node)
{
    // If node is a document, then throw a "NotSupportedError" DOMException.
    // TODO check shadow root node
    if (node->isDocument()) {
        throw new DOMException(executionContext(),
                               DOMException::Code::NOT_SUPPORTED_ERR, nullptr);
    }

    Node* oldDocument = node->document();
    if (node->parentNode()) {
        node->remove();
    }

    if (this != oldDocument) {
        node->setDocument(this);
        Node* next = node->firstChild();
        while (next) {
            next->setDocument(this);
            if (next->isElement()) {
                NamedNodeMap* map = next->asElement()->attributes();
                for (unsigned i = 0; i < map->length(); i++) {
                    Attr* attr = map->item(i);
                    attr->setDocument(this);
                }
            }
            next = Traverse::next(next, node);
        }
    }

    return node;
}

#ifdef STARFISH_ENABLE_OBSOLETE_SPEC
uint32_t Document::width()
{
    return window()->screen()->width();
}

uint32_t Document::height()
{
    return window()->screen()->height();
}
#endif

Attr* Document::createAttribute(String* name)
{
    return createAttribute(createAttributeName(name));
}

Attr* Document::createAttribute(QualifiedName localName)
{
    if (!QualifiedName::checkNameProductionRule(localName.localName())) {
        throw new DOMException(executionContext(),
                               DOMException::Code::INVALID_CHARACTER_ERR,
                               nullptr);
    }

    return new Attr(this, localName);
}

Attr* Document::createAttributeNS(Optional<String*> ns, String* name)
{
    QualifiedName qName = validateAndExtractQualifiedName(ns, name);
    return new Attr(this, qName);
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
    if (m_body) {
        return m_body->asHTMLElement();
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

void Document::setBody(Optional<HTMLElement*> element)
{
    if (!(element && element->isHTMLBodyElement())) {
        COMPOSE_MESSAGE(reason, ARG_TYPE_MISMATCH_2, "1", "body",
                        "HTMLBodyElement", "HTMLFrameSetElement");
        COMPOSE_MESSAGE(msg, FAILED_TO_SET_PROPERTY, "body", "Document",
                        reason);
        throw new DOMException(executionContext(),
                               DOMException::HIERARCHY_REQUEST_ERR, msg);
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
    Optional<String*> value = title->textContent();
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
            element = new HTMLTitleElement(
                document(), starfish()->staticStrings()->m_titleTagName);
        }
        // Append element to the head element.
        head->appendChild(element);
        // Act as if the textContent IDL attribute of element was set to the new
        // value being assigned.
        element->setTextContent(titleString);
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
    STARFISH_UNSUPPORTED_METHOD();
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
            starfish()->staticStrings()->m_visibilitychange.localName();
        Event* e = new Event(executionContext(), eventType, EventInit(true));
        EventTarget::dispatchEventByUA(this->asNode(), e);
    }
}

void Document::setReadyState(DocumentReadyState newState)
{
    DocumentReadyState old = m_readyState;
    m_readyState = newState;
    if (old != newState) {
        String* eventType =
            starfish()->staticStrings()->m_readystatechange.localName();
        Event* e =
            new Event(executionContext(), eventType, EventInit(false, false));
        dispatchEventByUA(e);
    }
}

Element* Document::scrollingElement()
{
    // https://drafts.csswg.org/cssom-view/#dom-document-scrollingelement
    if (inQuirksMode()) {
        if (body() && body()->frame() &&
            body()->asHTMLBodyElement()->isPotentiallyScrollable()) {
            return body();
        }
        return nullptr;
    }
    return documentElement();
}

ResourceURL* Document::documentURI() const
{
    return executionContext()->documentURI();
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
        setBaseURL(m_baseElementURL);
    } else {
        setBaseURL(fallbackBaseURL());
    }

    if (!executionContext()->baseURL()->isValid()) {
        setBaseURL(ResourceURL::aboutBlankURL());
    }
}

void Document::setBaseURL(ResourceURL* newURL)
{
    executionContext()->setBaseURL(newURL);
}

ResourceURL* Document::baseURL() const
{
    return executionContext()->baseURL();
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
    while (baseElement && (!href || (target && target->isEmpty()))) {
        if (!href && baseElement->hasAttribute(
                         starfish()->staticStrings()->m_href) != SIZE_MAX) {
            href = baseElement->getAttributeOrEmpty(
                starfish()->staticStrings()->m_href);
        }
        if (!target && baseElement->hasAttribute(
                           starfish()->staticStrings()->m_target) != SIZE_MAX) {
            target = baseElement->getAttributeOrEmpty(
                starfish()->staticStrings()->m_target);
        }
        baseElement = nextBaseElement(baseElement, this);
    }

    ResourceURL* baseElementURL = nullptr;
    if (href) {
        baseElementURL = new ResourceURL(href, fallbackBaseURL()->urlString());
        if (!contentSecurityPolicy()->allowSource(CSPDirectives::BaseURI,
                                                  baseElementURL)) {
            baseElementURL = nullptr;
        }
    }
    if (baseElementURL) {
        if (baseElementURL->isDataURL()) {
            webView()->console()->error(String::createASCIIString(
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
    invalidFocusRingCacheIfNeeded();
    clearDialogsInShowModalCache();
    window()->invalidateFramesIfNeeded();
}

void Document::attachNodeIterator(NodeIterator* ni)
{
    m_nodeIterators.push_back(ni);
}

void Document::willNodeBeRemoved(Node* parent, Node* oldChild)
{
    if (m_nodeIterators.size()) {
        for (NodeIterator* ni : m_nodeIterators) {
            ni->willNodeBeRemoved(oldChild);
        }
    }
}

void Document::didNodeInserted(Node* parent, Node* newChild)
{
    Node::didNodeInserted(parent, newChild);

    if (UNLIKELY(newChild->isHTMLBaseElement())) {
        processBaseElement();
    } else if (UNLIKELY(newChild->isHTMLUnknownElement())) {
        if (window()->hasCustomElements()) {
            window()->customElements()->upgrade(newChild, true);
        }
    } else if (UNLIKELY(newChild->isSVGElement())) {
        if (newChild->asSVGElement()->isPaintServerLikeElement()) {
            if (newChild->asElement()->atomicId().string()->length()) {
                notifyNeedsLayoutOrPaintingToSVGPaintClientElements(
                    newChild->asElement()->atomicId(), true);
            }
        }
    } else if (UNLIKELY(newChild->isHTMLBodyElement())) {
        m_body = childMatchedBy(
            this, [](Node* nd) -> bool { return nd->isHTMLBodyElement(); });
    }

    updateDOMVersion();
}

void Document::didNodeRemoved(Node* parent, Node* oldChild)
{
    Node::didNodeRemoved(parent, oldChild);

    if (UNLIKELY(oldChild->isHTMLBaseElement())) {
        processBaseElement();
    } else if (UNLIKELY(oldChild->isHTMLBodyElement())) {
        m_body = childMatchedBy(
            this, [](Node* nd) -> bool { return nd->isHTMLBodyElement(); });
    }

    updateDOMVersion();
}

HTMLCollection* Document::namedAccess(String* name)
{
    if (!m_nameIdFilter.mayContain(name)) {
        return nullptr;
    }

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
                          AtomicString::createAtomicString(starfish(), name));
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
    m_focusRingCache.clear();
    m_isFocusRingCacheValid = false;
}

const GCAtomicVector<Element*>& Document::focusRing()
{
    webView()->layoutIfNeeded(false);

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
                        STARFISH_ASSERT(m_nodeIndex >= o.m_nodeIndex);
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
                    if (e->document()->isInertNode(e)) {
                        return false;
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
        m_isFocusRingCacheValid = true;
    }

    return m_focusRingCache;
}

void Document::clearDialogsInShowModalCache()
{
    m_dialogsInShowModal.clear();
    m_isDialogsInShowModalCacheValid = false;
}

// https://html.spec.whatwg.org/multipage/interaction.html#inert
bool Document::isInertNode(Node* node)
{
    if (!m_isDialogsInShowModalCacheValid) {
        m_dialogsInShowModal.clear();
        GCVector<Element*> dialogsInShowModal;
        Traverse::collectDescendants(
            dialogsInShowModal, document()->rootElement(),
            [](Element* e) -> bool {
                if (e->isHTMLDialogElement() &&
                    e->asHTMLDialogElement()->isInShowModal()) {
                    return true;
                }
                return false;
            },
            false);
        m_dialogsInShowModal.insert(m_dialogsInShowModal.end(),
                                    dialogsInShowModal.begin(),
                                    dialogsInShowModal.end());
        m_isDialogsInShowModalCacheValid = true;
    }

    if (m_dialogsInShowModal.size() > 0) {
        for (auto dialog : m_dialogsInShowModal) {
            if (node->isDescendantOf(dialog)) {
                return false;
            }
        }
        return true;
    }

    return false;
}

void Document::invalidNamedAccessCacheIfNeeded(String* name,
                                               bool isNameAppeared,
                                               bool isNameDisappared)
{
    if (isNameAppeared) {
        m_nameIdFilter.add(name);
    }

    if (isNameDisappared) {
        m_nameIdFilter.remove(name);
    }

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
        node = node->renderingParentNode();
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

ScriptProxyObject Document::adoptedStyleSheetsObservableArray(
    Escargot::ExecutionStateRef* state)
{
    return AdoptedStyleSheets::observableArray(state, this);
}

void Document::setAdoptedStyleSheetsFromObservableArray(
    Escargot::ExecutionStateRef* state, Escargot::ValueRef* value)
{
    AdoptedStyleSheets::setFromObservableArray(state, this, value);
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
        RequestData* reqData = new RequestData();
        reqData->m_url = res->url();
        reqData->m_referrer = new ReferrerURL(String::emptyString);
        reqData->m_syncLevel = RequestSyncLevel::AlwaysSync;
        reqData->m_destination = RequestDestination::Image;

        res->request(reqData, false);

        m_brokenImage = res->imageData();
        m_didLoadBrokenImage = true;
        return m_brokenImage;
    }
}

QualifiedName Document::createAttributeName(String* name)
{
    if (isXMLDocument()) {
        return QualifiedName(
            AtomicString::createAtomicString(window()->starfish(), name));
    } else {
        return QualifiedName(
            AtomicString::createAttrAtomicString(window()->starfish(), name));
    }
}

QualifiedName Document::createAttributeNameNS(Optional<String*> ns,
                                              String* localName)
{
    // Case sensitive
    const AtomicString& localNameAtomic =
        AtomicString::createAtomicString(starfish(), localName);
    if (ns.hasValue()) {
        const AtomicString& nsAtomic =
            AtomicString::createAtomicString(starfish(), ns.getValue());
        return QualifiedName(nsAtomic, localNameAtomic);
    }
    return QualifiedName(localNameAtomic);
}

Range* Document::createRange()
{
    return Range::create(this);
}

DOMImplementation* Document::implementation()
{
    if (m_implementation == nullptr) {
        m_implementation = new DOMImplementation(this);
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
    if (browsingContext()->focusedNode()) {
        return true;
    }

    bool hasFocus = false;
    if (!browsingContext()->focusedNode()) {
        browsingContext()->iterateChildContext(
            [&hasFocus](BrowsingContext* ctx) {
                hasFocus |= ctx->document()->hasFocus();
            });
    }
    return hasFocus;
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
        browsingContext()->setFocusedNode(this, false);
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
    STARFISH_ASSERT(webOrigin() != nullptr);
    return webOrigin()->serialize();
}

WebOrigin* Document::webOrigin()
{
    return executionContext()->webOrigin();
}

void Document::setWebOrigin(WebOrigin* webOrigin)
{
    executionContext()->setWebOrigin(webOrigin);
}

// https://html.spec.whatwg.org/multipage/origin.html#dom-document-domain
String* Document::domain()
{
    STARFISH_ASSERT(webOrigin() != nullptr);
    if (!browsingContext()) {
        return String::emptyString;
    }

    Optional<String*> effectiveDomain = webOrigin()->domain();
    if (effectiveDomain.hasValue()) {
        return effectiveDomain.getValue();
    }
    return String::emptyString;
}

void Document::setDomain(String* domain)
{
    STARFISH_UNSUPPORTED_METHOD();
}

Optional<HTMLOrSVGScriptElement> Document::currentScript()
{
    if (m_currentScripts.size() == 0) {
        return Optional<HTMLOrSVGScriptElement>();
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
    STARFISH_LOG_INFO("Load built-in javascript polyfill");
    auto in = PlatformFile::open(localPath, PlatformFile::FileMode::Read);
    if (!in) {
        STARFISH_LOG_INFO("Invalid built-in polyfill path.");
        return;
    }
    Optional<String*> data = in->readAll();
    in.reset();
    if (!data.hasValue()) {
        STARFISH_LOG_INFO("Invalid built-in polyfill content.");
        return;
    }
    evaluateString(window()->scriptBindingInstance(), data.getValue());
    STARFISH_LOG_INFO("Built-in polyfill evaluated.");
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
            e = new Event(executionContext());
        }
        break;
    case 6:
        if (type->equals("events")) {
            e = new Event(executionContext());
        }
        break;
    case 7:
        if (type->equals("uievent")) {
            e = new UIEvent(executionContext());
        }
        break;
    case 8:
        if (type->equals("uievents")) {
            e = new UIEvent(executionContext());
        }
        break;
    case 9:
        if (type->equals("dragevent")) {
            STARFISH_UNSUPPORTED("DragEvent is unsupported(%s)",
                                 __PRETTY_FUNCTION__);
            e = new Event(executionContext());
        } else if (type->equals("svgevents")) {
            STARFISH_UNSUPPORTED("SVGEvents is unsupported(%s)",
                                 __PRETTY_FUNCTION__);
            e = new Event(executionContext());
        } else if (type->equals("textevent")) {
            STARFISH_UNSUPPORTED("TextEvent is unsupported(%s)",
                                 __PRETTY_FUNCTION__);
            e = new Event(executionContext());
        }
        break;
    case 10:
        switch (type->charAt(0)) {
        case 'c':
            if (type->equals("closeevent")) {
                STARFISH_UNSUPPORTED("CloseEvent is unsupported(%s)",
                                     __PRETTY_FUNCTION__);
                e = new Event(executionContext());
            }
            break;
        case 'e':
            if (type->equals("errorevent")) {
                STARFISH_UNSUPPORTED("ErrorEvent is unsupported(%s)",
                                     __PRETTY_FUNCTION__);
                e = new Event(executionContext());
            }
            break;
        case 'f':
            if (type->equals("focusevent")) {
                e = new FocusEvent(executionContext());
            }
            break;
        case 'h':
            if (type->equals("htmlevents")) {
                STARFISH_UNSUPPORTED("HTMLEvents is unsupported(%s)",
                                     __PRETTY_FUNCTION__);
                e = new Event(executionContext());
            }
            break;
        case 'm':
            if (type->equals("mouseevent")) {
                e = new MouseEvent(executionContext());
            }
            break;
        case 't':
            if (type->equals("touchevent")) {
                e = new TouchEvent(this);
            } else if (type->equals("trackevent")) {
                STARFISH_UNSUPPORTED("TrackEvent is unsupported(%s)",
                                     __PRETTY_FUNCTION__);
                e = new Event(executionContext());
            }
            break;
        case 'w':
            if (type->equals("wheelevent")) {
                STARFISH_UNSUPPORTED("WheelEvent is unsupported(%s)",
                                     __PRETTY_FUNCTION__);
                e = new Event(executionContext());
            }
            break;
        default:
            break;
        }
        break;
    case 11:
        if (type->equals("customevent")) {
            e = new CustomEvent(executionContext());
        } else if (type->equals("mouseevents")) {
            e = new MouseEvent(executionContext());
        }
        break;
    case 12:
        if (type->equals("messageevent")) {
            STARFISH_UNSUPPORTED("MessageEvent is unsupported(%s)",
                                 __PRETTY_FUNCTION__);
            e = new Event(executionContext());
        } else if (type->equals("storageevent")) {
            STARFISH_UNSUPPORTED("StorageEvent is unsupported(%s)",
                                 __PRETTY_FUNCTION__);
            e = new Event(executionContext());
        }
        break;
    case 13:
        if (type->equals("keyboardevent")) {
            e = new KeyboardEvent(executionContext());
        } else if (type->equals("popstateevent")) {
            STARFISH_UNSUPPORTED("PopStateEvent is unsupported(%s)",
                                 __PRETTY_FUNCTION__);
            e = new Event(executionContext());
        } else if (type->equals("mutationevent")) {
            STARFISH_UNSUPPORTED("MutationEvent is unsupported(%s)",
                                 __PRETTY_FUNCTION__);
            e = new Event(executionContext());
        }
        break;
    case 14:
        if (type->equals("animationevent")) {
            STARFISH_UNSUPPORTED("AnimationEvent is unsupported(%s)",
                                 __PRETTY_FUNCTION__);
            e = new Event(executionContext());
        } else if (type->equals("mutationevents")) {
            STARFISH_UNSUPPORTED("MutationEvents is unsupported(%s)",
                                 __PRETTY_FUNCTION__);
            e = new Event(executionContext());
        }
        break;
    case 15:
        if (type->equals("hashchangeevent")) {
            e = new HashChangeEvent(executionContext());
        } else if (type->equals("transitionevent")) {
            STARFISH_UNSUPPORTED("TransitionEvent is unsupported(%s)",
                                 __PRETTY_FUNCTION__);
            e = new Event(executionContext());
        }
        break;
    case 17:
        if (type->equals("beforeunloadevent")) {
            STARFISH_UNSUPPORTED("BeforeUnloadEvent is unsupported(%s)",
                                 __PRETTY_FUNCTION__);
            e = new Event(executionContext());
        } else if (type->equals("devicemotionevent")) {
            STARFISH_UNSUPPORTED("DeviceMotionEvent is unsupported(%s)",
                                 __PRETTY_FUNCTION__);
            e = new Event(executionContext());
        } else if (type->equals("webglcontextevent")) {
            STARFISH_UNSUPPORTED("WebGLContextEvent is unsupported(%s)",
                                 __PRETTY_FUNCTION__);
            e = new Event(executionContext());
        }
        break;
    case 19:
        if (type->equals("pagetransitionevent")) {
            STARFISH_UNSUPPORTED("PageTransitionEvent is unsupported(%s)",
                                 __PRETTY_FUNCTION__);
            e = new Event(executionContext());
        }
        break;
    case 21:
        if (type->equals("idbversionchangeevent")) {
            STARFISH_UNSUPPORTED("IDBVersionChangeEvent is unsupported(%s)",
                                 __PRETTY_FUNCTION__);
            e = new Event(executionContext());
        }
        break;
    case 22:
        if (type->equals("deviceorientationevent")) {
            STARFISH_UNSUPPORTED("DeviceOrientationEvent is unsupported(%s)",
                                 __PRETTY_FUNCTION__);
            e = new Event(executionContext());
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

    throw new DOMException(executionContext(),
                           DOMException::Code::NOT_SUPPORTED_ERR, nullptr);
}

void Document::notifyCountingOutdated()
{
    browsingContext()->setNeedsFrameTreeBuild();
    STARFISH_ASSERT(frame());
    frame()->asFrameDocument()->setCountingOutdatedFlag();
}

void Document::notifyQuoteOutdated()
{
    browsingContext()->setNeedsFrameTreeBuild();
    STARFISH_ASSERT(frame());
    frame()->asFrameDocument()->setQuoteOutdatedFlag();
}

MediaQueryListMatcher* Document::mediaQueryListMatcher()
{
    if (!m_mediaQueryListMatcher) {
        MediaQueryEvaluator* evaluator = const_cast<MediaQueryEvaluator*>(
            &styleResolver().mediaQueryEvaluator());
        m_mediaQueryListMatcher = new MediaQueryListMatcher(this, evaluator);
    }
    return m_mediaQueryListMatcher;
}

void Document::evalMediaQueryLists()
{
    if (m_mediaQueryListMatcher) {
        m_mediaQueryListMatcher->mediaFeaturesChanged();
    }
}

Event* Document::createSimulatedMouseClickEvent()
{
    // NOTE: we may consider creating an event dispatcher separated
    // if more events are needed.
    auto eventType = starfish()->staticStrings()->m_click.localName();
    MouseData clickData(MouseButtonValue::LeftButton,
                        MouseButtonsValue::LeftButtonDown, 0, 0, 1,
                        timestamp());
    MouseEvent* event =
        new MouseEvent(executionContext(), eventType, clickData);
    event->setBubbles(true);
    event->setCancelable(true);
    event->setView(this->window());
    return event;
}

bool Document::isElementInClickProgress(const Element* element) const
{
    return std::find(m_elementInClickProgressList.begin(),
                     m_elementInClickProgressList.end(), element);
}

void Document::markElementInClickProgress(Element* element)
{
    m_elementInClickProgressList.push_back(element);
}

void Document::unmarkElementInClickProgress(Element* element)
{
    m_elementInClickProgressList.erase(
        std::remove(m_elementInClickProgressList.begin(),
                    m_elementInClickProgressList.end(), element),
        m_elementInClickProgressList.end());
}

std::shared_ptr<NativeGradient> Document::findInNativeGradientCache(
    GradientDrawingInfo* key)
{
    auto iter = m_nativeGradientCache->find(key);
    if (iter != m_nativeGradientCache->end()) {
        auto hash = key->hashValue();
        auto iter2 = std::find_if(m_nativeGradientCacheLRUList.begin(),
                                  m_nativeGradientCacheLRUList.end(),
                                  [hash](const GradientDrawingInfo* key) {
                                      return key->hashValue() == hash;
                                  });

        if (iter2 != m_nativeGradientCacheLRUList.end()) {
            m_nativeGradientCacheLRUList.erase(iter2);
        }
        m_nativeGradientCacheLRUList.push_back(iter->first);
        return iter->second;
    }
    return nullptr;
}

void Document::cacheNativeGradient(GradientDrawingInfo* key,
                                   std::shared_ptr<NativeGradient> value)
{
    STARFISH_ASSERT(key);
    STARFISH_ASSERT(value);
    STARFISH_ASSERT(value->gradientImageDataCached());

    size_t bufferSize = value->gradientImageDataCached()->bufferSize();
    if (pruneNativeGradientCacheIfNeeds(bufferSize)) {
        auto iter = m_nativeGradientCache->find(key);
        if (iter == m_nativeGradientCache->end()) {
            m_nativeGradientCache->insert(std::make_pair(key, value));
        } else {
            m_nativeGradientCacheTotalSize -=
                iter->second->gradientImageDataCached()->bufferSize();
            iter.value() = value;
        }
        m_nativeGradientCacheLRUList.push_back(key);
        m_nativeGradientCacheTotalSize += bufferSize;
    }

    STARFISH_LOG_INFO(
        "NativeGradient cache size : %d KB / %d KB",
        static_cast<int>(m_nativeGradientCacheTotalSize / 1024),
        static_cast<int>(STARFISH_NATIVEGRADIENT_CACHE_SIZE / 1024));
}

bool Document::pruneNativeGradientCacheIfNeeds(size_t reserve)
{
    STARFISH_ASSERT(m_nativeGradientCache->size() ==
                    m_nativeGradientCacheLRUList.size());
    if (reserve > STARFISH_NATIVEGRADIENT_CACHE_SIZE) {
        STARFISH_LOG_INFO("Failed to reserve[%d KB] cache space",
                          static_cast<int>(reserve / 1024));
        return false;
    }
    size_t removedSize = 0;
    if (m_nativeGradientCacheTotalSize + reserve >
        STARFISH_NATIVEGRADIENT_CACHE_SIZE) {
        auto iter = m_nativeGradientCacheLRUList.begin();
        while (iter != m_nativeGradientCacheLRUList.end() &&
               removedSize < reserve) {
            auto iter2 = m_nativeGradientCache->find(*iter);
            if (iter2 != m_nativeGradientCache->end()) {
                removedSize +=
                    iter2->second->gradientImageDataCached()->bufferSize();
                m_nativeGradientCache->erase(iter2);
                iter = m_nativeGradientCacheLRUList.erase(iter);
            }
        }
        m_nativeGradientCacheTotalSize -= removedSize;
    }
    return true;
}

void Document::clearNativeGradientCacheIfNeeds()
{
    if (m_nativeGradientCache) {
        m_nativeGradientCache->clear();
        m_nativeGradientCacheLRUList.clear();
        m_nativeGradientCacheTotalSize = 0;
    }
}

void Document::setReferrer(ResourceURL* referrer)
{
    executionContext()->setReferrer(referrer);
}

void Document::setDocumentURI(ResourceURL* newURL)
{
    executionContext()->setDocumentURI(newURL);
}

String* Document::urlString()
{
    return executionContext()->urlString();
}

uint64_t Document::createdTick()
{
    return executionContext()->createdTick();
}

void Document::addIntersectionObserver(IntersectionObserver* observer)
{
    m_intersectionObservers.emplace_back(observer);
}

void Document::removeIntersectionObserver(IntersectionObserver* observer)
{
    if (m_intersectionObservers.size()) {
        m_intersectionObservers.erase(
            std::remove_if(m_intersectionObservers.begin(),
                           m_intersectionObservers.end(),
                           [observer](const IntersectionObserver* item) {
                               return item == observer;
                           }),
            m_intersectionObservers.end());
    }
}

bool Document::hasIntersectionObserver(IntersectionObserver* observer) const
{
    for (const auto* intersectionObserver : m_intersectionObservers) {
        if (intersectionObserver == observer) {
            return true;
        }
    }

    return false;
}

void Document::addResizeObserver(ResizeObserver* observer)
{
    m_resizeObservers.emplace_back(observer);
}

void Document::removeResizeObserver(ResizeObserver* observer)
{
    if (m_resizeObservers.size()) {
        m_resizeObservers.erase(
            std::remove_if(m_resizeObservers.begin(), m_resizeObservers.end(),
                           [observer](const ResizeObserver* item) {
                               return item == observer;
                           }),
            m_resizeObservers.end());
    }
}

bool Document::hasResizeObserver(ResizeObserver* observer) const
{
    for (const auto* resizeObserver : m_resizeObservers) {
        if (resizeObserver == observer) {
            return true;
        }
    }

    return false;
}

void Document::addMutationObserverTypes(MutationObserverOptionType types)
{
    m_mutationTypes |= types;
}

bool Document::hasMutationObserversOfType(MutationObserverOptionType type) const
{
    return !!(m_mutationTypes & type);
}

bool Document::hasMutationObservers() const
{
    return !!m_mutationTypes;
}

void Document::enqueueMutationObserverMicroTask(MutationObserver* observer)
{
    m_activeMuationObservers.insert(observer);

    if (m_isMutationObserverMicroTaskQueued) {
        return;
    }
    m_isMutationObserverMicroTaskQueued = true;

    enqueueMicrotask(
        scriptBindingInstance(),
        [](void* data) {
            auto* self = static_cast<Document*>(data);
            self->m_isMutationObserverMicroTaskQueued = false;
            GCUnorderedSet<MutationObserver*> notifySet;
            notifySet.swap(self->m_activeMuationObservers);
            for (auto* observer : notifySet) {
                observer->notify();
            }
        },
        this);
}

void Document::updateObservation()
{
    updateIntersectionObservation();
    updateResizeObservation();
}

void Document::updateResizeObservation()
{
    if (!frame()) {
        return;
    }

    double time = timestamp();
    GCVector<ResizeObserver*> observersToNotify;
    for (auto* observer : m_resizeObservers) {
        for (auto* target : observer->targets()) {
            LayoutSize currentSize;
            if (target->frame() && target->frame()->isFrameBox()) {
                currentSize =
                    LayoutSize(target->frame()->asFrameBox()->contentWidth(),
                               target->frame()->asFrameBox()->contentHeight());
            }
            ResizeObserverRegistration* registration =
                target->findResizeObserverRegistration(observer);

            bool isResizeRectChanged =
                registration->previousSize != currentSize;
            if (isResizeRectChanged) {
                DOMRect* targetBoundingClientRect =
                    target->getBoundingClientRect();
                ResizeObserverEntry* entry = new ResizeObserverEntry(
                    executionContext(),
                    new DOMRectReadOnly(executionContext(), 0, 0,
                                        currentSize.width().toFloat(),
                                        currentSize.height().toFloat()),
                    target);
                observer->queueResizeObserverEntry(entry);
            }
            registration->previousSize = currentSize;
        }

        if (observer->hasRecords()) {
            observersToNotify.emplace_back(observer);
        }
    }
    for (auto* observer : observersToNotify) {
        observer->notify();
    }
}

void Document::updateIntersectionObservation()
{
    if (!frame()) {
        return;
    }

    double time = timestamp();
    GCVector<IntersectionObserver*> observersToNotify;
    for (auto* observer : m_intersectionObservers) {
        for (auto* target : observer->targets()) {
            DOMRect* rootBoundingClientRect =
                observer->rootBoundingClientRect();

            IntersectionObserverEntryInit init;
            Unit::Rect rootRect;
            Optional<DOMRectInit> rootBounds;
            if (rootBoundingClientRect) {
                rootRect = Unit::Rect(rootBoundingClientRect->x(),
                                      rootBoundingClientRect->y(),
                                      rootBoundingClientRect->width(),
                                      rootBoundingClientRect->height());
                rootBounds =
                    DOMRectInit({ rootRect.x(), rootRect.y(), rootRect.width(),
                                  rootRect.height() });
                init.setRootBounds(rootBounds);
            }

            int32_t thresholdIndex = 0;
            Unit::Rect targetRect = { 0, 0, 0, 0 };
            Unit::Rect intersectRect = { 0, 0, 0, 0 };
            bool isIntersecting = false;
            double intersectionRatio = 0;
            bool foundThresholdIndex = false;
            if (observer->isValidTarget(target)) {
                DOMRect* targetBoundingClientRect =
                    target->getBoundingClientRect();
                targetRect = Unit::Rect(targetBoundingClientRect->x(),
                                        targetBoundingClientRect->y(),
                                        targetBoundingClientRect->width(),
                                        targetBoundingClientRect->height());

                // TODO: Implement Compute the intersection.
                // (https://w3c.github.io/IntersectionObserver/#compute-the-intersection)
                intersectRect = targetRect;
                if (rootBoundingClientRect) {
                    // TODO: apply rootMargin.
                    intersectRect.edgeInclusiveIntersect(rootRect);
                }

                isIntersecting =
                    intersectRect.width() || intersectRect.height();
                double targetArea = targetRect.width() * targetRect.height();
                double intersectionArea =
                    intersectRect.width() * intersectRect.height();
                if (targetArea) {
                    intersectionRatio = intersectionArea / targetArea;
                } else {
                    intersectionRatio = isIntersecting ? 1 : 0;
                }

                GCAtomicVector<double> thresholds = observer->thresholds();
                size_t thresholdsSize = thresholds.size();
                for (size_t i = 0; i < thresholdsSize; i++) {
                    if (thresholds[i] > intersectionRatio) {
                        thresholdIndex = i;
                        foundThresholdIndex = true;
                        break;
                    }
                }

                if (!foundThresholdIndex && thresholdsSize &&
                    intersectionRatio >= thresholds[thresholdsSize - 1]) {
                    thresholdIndex = thresholdsSize - 1;
                }
            }

            init.setTarget(target);
            init.setTime(time);
            init.setBoundingClientRect({ targetRect.x(), targetRect.y(),
                                         targetRect.width(),
                                         targetRect.height() });
            init.setIntersectionRect({ intersectRect.x(), intersectRect.y(),
                                       intersectRect.width(),
                                       intersectRect.height() });
            init.setIsIntersecting(isIntersecting);
            init.setIntersectionRatio(intersectionRatio);

            IntersectionObserverRegistration* registration =
                target->findIntersectionObserverRegistration(observer);

            // REMOVEME(isIntersectionRectChanged): The entry should be queued
            // properly even without this condition. it's probably a problem
            // related to Compute the intersection or root bound calculation.
            // Releated issue: the energy plugin
            // (https://github.sec.samsung.net/lws/lwe_rel/issues/926),
            // all charts in the Energy plugin must be shown properly without
            // this condition.
            bool isIntersectionRectChanged =
                registration->previousIntersectRect != intersectRect;
            if (isIntersectionRectChanged ||
                registration->previousThresholdIndex == -1 ||
                (thresholdIndex != registration->previousThresholdIndex) ||
                (isIntersecting != registration->previousIsIntersecting)) {
                IntersectionObserverEntry* entry =
                    new IntersectionObserverEntry(executionContext(), init);
                observer->queueIntersectionObserverEntry(entry);
            }

            registration->previousThresholdIndex = thresholdIndex;
            registration->previousIsIntersecting = isIntersecting;
            registration->previousIntersectRect = intersectRect;
        }

        if (observer->hasRecords()) {
            observersToNotify.emplace_back(observer);
        }
    }
    for (auto* observer : observersToNotify) {
        observer->notify();
    }
}

static bool compare(Optional<ElementOrDocument> o, Node* nd)
{
    if (o) {
        if (o.value().isDocumentValue() && o.value().getDocumentValue() == nd) {
            return true;
        } else if (o.value().isElementValue() &&
                   o.value().getElementValue() == nd) {
            return true;
        }
    }
    return false;
}

void Document::finalizeObservation(Node* node)
{
    for (size_t i = 0; i < m_intersectionObservers.size();) {
        auto* ob = m_intersectionObservers[i];
        if (compare(ob->root(), node)) {
            ob->disconnect();
            m_intersectionObservers.erase(i);
            continue;
        } else if (node->isElement()) {
            ob->unobserve(node->asElement());
        }
        i++;
    }

    for (size_t i = 0; i < m_resizeObservers.size();) {
        auto* ob = m_resizeObservers[i];
        if (compare(ob->root(), node)) {
            ob->disconnect();
            m_resizeObservers.erase(i);
            continue;
        } else if (node->isElement()) {
            ob->unobserve(node->asElement());
        }
        i++;
    }
}

ContentSecurityPolicy* Document::contentSecurityPolicy()
{
    return executionContext()->contentSecurityPolicy();
}

// https://html.spec.whatwg.org/multipage/browsers.html#fully-active
bool Document::isFullyActive()
{
    BrowsingContext* bContext = browsingContext();
    if (bContext->isTopLevelBrowsingContext()) {
        if (bContext && (bContext->document() == this)) {
            return true;
        }
    } else {
        BrowsingContext* pBContext = bContext->parentBrowsingContext();
        if (pBContext) {
            if (pBContext->document()) {
                return pBContext->document()->isFullyActive();
            }
        }
    }

    return false;
}

void Document::registerSVGPaintClientElements(const AtomicString& id,
                                              SVGElement* client)
{
    for (auto& pair : m_svgPaintClientElements) {
        if (pair.first == id) {
            for (auto* e : pair.second) {
                if (e == client) {
                    return;
                }
            }
            pair.second.push_back(client);
            return;
        }
    }

    GCVector<SVGElement*> v;
    v.push_back(client);
    m_svgPaintClientElements.push_back(std::make_pair(id, std::move(v)));
}

void Document::notifyNeedsLayoutOrPaintingToSVGPaintClientElements(
    const AtomicString& id, bool needsLayoutAlso)
{
    for (auto& pair : m_svgPaintClientElements) {
        if (pair.first == id) {
            for (auto* e : pair.second) {
                if (needsLayoutAlso) {
                    e->setNeedsLayout();
                }
                e->setNeedsPainting();
            }
            return;
        }
    }
}

void Document::removeSVGPaintClientElement(SVGElement* client)
{
    for (auto& pair : m_svgPaintClientElements) {
        auto iter = pair.second.begin();
        while (iter != pair.second.end()) {
            if (*iter == client) {
                pair.second.erase(iter);
                break;
            }
            iter++;
        }
    }
}

Optional<ResourceURL*> Document::resolveModuleSrcFromImportMap(String* src)
{
    if (src->startsWith("./")) {
        src = src->substring(2, src->length() - 2);
    }
    Optional<ResourceURL*> submatch;
    for (auto d : importMap()) {
        if (d->id->equals(src)) {
            return d->url;
        }
        if (d->id->contains("/") && src->startsWith(d->id)) {
            src = src->replaceAll(d->id, d->url->string());
            submatch = new ResourceURL(src, baseURI());
        }
    }
    return submatch;
}

DEFINE_EVENT_LISTENER(Document, abort);
DEFINE_EVENT_LISTENER(Document, blur);
DEFINE_EVENT_LISTENER(Document, click);
DEFINE_EVENT_LISTENER(Document, change);
DEFINE_EVENT_LISTENER(Document, error);
DEFINE_EVENT_LISTENER(Document, focus);
DEFINE_EVENT_LISTENER(Document, scroll);
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
DEFINE_EVENT_LISTENER(Document, mouseenter);
DEFINE_EVENT_LISTENER(Document, mouseleave);
DEFINE_EVENT_LISTENER(Document, pointerdown);
DEFINE_EVENT_LISTENER(Document, pointerup);
DEFINE_EVENT_LISTENER(Document, pointermove);
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
} // namespace Starfish
