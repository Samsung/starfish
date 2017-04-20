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

#ifndef __StarFishHTMLElement__
#define __StarFishHTMLElement__

#include "dom/Element.h"

namespace StarFish {

class HTMLElement : public Element {
public:
    HTMLElement(Document* document)
        : Element(document)
    {
    }

    /* 4.4 Interface Node */

    virtual String* nodeName()
    {
        return localName()->toUpper();
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLElement() const override
    {
        return true;
    }

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved);

    bool hasDirAttribute()
    {
        return m_hasDirAttribute;
    }

    int tabIndex();
    bool supportsFocus();
    LayoutRect offsetRect();
    long offsetWidth()
    {
        return (float)offsetRect().width() + .5f;
    }
    long offsetHeight()
    {
        return (float)offsetRect().height() + .5f;
    }

    String* dir();
    void setDir(String* dir);

    void click();
    void focus();

    DECLARE_EVENT_LISTENER(click);
    DECLARE_EVENT_LISTENER(mouseover);
    DECLARE_EVENT_LISTENER(keydown);
    DECLARE_EVENT_LISTENER(keyup);
    DECLARE_EVENT_LISTENER(focus);
    DECLARE_EVENT_LISTENER(error);
    DECLARE_EVENT_LISTENER(load);
    DECLARE_EVENT_LISTENER(unload);

    // https://drafts.csswg.org/cssom-view/#extensions-to-the-htmlelement-interface
    Element* offsetParent();
};
}

#endif
