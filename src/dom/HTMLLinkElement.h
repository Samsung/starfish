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

#ifndef __StarFishHTMLLinkElement__
#define __StarFishHTMLLinkElement__

#include "dom/HTMLElement.h"
#include "loader/TextResource.h"

namespace StarFish {

class HTMLLinkElement : public HTMLElement {
    friend class StyleSheetDownloadClient;
public:
    HTMLLinkElement(Document* document)
        : HTMLElement(document)
        , m_generatedSheet(nullptr)
        , m_styleSheetTextResource(nullptr)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */

    virtual String* localName()
    {
        return document()->window()->starFish()->staticStrings()->m_linkTagName.localName();
    }

    virtual QualifiedName name()
    {
        return document()->window()->starFish()->staticStrings()->m_linkTagName;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLLinkElement() const
    {
        return true;
    }

    URL href();

    virtual void didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved);
    virtual void didNodeInsertedToDocumenTree();
    virtual void didNodeRemovedFromDocumenTree();
    void checkLoadStyleSheet();
    void loadStyleSheet();
    void unloadStyleSheetIfExists();
    CSSStyleSheet* generatedSheet()
    {
        return m_generatedSheet;
    }
protected:
    CSSStyleSheet* m_generatedSheet;
    TextResource* m_styleSheetTextResource;
};

}

#endif
