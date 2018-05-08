/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishHTMLIFrameElement__
#define __StarFishHTMLIFrameElement__

#include "core/dom/HTMLElement.h"
#include "browser/history/HistoryManager.h"

#define STARFISH_DEFAULT_IFRAME_WIDTH 300
#define STARFISH_DEFAULT_IFRAME_HEIGHT 150

namespace StarFish {

class BrowsingContext;

class HTMLIFrameElement : public HTMLElement {
    friend class BrowsingContext;
    friend class ResourceLoader;

public:
    HTMLIFrameElement(Document* document);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLIFrameElement() const override;

    /* 4.4 Interface Node */
    virtual QualifiedName name() override;

    /* DOM APIs */
    void setSrc(String* src);
    String* src();

    String* width();
    void setWidth(String* width);

    String* height();
    void setHeight(String* height);

    String* scrolling();
    void setScrolling(String* scrolling);

    Document* contentDocument() const;
    Window* contentWindow() const;

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

    void navigate(ResourceURL* url, HistoryManager::Action type,
                  ResourceURL* referrerURL);

    String* nameAttr();
    void setNameAttr(String* name);

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLIFrameElement, m_browsingContext));
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLIFrameElement, m_historyManager));
        HTMLElement::fillGCDescriptor(desc);
    }

    virtual void childBrowsingContextLoaded();

    BrowsingContext* m_browsingContext;
    HistoryManager* m_historyManager;
    void loadSrc();
    void unloadSrc();
};
}

#endif
