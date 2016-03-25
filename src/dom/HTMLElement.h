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
        F(Meta) \
        F(Body) \
        F(Div) \
        F(Paragraph) \
        F(Image) \
        F(Span) \
        F(BR) \
        F(Audio)
#else
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
        initScriptWrappable(this);
        m_id = String::emptyString;
        m_className = String::emptyString;
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

    virtual void didAttributeChanged(QualifiedName name, String* old, String* value);

    ScriptValue onclick()
    {
        auto eventType = document()->window()->starFish()->staticStrings()->m_click;
        EventListener* l = getAttributeEventListener(eventType);
        if (!l)
            return ScriptValueNull;
        return l->scriptValue();
    }

    void setOnclick(ScriptValue f)
    {
        auto eventType = document()->window()->starFish()->staticStrings()->m_click;
        EventListener* l = new EventListener(f, true);
        setAttributeEventListener(eventType, l);
    }

    void clearOnClick()
    {
        auto eventType = document()->window()->starFish()->staticStrings()->m_click;
        clearAttributeEventListener(eventType);
    }

    ScriptValue onload()
    {
        auto eventType = document()->window()->starFish()->staticStrings()->m_load;
        EventListener* l = getAttributeEventListener(eventType);
        if (!l)
            return ScriptValueNull;
        return l->scriptValue();
    }

    void setOnload(ScriptValue f)
    {
        auto eventType = document()->window()->starFish()->staticStrings()->m_load;
        EventListener* l = new EventListener(f, true);
        setAttributeEventListener(eventType, l);
    }

    void clearOnload()
    {
        auto eventType = document()->window()->starFish()->staticStrings()->m_load;
        clearAttributeEventListener(eventType);
    }

    ScriptValue onunload()
    {
        auto eventType = document()->window()->starFish()->staticStrings()->m_onunload;
        EventListener* l = getAttributeEventListener(eventType);
        if (!l)
            return ScriptValueNull;
        return l->scriptValue();
    }

    void setOnunload(ScriptValue f)
    {
        auto eventType = document()->window()->starFish()->staticStrings()->m_onunload;
        EventListener* l = new EventListener(f, true);
        setAttributeEventListener(eventType, l);
    }

    void clearOnunload()
    {
        auto eventType = document()->window()->starFish()->staticStrings()->m_onunload;
        clearAttributeEventListener(eventType);
    }

protected:
};

}

#endif
