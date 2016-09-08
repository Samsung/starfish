/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#include "util/URL.h"
#include "dom/Node.h"
#include "platform/window/Window.h"
#include "loader/ResourceLoader.h"

namespace StarFish {

class Window;
class Attribute;
class NetworkRequest;
class DocumentBuilder;

#ifdef STARFISH_EXP
class DOMImplementation;
#endif

/* Page Visibility */
enum PageVisibilityState {
    PageVisibilityStateHidden,
    PageVisibilityStateVisible,
    PageVisibilityStatePrerender,
    PageVisibilityStateUnloaded
};

class Document : public Node {
#ifdef STARFISH_EXP
    friend class DOMImplementation;
#endif
    friend class Window;
    friend class ActiveNetworkRequestTracker;
    friend class HTMLMetaElement;
protected:
    Document(Window* window, ScriptBindingInstance* scriptBindingInstance, URL* url, String* charSet);
public:
    enum CompatibilityMode { QuirksMode, LimitedQuirksMode, NoQuirksMode };
    void setCompatibilityMode(CompatibilityMode m)
    {
        m_compatibilityMode = m;
    }
    CompatibilityMode compatibilityMode() const { return m_compatibilityMode; }
    bool inQuirksMode() const { return m_compatibilityMode == QuirksMode; }
    bool inLimitedQuirksMode() const { return m_compatibilityMode == LimitedQuirksMode; }
    bool inNoQuirksMode() const { return m_compatibilityMode == NoQuirksMode; }

    /* 4.2.2. Interface NonElementParentNode */
    Element* getElementById(String* id);

    /* 4.5. Interface Document */
    DocumentType* docType()
    {
        Node* node = getDocTypeChild();
        if (node != nullptr) {
            return node->asDocumentType();
        }
        return nullptr;
    }

    DocumentFragment* createDocumentFragment();
    Element* createElement(QualifiedName localName, bool shouldCheckName);
    Text* createTextNode(String* data);
    Comment* createComment(String* data);
    // Moved to Node as it is common to Document and Element
    // HTMLCollection* getElementsByTagName(String* qualifiedName);
    // HTMLCollection* getElementsByClassName(String* classNames);
    Attr* createAttribute(QualifiedName localName);

#ifdef STARFISH_EXP
    DOMImplementation* domImplementation()
    {
        return m_domImplementation;
    }
#endif

    /* Other methods */
    virtual NodeType nodeType()
    {
        return DOCUMENT_NODE;
    }

    virtual String* nodeName();
    virtual String* localName();
    virtual String* textContent()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return nullptr;
    }

    Element* documentElement();

    virtual bool isDocument() const
    {
        return true;
    }

    virtual Node* clone();

    Window* window()
    {
        return m_window;
    }

    ResourceLoader* resourceLoader()
    {
        return &m_resourceLoader;
    }

    StyleResolver* styleResolver()
    {
        return &m_styleResolver;
    }

    ScriptBindingInstance* scriptBindingInstance()
    {
        return m_scriptBindingInstance;
    }

    HTMLHtmlElement* rootElement();
    HTMLHeadElement* headElement();
    HTMLBodyElement* bodyElement();

    /* Page Visibility */
    bool hidden() const;
    String* visibilityState();
    void setVisibleState(PageVisibilityState visibilityState);

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

    URL* documentURI()
    {
        return m_documentURI;
    }

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

    std::vector<Element*, gc_allocator<Element*>>& elementExecutionStackForAttributeStringEventFunctionObject()
    {
        return m_elementExecutionStackForAttributeStringEventFunctionObject;
    }

    Element* elementFromPoint(float x, float y);
    ImageData* brokenImage();
    String* charset()
    {
        return m_charset;
    }
protected:
    // only used in html document builder
    friend class HTMLResourceClient;
    void setCharset(String* s)
    {
        m_charset = s;
    }
    bool m_inParsing : 1;
    bool m_didLoadBrokenImage : 1;
    CompatibilityMode m_compatibilityMode;
    Window* m_window;
    URL* m_documentURI;
    String* m_charset;
    ResourceLoader m_resourceLoader;
    StyleResolver m_styleResolver;
    DocumentBuilder* m_documentBuilder;
    ImageData* m_brokenImage;
    ScriptBindingInstance* m_scriptBindingInstance;
    PageVisibilityState m_pageVisibilityState;
    size_t m_domVersion;
    std::vector<NetworkRequest*, gc_allocator<NetworkRequest*>> m_activeNetworkRequests;
    ActiveHTMLCollectionList m_namedAccessActiveHTMLCollectionList;
    std::vector<Element*, gc_allocator<Element*>> m_elementExecutionStackForAttributeStringEventFunctionObject;
#ifdef STARFISH_TIZEN
    size_t m_tizenWidgetTransparentBackground;
#endif
#ifdef STARFISH_EXP
private:
    DOMImplementation* m_domImplementation;
#endif
};

void Node::setNeedsStyleRecalc()
{
    if (!m_needsStyleRecalc) {
        m_needsStyleRecalc = true;

        Node* node = parentNode();
        while (node && !node->childNeedsStyleRecalc()) {
            node->setChildNeedsStyleRecalc();
            node = node->parentNode();
        }
    }
    m_document->window()->setNeedsStyleRecalc();
}

void Node::setNeedsLayout()
{
    m_document->window()->setNeedsLayout();
}

void Node::setNeedsPainting()
{
    m_document->window()->setNeedsPainting();
}

void Node::setNeedsComposite()
{
    m_document->window()->setNeedsComposite();
}


}

#endif
