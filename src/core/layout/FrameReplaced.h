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

#ifndef __StarFishFrameReplaced__
#define __StarFishFrameReplaced__

#include "core/layout/FrameBox.h"

namespace StarFish {

class FrameReplacedImage;
class FrameReplacedVideo;
class FrameReplacedIFrame;

struct IntrinsicSize {
    bool m_isContentExists;
    bool m_hasAspectRatio;
    LayoutSize m_intrinsicContentSize;
};

struct IntrinsicSizeUsedInLayout {
    bool m_hasAspectRatio;
    LayoutSize m_intrinsicContentSize;
    std::pair<Length, Length> m_intrinsicSizeIsSpecifiedByAttributeOfElement;

    IntrinsicSizeUsedInLayout()
        : m_hasAspectRatio(false)
        , m_intrinsicContentSize(0, 0)
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

    virtual bool isFrameReplacedIFrame()
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

    FrameReplacedIFrame* asFrameReplacedIFrame()
    {
        STARFISH_ASSERT(isFrameReplacedIFrame());
        return (FrameReplacedIFrame*)this;
    }

    void applyMinMaxValueIfNeeds(LayoutUnit width, LayoutUnit height,
                                 LayoutUnit parentWidth,
                                 LayoutUnit parentHeight,
                                 bool parentWidthHasFixedValue = true,
                                 bool parentHeightHasFixedValue = true)
    {
        LayoutUnit newWidth = minMaxWidthAppliedIfNeeds(
            width, parentWidth, parentWidthHasFixedValue);
        LayoutUnit newHeight = minMaxHeightAppliedIfNeeds(
            height, parentWidth, parentHeightHasFixedValue);
        if (width != newWidth || height != newHeight) {
            if (height == 0 || newHeight == 0) {
                setContentWidth(newWidth);
                return;
            }
            if (newWidth < newHeight) {
                newHeight = newWidth * (height / width);
            } else if (newWidth > newHeight) {
                newWidth = newHeight * (width / height);
            }
        }
        setContentWidth(newWidth);
        setContentHeight(newHeight);
    }

    virtual const char* name()
    {
        return "FrameReplaced";
    }

    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat);
    virtual void computePreferredWidth(PreferredWidthContext& ctx);
    virtual void layoutInline(LineFormattingContext& ctx);
    void computeIntrinsicSize(LayoutUnit& intrinsicWidth,
                              LayoutUnit& intrinsicHeight, bool& hasAspectRatio,
                              LayoutUnit parentContentWidth,
                              Length parentContentHeight);

    virtual IntrinsicSize intrinsicSize() = 0;
    IntrinsicSizeUsedInLayout computeIntrinsicSizeForLayout();

    virtual void paintReplaced(Canvas* canvas)
    {
        if (!isFrameReplaced()) {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    virtual void paint(PaintingContext& ctx);

    virtual void paintStackingContextContent(Canvas* canvas)
    {
        paintReplaced(canvas);
    }

protected:
};
}

#endif
