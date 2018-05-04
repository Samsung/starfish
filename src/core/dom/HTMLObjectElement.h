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

#ifndef __StarFishHTMLObjectElement__
#define __StarFishHTMLObjectElement__

#include "core/dom/HTMLElement.h"

#define STARFISH_OBJECT_ELEMENT_DEFAULT_WIDTH 300
#define STARFISH_OBJECT_ELEMENT_DEFAULT_HEIGHT 150

namespace StarFish {

class Canvas;
class Compositor;
class HTMLObjectElementContent;

class HTMLObjectElement : public HTMLElement {
public:
    HTMLObjectElement(Document* document)
        : HTMLElement(document)
    {
        m_content = nullptr;
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLObjectElement() const override;

    /* 4.4 Interface Node */
    virtual QualifiedName name();

    /* Other methods (not in DOM API) */

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
    virtual ~HTMLObjectElementContent()
    {
    }
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

    virtual void drawContent(Compositor* canvas, const LayoutRect& videoRect,
                             const LayoutRect& absVideoRect) = 0;

    virtual bool needsGraphicsBuffer()
    {
        return false;
    }

protected:
    HTMLObjectElement* m_element;
};

class MockHTMLObjectElementContent : public HTMLObjectElementContent {
public:
    MockHTMLObjectElementContent(HTMLObjectElement* element)
        : HTMLObjectElementContent(element)
    {
    }

    virtual void drawContent(Compositor* canvas, const LayoutRect& contentRect,
                             const LayoutRect& absContentRect);
};

#ifdef STARFISH_ENABLE_AVPLAY
class AVPlayHTMLObjectElementContent : public HTMLObjectElementContent {
public:
    AVPlayHTMLObjectElementContent(HTMLObjectElement* element)
        : HTMLObjectElementContent(element)
    {
    }

    virtual bool needsGraphicsBuffer()
    {
        return true;
    }

    virtual void drawContent(Compositor* canvas, const LayoutRect& contentRect,
                             const LayoutRect& absContentRect);
};
#endif
}

#endif
