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

#ifndef __StarFishDocument__
#define __StarFishDocument__

#include "dom/Node.h"
#include "loader/ResourceLoader.h"
#include "style/Style.h"

namespace StarFish {

class Attr;
class CDATASection;
class Comment;
class DocumentType;
class DocumentFragment;
class DocumentBuilder;
class Element;
class HTMLBodyElement;
class HTMLHeadElement;
class HTMLHtmlElement;
class ImageData;
class NetworkRequest;
class Location;
class Text;
class URL;
class Window;

#ifdef STARFISH_EXP
class DOMImplementation;
#endif

/* VisibilityState */
enum VisibilityState {
    VisibilityStateHidden,
    VisibilityStateVisible,
    VisibilityStatePrerender,
    VisibilityStateUnloaded
};

class Document : public Node {
#ifdef STARFISH_EXP
    friend class DOMImplementation;
#endif
    friend class Window;
    friend class ActiveNetworkRequestTracker;
    friend class HTMLMetaElement;
    friend class DOMParser;

protected:
    Document(Window* window, ScriptBindingInstance* scriptBindingInstance,
             URL* url, String* charSet, bool isXMLDocument,
             bool doesParticipateInRendering);

public:
    enum CompatibilityMode { QuirksMode, LimitedQuirksMode, NoQuirksMode };
    void setCompatibilityMode(CompatibilityMode m)
    {
        m_compatibilityMode = m;
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

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
    virtual Element* createElement(AtomicString localName,
                                   bool shouldCheckName);
    Element* createElement(String* name);
    Text* createTextNode(String* data);
    CDATASection* createCDATASectionNode(String* data);
    Comment* createComment(String* data);
    // Moved to Node as it is common to Document and Element
    // HTMLCollection* getElementsByTagName(String* qualifiedName);
    // HTMLCollection* getElementsByClassName(String* classNames);

    Attr* createAttribute(QualifiedName localName);
    Attr* createAttribute(String* name);
    QualifiedName createAttributeName(String* name);

#ifdef STARFISH_EXP
    DOMImplementation* implementation()
    {
        return m_implementation;
    }
#endif

    /* Other methods */
    virtual NodeType nodeType() const override
    {
        return DOCUMENT_NODE;
    }

    virtual String* nodeName();
    virtual String* localName();

    Element* documentElement();

    virtual bool isDocument() const override
    {
        return true;
    }

    virtual Node* clone();

    Window* window()
    {
        return m_window;
    }

    ResourceLoader& resourceLoader()
    {
        return m_resourceLoader;
    }

    StyleResolver& styleResolver()
    {
        return m_styleResolver;
    }

    ScriptBindingInstance* scriptBindingInstance()
    {
        return m_scriptBindingInstance;
    }

    HTMLHtmlElement* rootElement();
    HTMLHeadElement* head();
    HTMLBodyElement* body();

    DECLARE_EVENT_LISTENER(click);
    DECLARE_EVENT_LISTENER(mouseover);
    DECLARE_EVENT_LISTENER(focus);
    DECLARE_EVENT_LISTENER(keydown);

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

    void updateDOMVersion()
    {
        m_domVersion++;
    }

    bool inParsing()
    {
        return m_inParsing;
    }

    void setInParsing(bool b)
    {
        m_inParsing = b;
    }

    String* urlString();

    URL* documentURI()
    {
        return m_documentURI;
    }

    void setDocumentURI(URL* newURL)
    {
        m_documentURI = newURL;
    }

    Location* location();

    void open();

    // method for script element
    void resumeDocumentParsing();
    void notifyDomContentLoaded();
    void close();

    DocumentBuilder* documentBuilder()
    {
        return m_documentBuilder;
    }

    virtual void didNodeInserted(Node* parent, Node* newChild);
    virtual void didNodeRemoved(Node* parent, Node* oldChild);

    HTMLCollection* namedAccess(String* name);
    void invalidNamedAccessCacheIfNeeded();

    GCVector<Element*>&
    elementExecutionStackForAttributeStringEventFunctionObject()
    {
        return m_elementExecutionStackForAttributeStringEventFunctionObject;
    }

    Element* elementFromPoint(float x, float y);
    ImageData* brokenImage();
    String* characterSet()
    {
        return m_characterSet;
    }
    String* contentType()
    {
        return String::createASCIIString("text/html");
    }

    bool isXMLDocument()
    {
        return m_isXMLDocument;
    }

    /* Document-level focus APIs */
    Element* activeElement();
    bool hasFocus() const;

protected:
    // only used in html document builder
    friend class HTMLResourceClient;
    void setCharacterSet(String* s)
    {
        m_characterSet = s;
    }
    bool m_inParsing : 1;
    bool m_didLoadBrokenImage : 1;
    bool m_isXMLDocument : 1;
    bool m_doesParticipateInRendering : 1;

    CompatibilityMode m_compatibilityMode;
    Window* m_window;
    URL* m_documentURI;
    String* m_characterSet;
    ResourceLoader m_resourceLoader;
    StyleResolver m_styleResolver;
    DocumentBuilder* m_documentBuilder;
    ImageData* m_brokenImage;
    ScriptBindingInstance* m_scriptBindingInstance;
    VisibilityState m_pageVisibilityState;
    size_t m_domVersion;
    GCVector<NetworkRequest*> m_activeNetworkRequests;
    ActiveHTMLCollectionList m_namedAccessActiveHTMLCollectionList;
    GCVector<Element*>
        m_elementExecutionStackForAttributeStringEventFunctionObject;
#ifdef STARFISH_TIZEN
    size_t m_tizenWidgetTransparentBackground;
#endif
#ifdef STARFISH_EXP
private:
    DOMImplementation* m_implementation;
#endif
};
}

#endif
