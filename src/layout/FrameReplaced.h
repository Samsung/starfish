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

    virtual void layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
    {
        IntrinsicSizeUsedInLayout s = intrinsicSize();
        LayoutUnit parentContentWidth = ctx.containingBlock(this)->asFrameBox()->contentWidth();

        if (resolveWhat & Frame::LayoutWantToResolve::ResolveWidth) {
            LayoutUnit intrinsicWidth, intrinsicHeight;
            computeBorderMarginPadding(parentContentWidth);

            if (isNormalFlow()) {
                Length parentContentHeight;
                if (ctx.parentHasFixedHeight(this)) {
                    parentContentHeight = Length(Length::Fixed, ctx.parentFixedHeight(this));
                } else {
                    parentContentHeight = Length(Length::Auto);
                }
                computeIntrinsicSize(intrinsicWidth, intrinsicHeight, parentContentWidth, parentContentHeight);

                if ((intrinsicWidth == 0 || intrinsicHeight == 0) && (style()->width().isAuto() || style()->height().isAuto())) {
                    setContentWidth(0);
                    setContentHeight(0);
                } else if (style()->width().isAuto() && style()->height().isAuto()) {
                    setContentWidth(intrinsicWidth);
                    setContentHeight(intrinsicHeight);
                } else if (style()->width().isSpecified() && style()->height().isAuto()) {
                    LayoutUnit w = style()->width().specifiedValue(ctx.parentContentWidth(this));
                    LayoutUnit h = w * (intrinsicHeight / intrinsicWidth);
                    setContentWidth(w);
                    setContentHeight(h);
                } else if (style()->width().isAuto() && style()->height().isSpecified()) {
                    if (style()->height().isFixed()) {
                        LayoutUnit h = style()->height().fixed();
                        LayoutUnit w = h * (intrinsicWidth / intrinsicHeight);
                        setContentWidth(w);
                        setContentHeight(h);
                    } else {
                        STARFISH_ASSERT(style()->height().isPercent());
                        if (ctx.parentHasFixedHeight(this)) {
                            LayoutUnit h = style()->height().percent() * ctx.parentFixedHeight(this);
                            LayoutUnit w = h * (intrinsicWidth / intrinsicHeight);
                            setContentWidth(w);
                            setContentHeight(h);
                        } else {
                            setContentWidth(intrinsicWidth);
                            setContentHeight(intrinsicHeight);
                        }
                    }
                } else {
                    STARFISH_ASSERT(style()->width().isSpecified() && style()->height().isSpecified());
                    setContentWidth(style()->width().specifiedValue(ctx.parentContentWidth(this)));
                    if (style()->height().isFixed()) {
                        setContentHeight(style()->height().fixed());
                    } else {
                        if (ctx.parentHasFixedHeight(this))
                            setContentHeight(style()->height().percent() * ctx.parentFixedHeight(this));
                        else
                            setContentHeight(intrinsicHeight);
                    }
                }

                if (style()->display() == BlockDisplayValue && style()->marginLeft().isAuto() && style()->marginRight().isAuto()) {
                    LayoutUnit remain = parentContentWidth;
                    remain -= contentWidth();
                    remain -= borderWidth();
                    remain -= paddingWidth();
                    if (remain > 0) {
                        setMarginLeft(remain / 2);
                        setMarginRight(remain / 2);
                    }
                }

                if (style()->display() == BlockDisplayValue) {
                    if (style()->marginLeft().isAuto() && style()->marginRight().isAuto()) {
                        LayoutUnit remain = parentContentWidth;
                        remain -= contentWidth();
                        remain -= borderWidth();
                        remain -= paddingWidth();
                        if (remain > 0) {
                            setMarginLeft(remain / 2);
                            setMarginRight(remain / 2);
                        }
                    } else if (style()->marginLeft().isAuto() && !style()->marginRight().isAuto()) {
                        LayoutUnit remain = parentContentWidth;
                        remain -= contentWidth();
                        remain -= borderWidth();
                        remain -= paddingWidth();
                        remain -= marginRight();
                        if (remain > 0) {
                            setMarginLeft(remain);
                        }
                    } else if (!style()->marginLeft().isAuto() && style()->marginRight().isAuto()) {
                        LayoutUnit remain = parentContentWidth;
                        remain -= contentWidth();
                        remain -= borderWidth();
                        remain -= paddingWidth();
                        remain -= marginLeft();
                        if (remain > 0) {
                            setMarginRight(remain);
                        }
                    }
                }
            } else {
                FrameBox* cb = ctx.containingBlock(this)->asFrameBox();
                LayoutUnit parentHeight = cb->contentHeight() + cb->paddingHeight();

                Length parentContentHeight;
                parentContentHeight = Length(Length::Fixed, parentHeight);
                computeIntrinsicSize(intrinsicWidth, intrinsicHeight, parentContentWidth, parentContentHeight);

                computeBorderMarginPadding(cb->contentWidth());

                FrameBox* parent = Frame::layoutParent()->asFrameBox();
                DirectionValue direction = ctx.blockContainer(this)->style()->direction();
                auto absLoc = parent->absolutePoint(cb);
                LayoutUnit absX = absLoc.x() - cb->borderLeft();
                auto setAbsX = [&](LayoutUnit x)
                {
                    setX(x - absX);
                };

                Length marginLeft = style()->marginLeft();
                Length marginRight = style()->marginRight();

                if (!marginLeft.isAuto() && !marginRight.isAuto()) {
                    if (style()->direction() == LtrDirectionValue) {
                        setMarginRight(0);
                    } else {
                        setMarginLeft(0);
                    }
                }

                if ((intrinsicWidth == 0 || intrinsicHeight == 0) && (style()->width().isAuto() || style()->height().isAuto())) {
                    setContentWidth(0);
                    setContentHeight(0);
                } else if (style()->width().isAuto() && style()->height().isAuto()) {
                    setContentWidth(intrinsicWidth);
                    setContentHeight(intrinsicHeight);
                } else if (style()->width().isSpecified() && style()->height().isAuto()) {
                    LayoutUnit w = style()->width().specifiedValue(cb->contentWidth());
                    LayoutUnit h = w * (intrinsicHeight / intrinsicWidth);
                    setContentWidth(w);
                    setContentHeight(h);
                } else if (style()->width().isAuto() && style()->height().isSpecified()) {
                    LayoutUnit h = style()->height().specifiedValue(cb->contentHeight());
                    LayoutUnit w = h * (intrinsicWidth / intrinsicHeight);
                    setContentWidth(w);
                    setContentHeight(h);
                } else {
                    STARFISH_ASSERT(style()->width().isSpecified() && style()->height().isSpecified());
                    setContentWidth(style()->width().specifiedValue(cb->contentWidth()));
                    if (style()->height().isFixed()) {
                        setContentHeight(style()->height().fixed());
                    } else {
                        if (ctx.parentHasFixedHeight(this))
                            setContentHeight(style()->height().specifiedValue(cb->contentHeight()));
                        else
                            setContentHeight(intrinsicHeight);
                    }
                }

                LayoutUnit containgBlockContentWidth = cb->contentWidth() + cb->paddingWidth();

                // If 'margin-left' or 'margin-right' is specified as 'auto' its used value is determined by the rules below.
                // If both 'left' and 'right' have the value 'auto'
                // then if the 'direction' property of the element establishing the static-position containing block is 'ltr', set 'left' to the static position;
                // else if 'direction' is 'rtl', set 'right' to the static position.
                if (style()->left().isAuto() && style()->right().isAuto()) {
                    // static location computed in normal flow processing
                } else if (!style()->left().isAuto() && style()->right().isAuto()) {
                    setAbsX(style()->left().specifiedValue(containgBlockContentWidth));
                } else if (style()->left().isAuto() && !style()->right().isAuto()) {
                    LayoutUnit r = style()->right().specifiedValue(containgBlockContentWidth);
                    setAbsX(containgBlockContentWidth - r - width());
                } else {
                    if (style()->direction() == LtrDirectionValue) {
                        setAbsX(style()->left().specifiedValue(containgBlockContentWidth));
                    } else {
                        LayoutUnit computedRight = style()->right().specifiedValue(containgBlockContentWidth);
                        setAbsX(containgBlockContentWidth - FrameBox::width() - computedRight);
                    }
                }

                if (!marginLeft.isAuto() && !marginRight.isAuto()) {
                    if (style()->direction() == LtrDirectionValue) {
                        moveX(FrameBox::marginLeft());
                    } else {
                        moveX(-FrameBox::marginRight());
                    }
                } else if (!marginLeft.isAuto() && marginRight.isAuto()) {
                    moveX(FrameBox::marginLeft());
                } else if (marginLeft.isAuto() && !marginRight.isAuto()) {
                    moveX(-FrameBox::marginRight());
                } else {
                }

                if (style()->left().isAuto() && style()->right().isAuto() && direction == DirectionValue::RtlDirectionValue) {
                    moveX(-this->width());
                }
            }
        }

        if (resolveWhat & Frame::LayoutWantToResolve::ResolveHeight) {
            if (isNormalFlow()) {

            } else {
                FrameBox* cb = ctx.containingBlock(this)->asFrameBox();
                FrameBox* parent = Frame::layoutParent()->asFrameBox();
                auto absLoc = parent->absolutePoint(cb);

                LayoutUnit parentHeight = cb->contentHeight() + cb->paddingHeight();
                Length marginTop = style()->marginTop();
                Length marginBottom = style()->marginBottom();

                LayoutUnit absY = absLoc.y() - cb->borderTop();
                auto setAbsY = [&](LayoutUnit y)
                {
                    setY(y - absY);
                };

                if (style()->top().isAuto() && style()->bottom().isAuto()) {
                    // static location computed in normal flow processing
                } else if (!style()->top().isAuto() && style()->bottom().isAuto()) {
                    setAbsY(style()->top().specifiedValue(parentHeight));
                } else if (style()->top().isAuto() && !style()->bottom().isAuto()) {
                    LayoutUnit b = style()->bottom().specifiedValue(parentHeight);
                    setAbsY(parentHeight - b - height());
                } else {
                    setAbsY(style()->top().specifiedValue(parentHeight));
                }

                if (!marginTop.isAuto() && !marginBottom.isAuto()) {
                    moveY(FrameBox::marginTop());
                } else if (!marginTop.isAuto() && marginBottom.isAuto()) {
                    moveY(FrameBox::marginTop());
                } else if (marginTop.isAuto() && !marginBottom.isAuto()) {
                    moveY(-FrameBox::marginBottom());
                } else {
                }
            }
        }
    }

    virtual void computePreferredWidth(ComputePreferredWidthContext& ctx)
    {
        LayoutUnit parentContentWidth = ctx.layoutContext().blockContainer(this)->asFrameBox()->contentWidth();
        LayoutUnit intrinsicWidth, intrinsicHeight;
        Length parentContentHeight;
        if (ctx.layoutContext().parentHasFixedHeight(this)) {
            parentContentHeight = Length(Length::Fixed, ctx.layoutContext().parentFixedHeight(this));
        } else {
            parentContentHeight = Length(Length::Auto);
        }
        computeIntrinsicSize(intrinsicWidth, intrinsicHeight, parentContentWidth, parentContentHeight);

        if (style()->width().isAuto() && style()->height().isAuto()) {
            ctx.setResult(intrinsicWidth);
            ctx.setMinimumWidth(intrinsicWidth);
        } else if (style()->width().isSpecified()) {
            if (style()->width().isFixed()) {
                ctx.setResult(style()->width().fixed());
                ctx.setMinimumWidth(style()->width().fixed());
            } else {
                ctx.setResult(intrinsicWidth);
                ctx.setMinimumWidth(intrinsicWidth);
            }
        } else if (style()->height().isSpecified()) {
            if (style()->height().isFixed()) {
                LayoutUnit h = style()->height().fixed();
                LayoutUnit w = h * (intrinsicWidth / intrinsicHeight);
                ctx.setResult(w);
                ctx.setMinimumWidth(w);
            } else {
                ctx.setResult(intrinsicWidth);
                ctx.setMinimumWidth(intrinsicWidth);
            }
        } else {
            ctx.setResult(intrinsicWidth);
        }
    }

    void computeIntrinsicSize(LayoutUnit& intrinsicWidth, LayoutUnit& intrinsicHeight, LayoutUnit parentContentWidth, Length parentContentHeight)
    {
        IntrinsicSizeUsedInLayout s = intrinsicSize();
        auto a = s.m_intrinsicSizeIsSpecifiedByAttributeOfElement;
        auto b = s.m_intrinsicContentSize;
        if (a.first.isAuto() && a.second.isAuto()) {
            intrinsicWidth = s.m_intrinsicContentSize.width();
            intrinsicHeight = s.m_intrinsicContentSize.height();
        } else if (a.first.isSpecified() && a.second.isAuto()) {
            intrinsicWidth = a.first.specifiedValue(parentContentWidth);
            intrinsicHeight = intrinsicWidth * (b.height() / b.width());
        } else if (a.first.isSpecified() && a.second.isFixed()) {
            intrinsicWidth = a.first.specifiedValue(parentContentWidth);
            intrinsicHeight = a.second.fixed();
        } else if (a.first.isSpecified() && a.second.isPercent()) {
            if (parentContentHeight.isFixed()) {
                intrinsicWidth = a.first.specifiedValue(parentContentWidth);
                intrinsicHeight = a.second.specifiedValue(parentContentHeight.fixed());
            } else {
                intrinsicWidth = a.first.specifiedValue(parentContentWidth);
                intrinsicHeight = intrinsicWidth * (b.height() / b.width());
            }
        } else if (a.first.isAuto() && a.second.isFixed()) {
            intrinsicHeight = a.second.fixed();
            intrinsicWidth = intrinsicHeight * (b.width() / b.height());
        } else {
            STARFISH_ASSERT(a.first.isAuto() && a.second.isPercent());
            if (parentContentHeight.isFixed()) {
                intrinsicHeight = a.second.specifiedValue(parentContentHeight.fixed());
                intrinsicWidth = intrinsicHeight * (b.width() / b.height());
            } else {
                intrinsicWidth = s.m_intrinsicContentSize.width();
                intrinsicHeight = s.m_intrinsicContentSize.height();
            }
            intrinsicHeight = a.second.fixed();
            intrinsicWidth = intrinsicHeight * (b.width() / b.height());
        }
    }

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
