#include "StarFishConfig.h"
#include "FrameBlockBox.h"
#include "FrameText.h"
#include "FrameInline.h"

namespace StarFish {

void FrameBlockBox::layout(LayoutContext& passedCtx)
{
    LayoutContext newCtx(this);
    LayoutContext& ctx = isEstablishesBlockFormattingContext() ? newCtx : passedCtx;

    if (isNormalFlow()) {
        // https://www.w3.org/TR/CSS2/visudet.html#the-width-property
        // Determine horizontal margins and width of this object.

        float parentContentWidth = ctx.parentContentWidth(this);
        computeBorderMarginPadding(parentContentWidth);

        // 'margin-left' + 'border-left-width' + 'padding-left' + 'width' + 'padding-right' + 'border-right-width' + 'margin-right' = width of containing block
        if (m_style->width().isAuto()) {
            if (m_flags.m_shouldComputePreferredWidth) {
                ComputePreferredWidthContext p(ctx, parentContentWidth);
                computePreferredWidth(p);
                setContentWidth(p.result());
            } else {
                float remainWidth = parentContentWidth;
                remainWidth -= marginWidth();
                remainWidth -= borderWidth();
                remainWidth -= paddingWidth();
                setContentWidth(remainWidth);
            }
        } else {
            if (m_style->width().isFixed()) {
                setContentWidth(m_style->width().fixed());
            } else {
                STARFISH_ASSERT(m_style->width().isPercent());
                setContentWidth(parentContentWidth * m_style->width().percent());
            }

            if (style()->marginLeft().isAuto() && style()->marginRight().isAuto()) {
                float remain = parentContentWidth;
                remain -= contentWidth();
                remain -= borderWidth();
                remain -= paddingWidth();
                if (remain > 0) {
                    setMarginLeft(remain / 2);
                    setMarginRight(remain / 2);
                }
            }
        }
    } else {
        float parentContentWidth = ctx.parentContentWidth(this);
        computeBorderMarginPadding(parentContentWidth);

        STARFISH_ASSERT(node() != nullptr);
        FrameBox* cb = ctx.containingBlock(this)->asFrameBox();
        FrameBox* parent = Frame::parent()->asFrameBox();
        auto absLoc = parent->absolutePoint(cb);
        float absX = absLoc.x() - cb->borderLeft();
        auto setAbsX = [&](float x) {
            setX(x - absX);
        };

        auto getPreferredWidth = [&](float parentWidth) -> float {
            STARFISH_ASSERT(style()->width().isAuto());
            ComputePreferredWidthContext p(ctx, parentWidth);
            computePreferredWidth(p);
            return p.result();
        };

        // 10.3.7 Absolutely positioned, non-replaced elements
        // The constraint that determines the used values for these elements is:
        // 'left' + 'margin-left' + 'border-left-width' + 'padding-left' + 'width' + 'padding-right' + 'border-right-width' + 'margin-right' + 'right' = width of containing block

        // Then, if the 'direction' property of the element establishing the static-position containing block is 'ltr' set 'left' to the static position and apply rule number three below;
        // otherwise, set 'right' to the static position and apply rule number one below.

        Length marginLeft = style()->marginLeft();
        Length marginRight = style()->marginRight();
        Length left = style()->left();
        Length right = style()->right();
        Length width = style()->width();

        float parentWidth = cb->contentWidth() + cb->paddingWidth();

        if (left.isAuto() && width.isAuto() && right.isAuto()) {
            // If all three of 'left', 'width', and 'right' are 'auto':
        } else if (!left.isAuto() && !width.isAuto() && !right.isAuto()) {
            // If none of the three is 'auto':
            // If both 'margin-left' and 'margin-right' are 'auto',
            // solve the equation under the extra constraint that the two margins get equal values,
            // unless this would make them negative, in which case when direction of the containing block is 'ltr' ('rtl'),
            // set 'margin-left' ('margin-right') to zero and solve for 'margin-right' ('margin-left').
            // If one of 'margin-left' or 'margin-right' is 'auto', solve the equation for that value.
            // If the values are over-constrained, ignore the value for 'left'
            // (in case the 'direction' property of the containing block is 'rtl') or 'right' (in case 'direction' is 'ltr') and solve for that value.
            if (style()->direction() == DirectionValue::LtrDirectionValue) {
                float computedLeft = left.specifiedValue(parentWidth);
                setAbsX(computedLeft);
            } else {
                // TODO direction == rtl
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        } else {
            // Otherwise, set 'auto' values for 'margin-left' and 'margin-right' to 0, and pick the one of the following six rules that applies.
            if (left.isAuto() && width.isAuto() && !right.isAuto()) {
                // 'left' and 'width' are 'auto' and 'right' is not 'auto', then the width is shrink-to-fit. Then solve for 'left'
                float w = getPreferredWidth(parentWidth);
                width = Length(Length::Fixed, w);
                setAbsX(parentWidth - right.specifiedValue(parentWidth) - w - paddingWidth() - borderWidth());
            } else if(left.isAuto() && right.isAuto() && !width.isAuto()) {
                // 'left' and 'right' are 'auto' and 'width' is not 'auto',
                // then if the 'direction' property of the element establishing the static-position containing block is 'ltr' set 'left' to the static position,
                // otherwise set 'right' to the static position. Then solve for 'left' (if 'direction is 'rtl') or 'right' (if 'direction' is 'ltr').
            } else if(width.isAuto() && right.isAuto() && !left.isAuto()) {
                // 'width' and 'right' are 'auto' and 'left' is not 'auto', then the width is shrink-to-fit . Then solve for 'right'
                float w = getPreferredWidth(parentWidth);
                width = Length(Length::Fixed, w);
                setAbsX(left.specifiedValue(parentWidth));
            } else if(left.isAuto() && !width.isAuto() && !right.isAuto()) {
                // 'left' is 'auto', 'width' and 'right' are not 'auto', then solve for 'left'
                float w = width.specifiedValue(parentWidth);
                setAbsX(parentWidth - right.specifiedValue(parentWidth) - w - paddingWidth() - borderWidth());
            } else if(width.isAuto() && !left.isAuto() && !right.isAuto()) {
                // 'width' is 'auto', 'left' and 'right' are not 'auto', then solve for 'width'
                float l = left.specifiedValue(parentWidth);
                float r = right.specifiedValue(parentWidth);
                float w = l - r + parentWidth;
                w = w - paddingWidth() - borderWidth();
                width = Length(Length::Fixed, w);
            } else {
                // 'right' is 'auto', 'left' and 'width' are not 'auto', then solve for 'right'
                STARFISH_ASSERT(right.isAuto() && !left.isAuto() && !width.isAuto());
                setAbsX(left.specifiedValue(parentWidth));
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

        if (width.isAuto()) {
            if (m_flags.m_shouldComputePreferredWidth) {
                ComputePreferredWidthContext p(ctx, parentWidth);
                computePreferredWidth(p);
                setContentWidth(p.result());
            } else {
                setContentWidth(parentWidth);
            }
        } else if (width.isFixed()) {
            setContentWidth(width.fixed());
        } else if (width.isPercent()) {
            setContentWidth(parentWidth * width.percent());
        }
    }

    float contentHeight;
    if (hasBlockFlow()) {
        contentHeight = layoutBlock(ctx);
    } else {
        contentHeight = layoutInline(ctx);
    }

    // Now the intrinsic height of the object is known because the children are placed

    // Determine the final height
    if (isNormalFlow()) {
        if (style()->height().isAuto()) {
            setContentHeight(contentHeight);
        } else if (style()->height().isFixed()) {
            setContentHeight(style()->height().fixed());
        } else {
            if (ctx.parentHasFixedHeight(this)) {
                setContentHeight(style()->height().percent() * ctx.parentFixedHeight(this));
            } else {
                setContentHeight(contentHeight);
            }
        }
    } else {
        FrameBox* cb = ctx.containingBlock(this)->asFrameBox();
        FrameBox* parent = Frame::parent()->asFrameBox();
        auto absLoc = parent->absolutePoint(cb);
        float absY = absLoc.y() - cb->borderTop();
        auto setAbsY = [&](float y) {
            setY(y - absY);
        };
        float parentHeight = cb->contentHeight() + cb->paddingHeight();

        Length marginTop = style()->marginTop();
        Length marginBottom = style()->marginBottom();
        Length top = style()->top();
        Length bottom = style()->bottom();
        Length height = style()->height();

        // 10.6.4 Absolutely positioned, non-replaced elements

        // For absolutely positioned elements, the used values of the vertical dimensions must satisfy this constraint:
        // 'top' + 'margin-top' + 'border-top-width' + 'padding-top' + 'height' + 'padding-bottom' + 'border-bottom-width' + 'margin-bottom' + 'bottom' = height of containing block

        if (top.isAuto() && height.isAuto() && bottom.isAuto()) {
            // If all three of 'top', 'height', and 'bottom' are auto, set 'top' to the static position and apply rule number three below.
            // TODO add margin-top, margin-bottom
        } else if(!top.isAuto() && !height.isAuto() && !bottom.isAuto()) {
            // If none of the three are 'auto': If both 'margin-top' and 'margin-bottom' are 'auto',
            // solve the equation under the extra constraint that the two margins get equal values.
            // If one of 'margin-top' or 'margin-bottom' is 'auto', solve the equation for that value.
            // If the values are over-constrained, ignore the value for 'bottom' and solve for that value.
            // TODO add margin-top, margin-bottom
            setAbsY(top.specifiedValue(parentHeight));
        } else if(top.isAuto() && height.isAuto() && !bottom.isAuto()) {
            // 'top' and 'height' are 'auto' and 'bottom' is not 'auto', then the height is based on the content per 10.6.7
            // set 'auto' values for 'margin-top' and 'margin-bottom' to 0, and solve for 'top'
            setContentHeight(contentHeight);
            // TODO add margin-top, margin-bottom
            setAbsY(parentHeight - contentHeight - paddingHeight() - borderHeight() - bottom.specifiedValue(parentHeight));
        } else if (top.isAuto() && bottom.isAuto() && !height.isAuto()) {
            // 'top' and 'bottom' are 'auto' and 'height' is not 'auto', then set 'top' to the static position
            // set 'auto' values for 'margin-top' and 'margin-bottom' to 0, and solve for 'bottom'
        } else if (height.isAuto() && bottom.isAuto() && !top.isAuto()) {
            // 'height' and 'bottom' are 'auto' and 'top' is not 'auto', then the height is based on the content per 10.6.7,
            // set 'auto' values for 'margin-top' and 'margin-bottom' to 0, and solve for 'bottom'
            setY(top.specifiedValue(parentHeight));
        } else if (top.isAuto() && !height.isAuto() && !bottom.isAuto()) {
            // 'top' is 'auto', 'height' and 'bottom' are not 'auto', then set 'auto' values for 'margin-top' and 'margin-bottom' to 0, and solve for 'top'
            setAbsY(parentHeight - height.specifiedValue(parentHeight) - paddingHeight() - borderHeight() - bottom.specifiedValue(parentHeight));
        } else if(height.isAuto() && !top.isAuto() && !bottom.isAuto()) {
            // 'height' is 'auto', 'top' and 'bottom' are not 'auto', then 'auto' values for 'margin-top' and 'margin-bottom' are set to 0 and solve for 'height'
            float t = top.specifiedValue(parentHeight);
            float b = bottom.specifiedValue(parentHeight);
            float h = t - b + parentHeight;
            h = h - paddingHeight() - borderHeight();
            height = Length(Length::Fixed, h);
            setAbsY(t);
        } else {
            // 'bottom' is 'auto', 'top' and 'height' are not 'auto', then set 'auto' values for 'margin-top' and 'margin-bottom' to 0 and solve for 'bottom'
            STARFISH_ASSERT(bottom.isAuto() && !top.isAuto() && !height.isAuto());
            setAbsY(top.specifiedValue(parentHeight));
        }

        if (height.isAuto()) {
            setContentHeight(contentHeight);
        } else {
            setContentHeight(height.specifiedValue(parentHeight));
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

    if (style()->position() == PositionValue::RelativePositionValue)
        ctx.registerRelativePositionedFrames(this);

    // layout absolute positioned blocks
    ctx.layoutRegisteredAbsolutePositionedFrames(this, [&](const std::vector<Frame*>& frames) {
        for (size_t i = 0; i < frames.size(); i ++) {
            Frame* f = frames[i];
            f->layout(ctx);
        }
    });

    // layout relative positioned blocks
    ctx.layoutRegisteredRelativePositionedFrames(this, [&](const std::vector<Frame*>& frames) {
        for (size_t i = 0; i < frames.size(); i ++) {
            Frame* f = frames[i];

            Length left = f->style()->left();
            Length right = f->style()->right();
            Length top = f->style()->top();
            Length bottom = f->style()->bottom();
            Frame* cb = ctx.containingBlock(f);
            float parentWidth = cb->asFrameBox()->contentWidth();
            float parentHeight = cb->asFrameBox()->contentHeight();
            float mX = 0;
            float mY = 0;

            // left, right
            if (!left.isAuto() && !right.isAuto()) {
                STARFISH_ASSERT(f->style()->direction() == LtrDirectionValue);
                mX = left.specifiedValue(parentWidth);
            } else if (!left.isAuto() && right.isAuto()) {
                mX = left.specifiedValue(parentWidth);
            } else if (left.isAuto() && !right.isAuto()) {
                mX = -right.specifiedValue(parentWidth);
            }

            // top, bottom
            if (!top.isAuto() && !bottom.isAuto()) {
                mY = top.specifiedValue(parentHeight);
            } else if (!top.isAuto() && bottom.isAuto()) {
                mY = top.specifiedValue(parentHeight);
            } else if (top.isAuto() && !bottom.isAuto()) {
                mY = -bottom.specifiedValue(parentHeight);
            }

            f->asFrameBox()->moveX(mX);
            f->asFrameBox()->moveY(mY);
        }
    });

    if (isEstablishesBlockFormattingContext()) {
        ctx.propagatePositionedFrames(passedCtx);
    }
}

Frame* FrameBlockBox::hitTest(float x, float y,HitTestStage stage)
{
    if (isEstablishesStackingContext()) {
        ASSERT(stage == HitTestStackingContext);
        Frame* result = nullptr;
        Frame* child;
        // TODO the child stacking contexts with positive stack levels (least positive first).

        // the child stacking contexts with stack level 0 and the positioned descendants with stack level 0.
        child = lastChild();
        while (child) {
            float cx = x - child->asFrameBox()->x();
            float cy = y - child->asFrameBox()->y();
            result = child->hitTest(cx ,cy , HitTestPositionedElements);
            if (result)
                return result;
            child = child->previous();
        }

        // the in-flow, inline-level, non-positioned descendants, including inline tables and inline blocks.
        child = lastChild();
        while (child) {
            float cx = x - child->asFrameBox()->x();
            float cy = y - child->asFrameBox()->y();
            result = child->hitTest(cx ,cy , HitTestNormalFlowInline);
            if (result)
                return result;
            child = child->previous();
        }

        // TODO the non-positioned floats.

        // the in-flow, non-inline-level, non-positioned descendants.
        ASSERT(hasBlockFlow());
        child = lastChild();
        while (child) {
            float cx = x - child->asFrameBox()->x();
            float cy = y - child->asFrameBox()->y();
            result = child->hitTest(cx, cy, HitTestNormalFlowBlock);
            if (result)
                return result;
            child = child->previous();
        }

        // the background and borders of the element forming the stacking context.
        result = FrameBox::hitTest(x, y, stage);
        if (result)
            return result;

        // TODO the child stacking contexts with negative stack levels (most negative first).
        return nullptr;
    } else if(isPositionedElement() && stage == HitTestPositionedElements) {
        Frame* result = nullptr;
        HitTestStage s = HitTestStage::HitTestPositionedElements;
        while (s != HitTestStageEnd) {
            if (hasBlockFlow()) {
                Frame* child = lastChild();
                while (child) {
                    float cx = x - child->asFrameBox()->x();
                    float cy = y - child->asFrameBox()->y();
                    result = child->hitTest(cx ,cy , s);
                    if (result)
                        return result;
                    child = child->previous();
                }
            } else {
                for (size_t i = 0; i < m_lineBoxes.size(); i ++) {
                    LineBox& b = *m_lineBoxes[i];
                    float cx = x - b.m_frameRect.x();
                    float cy = y - b.m_frameRect.y();

                    for (size_t k = 0; k < b.m_boxes.size(); k ++) {
                        FrameBox* childBox = b.m_boxes[k];
                        float cx2 = cx - childBox->x();
                        float cy2 = cy - childBox->y();
                        result = childBox->hitTest(cx2, cy2, s);
                        if (result)
                            return result;
                    }
                }
            }
            s = (HitTestStage)(s + 1);
        }

        return result;
    } else {
        if (stage == HitTestNormalFlowBlock) {
            if (hasBlockFlow()) {
                Frame* result = nullptr;
                Frame* child = lastChild();
                while (child) {
                    float cx = x - child->asFrameBox()->x();
                    float cy = y - child->asFrameBox()->y();
                    result = child->hitTest(cx ,cy , stage);
                    if (result)
                        return result;
                    child = child->previous();
                }

                return FrameBox::hitTest(x, y, stage);
            } else {
                return node() ? FrameBox::hitTest(x, y, stage) : nullptr;
            }
        } else {
            if (!hasBlockFlow()) {
                Frame* result = nullptr;
                for (size_t i = 0; i < m_lineBoxes.size(); i ++) {
                    LineBox& b = *m_lineBoxes[i];
                    float cx = x - b.m_frameRect.x();
                    float cy = y - b.m_frameRect.y();

                    for (size_t k = 0; k < b.m_boxes.size(); k ++) {
                        FrameBox* childBox = b.m_boxes[k];
                        float cx2 = cx - childBox->x();
                        float cy2 = cy - childBox->y();
                        result = childBox->hitTest(cx2, cy2, stage);
                        if (result)
                            return result;
                    }
                }

                return nullptr;
            } else {
                Frame* result = nullptr;
                Frame* child = lastChild();
                while (child) {
                    float cx = x - child->asFrameBox()->x();
                    float cy = y - child->asFrameBox()->y();
                    result = child->hitTest(cx ,cy , stage);
                    if (result)
                        return result;
                    child = child->previous();
                }
                return nullptr;
            }
        }
    }
    return nullptr;
}

void FrameBlockBox::paint(Canvas* canvas, PaintingStage stage)
{
    if (isEstablishesStackingContext()) {
        if (stage != PaintingStackingContext) {
            return;
        }
        // Within each stacking context, the following layers are painted in back-to-front order:

        // the background and borders of the element forming the stacking context.
        paintBackgroundAndBorders(canvas);

        // TODO the child stacking contexts with negative stack levels (most negative first).

        // the in-flow, non-inline-level, non-positioned descendants.
        ASSERT(hasBlockFlow());
        Frame* child = firstChild();
        while (child) {
            canvas->save();
            canvas->translate(child->asFrameBox()->x(), child->asFrameBox()->y());
            child->paint(canvas, PaintingNormalFlowBlock);
            canvas->restore();
            child = child->next();
        }

        // TODO the non-positioned floats.
        /*
        child = firstChild();
        while (child) {
            canvas->save();
            canvas->translate(child->asFrameBox()->x(), child->asFrameBox()->y());
            child->paint(canvas, PaintingNonPositionedFloats);
            canvas->restore();
            child = child->next();
        }
        */

        // the in-flow, inline-level, non-positioned descendants, including inline tables and inline blocks.
        child = firstChild();
        while (child) {
            canvas->save();
            canvas->translate(child->asFrameBox()->x(), child->asFrameBox()->y());
            child->paint(canvas, PaintingNormalFlowInline);
            canvas->restore();
            child = child->next();
        }

        // the child stacking contexts with stack level 0 and the positioned descendants with stack level 0.
        child = firstChild();
        while (child) {
            canvas->save();
            canvas->translate(child->asFrameBox()->x(), child->asFrameBox()->y());
            child->paint(canvas, PaintingPositionedElements);
            canvas->restore();
            child = child->next();
        }

        // TODO the child stacking contexts with positive stack levels (least positive first).
    } else {
        if (style()->overflow() == OverflowValue::HiddenOverflow) {
            canvas->save();
            canvas->clip(Rect(0, 0, width(), height()));
        }
        if(isPositionedElement()) {
            if (stage == PaintingPositionedElements) {
                paintBackgroundAndBorders(canvas);
                PaintingStage s = PaintingStage::PaintingStackingContext;
                while (s != PaintingStageEnd) {
                    paintChildrenWith(this, canvas, s);
                    s = (PaintingStage)(s + 1);
                }
            }
        } else {
            if (stage == PaintingNormalFlowBlock) {
                paintBackgroundAndBorders(canvas);
                paintChildrenWith(this, canvas, stage);
            } else {
                paintChildrenWith(this, canvas, stage);
            }
        }
        if (style()->overflow() == OverflowValue::HiddenOverflow) {
            canvas->restore();
        }
    }
}

void FrameBlockBox::dump(int depth)
{
    FrameBox::dump(depth);
    if (!hasBlockFlow()) {
        if (m_lineBoxes.size() && m_lineBoxes[0]->firstChild()) {
            for (size_t i = 0; i < m_lineBoxes.size(); i ++) {
                puts("");
                for(int k = 0; k < depth + 1; k ++)
                    printf("  ");
                printf("LineBox(%g,%g,%g,%g)\n", m_lineBoxes[i]->m_frameRect.x(), m_lineBoxes[i]->m_frameRect.y()
                    , m_lineBoxes[i]->m_frameRect.width(), m_lineBoxes[i]->m_frameRect.height());

                LineBox& lb = *m_lineBoxes[i];
                for (size_t k = 0; k < lb.m_boxes.size(); k ++) {
                    FrameBox* childBox = lb.m_boxes[k];
                    for(int k = 0; k < depth + 2; k ++)
                        printf("  ");
                    printf("%s", childBox->name());
                    childBox->dump(depth + 3);
                    if (k == lb.m_boxes.size() - 1)
                        puts("");
                }
            }
        }
    }
}


}
