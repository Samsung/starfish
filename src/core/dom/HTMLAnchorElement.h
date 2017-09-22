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

#ifndef __StarFishHTMLAnchorElement__
#define __StarFishHTMLAnchorElement__

#include "core/dom/HTMLElement.h"

namespace StarFish {

class HTMLAnchorElement : public HTMLElement {
public:
    HTMLAnchorElement(Document* document)
        : HTMLElement(document)
    {
        m_tabIndexWasSetExplicitly = true;
        m_tabIndex = 0;
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLAnchorElement() const;

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* val, bool attributeCreated,
                                     bool attributeRemoved) override;

    /* 4.4 Interface Node */

    virtual QualifiedName name();

    /* Other methods (not in DOM API) */

    virtual bool handleDefaultEvent(Event* event) override;

    bool supportsFocus() const override;

    String* href();
    void setHref(String* href);

    String* host();
    void setHost(String* host);

    String* protocol();
    void setProtocol(String* protocol);
};
}

#endif
