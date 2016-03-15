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

    virtual void layout(LayoutContext& ctx)
    {
        LayoutSize s = intrinsicSize();
        LayoutUnit parentContentWidth = ctx.blockContainer(this)->asFrameBox()->contentWidth();
        computeBorderMarginPadding(parentContentWidth);

        if (isNormalFlow()) {
            if ((s.width() == 0 || s.height() == 0) && (style()->width().isAuto() || style()->height().isAuto())) {
                setContentWidth(0);
                setContentHeight(0);
            } else if (style()->width().isAuto() && style()->height().isAuto()) {
                setContentWidth(s.width());
                setContentHeight(s.height());
            } else if (style()->width().isSpecified() && style()->height().isAuto()) {
                LayoutUnit w = style()->width().specifiedValue(ctx.parentContentWidth(this));
                LayoutUnit h = w * (s.height() / s.width());
                setContentWidth(w);
                setContentHeight(h);
            } else if (style()->width().isAuto() && style()->height().isSpecified()) {
                if (style()->height().isFixed()) {
                    LayoutUnit h = style()->height().fixed();
                    LayoutUnit w = h * (s.width() / s.height());
                    setContentWidth(w);
                    setContentHeight(h);
                } else {
                    // FIXME
                    setContentWidth(s.width());
                    setContentHeight(s.height());
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
                        setContentHeight(s.height());
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
        } else {
            // The used value of 'width' is determined as for inline replaced elements.
            if ((s.width() == 0 || s.height() == 0) && (style()->width().isAuto() || style()->height().isAuto())) {
                setContentWidth(0);
                setContentHeight(0);
            } else if (style()->width().isAuto() && style()->height().isAuto()) {
                setContentWidth(s.width());
                setContentHeight(s.height());
            } else if (style()->width().isSpecified() && style()->height().isAuto()) {
                LayoutUnit w = style()->width().specifiedValue(ctx.containingBlock(this)->asFrameBox()->contentWidth());
                LayoutUnit h = w * (s.height() / s.width());
                setContentWidth(w);
                setContentHeight(h);
            } else if (style()->width().isAuto() && style()->height().isSpecified()) {
                if (style()->height().isFixed()) {
                    LayoutUnit h = style()->height().fixed();
                    LayoutUnit w = h * (s.width() / s.height());
                    setContentWidth(w);
                    setContentHeight(h);
                } else {
                    // FIXME
                    setContentWidth(s.width());
                    setContentHeight(s.height());
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
                        setContentHeight(s.height());
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
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
            }

            if (!marginLeft.isAuto() && !marginRight.isAuto()) {
                if (style()->direction() == LtrDirectionValue) {
                    moveX(FrameBox::marginLeft());
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
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

    virtual LayoutSize intrinsicSize() = 0;

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
