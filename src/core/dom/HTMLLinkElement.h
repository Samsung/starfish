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
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLLinkElement() const override;

    /* 4.4 Interface Node */
    virtual QualifiedName name();

    /* Other methods (not in DOM API) */

    String* href();
    void setHref(String* href);

    String* media();
    void setMedia(String* media);

    String* rel();
    void setRel(String* rel);

    String* type();
    void setType(String* type);

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
};
}

#endif
