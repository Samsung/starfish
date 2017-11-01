/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
    HTMLIFrameElement(Document* document)
        : HTMLElement(document)
        , m_browsingContext(nullptr)
        , m_historyManager(nullptr)
    {
        m_tabIndexWasSetExplicitly = true;
        m_tabIndex = 0;
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLIFrameElement() const override;

    /* 4.4 Interface Node */
    virtual QualifiedName name();

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

    /* Other methods (not in DOM API) */
    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved);
    virtual void didNodeInsertedToDocumentTree();
    virtual void didNodeRemovedFromDocumentTree();
    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues);

    uint32_t frameWidth();
    uint32_t frameHeight();

    BrowsingContext* browsingContext()
    {
        return m_browsingContext;
    }

    void navigate(ResourceURL* url, HistoryManager::Action type,
                  ResourceURL* referrerURL);

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
