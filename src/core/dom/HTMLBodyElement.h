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

#ifndef __StarFishHTMLBodyElement__
#define __StarFishHTMLBodyElement__

#include "core/dom/HTMLElement.h"

namespace StarFish {

class HTMLBodyElement : public HTMLElement {
public:
    HTMLBodyElement(Document* document)
        : HTMLElement(document)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLBodyElement() const override;

    /* 4.4 Interface Node */
    virtual QualifiedName name() override;

/* Other methods (not in DOM API) */

#define VIRTUAL virtual
#define OVERRIDE override
    DECLARE_EVENT_LISTENER(blur);
    DECLARE_EVENT_LISTENER(error);
    DECLARE_EVENT_LISTENER(focus);
    DECLARE_EVENT_LISTENER(load);
    DECLARE_EVENT_LISTENER(resize);
// DECLARE_EVENT_LISTENER(scroll);
#undef VIRTUAL
#undef OVERRIDE

#define VIRTUAL
#define OVERRIDE
    // https://html.spec.whatwg.org/multipage/webappapis.html#windoweventhandlers
    // DECLARE_EVENT_LISTENER(afterprint);
    // DECLARE_EVENT_LISTENER(beforeprint);
    // DECLARE_EVENT_LISTENER(beforeunload);
    // DECLARE_EVENT_LISTENER(hashchange);
    // DECLARE_EVENT_LISTENER(languagechange);
    DECLARE_EVENT_LISTENER(message);
    DECLARE_EVENT_LISTENER(messageerror);
    // DECLARE_EVENT_LISTENER(offline);
    // DECLARE_EVENT_LISTENER(online);
    // DECLARE_EVENT_LISTENER(pagehide);
    // DECLARE_EVENT_LISTENER(pageshow);
    // DECLARE_EVENT_LISTENER(popstate);
    // DECLARE_EVENT_LISTENER(rejectionhandled);
    // DECLARE_EVENT_LISTENER(storage);
    // DECLARE_EVENT_LISTENER(unhandledrejection);
    DECLARE_EVENT_LISTENER(unload);
#undef VIRTUAL
#undef OVERRIDE

    virtual void didComputedStyleChanged(ComputedStyle* oldStyle,
                                         ComputedStyle* newStyle) override;

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues) override;

    bool isPotentiallyScrollable();
};
}

#endif
