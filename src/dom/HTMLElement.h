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

#define FOR_EACH_HTML_ELEMENT_OP_DEFAULT(F) \
    F(Html)                                 \
    F(Head)                                 \
    F(Script)                               \
    F(Style)                                \
    F(Link)                                 \
    F(Meta)                                 \
    F(Body)                                 \
    F(Div)                                  \
    F(Paragraph)                            \
    F(Image)                                \
    F(Span)                                 \
    F(BR)                                   \
    F(Object)                               \
    F(Table)                                \
    F(Caption)                              \
    F(TBody)                                \
    F(TD)                                   \
    F(TFoot)                                \
    F(THead)                                \
    F(TH)                                   \
    F(TR)                                   \
    F(Col)                                  \
    F(ColGroup)                             \
    F(Unknown)

#ifdef STARFISH_ENABLE_MULTIMEDIA
#define FOR_EACH_HTML_ELEMENT_OP_MEDIA(F) \
    F(Media)                              \
    F(Video)                              \
    F(Audio)                              \
    F(Track)                              \
    F(Source)
#else
#define FOR_EACH_HTML_ELEMENT_OP_MEDIA(F)
#endif

#ifdef STARFISH_ENABLE_MULTI_PAGE
#define FOR_EACH_HTML_ELEMENT_OP_MULTI_PAGE(F) F(Anchor)
#else
#define FOR_EACH_HTML_ELEMENT_OP_MULTI_PAGE(F)
#endif

#define FOR_EACH_HTML_ELEMENT_OP(F)     \
    FOR_EACH_HTML_ELEMENT_OP_DEFAULT(F) \
    FOR_EACH_HTML_ELEMENT_OP_MEDIA(F)   \
    FOR_EACH_HTML_ELEMENT_OP_MULTI_PAGE(F)

#define FORWRAD_DECLARE_NAME(kind) class HTML##kind##Element;

#define IS_KIND_ELEMENT(kind)                            \
    virtual bool isHTML##kind##Element() const           \
    {                                                    \
        return false;                                    \
    }                                                    \
    virtual HTML##kind##Element* asHTML##kind##Element() \
    {                                                    \
        STARFISH_ASSERT(isHTML##kind##Element());        \
        return (HTML##kind##Element*)this;               \
    }

FOR_EACH_HTML_ELEMENT_OP(FORWRAD_DECLARE_NAME)

class HTMLElement : public Element {
public:
    HTMLElement(Document* document) : Element(document)
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
    uint32_t offsetWidth()
    {
        return (float)offsetRect().width() + .5f;
    }
    uint32_t offsetHeight()
    {
        return (float)offsetRect().height() + .5f;
    }

protected:
};
}

#endif
