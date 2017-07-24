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
                                 LayoutUnit parentHeight, bool hasAspectRatio,
                                 bool parentHeightHasFixedValue = true)
    {
        LayoutUnit newWidth = width;
        LayoutUnit newHeight = height;
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
            newWidth = contentWidthApplyingBoxSizing(
                minWidth.specifiedValue(parentWidth));
            if (canApplyMinHeight) {
                // in the case minWidth and minHeight, the one whose value
                // is higher is applied.
                newHeight = contentHeightApplyingBoxSizing(
                    minHeight.specifiedValue(parentHeight));
                if (hasAspectRatio) {
                    if (newWidth > newHeight) {
                        newHeight = newWidth * (height / width);
                    } else if (newWidth < newHeight) {
                        newWidth = newHeight * (width / height);
                    }
                }
            } else if (canApplyMaxHeight) {
                // in the case minWidth and maxHeight, then apply values
                // respectively.
                newHeight = contentHeightApplyingBoxSizing(
                    maxHeight.specifiedValue(parentHeight));
            } else {
                if (hasAspectRatio) {
                    newHeight = newWidth * (height / width);
                }
            }
        } else if (maxWidth.isSpecified()) {
            newWidth = contentWidthApplyingBoxSizing(
                maxWidth.specifiedValue(parentWidth));
            if (canApplyMinHeight) {
                // in the case maxWidth and minHeight, then apply values
                // respectively.
                newHeight = contentHeightApplyingBoxSizing(
                    minHeight.specifiedValue(parentHeight));
            } else if (canApplyMaxHeight) {
                // in the case maxWidth and maxHeight, the one whose value
                // is lower is applied.
                newHeight = contentHeightApplyingBoxSizing(
                    maxHeight.specifiedValue(parentHeight));
                if (hasAspectRatio) {
                    if (newWidth > newHeight) {
                        newWidth = newHeight * (width / height);
                    } else if (newWidth < newHeight) {
                        newHeight = newWidth * (height / width);
                    }
                }
            } else {
                if (hasAspectRatio) {
                    newHeight = newWidth * (height / width);
                }
            }
        } else {
            if (canApplyMinHeight) {
                newHeight = contentHeightApplyingBoxSizing(
                    minHeight.specifiedValue(parentHeight));
                if (hasAspectRatio) {
                    newWidth = newHeight * (width / height);
                }
            } else if (canApplyMaxHeight) {
                newHeight = contentHeightApplyingBoxSizing(
                    maxHeight.specifiedValue(parentHeight));
                if (hasAspectRatio) {
                    newWidth = newHeight * (width / height);
                }
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
