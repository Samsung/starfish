/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishHTMLBaseElement__
#define __StarFishHTMLBaseElement__

#include "core/dom/HTMLElement.h"

namespace StarFish {

class HTMLBaseElement : public HTMLElement {
public:
    HTMLBaseElement(Document* document)
        : HTMLElement(document)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLBaseElement() const override;

    /* 4.4 Interface Node */
    virtual QualifiedName name();

    String* href() const;
    void setHref(String* href);

    String* target() const;
    void setTarget(String* target);

    /* Other methods (not in DOM API) */
    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved);
};
}

#endif
