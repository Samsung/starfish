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

#define STARFISH_DEFAULT_IFRAME_WIDTH 300
#define STARFISH_DEFAULT_IFRAME_HEIGHT 150

namespace StarFish {

class BrowsingContext;

class HTMLIFrameElement : public HTMLElement {
public:
    HTMLIFrameElement(Document* document)
        : HTMLElement(document)
        , m_browsingContenxt(nullptr)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLIFrameElement() const override;

    /* 4.4 Interface Node */

    virtual String* localName();
    virtual QualifiedName name();

    void setSrc(String* src);
    String* src();

    String* width();
    void setWidth(String* width);

    String* height();
    void setHeight(String* height);

    /* Other methods (not in DOM API) */

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved);
    virtual void didNodeAdopted();

private:
    BrowsingContext* m_browsingContenxt;
    void loadSrc();
    void unloadSrc();
};
}

#endif
