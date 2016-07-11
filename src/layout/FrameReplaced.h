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

    FrameReplacedImage* asFrameReplacedImage()
    {
        STARFISH_ASSERT(isFrameReplacedImage());
        return (FrameReplacedImage*)this;
    }

    virtual const char* name()
    {
        return "FrameReplaced";
    }

    virtual void layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat);
    virtual void computePreferredWidth(ComputePreferredWidthContext& ctx);
    virtual void computeIntrinsicSize(LayoutUnit& intrinsicWidth, LayoutUnit& intrinsicHeight, LayoutUnit parentContentWidth, Length parentContentHeight);

    virtual IntrinsicSizeUsedInLayout intrinsicSize() = 0;

    virtual void paintReplaced(Canvas* canvas)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void paint(Canvas* canvas, PaintingStage stage)
    {
        if (isEstablishesStackingContext())
            return;

        if (isPositionedElement() && stage == PaintingPositionedElements) {
            paintBackgroundAndBorders(canvas);
            paintReplaced(canvas);
        } else if (!isPositionedElement() && isNormalFlow() && stage == PaintingNormalFlowInline) {
            paintBackgroundAndBorders(canvas);
            paintReplaced(canvas);
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
