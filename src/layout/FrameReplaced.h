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

#ifndef __StarFishFrameReplaced__
#define __StarFishFrameReplaced__

#include "layout/FrameBox.h"

namespace StarFish {

class FrameReplacedImage;
class FrameReplacedVideo;

struct IntrinsicSize {
    bool m_isContentExists;
    LayoutSize m_intrinsicContentSize;
};

struct IntrinsicSizeUsedInLayout {
    LayoutSize m_intrinsicContentSize;
    std::pair<Length, Length> m_intrinsicSizeIsSpecifiedByAttributeOfElement;

    IntrinsicSizeUsedInLayout()
        : m_intrinsicContentSize(0, 0)
    {

    }
};

class FrameReplaced : public FrameBox {
public:
    FrameReplaced(Node* node, ComputedStyle* style)
        : FrameBox(node, style)
    {
    }

    virtual bool isFrameReplaced()
    {
        return true;
    }

    virtual bool isFrameReplacedImage()
    {
        return false;
    }

    virtual bool isFrameReplacedVideo()
    {
        return false;
    }

    FrameReplacedImage* asFrameReplacedImage()
    {
        STARFISH_ASSERT(isFrameReplacedImage());
        return (FrameReplacedImage*)this;
    }

    FrameReplacedVideo* asFrameReplacedVideo()
    {
        STARFISH_ASSERT(isFrameReplacedVideo());
        return (FrameReplacedVideo*)this;
    }

    virtual const char* name()
    {
        return "FrameReplaced";
    }

    virtual void layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat);
    virtual void computePreferredWidth(ComputePreferredWidthContext& ctx);
    virtual void computeIntrinsicSize(LayoutUnit& intrinsicWidth, LayoutUnit& intrinsicHeight, LayoutUnit parentContentWidth, Length parentContentHeight);

    virtual IntrinsicSize intrinsicSize() = 0;
    IntrinsicSizeUsedInLayout computeIntrinsicSizeForLayout();

    virtual void paintReplaced(Canvas* canvas)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void paint(PaintingContext& ctx)
    {
        if (isEstablishesStackingContext())
            return;


        if (isPositionedElement() && ctx.m_paintingStage == PaintingPositionedElements) {
            paintBackgroundAndBorders(ctx.m_canvas);
            paintReplaced(ctx.m_canvas);
        } else if (!isPositionedElement()) {
            if (ctx.m_paintingStage == PaintingNormalFlowBlock) {
                if (style()->display() != DisplayValue::InlineDisplayValue) {
                    paintBackgroundAndBorders(ctx.m_canvas);
                }
            } else if (ctx.m_paintingStage == PaintingNormalFlowInline) {
                if (style()->display() == DisplayValue::InlineDisplayValue) {
                    if (ctx.m_paintingInlineStage == PaintingInlineLevelElements) {
                        paintBackgroundAndBorders(ctx.m_canvas);
                        paintReplaced(ctx.m_canvas);
                    }
                } else if (style()->display() == DisplayValue::InlineBlockDisplayValue) {
                    if (ctx.m_paintingInlineStage == PaintingInlineBlock) {
                        paintReplaced(ctx.m_canvas);
                    }
                } else {
                    paintReplaced(ctx.m_canvas);
                }
            }
        }
    }

    virtual void paintStackingContextContent(Canvas* canvas)
    {
        paintReplaced(canvas);
    }
protected:
};
}

#endif
