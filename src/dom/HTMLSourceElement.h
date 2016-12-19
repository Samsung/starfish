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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined (__StarFishHTMLSourceElement__)
#define __StarFishHTMLSourceElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLSourceElement : public HTMLElement {
public:
    HTMLSourceElement(Document* document)
        : HTMLElement(document)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */

    virtual String* localName();
    virtual QualifiedName name();

    /* Other methods (not in DOM API) */
    virtual bool isHTMLSourceElement() const
    {
        return true;
    }

    // https://html.spec.whatwg.org/multipage/embedded-content.html#the-source-element
    // Attributes
    // NOTE : Current version of HTMLSourceElement considers only media element related case.
    //        (picture element related case has not been considered.
    //         Hence attribute "media", "srcset", "sizes" are not currently supported)
    String* src()
    {
        return getAttribute(document()->window()->starFish()->staticStrings()->m_src);
    }

    String* typeAttr()
    {
        return getAttribute(document()->window()->starFish()->staticStrings()->m_type);
    }

    void setSrc(String* src)
    {
        setAttribute(document()->window()->starFish()->staticStrings()->m_src, src);
    }

    void setTypeAttr(String* type)
    {
        setAttribute(document()->window()->starFish()->staticStrings()->m_type, type);
    }
    // TODO
protected:
};

}

#endif
