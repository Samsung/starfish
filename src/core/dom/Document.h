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

#ifndef __StarFishDocument__
#define __StarFishDocument__

#include "core/dom/Node.h"
#include "platform/loader/ResourceLoader.h"
#include "core/util/BloomFilter.h"
#include "core/style/Style.h"
#include "core/style/WebFont.h"
#include "core/dom/parser/PreloadScanner.h"
#include "binding/HTMLScriptElementOrSVGScriptElementUnion.h"

namespace StarFish {

class Attr;
class CDATASection;
class Comment;
class ProcessingInstruction;
class DocumentType;
class DocumentFragment;
class DocumentBuilder;
class Element;
class HTMLBodyElement;
class HTMLHeadElement;
class HTMLHtmlElement;
class MediaQueryListMatcher;
class NativeImageData;
class Location;
class ResourceRequest;
class StyleSheetList;
class Text;
class Range;
class ResourceURL;
class WebOrigin;
class Window;
class BrowsingContext;
class AnimationExecutor;
class DOMImplementation;
class DeferredScriptDownloadClient;
class PreloadScanner;

/* VisibilityState */
enum VisibilityState ENSURE_ENUM_UNSIGNED {
    VisibilityStateHidden,
    VisibilityStateVisible,
    VisibilityStatePrerender,
    VisibilityStateUnloaded
};

enum DocumentReadyState ENSURE_ENUM_UNSIGNED {
    DocumentReadyStateLoading,
    DocumentReadyStateInteractive,
    DocumentReadyStateComplete,
};

// Namespaces
#define HTML_NAMESPACE "http://www.w3.org/1999/xhtml"
#define MathML_NAMESPACE "http://www.w3.org/1998/Math/MathML"
#define SVG_NAMESPACE "http://www.w3.org/2000/svg"
#define XML_NAMESPACE "http://www.w3.org/XML/1998/namespace"
#define XMLNS_NAMESPACE "http://www.w3.org/2000/xmlns/"

typedef HTMLScriptElementOrSVGScriptElement HTMLOrSVGScriptElement;

class Document : public Node {
    friend class DOMImplementation;
    friend class Window;
    friend class ActiveResourceRequestTracker;
    friend class HTMLMetaElement;
    friend class DOMParser;
    friend class ResourceLoader;
    friend class BrowsingContext;
    friend class FontSelector;
    friend class DeferredScriptDownloadClient;
    friend class HTMLResourceClient;

protected:
    Document(Window* window, ScriptBindingInstance* scriptBindingInstance,
             ResourceURL* url, String* charSet,
             bool doesParticipateInRendering);

public:
    enum CompatibilityMode ENSURE_ENUM_UNSIGNED {
        QuirksMode,
        LimitedQuirksMode,
        NoQuirksMode
    };
    void setCompatibilityMode(CompatibilityMode m)
    {
        m_compatibilityMode = m;
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isDocument() const override;

    CompatibilityMode compatibilityMode() const
    {
        return m_compatibilityMode;
    }
    bool inQuirksMode() const
    {
        return m_compatibilityMode == QuirksMode;
    }
    bool inLimitedQuirksMode() const
    {
        return m_compatibilityMode == LimitedQuirksMode;
    }
    bool inNoQuirksMode() const
    {
        return m_compatibilityMode == NoQuirksMode;
    }
    bool doesParticipateInRendering()
    {
        return m_doesParticipateInRendering;
    }

    String* compatMode()
    {
        if (inNoQuirksMode() || inLimitedQuirksMode()) {
            return String::createASCIIString("CSS1Compat");
        } else {
            return String::createASCIIString("BackCompat");
        }
    }

    /* 4.2.2. Interface NonElementParentNode */
    Element* getElementById(String* id);

    /* 4.5. Interface Document */
    DocumentType* doctype()
    {
        Node* node = getDoctypeChild();
        if (node != nullptr) {
            return node->asDocumentType();
        }
        return nullptr;
    }

    DocumentFragment* createDocumentFragment();
    Element* createElement(String* name);
    Element* createElementNS(Nullable<String*> namespaceString,
                             String* qualifiedName);
    Text* createTextNode(String* data);
    CDATASection* createCDATASection(String* data);
    Comment* createComment(String* data);
    ProcessingInstruction* createProcessingInstruction(String* target,
                                                       String* data);

    // Moved to Node as it is common to Document and Element
    // HTMLCollection* getElementsByTagName(String* qualifiedName);
    // HTMLCollection* getElementsByClassName(String* classNames);
    NodeList* getElementsByName(String* elementName);

    HTMLCollection* images();
    HTMLCollection* links();
    HTMLCollection* forms();
    HTMLCollection* scripts();
    HTMLCollection* anchors();

// TODO : Return empty string on getting and do nothing on setting when body is
// HTMLFrameSetElement
#define REFLECT_ATTR_GETTER_FROM_BODY(NAME, ATTR)                       \
    String* NAME()                                                      \
    {                                                                   \
        auto b = body();                                                \
        if (b != nullptr) {                                             \
            auto nullable =                                             \
                b->getAttribute(starFish()->staticStrings()->m_##ATTR); \
            if (nullable.hasValue()) {                                  \
                return nullable.getValue();                             \
            }                                                           \
        }                                                               \
        return String::emptyString;                                     \
    }

#define REFLECT_ATTR_SETTER_TO_BODY(NAME, ATTR)                            \
    void set##NAME(String* value)                                          \
    {                                                                      \
        auto b = body();                                                   \
        if (b != nullptr) {                                                \
            b->setAttribute(starFish()->staticStrings()->m_##ATTR, value); \
        }                                                                  \
    }

    REFLECT_ATTR_GETTER_FROM_BODY(fgColor, text)
    REFLECT_ATTR_SETTER_TO_BODY(FgColor, text)

    REFLECT_ATTR_GETTER_FROM_BODY(linkColor, link)
    REFLECT_ATTR_SETTER_TO_BODY(LinkColor, link)

    REFLECT_ATTR_GETTER_FROM_BODY(vlinkColor, vlink)
    REFLECT_ATTR_SETTER_TO_BODY(VlinkColor, vlink)

    REFLECT_ATTR_GETTER_FROM_BODY(alinkColor, alink)
    REFLECT_ATTR_SETTER_TO_BODY(AlinkColor, alink)

    REFLECT_ATTR_GETTER_FROM_BODY(bgColor, bgcolor)
    REFLECT_ATTR_SETTER_TO_BODY(BgColor, bgcolor)

#undef REFLECT_ATTR_GETTER_FROM_BODY
#undef REFLECT_ATTR_SETTER_TO_BODY

    Attr* createAttribute(QualifiedName localName);
    Attr* createAttribute(String* name);
    Attr* createAttributeNS(Nullable<String*> ns, String* name);
    QualifiedName createAttributeName(String* name);
    QualifiedName createAttributeNameNS(Nullable<String*> ns, String* name);

    Range* createRange();

    DOMImplementation* implementation();

    NodeIterator* createNodeIterator(Node* root, unsigned whatToShow,
                                     ScriptValue filter);
    TreeWalker* createTreeWalker(Node* root, unsigned whatToShow,
                                 ScriptValue filter);

    /* Other methods */
    virtual NodeType nodeType() const override
    {
        return DOCUMENT_NODE;
    }

    virtual String* nodeName() override;
    virtual String* localName() override;

    Element* documentElement();

    virtual Node* clone() override
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    Window* window() const
    {
        return m_window;
    }

    BrowsingContext* browsingContext() const;

    ResourceLoader& resourceLoader()
    {
        return *m_resourceLoader;
    }

    StyleResolver& styleResolver()
    {
        return *m_styleResolver;
    }

    PreloadScanner* preloadScanner()
    {
        return m_preloadScanner;
    }

    ScriptBindingInstance* scriptBindingInstance() override;

    HTMLHtmlElement* rootElement();
    HTMLHeadElement* head();
    HTMLElement* body();
    HTMLElement* html();
    void setBody(HTMLElement* element);

    // https://html.spec.whatwg.org/multipage/dom.html#document.title
    String* title();
    void setTitle(String* title);
    // https://html.spec.whatwg.org/multipage/dom.html#dom-document-dir
    String* dir();
    void setDir(String* dir);

    /* Page Visibility */
    bool hidden() const;
    String* visibilityState()
    {
        String* str = String::emptyString;
        switch (m_pageVisibilityState) {
        case VisibilityState::VisibilityStateHidden:
            str = String::createASCIIString("hidden");
            break;
        case VisibilityState::VisibilityStatePrerender:
            str = String::createASCIIString("prerender");
            break;
        case VisibilityState::VisibilityStateUnloaded:
            str = String::createASCIIString("unloaded");
            break;
        case VisibilityState::VisibilityStateVisible:
            str = String::createASCIIString("visible");
            break;
        }

        return str;
    }

    void setVisibilityState(VisibilityState visibilityState);

    String* readyState()
    {
        String* str = String::emptyString;
        switch (m_readyState) {
        case DocumentReadyStateLoading:
            str = String::createASCIIString("loading");
            break;
        case DocumentReadyStateInteractive:
            str = String::createASCIIString("interactive");
            break;
        case DocumentReadyStateComplete:
            str = String::createASCIIString("complete");
            break;
        }

        return str;
    }

    void setReadyState(DocumentReadyState state);

    Element* scrollingElement();

    void updateDOMVersion();
    size_t domVersion()
    {
        return m_domVersion;
    }

    bool inParsing()
    {
        return m_inParsing;
    }

    void setInParsing(bool b)
    {
        m_inParsing = b;
    }

    String* domain();
    void setDomain(String* domain);

    String* urlString();

    ResourceURL* documentURI() const
    {
        return m_documentURI;
    }
    void setDocumentURI(ResourceURL* newURL)
    {
        m_documentURI = newURL;
    }

    Document* parentDocument() const;

    ResourceURL* fallbackBaseURL() const;
    ResourceURL* baseURL() const;
    void setBaseURL(ResourceURL* newURL);
    void updateBaseURL();
    void processBaseElement();

    String* origin();

    WebOrigin* webOrigin()
    {
        return m_webOrigin;
    }

    void setWebOrigin(WebOrigin* webOrigin)
    {
        m_webOrigin = webOrigin;
    }

    Location* location();
    String* referrer();

    String* cookie();
    void setCookie(String* cookie);

    void init(ResourceURL* referrerURL);
    void dispose();

    Document* open(String* type, String* replace);
    Window* open(String* url, String* name, String* features);
    bool openFunctionExplicitCalled()
    {
        return m_openFunctionExplicitCalled;
    }
    void close();
    void unload();
    void write(const GCVector<String*>& str);
    void writeln(const GCVector<String*>& str);

    // method for script element
    void resumeDocumentParsing();
    void endDocumentParsing();
    void notifyDomContentLoaded();

    DocumentBuilder* documentBuilder()
    {
        return m_documentBuilder;
    }

    virtual void didNodeInserted(Node* parent, Node* newChild) override;
    virtual void didNodeRemoved(Node* parent, Node* oldChild) override;

    ScriptWrappable* defaultNamedGetter(String* name);
    HTMLCollection* namedAccess(String* name);
    void invalidNamedAccessCacheIfNeeded(String* name, bool isNameAppeared,
                                         bool isNameDisappared);
    void invalidFocusRingCacheIfNeeded();

    const GCAtomicVector<Element*>& focusRing();

    Element* elementFromPoint(float x, float y);

    StyleSheetList* styleSheets();

    const GCVector<FontResource*>& loadedWebFontList()
    {
        return m_loadedWebFontList;
    }

    NativeImageData* brokenImage();
    AnimationExecutor* animationExecutor()
    {
        return m_animationExecutor;
    }

    FontSelector* fontSelector()
    {
        return m_fontSelector;
    }

#if defined(PORT_CANVAS_BACKEND_EFL)
    FontSelector* fontSelectorGeneric()
    {
        return m_fontSelectorGeneric;
    }
#endif

    String* characterSet()
    {
        return m_characterSet;
    }

    String* contentType()
    {
        return m_contentType;
    }

    void setContentType(String* c)
    {
        m_contentType = c;
    }
#ifdef STARFISH_TIZEN
    size_t tizenWidgetTransparentBackground()
    {
        return m_tizenWidgetTransparentBackground;
    }
#endif

    Event* createEvent(String* type);

    /* Document-level focus APIs */
    Element* activeElement();
    bool hasFocus() const;

    String* designMode();
    void setDesignMode(String* value);

    bool inDesignMode()
    {
        return m_designMode;
    }

    QualifiedName validateAndExtractQualifiedName(Nullable<String*> ns,
                                                  String* qualifiedName);

    Nullable<HTMLOrSVGScriptElement> currentScript();
    void appendCurrentScript(HTMLScriptElement* element)
    {
        m_currentScripts.push_back(element);
    }
    void appendCurrentScript(SVGScriptElement* element)
    {
        m_currentScripts.push_back(element);
    }
    void popCurrentScript()
    {
        if (m_currentScripts.size() > 0) {
            m_currentScripts.pop_back();
        }
    }

    void appendRange(Range* range)
    {
        m_ranges.push_back(range);
    }

    void removeRange(Range* range)
    {
        size_t idx;
        for (idx = 0; idx < m_ranges.size(); idx++) {
            if (m_ranges[idx] == range) {
                break;
            }
        }
        STARFISH_ASSERT(idx < m_ranges.size());
        m_ranges.erase(m_ranges.begin() + idx);
    }

    void attachNodeIterator(NodeIterator* ni);
    void willNodeBeRemoved(Node* parent, Node* oldChild);

    bool onLoadFired() const
    {
        return m_onLoadFired;
    }

    String* contentLanguage()
    {
        return m_contentLanguage;
    }

    void setContentLanguage(String* value);

    void loadBuiltinPolyfill(String* localpath);

    void notifyCountingOutdated();

    void notifyQuoteOutdated();

    MediaQueryListMatcher* mediaQueryListMatcher();
    void evalMediaQueryLists();

#define VIRTUAL
#define OVERRIDE
    // https://html.spec.whatwg.org/multipage/webappapis.html#globaleventhandlers
    DECLARE_EVENT_LISTENER(abort);
    // DECLARE_EVENT_LISTENER(auxclick);
    DECLARE_EVENT_LISTENER(blur);
    // DECLARE_EVENT_LISTENER(cancel);
    DECLARE_EVENT_LISTENER(change);
    DECLARE_EVENT_LISTENER(click);
    // DECLARE_EVENT_LISTENER(close);
    // DECLARE_EVENT_LISTENER(contextmenu);
    // DECLARE_EVENT_LISTENER(cuechange);
    // DECLARE_EVENT_LISTENER(dblclick);
    // DECLARE_EVENT_LISTENER(drag);
    // DECLARE_EVENT_LISTENER(dragend);
    // DECLARE_EVENT_LISTENER(dragenter);
    // DECLARE_EVENT_LISTENER(dragexit);
    // DECLARE_EVENT_LISTENER(dragleave);
    // DECLARE_EVENT_LISTENER(dragover);
    // DECLARE_EVENT_LISTENER(dragstart);
    // DECLARE_EVENT_LISTENER(drop);
    DECLARE_EVENT_LISTENER(error);
    DECLARE_EVENT_LISTENER(focus);
    DECLARE_EVENT_LISTENER(input);
    DECLARE_EVENT_LISTENER(invalid);
    DECLARE_EVENT_LISTENER(keydown);
    DECLARE_EVENT_LISTENER(keypress);
    DECLARE_EVENT_LISTENER(keyup);
    DECLARE_EVENT_LISTENER(load);
    // DECLARE_EVENT_LISTENER(loadend);
    DECLARE_EVENT_LISTENER(loadstart);
    DECLARE_EVENT_LISTENER(mousedown);
    // DECLARE_EVENT_LISTENER(mouseenter);
    // DECLARE_EVENT_LISTENER(mouseleave);
    DECLARE_EVENT_LISTENER(mousemove);
    DECLARE_EVENT_LISTENER(mouseout);
    DECLARE_EVENT_LISTENER(mouseover);
    DECLARE_EVENT_LISTENER(mouseup);
    // DECLARE_EVENT_LISTENER(wheel);
    DECLARE_EVENT_LISTENER(progress);
    // DECLARE_EVENT_LISTENER(reset);
    DECLARE_EVENT_LISTENER(resize);
    // DECLARE_EVENT_LISTENER(scroll);
    // DECLARE_EVENT_LISTENER(select);
    // DECLARE_EVENT_LISTENER(show);
    DECLARE_EVENT_LISTENER(submit);
    // DECLARE_EVENT_LISTENER(toggle);
    DECLARE_EVENT_LISTENER(readystatechange);
#ifdef STARFISH_ENABLE_MULTIMEDIA
    DECLARE_EVENT_LISTENER(suspend);
    DECLARE_EVENT_LISTENER(emptied);
    DECLARE_EVENT_LISTENER(stalled);
    DECLARE_EVENT_LISTENER(loadedmetadata);
    DECLARE_EVENT_LISTENER(loadeddata);
    DECLARE_EVENT_LISTENER(canplay);
    DECLARE_EVENT_LISTENER(canplaythrough);
    DECLARE_EVENT_LISTENER(playing);
    DECLARE_EVENT_LISTENER(waiting);
    DECLARE_EVENT_LISTENER(seeking);
    DECLARE_EVENT_LISTENER(seeked);
    DECLARE_EVENT_LISTENER(ended);
    DECLARE_EVENT_LISTENER(durationchange);
    DECLARE_EVENT_LISTENER(timeupdate);
    DECLARE_EVENT_LISTENER(play);
    DECLARE_EVENT_LISTENER(pause);
    DECLARE_EVENT_LISTENER(ratechange);
    DECLARE_EVENT_LISTENER(volumechange);
#endif
#undef VIRTUAL
#undef OVERRIDE

    // Must do nothing.
    void clear()
    {
    }
    void captureEvents()
    {
    }
    void releaseEvents()
    {
    }

protected:
    Element* nextBaseElement(Node* node, Node* root);

    static inline void fillGCDescriptor(GC_word* desc)
    {
        Node::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_window));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_documentURI));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_baseURL));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_baseElementURL));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_baseTarget));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_referrer));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_webOrigin));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_characterSet));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_contentType));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_resourceLoader));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_fontSelector));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_preloadScanner));
#if defined(PORT_CANVAS_BACKEND_EFL)
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_fontSelectorGeneric));
#endif
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_webFontList));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_loadedWebFontList));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_styleResolver));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_documentBuilder));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_styleSheetList));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_brokenImage));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_animationExecutor));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_scriptBindingInstance));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_activeResourceRequests));
        GC_set_bit(desc, GC_WORD_OFFSET(Document,
                                        m_namedAccessActiveHTMLCollectionList));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_implementation));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_currentScripts));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_focusRingCache));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_ranges));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_nodeIterators));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_contentLanguage));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_mediaQueryListMatcher));
        GC_set_bit(desc, GC_WORD_OFFSET(Document, m_deferredScriptElements));
    }

    // only used in html document builder
    void setCharacterSet(String* s)
    {
        m_characterSet = s;
    }
    bool m_inParsing : 1;
    bool m_didLoadBrokenImage : 1;
    bool m_isXMLDocument : 1;
    bool m_doesParticipateInRendering : 1;
    bool m_designMode : 1;

    CompatibilityMode m_compatibilityMode : 2;
    VisibilityState m_pageVisibilityState : 2;
    DocumentReadyState m_readyState : 2;

    bool m_throwOnDynamicMarkupInsertion : 1;
    bool m_ignoreOpensDuringUnloadCounter : 1;
    bool m_salvageable : 1;
    bool m_openFunctionExplicitCalled : 1;
    bool m_domContentLoadedFired : 1;
    bool m_onLoadFired : 1;
    bool m_isFocusRingCacheValid : 1;

    Window* m_window;
    ResourceURL* m_documentURI;
    ResourceURL* m_baseURL;
    ResourceURL* m_baseElementURL;
    String* m_baseTarget;
    ResourceURL* m_referrer;
    WebOrigin* m_webOrigin;
    String* m_characterSet;
    String* m_contentType;
    ResourceLoader* m_resourceLoader;
    FontSelector* m_fontSelector;
#if defined(PORT_CANVAS_BACKEND_EFL)
    FontSelector* m_fontSelectorGeneric;
#endif
    GCVector<WebFont> m_webFontList;
    GCVector<FontResource*> m_loadedWebFontList;
    PreloadScanner* m_preloadScanner;
    StyleResolver* m_styleResolver;
    DocumentBuilder* m_documentBuilder;
    StyleSheetList* m_styleSheetList;
    NativeImageData* m_brokenImage;
    AnimationExecutor* m_animationExecutor;
    ScriptBindingInstance* m_scriptBindingInstance;
    size_t m_domVersion;
    GCVector<ResourceRequest*> m_activeResourceRequests;
    ActiveHTMLCollectionList m_namedAccessActiveHTMLCollectionList;
    DOMImplementation* m_implementation;
    GCVector<Element*> m_currentScripts;
    GCAtomicVector<Element*>
        m_focusRingCache; // using atomic vector is not accident
    GCVector<Range*> m_ranges;
    GCVector<NodeIterator*> m_nodeIterators;
    // each element has strong reference by DOM tree already
    size_t m_pendingDocumentParsingIdlerHandle;
    String* m_contentLanguage;
    MediaQueryListMatcher* m_mediaQueryListMatcher;
    GCVector<std::pair<HTMLScriptElement*, DeferredScriptDownloadClient*>>
        m_deferredScriptElements;
    BloomFilter<12> m_nameIdFilter;
#ifdef STARFISH_TIZEN
    size_t m_tizenWidgetTransparentBackground;
#endif
};
}

#endif
