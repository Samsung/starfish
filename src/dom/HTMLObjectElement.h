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

#ifndef __StarFishHTMLObjectElement__
#define __StarFishHTMLObjectElement__

#include "dom/HTMLElement.h"

#define STARFISH_OBJECT_ELEMENT_DEFAULT_WIDTH 300
#define STARFISH_OBJECT_ELEMENT_DEFAULT_HEIGHT 150

namespace StarFish {

class Canvas;
class HTMLObjectElementContent;

class HTMLObjectElement : public HTMLElement {
public:
    HTMLObjectElement(Document* document)
        : HTMLElement(document)
    {
        m_content = nullptr;
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */

    virtual String* localName();
    virtual QualifiedName name();

    /* Other methods (not in DOM API) */

    virtual bool isHTMLObjectElement() const override
    {
        return true;
    }

    HTMLObjectElementContent* content()
    {
        return m_content;
    }

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved);

protected:
    HTMLObjectElementContent* m_content;
};

class HTMLObjectElementContent : public gc {
public:
    HTMLObjectElementContent(HTMLObjectElement* element)
        : m_element(element)
    {
    }

    HTMLObjectElement* element()
    {
        return m_element;
    }

    virtual void load()
    {
    }

    virtual void unload()
    {
    }

    virtual LayoutUnit width()
    {
        return STARFISH_OBJECT_ELEMENT_DEFAULT_WIDTH;
    }

    virtual LayoutUnit height()
    {
        return STARFISH_OBJECT_ELEMENT_DEFAULT_WIDTH;
    }

    virtual void drawContent(Canvas* canvas, const LayoutRect& videoRect,
                             const LayoutRect& absVideoRect) = 0;

protected:
    HTMLObjectElement* m_element;
};

class MockHTMLObjectElementContent : public HTMLObjectElementContent {
public:
    MockHTMLObjectElementContent(HTMLObjectElement* element)
        : HTMLObjectElementContent(element)
    {
    }

    virtual void drawContent(Canvas* canvas, const LayoutRect& contentRect,
                             const LayoutRect& absContentRect);
};
}

#endif
