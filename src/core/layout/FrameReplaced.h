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

    virtual bool isFrameReplaced() override
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

    LayoutRect computeObjectFit(const LayoutUnit& w, const LayoutUnit& h);

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

    virtual const char* name() override
    {
        return "FrameReplaced";
    }

    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat) override;
    virtual void computePreferredWidth(PreferredWidthContext& ctx) override;
    virtual void layoutInline(LineFormattingContext& ctx) override;
    void computeIntrinsicSize(LayoutContext& ctx, LayoutUnit& intrinsicWidth,
                              LayoutUnit& intrinsicHeight, bool& hasAspectRatio,
                              LayoutUnit parentContentWidth,
                              Length parentContentHeight);

    virtual IntrinsicSize intrinsicSize() = 0;
    IntrinsicSizeUsedInLayout computeIntrinsicSizeForLayout();

    virtual void paintReplaced(Canvas* canvas)
    {
        const LayoutRect rect(0, 0, width(), height());
        applyBorderRadiusClippingIfNeeds(canvas, rect);
        if (!isFrameReplaced()) {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    virtual void paintContent(PaintingContext& ctx) override;
    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y,
                           HitTestStage stage) override;

    virtual void paintStackingContextContent(Canvas* canvas) override
    {
        paintReplaced(canvas);
    }

protected:
    virtual bool hasFrameTreeItemModel() override
    {
        return true;
    }

    virtual FrameTreeItemModel* frameTreeItemModel() override
    {
        return &m_treeItemModel;
    }

    FrameTreeItemModel m_treeItemModel;
    void computeContentWidthAndHeight(LayoutContext& ctx, FrameBox* cb);
};
}

#endif
