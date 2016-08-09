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

#ifndef __StarFishHTMLElement__
#define __StarFishHTMLElement__

#include "dom/Element.h"

namespace StarFish {

#define __FOR_EACH_HTML_ELEMENT_OP(F) \
        F(Html) \
        F(Head) \
        F(Script) \
        F(Style) \
        F(Link) \
        F(Meta) \
        F(Body) \
        F(Div) \
        F(Paragraph) \
        F(Image) \
        F(Span) \
        F(BR) \
        F(Unknown)

#ifdef STARFISH_ENABLE_MULTIMEDIA
#define FOR_EACH_HTML_ELEMENT_OP(F) \
        __FOR_EACH_HTML_ELEMENT_OP(F) \
        F(Media) \
        F(Video) \
        F(Audio)
#else
#define FOR_EACH_HTML_ELEMENT_OP(F) \
        __FOR_EACH_HTML_ELEMENT_OP(F)
#endif

#define FORWRAD_DECLARE_NAME(kind) class HTML##kind##Element;

#define IS_KIND_ELEMENT(kind) \
    virtual bool isHTML##kind##Element() const { return false; } \
    virtual HTML##kind##Element* asHTML##kind##Element() \
    { \
        STARFISH_ASSERT(isHTML##kind##Element()); \
        return (HTML##kind##Element*)this; \
    }

FOR_EACH_HTML_ELEMENT_OP(FORWRAD_DECLARE_NAME)

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

    virtual bool isHTMLElement() const
    {
        return true;
    }

    FOR_EACH_HTML_ELEMENT_OP(IS_KIND_ELEMENT);

    virtual void didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved);

    bool hasDirAttribute()
    {
        return m_hasDirAttribute;
    }

protected:

};

}

#endif
