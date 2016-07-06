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
        std::pair<Length, Length> s = intrinsicSize();
        LayoutUnit parentContentWidth = ctx.blockContainer(this)->asFrameBox()->contentWidth();
        computeBorderMarginPadding(parentContentWidth);
        LayoutUnit intrinsicWidth;
        LayoutUnit intrinsicHeight;
        if (s.first.isFixed()) {
            intrinsicWidth = s.first.fixed();
        } else {
            intrinsicWidth = s.first.percent() * parentContentWidth;
        }
        if (s.second.isFixed()) {
            intrinsicHeight = s.second.fixed();
        } else {
            intrinsicHeight = s.second.percent() * parentContentWidth;
        }

        if (isNormalFlow()) {
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
                } else if (!style()->marginLeft().isAuto() && !style()->marginRight().isAuto()) {
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
            // The used value of 'width' is determined as for inline replaced elements.
            if ((intrinsicWidth == 0 || intrinsicHeight == 0) && (style()->width().isAuto() || style()->height().isAuto())) {
                setContentWidth(0);
                setContentHeight(0);
            } else if (style()->width().isAuto() && style()->height().isAuto()) {
                setContentWidth(intrinsicWidth);
                setContentHeight(intrinsicHeight);
            } else if (style()->width().isSpecified() && style()->height().isAuto()) {
                LayoutUnit w = style()->width().specifiedValue(ctx.containingBlock(this)->asFrameBox()->contentWidth());
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
                    // FIXME
                    setContentWidth(intrinsicWidth);
                    setContentHeight(intrinsicHeight);
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

            FrameBox* cb = ctx.containingBlock(this)->asFrameBox();
            auto absLoc = layoutParent()->asFrameBox()->absolutePoint(cb);
            LayoutUnit absX = absLoc.x() - cb->borderLeft();
            auto setAbsX = [&](LayoutUnit x)
            {
                setX(x - absX);
            };

            LayoutUnit absY = absLoc.y() - cb->borderTop();
            auto setAbsY = [&](LayoutUnit y)
            {
                setY(y - absY);
            };

            LayoutUnit parentWidth = cb->contentWidth() + cb->paddingWidth();

            Length marginLeft = style()->marginLeft();
            Length marginRight = style()->marginRight();

            // If 'margin-left' or 'margin-right' is specified as 'auto' its used value is determined by the rules below.
            // If both 'left' and 'right' have the value 'auto'
            // then if the 'direction' property of the element establishing the static-position containing block is 'ltr', set 'left' to the static position;
            // else if 'direction' is 'rtl', set 'right' to the static position.
            if (style()->left().isAuto() && style()->right().isAuto()) {
                // static location computed in normal flow processing
            } else if (!style()->left().isAuto() && style()->right().isAuto()) {
                setAbsX(style()->left().specifiedValue(parentWidth));
            } else if (style()->left().isAuto() && !style()->right().isAuto()) {
                LayoutUnit r = style()->right().specifiedValue(parentWidth);
                setAbsX(parentWidth - r - width());
            } else {
                if (style()->direction() == LtrDirectionValue) {
                    setAbsX(style()->left().specifiedValue(parentWidth));
                } else {
                    LayoutUnit computedRight = style()->right().specifiedValue(parentWidth);
                    setAbsX(parentWidth - FrameBox::width() - computedRight);
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

            LayoutUnit parentHeight = cb->contentHeight() + cb->paddingHeight();
            Length marginTop = style()->marginTop();
            Length marginBottom = style()->marginBottom();

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

    virtual void computePreferredWidth(ComputePreferredWidthContext& ctx)
    {
        std::pair<Length, Length> s = intrinsicSize();
        LayoutUnit parentContentWidth = ctx.layoutContext().blockContainer(this)->asFrameBox()->contentWidth();
        LayoutUnit intrinsicWidth;
        LayoutUnit intrinsicHeight;
        if (s.first.isFixed()) {
            intrinsicWidth = s.first.fixed();
        } else {
            intrinsicWidth = s.first.percent() * parentContentWidth;
        }
        if (s.second.isFixed()) {
            intrinsicHeight = s.second.fixed();
        } else {
            intrinsicHeight = s.second.percent() * parentContentWidth;
        }

        if (style()->width().isAuto() && style()->height().isAuto()) {
            ctx.setResult(intrinsicWidth);
        } else if (style()->width().isSpecified()) {
            if (style()->width().isFixed()) {
                ctx.setResult(style()->width().fixed());
            } else {
                ctx.setResult(intrinsicWidth);
            }
        } else if (style()->height().isSpecified()) {
            if (style()->height().isFixed()) {
                LayoutUnit h = style()->height().fixed();
                LayoutUnit w = h * (intrinsicWidth / intrinsicHeight);
                ctx.setResult(w);
            } else {
                ctx.setResult(intrinsicWidth);
            }
        } else {
            // TODO support width, height attribute
            ctx.setResult(intrinsicWidth);
        }
    }

    virtual std::pair<Length, Length> intrinsicSize() = 0;

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
