/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#ifndef __StarFishHTMLElement__
#define __StarFishHTMLElement__

#include "core/dom/Element.h"

namespace StarFish {

class HTMLElement : public Element {
public:
    HTMLElement(Document* document)
        : Element(document)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLElement() const override;

    /* Other methods (not in DOM API) */

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved);

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues);

    int tabIndex() const override;

    LayoutRect offsetRect();
    long offsetWidth()
    {
        return (float)offsetRect().width() + .5f;
    }
    long offsetHeight()
    {
        return (float)offsetRect().height() + .5f;
    }

    long offsetTop();
    long offsetLeft();

    // https://html.spec.whatwg.org/multipage/dom.html#the-innertext-idl-attribute
    String* innerText();
    void setInnerText(String* text);

    String* title();
    void setTitle(String* title);

    String* lang();
    void setLang(String* lang);

    String* dir();
    void setDir(String* dir);

    void click();

    String* contentEditable();
    void setContentEditable(const String* value);
    bool isContentEditable();

    virtual bool disabled()
    {
        return false;
    }

#define VIRTUAL
#define OVERRIDE
    // https://html.spec.whatwg.org/multipage/webappapis.html#globaleventhandlers
    // https://html.spec.whatwg.org/multipage/webappapis.html#globaleventhandlers
    DECLARE_EVENT_LISTENER(abort);
    // DECLARE_EVENT_LISTENER(auxclick);
    // DECLARE_EVENT_LISTENER(cancel);
    DECLARE_EVENT_LISTENER(change);
    DECLARE_EVENT_LISTENER(click);
    // DECLARE_EVENT_LISTENER(close);
    // DECLARE_EVENT_LISTENER(contextmenu);
    // DECLARE_EVENT_LISTENER(cuechange);
    // DECLARE_EVENT_LISTENER(dblclick);
    // DECLARE_EVENT_LISTENER(drag);
    // DECLARE_EVENT_LISTENER(dragend);
    // DECLARE_EVENT_LISTENER(dragenter);
    // DECLARE_EVENT_LISTENER(dragexit);
    // DECLARE_EVENT_LISTENER(dragleave);
    // DECLARE_EVENT_LISTENER(dragover);
    // DECLARE_EVENT_LISTENER(dragstart);
    // DECLARE_EVENT_LISTENER(drop);
    DECLARE_EVENT_LISTENER(input);
    // DECLARE_EVENT_LISTENER(invalid);
    DECLARE_EVENT_LISTENER(keydown);
    DECLARE_EVENT_LISTENER(keypress);
    DECLARE_EVENT_LISTENER(keyup);
    // DECLARE_EVENT_LISTENER(loadend);
    DECLARE_EVENT_LISTENER(loadstart);
    DECLARE_EVENT_LISTENER(mousedown);
    DECLARE_EVENT_LISTENER(mouseenter);
    DECLARE_EVENT_LISTENER(mouseleave);
    DECLARE_EVENT_LISTENER(mousemove);
    DECLARE_EVENT_LISTENER(mouseout);
    DECLARE_EVENT_LISTENER(mouseover);
    DECLARE_EVENT_LISTENER(mouseup);
    // DECLARE_EVENT_LISTENER(wheel);
    DECLARE_EVENT_LISTENER(progress);
    // DECLARE_EVENT_LISTENER(reset);
    // DECLARE_EVENT_LISTENER(scroll);
    // DECLARE_EVENT_LISTENER(select);
    // DECLARE_EVENT_LISTENER(show);
    DECLARE_EVENT_LISTENER(submit);
// DECLARE_EVENT_LISTENER(toggle);
#ifdef STARFISH_ENABLE_MULTIMEDIA
    DECLARE_EVENT_LISTENER(suspend);
    DECLARE_EVENT_LISTENER(emptied);
    DECLARE_EVENT_LISTENER(stalled);
    DECLARE_EVENT_LISTENER(loadedmetadata);
    DECLARE_EVENT_LISTENER(loadeddata);
    DECLARE_EVENT_LISTENER(canplay);
    DECLARE_EVENT_LISTENER(canplaythrough);
    DECLARE_EVENT_LISTENER(playing);
    DECLARE_EVENT_LISTENER(waiting);
    DECLARE_EVENT_LISTENER(seeking);
    DECLARE_EVENT_LISTENER(seeked);
    DECLARE_EVENT_LISTENER(ended);
    DECLARE_EVENT_LISTENER(durationchange);
    DECLARE_EVENT_LISTENER(timeupdate);
    DECLARE_EVENT_LISTENER(play);
    DECLARE_EVENT_LISTENER(pause);
    DECLARE_EVENT_LISTENER(ratechange);
    DECLARE_EVENT_LISTENER(volumechange);
#endif

// https://html.spec.whatwg.org/multipage/webappapis.html#documentandelementeventhandlers
// DECLARE_EVENT_LISTENER(copy);
// DECLARE_EVENT_LISTENER(cut);
// DECLARE_EVENT_LISTENER(paste);

#undef VIRTUAL
#undef OVERRIDE

#define VIRTUAL virtual
#define OVERRIDE
    DECLARE_EVENT_LISTENER(blur);
    DECLARE_EVENT_LISTENER(error);
    DECLARE_EVENT_LISTENER(focus);
    DECLARE_EVENT_LISTENER(load);
    DECLARE_EVENT_LISTENER(resize);
// DECLARE_EVENT_LISTENER(scroll);
#undef VIRTUAL
#undef OVERRIDE

    // https://drafts.csswg.org/cssom-view/#extensions-to-the-htmlelement-interface
    Element* offsetParent();

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        Element::fillGCDescriptor(desc);
    }
};

class HTMLKnownElement : public HTMLElement {
public:
    HTMLKnownElement(Document* document, AtomicString localName);

    /* 4.4 Interface Node */
    virtual QualifiedName name()
    {
        return m_name;
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    QualifiedName m_name;
};
}

#endif
