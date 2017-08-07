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

#ifndef __StarFishHTMLFontElement__
#define __StarFishHTMLFontElement__

#include "core/dom/HTMLElement.h"

namespace StarFish {

class HTMLFontElement : public HTMLElement {
public:
    HTMLFontElement(Document* document)
        : HTMLElement(document)
        , m_hasColorAttribute(false)
        , m_hasSizeAttribute(false)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLFontElement() const override;
    virtual QualifiedName name();

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved);

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues);

    /* DOM APIs */
    String* color();
    void setColor(String* color);

    String* size();
    void setSize(String* size);

private:
    bool m_hasColorAttribute;
    bool m_hasSizeAttribute;
};
}

#endif
