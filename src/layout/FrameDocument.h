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

#ifndef __StarFishFrameDocument__
#define __StarFishFrameDocument__

#include "layout/FrameBlockBox.h"

namespace StarFish {

class FrameDocument : public FrameBlockBox {
public:
    FrameDocument(Node* node)
        : FrameBlockBox(node, nullptr)
    {

    }

    virtual const char* name()
    {
        return "FrameDocument";
    }

    virtual bool isFrameDocument()
    {
        return true;
    }

    virtual void layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat);
    virtual void paint(PaintingContext& ctx)
    {
        STARFISH_ASSERT(ctx.m_paintingStage == PaintingStageEnd);
        STARFISH_ASSERT(firstChild() == lastChild());
        if (!firstChild())
            return;
        ctx.m_canvas->save();
        ctx.m_canvas->translate(firstChild()->asFrameBox()->x(), firstChild()->asFrameBox()->y());
        firstChild()->asFrameBox()->stackingContext()->paintStackingContext(ctx.m_canvas);
        ctx.m_canvas->restore();
    }

    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage)
    {
        STARFISH_ASSERT(stage == HitTestStageEnd);
        STARFISH_ASSERT(firstChild() == lastChild());
        if (!firstChild())
            return nullptr;

        Frame* result = firstChild()->asFrameBox()->stackingContext()->hitTestStackingContext(x, y);
        if (result)
            return result;
        return this;
    }
};

}

#endif
