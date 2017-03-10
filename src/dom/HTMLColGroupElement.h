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

#ifndef __StarFishHTMLColgroupElement__
#define __StarFishHTMLColgroupElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLColGroupElement : public HTMLElement {
public:
    HTMLColGroupElement(Document* document) : HTMLElement(document)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */

    virtual String* localName()
    {
        return document()
            ->window()
            ->starFish()
            ->staticStrings()
            ->m_colgroupTagName.localName();
    }

    virtual QualifiedName name()
    {
        return document()
            ->window()
            ->starFish()
            ->staticStrings()
            ->m_colgroupTagName;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLColGroupElement() const
    {
        return true;
    }

    void setSpan(int span)
    {
        setAttribute(document()->window()->starFish()->staticStrings()->m_span,
                     String::fromInt(span));
    }

    String* span()
    {
        return getAttribute(
            document()->window()->starFish()->staticStrings()->m_span);
    }

protected:
};
}

#endif
