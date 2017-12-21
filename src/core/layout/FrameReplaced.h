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
#ifdef STARFISH_ENABLE_MULTIMEDIA
class FrameReplacedVideo;
#endif
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

#ifdef STARFISH_ENABLE_MULTIMEDIA
    virtual bool isFrameReplacedVideo()
    {
        return false;
    }
#endif

    virtual bool isFrameReplacedIFrame()
    {
        return false;
    }

    FrameReplacedImage* asFrameReplacedImage()
    {
        STARFISH_ASSERT(isFrameReplacedImage());
        return (FrameReplacedImage*)this;
    }

#ifdef STARFISH_ENABLE_MULTIMEDIA
    FrameReplacedVideo* asFrameReplacedVideo()
    {
        STARFISH_ASSERT(isFrameReplacedVideo());
        return (FrameReplacedVideo*)this;
    }
#endif

    FrameReplacedIFrame* asFrameReplacedIFrame()
    {
        STARFISH_ASSERT(isFrameReplacedIFrame());
        return (FrameReplacedIFrame*)this;
    }

    void applyMinMaxWidthAndHeightIfNeeds(LayoutContext& ctx, LayoutUnit width,
                                          LayoutUnit height,
                                          LayoutUnit parentWidth,
                                          LayoutUnit parentHeight,
                                          bool hasAspectRatio,
                                          bool parentHeightHasFixedValue)
    {
        auto widthAndHeight = minMaxWidthAndHeightAppliedIfNeeds(
            ctx, width, height, parentWidth, parentHeight, hasAspectRatio,
            parentHeightHasFixedValue);

        setContentWidth(widthAndHeight.first);
        setContentHeight(widthAndHeight.second);
    }

    std::pair<LayoutUnit, LayoutUnit> minMaxWidthAndHeightAppliedIfNeeds(
        LayoutContext& ctx, LayoutUnit w, LayoutUnit h, LayoutUnit parentWidth,
        LayoutUnit parentHeight, bool hasAspectRatio,
        bool parentHeightHasFixedValue);

    virtual const char* name()
    {
        return "FrameReplaced";
    }

    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat);
    virtual void computePreferredWidth(PreferredWidthContext& ctx);
    virtual void layoutInline(LineFormattingContext& ctx);
    void computeIntrinsicSize(LayoutContext& ctx, LayoutUnit& intrinsicWidth,
                              LayoutUnit& intrinsicHeight, bool& hasAspectRatio,
                              LayoutUnit parentContentWidth,
                              Length parentContentHeight);

    virtual IntrinsicSize intrinsicSize() = 0;
    IntrinsicSizeUsedInLayout computeIntrinsicSizeForLayout();

    virtual void paintReplaced(Canvas* canvas)
    {
        applyBorderRadiusClippingIfNeeds(canvas);
        if (!isFrameReplaced()) {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    virtual void paintContent(PaintingContext& ctx);
    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage);

    virtual void paintStackingContextContent(Canvas* canvas)
    {
        paintReplaced(canvas);
    }

protected:
    virtual bool hasFrameTreeItemModel()
    {
        return true;
    }

    virtual FrameTreeItemModel* frameTreeItemModel()
    {
        return &m_treeItemModel;
    }

    FrameTreeItemModel m_treeItemModel;
    void computeContentWidthAndHeight(LayoutContext& ctx, FrameBox* cb);
};
}

#endif
