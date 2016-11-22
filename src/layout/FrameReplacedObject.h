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

#ifndef __StarFishFrameReplacedObject__
#define __StarFishFrameReplacedObject__

#include "layout/FrameReplaced.h"
#include "dom/HTMLObjectElement.h"

namespace StarFish {

class FrameReplacedObject : public FrameReplaced {
public:
    FrameReplacedObject(Node* node)
        : FrameReplaced(node, nullptr)
    {
        computeStyleFlags();
    }

    virtual void computeStyleFlags()
    {
        FrameReplaced::computeStyleFlags();
        m_flags.m_isEstablishesStackingContext = true;
#ifndef STARFISH_FRAME_REPLACED_VIDEO_NEEDS_GRAPHICS_BUFFER
#define STARFISH_FRAME_REPLACED_VIDEO_NEEDS_GRAPHICS_BUFFER true
#endif
        m_flags.m_needsGraphicsBuffer = STARFISH_FRAME_REPLACED_VIDEO_NEEDS_GRAPHICS_BUFFER;
    }

    virtual bool isFrameReplacedObject()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameReplacedObject";
    }

    virtual void paintReplaced(Canvas* canvas)
    {
        if (!m_flags.m_needsGraphicsBuffer)
            didCompsiteStackingContext(canvas);
    }

    virtual IntrinsicSize intrinsicSize()
    {
        IntrinsicSize result;
        result.m_isContentExists = true;
        auto v = node()->asElement()->asHTMLElement()->asHTMLObjectElement();
        if (v->content()) {
            result.m_intrinsicContentSize = LayoutSize(v->content()->width(), v->content()->height());
        }
        return result;
    }

    virtual void willCompsiteStackingContext(Canvas* c)
    {
    }

    virtual void didCompsiteStackingContext(Canvas* c)
    {
        auto v = node()->asElement()->asHTMLElement()->asHTMLObjectElement();
        LayoutRect contentRect(borderLeft() + paddingLeft(), borderTop() + paddingTop(), contentWidth(), contentHeight());
        LayoutRect absContentRect(contentRect);
        c->applyMatrixTo(absContentRect);
        v->content()->drawContent(c, contentRect, absContentRect);
    }

    virtual void compsitingStackingContext(Canvas* c)
    {
        if (!m_flags.m_needsGraphicsBuffer)
            didCompsiteStackingContext(c);
    }

protected:
};
}

#endif
