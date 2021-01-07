/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishHTMLIFrameElement__
#define __StarfishHTMLIFrameElement__

#include "core/dom/HTMLElement.h"
#include "browser/history/HistoryManager.h"
#include "binding/WindowProxy.h"

#define STARFISH_DEFAULT_IFRAME_WIDTH 300
#define STARFISH_DEFAULT_IFRAME_HEIGHT 150

namespace Starfish {

class BrowsingContext;

enum class CustomHTMLIFrameElementType {
    NotCustomHTMLIFrameElement,
    SVG,
};

class HTMLIFrameElement : public HTMLElement {
    friend class BrowsingContext;
    friend class ResourceLoader;
    friend class Window;

public:
    HTMLIFrameElement(Document* document, const QualifiedName& qname);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLIFrameElement() const override;

    /* DOM APIs */
    void setSrc(String* src);
    String* src();

    void setSrcdoc(String* srcDoc);
    String* srcdoc();

    String* width();
    void setWidth(String* width);

    String* height();
    void setHeight(String* height);

    String* scrolling();
    void setScrolling(String* scrolling);

    Document* contentDocument();
    WindowProxy* contentWindow();

    String* referrerPolicy();
    void setReferrerPolicy(String* policy);

    /* Other methods (not in DOM API) */
    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;
    virtual void didNodeInsertedToDocumentTree() override;
    virtual void didNodeRemovedFromDocumentTree() override;
    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues) override;

    uint32_t frameWidth();
    uint32_t frameHeight();

    BrowsingContext* browsingContext()
    {
        return m_browsingContext;
    }

    void navigate(ResourceURL* url, HistoryManagerAction type,
                  ReferrerURL* referrerURL,
                  CustomHTMLIFrameElementType elementType =
                      CustomHTMLIFrameElementType::NotCustomHTMLIFrameElement);

    String* nameAttr();
    void setNameAttr(String* name);
    void markContentDocumentDisabled()
    {
        m_isContentDocumentDisabled = true;
    }
    void unmarkContentDocumentDisabled()
    {
        m_isContentDocumentDisabled = false;
    }

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLIFrameElement, m_browsingContext));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(HTMLIFrameElement, m_contentWindowProxy));
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLIFrameElement, m_historyManager));
        HTMLElement::fillGCDescriptor(desc);
    }

    virtual void childBrowsingContextLoaded();

    BrowsingContext* m_browsingContext;
    WindowProxy* m_contentWindowProxy;
    HistoryManager* m_historyManager;
    bool m_isContentDocumentDisabled;

    void loadSrc();
    void unloadSrc();
    void loadSrcDoc();
};
} // namespace Starfish

#endif
