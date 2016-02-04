#ifndef __StarFishHTMLElement__
#define __StarFishHTMLElement__

#include "dom/Element.h"

namespace StarFish {

#define FOR_EACH_HTML_ELEMENT_OP(F) \
        F(Html) \
        F(Head) \
        F(Script) \
        F(Style) \
        F(Meta) \
        F(Body) \
        F(Div) \
        F(Paragraph) \
        F(Image) \
        F(Span) \
        F(BR) \
        F(Audio)

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
        initScriptWrappable(this);
        m_id = String::emptyString;
        m_className = String::emptyString;
        m_onclick = ScriptValueNull;
    }

    /* 4.4 Interface Node */

    virtual String* nodeName()
    {
        return localName()->toUpper();
    }

    virtual void setTextContent(String* val)
    {
        Text* node = new Text(m_document, String::emptyString);
        node->setTextContent(val);
        while(firstChild()) {
            removeChild(firstChild());
        }

        appendChild(node);
        setNeedsStyleRecalc();
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLElement() const
    {
        return true;
    }

    FOR_EACH_HTML_ELEMENT_OP(IS_KIND_ELEMENT);

    virtual void didAttributeChanged(QualifiedName name, String* old, String* value);

    ScriptValue onclick()
    {
        return m_onclick;
    }

    void setOnclick(ScriptValue f)
    {
        m_onclick = f;
    }
protected:
    String* m_id;
    String* m_className;

    ScriptValue m_onclick;
};

}

#endif
