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

#ifndef __StarFishHTMLAnchorElement__
#define __StarFishHTMLAnchorElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLAnchorElement : public HTMLElement {
public:
    HTMLAnchorElement(Document* document)
        : HTMLElement(document)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */

    virtual String* localName()
    {
        return document()->window()->starFish()->staticStrings()->m_aTagName.localName();
    }

    virtual QualifiedName name()
    {
        return document()->window()->starFish()->staticStrings()->m_aTagName;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLAnchorElement() const
    {
        return true;
    }

    virtual void handleDefaultEvent(Event* event)
    {
        if (event->isUIEvent() && event->asUIEvent()->isMouseEvent()) {
            if (event->eventType()->equals("click")) {
                auto href = document()->window()->starFish()->staticStrings()->m_href;
                size_t s = hasAttribute(href);
                if (s != SIZE_MAX) {
                    String* h = getAttribute(s)->trim();
                    if (h->length()) {
                        document()->window()->navigateAsync(URL::createURL(document()->documentURI()->urlString(), h));
                    } else {
                        document()->window()->navigateAsync(document()->documentURI());
                    }
                }
            }
        }
    }

protected:
};

}

#endif
