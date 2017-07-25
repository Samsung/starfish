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

    void applyMinMaxWidthAndHeightIfNeeds(LayoutUnit width, LayoutUnit height,
                                          LayoutUnit parentWidth,
                                          LayoutUnit parentHeight,
                                          bool hasAspectRatio,
                                          bool parentHeightHasFixedValue)
    {
        auto widthAndHeight = minMaxWidthAndHeightAppliedIfNeeds(
            width, height, parentWidth, parentHeight, hasAspectRatio,
            parentHeightHasFixedValue);

        setContentWidth(widthAndHeight.first);
        setContentHeight(widthAndHeight.second);
    }

    std::pair<LayoutUnit, LayoutUnit> minMaxWidthAndHeightAppliedIfNeeds(
        LayoutUnit w, LayoutUnit h, LayoutUnit parentWidth,
        LayoutUnit parentHeight, bool hasAspectRatio,
        bool parentHeightHasFixedValue)
    {
        LayoutUnit newWidth = w;
        LayoutUnit newHeight = h;
        Length width = style()->width();
        Length height = style()->height();
        Length minWidth = style()->minWidth();
        Length maxWidth = style()->maxWidth();
        Length minHeight = style()->minHeight();
        Length maxHeight = style()->maxHeight();
        bool canApplyMinHeight =
            minHeight.isFixed() ||
            (minHeight.isPercent() && parentHeightHasFixedValue);
        bool canApplyMaxHeight =
            maxHeight.isFixed() ||
            (maxHeight.isPercent() && parentHeightHasFixedValue);

        if (minWidth.isSpecified()) {
            newWidth = std::max(w, contentWidthApplyingBoxSizing(
                                       minWidth.specifiedValue(parentWidth)));
            if (canApplyMinHeight) {
                newHeight =
                    std::max(h, contentHeightApplyingBoxSizing(
                                    minHeight.specifiedValue(parentHeight)));
                if (width.isAuto() && height.isAuto()) {
                    if (hasAspectRatio) {
                        if (newWidth > newHeight) {
                            newHeight = newWidth * (h / w);
                        } else if (newWidth < newHeight) {
                            newWidth = newHeight * (w / h);
                        }
                    }
                } else if (width.isAuto()) {
                    if (hasAspectRatio && newWidth < newHeight) {
                        newWidth = newHeight * (w / h);
                    }
                } else if (height.isAuto()) {
                    if (hasAspectRatio && newWidth > newHeight) {
                        newHeight = newWidth * (h / w);
                    }
                }
            } else if (canApplyMaxHeight) {
                // in the case minWidth and maxHeight, then apply values
                // respectively.
                newHeight =
                    std::min(h, contentHeightApplyingBoxSizing(
                                    maxHeight.specifiedValue(parentHeight)));
            } else {
                if (hasAspectRatio && height.isAuto()) {
                    newHeight = newWidth * (h / w);
                }
            }
        } else if (maxWidth.isSpecified()) {
            newWidth = std::min(w, contentWidthApplyingBoxSizing(
                                       maxWidth.specifiedValue(parentWidth)));
            if (canApplyMinHeight) {
                // in the case maxWidth and minHeight, then apply values
                // respectively.
                newHeight =
                    std::max(h, contentHeightApplyingBoxSizing(
                                    minHeight.specifiedValue(parentHeight)));
            } else if (canApplyMaxHeight) {
                newHeight =
                    std::min(h, contentHeightApplyingBoxSizing(
                                    maxHeight.specifiedValue(parentHeight)));
                if (width.isAuto() && height.isAuto()) {
                    if (hasAspectRatio) {
                        if (newWidth > newHeight) {
                            newWidth = newHeight * (w / h);
                        } else if (newWidth < newHeight) {
                            newHeight = newWidth * (h / w);
                        }
                    }
                } else if (width.isAuto()) {
                    if (hasAspectRatio && newWidth > newHeight) {
                        newWidth = newHeight * (w / h);
                    }
                } else if (height.isAuto()) {
                    if (hasAspectRatio && newWidth < newHeight) {
                        newHeight = newWidth * (h / w);
                    }
                }
            } else {
                if (hasAspectRatio && height.isAuto()) {
                    newHeight = newWidth * (h / w);
                }
            }
        } else {
            if (canApplyMinHeight) {
                newHeight =
                    std::max(h, contentHeightApplyingBoxSizing(
                                    minHeight.specifiedValue(parentHeight)));
                if (hasAspectRatio && width.isAuto()) {
                    newWidth = newHeight * (w / h);
                }
            } else if (canApplyMaxHeight) {
                newHeight =
                    std::min(h, contentHeightApplyingBoxSizing(
                                    maxHeight.specifiedValue(parentHeight)));
                if (hasAspectRatio && width.isAuto()) {
                    newWidth = newHeight * (w / h);
                }
            }
        }

        return std::make_pair(newWidth, newHeight);
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
