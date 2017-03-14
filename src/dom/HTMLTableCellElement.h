/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#ifndef __StarFishHTMLTableCellElement__
#define __StarFishHTMLTableCellElement__

#include "dom/HTMLElement.h"

namespace StarFish {

class HTMLTableCellElement : public HTMLElement {
public:
    HTMLTableCellElement(Document* document)
        : HTMLElement(document)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */

    virtual String* localName() = 0;
    virtual QualifiedName name() = 0;

    /* table cell related */

    virtual void setColspan(int colspan)
    {
        setAttribute(
            document()->window()->starFish()->staticStrings()->m_colspan,
            String::fromInt(colspan));
    }

    virtual String* colspan()
    {
        return getAttribute(
            document()->window()->starFish()->staticStrings()->m_colspan);
    }

    virtual void setRowspan(int rowspan)
    {
        setAttribute(
            document()->window()->starFish()->staticStrings()->m_colspan,
            String::fromInt(rowspan));
    }

    virtual String* rowspan()
    {
        return getAttribute(
            document()->window()->starFish()->staticStrings()->m_rowspan);
    }

protected:
};

class HTMLTDElement : public HTMLTableCellElement {
public:
    HTMLTDElement(Document* document)
        : HTMLTableCellElement(document)
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
            ->m_tdTagName.localName();
    }

    virtual QualifiedName name()
    {
        return document()->window()->starFish()->staticStrings()->m_tdTagName;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLTDElement() const
    {
        return true;
    }

protected:
};

class HTMLTHElement : public HTMLTableCellElement {
public:
    HTMLTHElement(Document* document)
        : HTMLTableCellElement(document)
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
            ->m_thTagName.localName();
    }

    virtual QualifiedName name()
    {
        return document()->window()->starFish()->staticStrings()->m_thTagName;
    }

    /* Other methods (not in DOM API) */

    virtual bool isHTMLTHElement() const
    {
        return true;
    }

protected:
};
}

#endif
