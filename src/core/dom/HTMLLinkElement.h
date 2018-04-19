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

#ifndef __StarFishHTMLLinkElement__
#define __StarFishHTMLLinkElement__

#include "core/dom/HTMLElement.h"

namespace StarFish {

class CSSStyleSheet;
class TextResource;
class URL;

class HTMLLinkElement : public HTMLElement {
    friend class StyleSheetDownloadClient;

public:
    HTMLLinkElement(Document* document)
        : HTMLElement(document)
        , m_generatedSheet(nullptr)
        , m_styleSheetTextResource(nullptr)
        , m_relList(nullptr)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLLinkElement() const override;

    String* href();
    void setHref(String* href);

    String* media();
    void setMedia(String* media);

    String* rel();
    void setRel(String* rel);

    DOMTokenList* relList();

    String* type();
    void setType(String* type);

    /* 4.4 Interface Node */
    virtual QualifiedName name();

    /* Other methods (not in DOM API) */
    StyleSheet* sheet();

    ResourceURL* url();

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved);
    virtual void didNodeInsertedToDocumentTree();
    virtual void didNodeRemovedFromDocumentTree();
    void checkLoadStyleSheet();
    void loadStyleSheet();
    void unloadStyleSheetIfExists();
    CSSStyleSheet* generatedSheet()
    {
        return m_generatedSheet;
    }

protected:
    void willStyleSheetLoad();
    void didStyleSheetLoadComplete();
    CSSStyleSheet* m_generatedSheet;
    TextResource* m_styleSheetTextResource;

private:
    DOMTokenList* m_relList;
};
}

#endif
