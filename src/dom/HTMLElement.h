#ifndef __StarFishHTMLElement__
#define __StarFishHTMLElement__

#include "dom/Element.h"

namespace StarFish {

#ifdef STARFISH_ENABLE_AUDIO
#define FOR_EACH_HTML_ELEMENT_OP(F) \
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
        F(Unknown) \
        F(Audio)
#else
#define FOR_EACH_HTML_ELEMENT_OP(F) \
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
        F(Unknown) \
        F(BR)
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
