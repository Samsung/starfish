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

#ifndef __StarFishHTMLTableElement__
#define __StarFishHTMLTableElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLTableElement : public HTMLElement {
public:
    HTMLTableElement(Document* document)
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
        return document()
            ->window()
            ->starFish()
            ->staticStrings()
            ->m_tableTagName.localName();
    }

    virtual QualifiedName name()
    {
        return document()
            ->window()
            ->starFish()
            ->staticStrings()
            ->m_tableTagName;
    }

    /* Not in HTML5 */
    String* width()
    {
        return getAttribute(
            document()->window()->starFish()->staticStrings()->m_width);
    }

    void setWidth(int width)
    {
        setAttribute(document()->window()->starFish()->staticStrings()->m_width,
                     String::fromInt(width));
    }

    String* bgColor()
    {
        return getAttribute(
            document()->window()->starFish()->staticStrings()->m_bgColor);
    }

    void setBgColor(String* bgColor)
    {
        setAttribute(
            document()->window()->starFish()->staticStrings()->m_bgColor,
            bgColor);
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLTableElement() const
    {
        return true;
    }

protected:
};
}

#endif
