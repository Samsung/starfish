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
        FrameBox* parent = m_parent->asFrameBox();
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
        FrameBox* parent = m_parent->asFrameBox();
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

float FrameBlockBox::layoutBlock(LayoutContext& ctx)
{
    float top = paddingTop() + borderTop();
    float normalFlowHeight = 0;
    Frame* child = firstChild();
    while (child) {
        // Place the child.
        child->asFrameBox()->setX(paddingLeft() + borderLeft());
        child->asFrameBox()->setY(normalFlowHeight + top);

        // Lay out the child
        if (child->isNormalFlow()) {
            child->layout(ctx);
            Length marginLeft = child->style()->marginLeft();
            Length marginRight = child->style()->marginRight();
            float mX = 0;
            if (!marginLeft.isAuto() && !marginRight.isAuto()) {
                // FIXME what "direction value" should we watch? self? child?
                if (style()->direction() == LtrDirectionValue) {
                    mX = child->asFrameBox()->marginLeft();
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
            } else if (marginLeft.isAuto() && !marginRight.isAuto()) {
                mX = contentWidth() - child->asFrameBox()->width();
                mX -= child->asFrameBox()->marginRight();
            } else if (!marginLeft.isAuto() && marginRight.isAuto()) {
                mX = child->asFrameBox()->marginLeft();
            } else {
                // auto-auto
                mX = child->asFrameBox()->marginLeft();
            }
            child->asFrameBox()->moveX(mX);
            // TODO implement margin-collapse
            child->asFrameBox()->moveY(child->asFrameBox()->marginTop());
        }

        if (child->isNormalFlow()) {
            // TODO implement margin-collapse
            normalFlowHeight = child->asFrameBox()->height() + child->asFrameBox()->y() + child->asFrameBox()->marginBottom() - top;
        } else
            ctx.registerAbsolutePositionedFrames(child);

        child = child->next();
    }
    return normalFlowHeight;
}

template <typename fn>
void textDividerForLayout(String* txt, fn f)
{
    // TODO consider white-space
    // TODO consider letter-spacing
    unsigned offset = 0;
    while(true) {
        if(offset>=txt->length())
            break;
        bool isWhiteSpace = false;
        if(String::isSpaceOrNewline(txt->charAt(offset))) {
            isWhiteSpace = true;
        }

        // find next space
        unsigned nextOffset = offset+1;
        if(isWhiteSpace) {
            while(nextOffset < txt->length() && String::isSpaceOrNewline((*txt)[nextOffset])) {
                nextOffset++;
            }
        }
        else {
            while(nextOffset < txt->length() && !String::isSpaceOrNewline((*txt)[nextOffset])) {
                nextOffset++;
            }
        }

        f(offset, nextOffset, isWhiteSpace);
        offset = nextOffset;
    }
}

float FrameBlockBox::layoutInline(LayoutContext& ctx)
{
    float lineBoxX = paddingLeft() + borderLeft();
    float lineBoxY = paddingTop() + borderTop();
    float inlineContentWidth = contentWidth();
    LineFormattingContext lineFormattingContext(*this, ctx);

    Frame* f = firstChild();
    while(f) {
        if (!f->isNormalFlow()) {
            lineFormattingContext.currentLine()->m_boxes.push_back(f->asFrameBox());
            f->setLayoutParent(lineFormattingContext.currentLine());
            ctx.registerAbsolutePositionedFrames(f->asFrameBox());
            f = f->next();
            continue;
        }

        if (f->isFrameText()) {
            String* txt = f->asFrameText()->text();
            //fast path
            if (f == lastChild()) {
                if (txt->containsOnlyWhitespace()) {
                    f = f->next();
                    continue;
                }
            }

            textDividerForLayout(txt, [&](size_t offset, size_t nextOffset, bool isWhiteSpace) {
                textAppendRetry:
                if (lineFormattingContext.m_currentLineWidth == 0 && isWhiteSpace) {
                    return;
                }
                if (f == lastChild() && offset != 0 && nextOffset == txt->length() && isWhiteSpace) {
                    return;
                }

                float textWidth;
                String* resultString;
                if(isWhiteSpace) {
                    resultString = String::spaceString;
                    textWidth = f->style()->font()->spaceWidth();
                }
                else {
                    String* ss = txt->substring(offset,nextOffset - offset);
                    resultString = ss;
                    textWidth = f->style()->font()->measureText(ss);
                }

                if(textWidth <= inlineContentWidth - lineFormattingContext.m_currentLineWidth || lineFormattingContext.m_currentLineWidth == 0) {
                    lineFormattingContext.m_currentLineWidth += textWidth;
                } else {
                    if (isWhiteSpace)
                        return;
                    // try this at nextline
                    lineFormattingContext.breakLine();
                    goto textAppendRetry;
                }

                lineFormattingContext.currentLine()->m_boxes.push_back(new InlineTextBox(f->node(), f->style(), lineFormattingContext.currentLine(), resultString));
                lineFormattingContext.currentLine()->m_boxes.back()->setWidth(textWidth);
                lineFormattingContext.currentLine()->m_boxes.back()->setHeight(f->style()->font()->metrics().m_fontHeight);
                lineFormattingContext.registerInlineContent();
            });

        } else if (f->isFrameReplaced()) {
            FrameReplaced* r = f->asFrameReplaced();
            r->layout(ctx);

            insertReplacedBox:
            if ((r->width() + r->marginWidth()) <= (inlineContentWidth - lineFormattingContext.m_currentLineWidth) || lineFormattingContext.m_currentLineWidth == 0) {
                lineFormattingContext.currentLine()->m_boxes.push_back(new InlineReplacedBox(f->node(), f->style(), lineFormattingContext.currentLine(), r));
                lineFormattingContext.currentLine()->m_boxes.back()->setMarginLeft(r->marginLeft());
                lineFormattingContext.currentLine()->m_boxes.back()->setMarginTop(r->marginTop());
                lineFormattingContext.currentLine()->m_boxes.back()->setMarginRight(r->marginRight());
                lineFormattingContext.currentLine()->m_boxes.back()->setMarginBottom(r->marginBottom());
                lineFormattingContext.currentLine()->m_boxes.back()->setWidth(r->width());
                lineFormattingContext.currentLine()->m_boxes.back()->setHeight(r->height());
                lineFormattingContext.m_currentLineWidth += (r->width() + r->marginWidth());
                lineFormattingContext.registerInlineContent();
            } else {
                lineFormattingContext.breakLine();
                goto insertReplacedBox;
            }
        } else if (f->isFrameBlockBox()) {
            // inline-block
            FrameBlockBox* r = f->asFrameBlockBox();
            f->layout(ctx);

            float ascender = 0;
            if (ctx.lastLineBox() && r->isAncestorOf(ctx.lastLineBox())) {
                float topToLineBox = ctx.lastLineBox()->absolutePoint(r).y();
                ascender = topToLineBox + ctx.lastLineBox()->m_ascender;
            } else {
                ascender = f->asFrameBox()->height();
            }

            insertBlockBox:
            if ((r->width() + r->marginWidth()) <= (inlineContentWidth - lineFormattingContext.m_currentLineWidth) || lineFormattingContext.m_currentLineWidth == 0) {
                lineFormattingContext.currentLine()->m_boxes.push_back(new InlineBlockBox(f->node(), f->style(), lineFormattingContext.currentLine(), r, ascender));
                lineFormattingContext.currentLine()->m_boxes.back()->setMarginLeft(r->marginLeft());
                lineFormattingContext.currentLine()->m_boxes.back()->setMarginTop(r->marginTop());
                lineFormattingContext.currentLine()->m_boxes.back()->setMarginRight(r->marginRight());
                lineFormattingContext.currentLine()->m_boxes.back()->setMarginBottom(r->marginBottom());
                lineFormattingContext.currentLine()->m_boxes.back()->setWidth(r->width());
                lineFormattingContext.currentLine()->m_boxes.back()->setHeight(r->height());
                lineFormattingContext.m_currentLineWidth += (r->width() + r->marginWidth());
                lineFormattingContext.registerInlineContent();
            } else {
                lineFormattingContext.breakLine();
                goto insertBlockBox;
            }
        } else if (f->isFrameLineBreak()) {
            lineFormattingContext.breakLine(true);
        } else if (f->isFrameInline()) {
            InlineNonReplacedBox* inlineBox = new InlineNonReplacedBox(f->node(), f->node()->style(), nullptr, f->asFrameInline());
            InlineNonReplacedBox::layoutInline(inlineBox, ctx, this, &lineFormattingContext, nullptr, true);
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }

        f = f->next();
    }

    // position each line
    float contentHeight = 0;
    for (size_t i = 0; i < m_lineBoxes.size(); i ++) {
        LineBox& b = *m_lineBoxes[i];

        b.m_frameRect.setX(lineBoxX);
        b.m_frameRect.setY(contentHeight + lineBoxY);

        // align boxes
        float x = 0;
        for (size_t j = 0; j < b.m_boxes.size(); j ++) {
            b.m_boxes[j]->setX(x + b.m_boxes[j]->marginLeft());
            if (b.m_boxes[j]->isNormalFlow())
                x += b.m_boxes[j]->width() + b.m_boxes[j]->marginWidth();
        }

        // text align
        if (style()->textAlign() == TextAlignValue::LeftTextAlignValue) {
        } else if (style()->textAlign() == TextAlignValue::RightTextAlignValue) {
            float diff = (inlineContentWidth - x);
            for (size_t j = 0; j < b.m_boxes.size(); j ++) {
                FrameBox* box = b.m_boxes[j];
                box->moveX(diff);
            }
        } else if (style()->textAlign() == TextAlignValue::JustifyTextAlignValue) {
            if (lineFormattingContext.isBreakedLineWithoutBR(i)) {
                float remainSpace = (inlineContentWidth - x);
                if (remainSpace > 0) {
                    size_t spaceBoxCnt = 0;
                    for (size_t j = 0; j < b.m_boxes.size(); j ++) {
                        FrameBox* box = b.m_boxes[j];
                        if (box->isInlineBox()) {
                            if (box->asInlineBox()->isInlineTextBox()) {
                                String* str = box->asInlineBox()->asInlineTextBox()->text();
                                if (str->equals(str)) {
                                    spaceBoxCnt++;
                                }
                            }
                        }
                    }

                    if (spaceBoxCnt) {
                        float moreWidthForSpace = remainSpace / spaceBoxCnt;
                        float diff = 0;
                        for (size_t j = 0; j < b.m_boxes.size(); j ++) {
                            FrameBox* box = b.m_boxes[j];
                            box->moveX(diff);
                            if (box->isInlineBox()) {
                                if (box->asInlineBox()->isInlineTextBox()) {
                                    String* str = box->asInlineBox()->asInlineTextBox()->text();
                                    if (str->equals(str)) {
                                        diff += moreWidthForSpace;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            STARFISH_ASSERT(style()->textAlign() == TextAlignValue::CenterTextAlignValue);
            float diff = (inlineContentWidth - x) / 2;
            for (size_t j = 0; j < b.m_boxes.size(); j ++) {
                FrameBox* box = b.m_boxes[j];
                box->setX(box->x() + diff);
            }
        }

        // TODO vertical align(length, text-top.., etc)
        bool hasSpecialCase = false;
        float maxAscender = 0;
        float maxDescender = 0;
        float mostBiggestBoxHeight = 0;

        for (size_t j = 0; j < b.m_boxes.size(); j ++) {
            if (b.m_boxes[j]->isInlineBox()) {
                // normal flow
                InlineBox* ib = b.m_boxes[j]->asInlineBox();

                if (ib->isInlineTextBox()) {
                    maxAscender = std::max(ib->asInlineTextBox()->style()->font()->metrics().m_ascender, maxAscender);
                    maxDescender = std::min(ib->asInlineTextBox()->style()->font()->metrics().m_descender, maxDescender);
                    continue;
                }
                VerticalAlignValue va = ib->style()->verticalAlign();
                if (va == VerticalAlignValue::BaselineVAlignValue) {
                    if (ib->isInlineReplacedBox()) {
                        float asc = ib->marginTop() + ib->asInlineReplacedBox()->replacedBox()->borderTop() + ib->asInlineReplacedBox()->replacedBox()->paddingTop()
                                + ib->asInlineReplacedBox()->replacedBox()->contentHeight();
                        float dec = -(ib->asInlineReplacedBox()->replacedBox()->paddingBottom() + ib->asInlineReplacedBox()->replacedBox()->borderBottom()
                                + ib->marginBottom());
                        maxAscender = std::max(asc, maxAscender);
                        maxDescender = std::min(dec, maxDescender);
                    } else if (ib->isInlineBlockBox()) {
                        if (ib->asInlineBlockBox()->m_ascender == ib->height()) {
                            maxAscender = std::max(ib->asInlineBlockBox()->m_ascender + ib->marginHeight(), maxAscender);
                            maxDescender = std::min(0.f, maxDescender);
                        } else {
                            float dec = -(ib->height() - ib->asInlineBlockBox()->m_ascender) - ib->marginBottom();
                            maxAscender = std::max(ib->asInlineBlockBox()->m_ascender + ib->marginTop(), maxAscender);
                            maxDescender = std::min(dec, maxDescender);
                        }
                    } else {
                        STARFISH_ASSERT(ib->isInlineNonReplacedBox());
                        maxAscender = std::max(ib->asInlineNonReplacedBox()->ascender(), maxAscender);
                        maxDescender = std::min(ib->asInlineNonReplacedBox()->decender(), maxDescender);
                    }
                } else if (va == VerticalAlignValue::MiddleVAlignValue) {
                    float height = ib->height();
                    maxAscender = std::max(height/2 + ib->marginTop(), maxAscender);
                    maxDescender = std::min(-height/2 - ib->marginBottom(), maxDescender);
                } else if (va == VerticalAlignValue::TopVAlignValue || va == VerticalAlignValue::BottomVAlignValue) {
                    hasSpecialCase = true;
                    float height = ib->height() + ib->marginHeight();
                    mostBiggestBoxHeight = std::max(mostBiggestBoxHeight, height);
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }

            } else {
                // out of flow boxes
                b.m_boxes[j]->setY(b.m_boxes[j]->marginTop());
                continue;
            }
        }

        if (hasSpecialCase) {
            while((maxAscender - maxDescender) < mostBiggestBoxHeight) {
                for (size_t j = 0; j < b.m_boxes.size(); j ++) {
                    InlineBox* ib = b.m_boxes[j]->asInlineBox();
                    VerticalAlignValue va = ib->style()->verticalAlign();
                    float height = ib->height() + ib->marginHeight();
                    if (height > maxAscender - maxDescender) {
                        if (va == VerticalAlignValue::TopVAlignValue) {
                            float dec = -height + maxAscender;
                            maxDescender = std::min(maxDescender, dec);
                        } else if (va == VerticalAlignValue::BottomVAlignValue) {
                            float asc = height + maxDescender;
                            maxAscender = std::max(maxAscender, asc);
                        }
                    }
                }
            }
        }
        b.m_ascender = maxAscender;
        b.m_descender = maxDescender;

        float height = maxAscender - maxDescender;
        for (size_t j = 0; j < b.m_boxes.size(); j ++) {
            if (b.m_boxes[j]->isInlineBox()) {
                InlineBox* ib = b.m_boxes[j]->asInlineBox();
                if (ib->isInlineTextBox()) {
                    ib->setY(height + maxDescender - ib->height() - ib->asInlineTextBox()->style()->font()->metrics().m_descender);
                } else {
                    VerticalAlignValue va = ib->style()->verticalAlign();
                    if (va == VerticalAlignValue::BaselineVAlignValue) {
                        if (ib->isInlineReplacedBox()) {
                            float asc = ib->marginTop() + ib->asInlineReplacedBox()->replacedBox()->borderTop() + ib->asInlineReplacedBox()->replacedBox()->paddingTop()
                                        + ib->asInlineReplacedBox()->replacedBox()->contentHeight();
                            ib->setY(height + maxDescender - asc + ib->marginTop());
                        } else if (ib->isInlineBlockBox()) {
                            if (ib->asInlineBlockBox()->m_ascender == ib->height()) {
                                float dec = 0;
                                ib->setY(height + maxDescender - ib->height() - dec - ib->marginTop());
                            } else {
                                float dec = -(ib->height() - ib->asInlineBlockBox()->m_ascender) - ib->marginBottom();
                                ib->setY(height + maxDescender - ib->height() - dec - ib->marginTop());
                            }
                        } else {
                            STARFISH_ASSERT(ib->isInlineNonReplacedBox());
                            ib->setY(height + maxDescender - ib->height() - ib->asInlineNonReplacedBox()->decender());
                        }
                    } else if (va == VerticalAlignValue::MiddleVAlignValue) {
                        float dec = -(ib->height() / 2);
                        ib->setY(height + maxDescender - ib->height() - dec - ib->marginTop());
                    } else if (va == VerticalAlignValue::TopVAlignValue) {
                        ib->setY(ib->marginTop());
                    } else if (va == VerticalAlignValue::BottomVAlignValue) {
                        ib->setY(height - ib->height() - ib->marginBottom());
                    } else {
                        STARFISH_RELEASE_ASSERT_NOT_REACHED();
                    }
                }

                if (ib->style()->position() == PositionValue::RelativePositionValue)
                    ctx.registerRelativePositionedFrames(ib);
            } else {
                // out of flow boxes
            }
        }

        b.m_frameRect.setWidth(inlineContentWidth);
        b.m_frameRect.setHeight(height);
        contentHeight += b.m_frameRect.height();
    }

    return contentHeight;
}

bool isInlineBox(Frame* f)
{
    return f->isFrameText() || f->isFrameReplaced() || f->isFrameBlockBox() || f->isFrameLineBreak();
}

void collectInlineBox(std::vector<Frame*>& result, Frame* current, Frame* top)
{
    if (current != top) {
        if (isInlineBox(current)) {
            result.push_back(current);
            return;
        }
    }

    Frame* f = current->firstChild();
    while (f) {
        collectInlineBox(result, f, top);
        f = f->next();
    }
}

void FrameBlockBox::computePreferredWidth(ComputePreferredWidthContext& ctx)
{
    float remainWidth = ctx.lastKnownWidth();
    float minWidth = 0;

    if (style()->marginLeft().isFixed())
        minWidth += style()->marginLeft().fixed();
    if (style()->marginRight().isFixed())
        minWidth += style()->marginRight().fixed();
    if (style()->borderLeftWidth().isFixed())
        minWidth += style()->borderLeftWidth().fixed();
    if (style()->borderRightWidth().isFixed())
        minWidth += style()->borderRightWidth().fixed();
    if (style()->paddingLeft().isFixed())
        minWidth += style()->paddingLeft().fixed();
    if (style()->paddingRight().isFixed())
        minWidth += style()->paddingRight().fixed();
    ctx.setResult(minWidth);

    if (hasBlockFlow()) {
        if (style()->width().isFixed()) {
            minWidth += style()->width().fixed();
            ctx.setResult(minWidth);
        } else if (ctx.lastKnownWidth() - minWidth > 0) {
            Frame* child = firstChild();
            while (child) {
                if (child->isNormalFlow()) {
                    ComputePreferredWidthContext newCtx(ctx.layoutContext(), ctx.lastKnownWidth() - minWidth);
                    child->computePreferredWidth(newCtx);
                    ctx.setResult(newCtx.result() + minWidth);
                }
                child = child->next();
            }
        }

    } else {

        float currentLineWidth = 0;
        std::function<void(Frame*)> computeInlineLayout = [&](Frame* f) {

            // current
            if (!f->isNormalFlow()) {
                return;
            }

            if (f->isFrameText()) {
                String* s = f->asFrameText()->text();
                textDividerForLayout(s, [&](size_t offset, size_t nextOffset, bool isWhiteSpace){
                    if (currentLineWidth == 0 && isWhiteSpace) {
                        return;
                    }
                    if (f->parent()->lastChild() == f && offset != 0 && nextOffset == s->length() && isWhiteSpace) {
                        return;
                    }

                    float w = 0;
                    if (isWhiteSpace) {
                        w = f->style()->font()->spaceWidth();
                    } else {
                        w = f->style()->font()->measureText(s->substring(offset, nextOffset - offset));
                    }

                    if (currentLineWidth + w < remainWidth) {
                        currentLineWidth += w;
                    } else {
                        // CHECK THIS
                        // ctx.setResult(currentLineWidth);
                        ctx.setResult(remainWidth);
                        currentLineWidth = 0;
                    }

                    if (w > remainWidth) {
                        ctx.setResult(currentLineWidth);
                        currentLineWidth = 0;
                    }
                });
            } else if (f->isFrameBlockBox()) {
                ComputePreferredWidthContext newCtx(ctx.layoutContext(), remainWidth - minWidth);
                f->computePreferredWidth(newCtx);
                ctx.setResult(newCtx.result() + minWidth);
            } else if (f->isFrameLineBreak()) {
                // linebreaks
                ctx.setResult(currentLineWidth);
                currentLineWidth = 0;
            } else if (f->isFrameInline()) {
                auto checkMBP = [&](Length l) {
                    if (l.isFixed()) {
                        if (currentLineWidth + l.fixed() < remainWidth) {
                            currentLineWidth += l.fixed();
                        } else {
                            // CHECK THIS
                            // ctx.setResult(currentLineWidth);
                            ctx.setResult(remainWidth);
                            currentLineWidth = l.fixed();
                        }
                    }
                };

                checkMBP(f->style()->marginLeft());
                checkMBP(f->style()->borderLeftWidth());
                checkMBP(f->style()->paddingLeft());
                checkMBP(f->style()->paddingRight());
                checkMBP(f->style()->borderRightWidth());
                checkMBP(f->style()->marginRight());

            } else {
                STARFISH_ASSERT(f->isFrameReplaced());
                float w = f->asFrameReplaced()->intrinsicSize().width();

                if (f->style()->width().isFixed()) {
                    w = f->style()->width().fixed();
                }

                ctx.setResult(w);

                if (currentLineWidth + w < remainWidth) {
                    currentLineWidth += w;
                } else {
                    ctx.setResult(currentLineWidth);
                    currentLineWidth = w;
                }

                if (currentLineWidth > remainWidth) {
                    // linebreaks
                    ctx.setResult(currentLineWidth);
                    currentLineWidth = 0;
                }
            }

            Frame* c = f->firstChild();
            while (c) {
                computeInlineLayout(c);
                c = c->next();
            }

        };

        Frame* c = this->firstChild();
        while (c) {
            computeInlineLayout(c);
            c = c->next();
        }
        ctx.setResult(currentLineWidth);
    }
}

void FrameBlockBox::paintChildrenWith(FrameBlockBox* block, Canvas* canvas, PaintingStage stage)
{
    if (block->hasBlockFlow()) {
        Frame* child = block->firstChild();
        while (child) {
            canvas->save();
            canvas->translate(child->asFrameBox()->x(), child->asFrameBox()->y());
            child->paint(canvas, stage);
            canvas->restore();
            child = child->next();
        }
    } else {
        for (size_t i = 0; i < block->m_lineBoxes.size(); i ++) {
            canvas->save();
            LineBox& b = *block->m_lineBoxes[i];
            canvas->translate(b.frameRect().x(), b.frameRect().y());
            for (size_t j = 0; j < b.m_boxes.size(); j ++) {
                canvas->save();
                canvas->translate(b.m_boxes[j]->x(), b.m_boxes[j]->y());
                b.m_boxes[j]->paint(canvas, stage);
                canvas->restore();
            }
            canvas->restore();
        }
    }
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

                    for (size_t j = 0; j < b.m_boxes.size(); j ++) {
                        float cx2 = cx - b.m_boxes[j]->x();
                        float cy2 = cy - b.m_boxes[j]->y();
                        result = b.m_boxes[j]->hitTest(cx2, cy2, s);
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

                    for (size_t j = 0; j < b.m_boxes.size(); j ++) {
                        float cx2 = cx - b.m_boxes[j]->x();
                        float cy2 = cy - b.m_boxes[j]->y();
                        result = b.m_boxes[j]->hitTest(cx2, cy2, stage);
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

void InlineBlockBox::paint(Canvas* canvas, PaintingStage stage)
{
    if (stage == PaintingNormalFlowInline) {
        STARFISH_ASSERT(!m_frameBlockBox->isEstablishesStackingContext());
        PaintingStage s = PaintingStage::PaintingNormalFlowBlock;
        while (s != PaintingStageEnd) {
            m_frameBlockBox->paint(canvas, s);
            s = (PaintingStage)(s + 1);
        }
    }
}

Frame* InlineBlockBox::hitTest(float x, float y, HitTestStage stage)
{
    if (stage == HitTestStage::HitTestNormalFlowInline) {
        Frame* result = nullptr;
        HitTestStage s = HitTestStage::HitTestPositionedElements;
        while (s != HitTestStageEnd) {
            result = m_frameBlockBox->hitTest(x, y, s);
            if (result)
                return result;
            s = (HitTestStage)(s + 1);
        }
    }
    return nullptr;
}

InlineNonReplacedBox* InlineNonReplacedBox::layoutInline(InlineNonReplacedBox* self, LayoutContext& ctx,
        FrameBlockBox* blockBox, LineFormattingContext* lfc, FrameBox* layoutParentBox, bool freshStart)
{
    LineFormattingContext& lineFormattingContext = *lfc;
    // just lay out children are horizontally.
    float inlineContentWidth = blockBox->contentWidth();

    if (freshStart)
        self->computeBorderMarginPadding(inlineContentWidth);
    float w = self->marginLeft() + self->borderLeft() + self->paddingLeft();
    if (freshStart) {
        self->m_orgBorder = self->m_border;
        self->m_orgPadding = self->m_padding;
        self->m_orgMargin = self->m_margin;

        self->setMarginTop(0);
        self->setMarginBottom(0);
        self->setBorderTop(0);
        self->setBorderBottom(0);
        self->setPaddingTop(0);
        self->setPaddingBottom(0);

        if (w > (inlineContentWidth - lineFormattingContext.m_currentLineWidth) && layoutParentBox == nullptr) {
            lineFormattingContext.breakLine();
        }

        if (!layoutParentBox) {
            lineFormattingContext.currentLine()->m_boxes.push_back(self);
            self->setParent(lineFormattingContext.currentLine());
        } else {
            layoutParentBox->appendChild(self);
        }

        lineFormattingContext.registerInlineContent();
    }


    lineFormattingContext.m_currentLineWidth += w;
    self->setWidth(self->paddingLeft() + self->borderLeft());

    auto breakLine = [&]() {
        std::vector<InlineNonReplacedBox*> stack;

        FrameBox* addingUpWidth = nullptr;
        Frame* currentSelf = self;
        while (!currentSelf->asFrameBox()->isLineBox()) {
            stack.push_back(currentSelf->asFrameBox()->asInlineBox()->asInlineNonReplacedBox());

            if (addingUpWidth) {
                currentSelf->asFrameBox()->setWidth(currentSelf->asFrameBox()->width() + addingUpWidth->width() + addingUpWidth->marginWidth());
            }

            addingUpWidth = currentSelf->asFrameBox();
            currentSelf = currentSelf->parent();
        }

        lineFormattingContext.breakLine();

        bool first = true;
        auto iter = stack.rbegin();
        InlineNonReplacedBox* last = nullptr;
        while (stack.rend() != iter) {
            InlineNonReplacedBox* origin = *iter;
            InlineNonReplacedBox* newBox = new InlineNonReplacedBox(origin->node(), origin->node()->style(), nullptr, origin->m_origin);

            if (first) {
                newBox->setParent(lineFormattingContext.currentLine());
                lineFormattingContext.currentLine()->m_boxes.push_back(newBox);
                first = false;
            } else {
                last->appendChild(newBox);
            }
            last = newBox;

            newBox->m_margin.setRight(origin->m_orgMargin.right());
            newBox->m_border.setRight(origin->m_orgBorder.right());
            newBox->m_padding.setRight(origin->m_orgPadding.right());

            newBox->m_orgMargin.setRight(origin->m_orgMargin.right());
            newBox->m_orgBorder.setRight(origin->m_orgBorder.right());
            newBox->m_orgPadding.setRight(origin->m_orgPadding.right());

            origin->m_margin.setRight(0);
            origin->m_border.setRight(0);
            origin->m_padding.setRight(0);

            origin->m_orgMargin.setRight(0);
            origin->m_orgBorder.setRight(0);
            origin->m_orgPadding.setRight(0);

            newBox->m_orgMargin.setTop(origin->m_orgMargin.top());
            newBox->m_orgMargin.setBottom(origin->m_orgMargin.bottom());

            newBox->m_orgBorder.setTop(origin->m_orgBorder.top());
            newBox->m_orgBorder.setBottom(origin->m_orgBorder.bottom());

            newBox->m_orgPadding.setTop(origin->m_orgPadding.top());
            newBox->m_orgPadding.setBottom(origin->m_orgPadding.bottom());

            self = newBox;
            iter++;
        }
    };

    auto finishLayout = [&](bool lastNode = false) {
        InlineNonReplacedBox* selfForFinishLayout = self;
        while (selfForFinishLayout) {
            float end = lineFormattingContext.m_currentLineWidth;
            if (end + selfForFinishLayout->paddingRight() + selfForFinishLayout->borderRight() + selfForFinishLayout->marginRight() > inlineContentWidth) {
                selfForFinishLayout->m_margin.setRight(0);
                selfForFinishLayout->m_border.setRight(0);
                selfForFinishLayout->m_padding.setRight(0);
                if (lastNode) {
                    selfForFinishLayout->m_orgMargin.setRight(0);
                    selfForFinishLayout->m_orgBorder.setRight(0);
                    selfForFinishLayout->m_orgPadding.setRight(0);
                }
            }

            lineFormattingContext.m_currentLineWidth += selfForFinishLayout->paddingRight() + selfForFinishLayout->borderRight() + selfForFinishLayout->marginRight();

            selfForFinishLayout->setWidth(selfForFinishLayout->width() + selfForFinishLayout->paddingRight() + selfForFinishLayout->borderRight());
            if (selfForFinishLayout->width() < 0) {
                selfForFinishLayout->setWidth(0);
            }

            // TODO vertical align(length, text-top.., etc)
            bool hasSpecialCase = false;
            float maxAscender = selfForFinishLayout->style()->font()->metrics().m_ascender;
            float maxDescender = selfForFinishLayout->style()->font()->metrics().m_descender;
            float mostBiggestBoxHeight = 0;

            Frame* f = selfForFinishLayout->firstChild();
            while (f) {
                if (f->asFrameBox()->isInlineBox()) {
                    InlineBox* ib = f->asFrameBox()->asInlineBox();
                    if (ib->isInlineBox()) {
                        // normal flow
                        if (ib->isInlineTextBox()) {
                            maxAscender = std::max(ib->asInlineTextBox()->style()->font()->metrics().m_ascender, maxAscender);
                            maxDescender = std::min(ib->asInlineTextBox()->style()->font()->metrics().m_descender, maxDescender);
                            f = f->next();
                            continue;
                        }
                        VerticalAlignValue va = ib->style()->verticalAlign();
                        if (va == VerticalAlignValue::BaselineVAlignValue) {
                            if (ib->isInlineReplacedBox()) {
                                float asc = ib->marginTop() + ib->asInlineReplacedBox()->replacedBox()->borderTop() + ib->asInlineReplacedBox()->replacedBox()->paddingTop()
                                        + ib->asInlineReplacedBox()->replacedBox()->contentHeight();
                                float dec = -(ib->asInlineReplacedBox()->replacedBox()->paddingBottom() + ib->asInlineReplacedBox()->replacedBox()->borderBottom()
                                        + ib->marginBottom());
                                maxAscender = std::max(asc, maxAscender);
                                maxDescender = std::min(dec, maxDescender);
                            } else if (ib->isInlineBlockBox()) {
                                if (ib->asInlineBlockBox()->ascender() == ib->height()) {
                                    maxAscender = std::max(ib->asInlineBlockBox()->ascender() + ib->marginHeight(), maxAscender);
                                    maxDescender = std::min(0.f, maxDescender);
                                } else {
                                    float dec = -(ib->height() - ib->asInlineBlockBox()->ascender()) - ib->marginBottom();
                                    maxAscender = std::max(ib->asInlineBlockBox()->ascender() + ib->marginTop(), maxAscender);
                                    maxDescender = std::min(dec, maxDescender);
                                }
                            } else {
                                maxAscender = std::max(ib->asInlineNonReplacedBox()->ascender(), maxAscender);
                                maxDescender = std::min(ib->asInlineNonReplacedBox()->decender(), maxDescender);
                            }
                        } else if (va == VerticalAlignValue::MiddleVAlignValue) {
                            float height = ib->height();
                            maxAscender = std::max(height/2 + ib->marginTop(), maxAscender);
                            maxDescender = std::min(-height/2 - ib->marginBottom(), maxDescender);
                        } else if (va == VerticalAlignValue::TopVAlignValue || va == VerticalAlignValue::BottomVAlignValue) {
                            hasSpecialCase = true;
                            float height = ib->height() + ib->marginHeight();
                            mostBiggestBoxHeight = std::max(mostBiggestBoxHeight, height);
                        } else {
                            STARFISH_RELEASE_ASSERT_NOT_REACHED();
                        }

                    } else {
                        // out of flow boxes
                        f->asFrameBox()->setY(f->asFrameBox()->marginTop());
                    }
                }
                f = f->next();
            }

            if (hasSpecialCase) {
                while((maxAscender - maxDescender) < mostBiggestBoxHeight) {
                    f = selfForFinishLayout->firstChild();
                    while (f) {
                        InlineBox* ib = f->asFrameBox()->asInlineBox();
                        VerticalAlignValue va = ib->style()->verticalAlign();
                        float height = ib->height() + ib->marginHeight();
                        if (height > maxAscender - maxDescender) {
                            if (va == VerticalAlignValue::TopVAlignValue) {
                                float dec = -height + maxAscender;
                                maxDescender = std::min(maxDescender, dec);
                            } else if (va == VerticalAlignValue::BottomVAlignValue) {
                                float asc = height + maxDescender;
                                maxAscender = std::max(maxAscender, asc);
                            }
                        }
                        f = f->next();
                    }
                }
            }
            selfForFinishLayout->m_ascender = maxAscender;
            selfForFinishLayout->m_descender = maxDescender;

            float height = maxAscender - maxDescender;
            f = selfForFinishLayout->firstChild();
            while (f) {
                if (f->asFrameBox()->isInlineBox()) {
                    InlineBox* ib = f->asFrameBox()->asInlineBox();
                    if (ib->isInlineTextBox()) {
                        ib->setY(height + maxDescender - ib->height() - ib->asInlineTextBox()->style()->font()->metrics().m_descender);
                    } else {
                        VerticalAlignValue va = ib->style()->verticalAlign();
                        if (va == VerticalAlignValue::BaselineVAlignValue) {
                            if (ib->isInlineReplacedBox()) {
                                float asc = ib->marginTop() + ib->asInlineReplacedBox()->replacedBox()->borderTop() + ib->asInlineReplacedBox()->replacedBox()->paddingTop()
                                            + ib->asInlineReplacedBox()->replacedBox()->contentHeight();
                                ib->setY(height + maxDescender - asc + ib->marginTop());
                            } else if (ib->isInlineBlockBox()) {
                                if (ib->asInlineBlockBox()->ascender() == ib->height()) {
                                    float dec = 0;
                                    ib->setY(height + maxDescender - ib->height() - dec - ib->marginTop());
                                } else {
                                    float dec = -(ib->height() - ib->asInlineBlockBox()->ascender()) - ib->marginBottom();
                                    ib->setY(height + maxDescender - ib->height() - dec - ib->marginTop());
                                }
                            } else {
                                STARFISH_ASSERT(ib->isInlineNonReplacedBox());
                                ib->setY(height + maxDescender - ib->height() - ib->asInlineNonReplacedBox()->decender());
                            }
                        } else if (va == VerticalAlignValue::MiddleVAlignValue) {
                            float dec = -(ib->height() / 2);
                            ib->setY(height + maxDescender - ib->height() - dec - ib->marginTop());
                        } else if (va == VerticalAlignValue::TopVAlignValue) {
                            ib->setY(ib->marginTop());
                        } else if (va == VerticalAlignValue::BottomVAlignValue) {
                            ib->setY(height - ib->height() - ib->marginBottom());
                        } else {
                            STARFISH_RELEASE_ASSERT_NOT_REACHED();
                        }
                    }

                    if (ib->style()->position() == PositionValue::RelativePositionValue)
                        ctx.registerRelativePositionedFrames(ib);
                } else {
                    // out of flow boxes
                }
                f = f->next();
            }

            selfForFinishLayout->setContentHeight(maxAscender - maxDescender);
            if (selfForFinishLayout->parent()->asFrameBox()->isLineBox()) {
                break;
            }
            if (lastNode) {
                break;
            }
            selfForFinishLayout = selfForFinishLayout->parent()->asFrameBox()->asInlineBox()->asInlineNonReplacedBox();
        }
    };

#define BREAK_LINE() \
        finishLayout(); \
        breakLine();


    Frame* f = self->m_origin->firstChild();
    while (f) {
        if (!f->isNormalFlow()) {
            lineFormattingContext.currentLine()->m_boxes.push_back(f->asFrameBox());
            f->setLayoutParent(lineFormattingContext.currentLine());
            ctx.registerAbsolutePositionedFrames(f->asFrameBox());
            f = f->next();
            continue;
        }

        if (f->isFrameText()) {
            String* txt = f->asFrameText()->text();
            //fast path
            if (f == self->m_origin->lastChild()) {
                if (txt->containsOnlyWhitespace()) {
                    f = f->next();
                    continue;
                }
            }

            textDividerForLayout(txt, [&](size_t offset, size_t nextOffset, bool isWhiteSpace) {
                textAppendRetry:
                if (lineFormattingContext.m_currentLineWidth == 0 && isWhiteSpace) {
                    return;
                }

                // CHECK
                // is (f == self->m_origin->lastChild()) needs?
                if (f == self->m_origin->lastChild() && offset != 0 && nextOffset == txt->length() && isWhiteSpace) {
                    return;
                }

                float textWidth;
                String* resultString;
                if(isWhiteSpace) {
                    resultString = String::spaceString;
                    textWidth = f->style()->font()->spaceWidth();
                }
                else {
                    String* ss = txt->substring(offset,nextOffset - offset);
                    resultString = ss;
                    textWidth = f->style()->font()->measureText(ss);
                }

                if(textWidth <= inlineContentWidth - lineFormattingContext.m_currentLineWidth || lineFormattingContext.m_currentLineWidth == 0) {
                    lineFormattingContext.m_currentLineWidth += textWidth;
                } else {
                    if (isWhiteSpace)
                        return;
                    // try this at nextline
                    BREAK_LINE();
                    goto textAppendRetry;
                }

                InlineTextBox* box = new InlineTextBox(f->node(), f->style(), self, resultString);
                self->appendChild(box);
                box->setWidth(textWidth);
                box->setHeight(f->style()->font()->metrics().m_fontHeight);
                box->setX(self->width());
                self->setWidth(self->width() + box->width());
            });

        } else if (f->isFrameReplaced()) {
            FrameReplaced* r = f->asFrameReplaced();
            r->layout(ctx);

            insertReplacedBox:
            if ((r->width() + r->marginWidth()) <= (inlineContentWidth - lineFormattingContext.m_currentLineWidth) || lineFormattingContext.m_currentLineWidth == 0) {
                InlineBox* b = new InlineReplacedBox(f->node(), f->style(), self, r);
                self->appendChild(b);
                b->setMarginLeft(r->marginLeft());
                b->setMarginTop(r->marginTop());
                b->setMarginRight(r->marginRight());
                b->setMarginBottom(r->marginBottom());
                b->setWidth(r->width());
                b->setHeight(r->height());

                b->setX(self->width() + b->marginLeft());
                self->setWidth(self->width() + b->width() + b->marginWidth());
                lineFormattingContext.m_currentLineWidth += b->width() + b->marginWidth();
            } else {
                BREAK_LINE();
                goto insertReplacedBox;
            }
        } else if (f->isFrameBlockBox()) {
            // inline-block
            FrameBlockBox* r = f->asFrameBlockBox();
            f->layout(ctx);

            float ascender = 0;
            if (ctx.lastLineBox() && r->isAncestorOf(ctx.lastLineBox())) {
                float topToLineBox = ctx.lastLineBox()->absolutePoint(r).y();
                ascender = topToLineBox + ctx.lastLineBox()->ascender();
            } else {
                ascender = f->asFrameBox()->height();
            }

            insertBlockBox:
            if ((r->width() + r->marginWidth()) <= (inlineContentWidth - lineFormattingContext.m_currentLineWidth) || lineFormattingContext.m_currentLineWidth == 0) {
                InlineBox* b = new InlineBlockBox(f->node(), f->style(), self, r, ascender);
                self->appendChild(b);
                b->setMarginLeft(r->marginLeft());
                b->setMarginTop(r->marginTop());
                b->setMarginRight(r->marginRight());
                b->setMarginBottom(r->marginBottom());
                b->setWidth(r->width());
                b->setHeight(r->height());

                b->setX(self->width() + b->marginLeft());
                self->setWidth(self->width() + b->width() + b->marginWidth());
                lineFormattingContext.m_currentLineWidth += b->width() + b->marginWidth();
            } else {
                BREAK_LINE();
                goto insertBlockBox;
            }


        } else if (f->isFrameLineBreak()) {
            BREAK_LINE();
        } else if (f->isFrameInline()) {
            InlineNonReplacedBox* inlineBox = new InlineNonReplacedBox(f->node(), f->node()->style(), self, f->asFrameInline());
            inlineBox->setX(self->width());
            InlineNonReplacedBox* result = InlineNonReplacedBox::layoutInline(inlineBox, ctx, blockBox, lfc, self, true);
            Frame* newSelfCandi = result->parent();
            if (newSelfCandi->asFrameBox()->isInlineBox() && newSelfCandi->asFrameBox()->asInlineBox()->isInlineNonReplacedBox()) {
                InlineNonReplacedBox* newSelf = newSelfCandi->asFrameBox()->asInlineBox()->asInlineNonReplacedBox();
                if (self != newSelf) {
                    self = newSelf;
                }
            }
            self->setWidth(self->width() + result->width() + result->marginWidth());
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        f = f->next();
    }

    finishLayout(true);

    return self;
}

void InlineNonReplacedBox::paint(Canvas* canvas, PaintingStage stage)
{
    // CHECK THIS at https://www.w3.org/TR/CSS2/zindex.html#stacking-defs
    if (stage == PaintingNormalFlowInline) {
        Rect frameRectBack = m_frameRect;
        BoxSurroundData paddingBack = m_padding, borderBack = m_border, marginBack = m_margin;

        m_padding = m_orgPadding;
        m_margin = m_orgMargin;
        m_border = m_orgBorder;

        canvas->save();
        canvas->translate(0 ,-y());
        moveY(height());
        setContentHeight(style()->font()->metrics().m_ascender - style()->font()->metrics().m_descender);
        moveY(-contentHeight() - paddingTop() - borderTop());
        setHeight(contentHeight() + paddingHeight() + borderHeight());

        canvas->translate(0 ,y());
        paintBackgroundAndBorders(canvas);
        canvas->restore();

        m_frameRect = frameRectBack;
        m_padding = paddingBack;
        m_border = borderBack;
        m_margin = marginBack;
    }
    Frame* child = firstChild();
    while (child) {
        canvas->save();
        canvas->translate(child->asFrameBox()->x(), child->asFrameBox()->y());
        child->paint(canvas, stage);
        canvas->restore();
        child = child->next();
    }
}

Frame* InlineNonReplacedBox::hitTest(float x, float y, HitTestStage stage)
{
    if (stage == HitTestStage::HitTestNormalFlowInline) {
        Frame* result = nullptr;
        HitTestStage s = HitTestStage::HitTestPositionedElements;

        while (s != HitTestStageEnd) {
            Frame* f = lastChild();
            while (f) {
                float cx = x - f->asFrameBox()->x();
                float cy = y - f->asFrameBox()->y();
                result = f->hitTest(cx, cy, s);
                if (result)
                    return result;
                f = f->previous();
            }
            s = (HitTestStage)(s + 1);
        }
    }
    return nullptr;
}


void InlineNonReplacedBox::dump(int depth)
{
    InlineBox::dump(depth);
    puts("");
    if (firstChild()) {
        Frame* f = firstChild();
        while (f) {
            for(int k = 0; k < depth + 1; k ++)
                printf("  ");
            printf("%s", f->name());
            f->dump(depth + 2);
            puts("");
            f = f->next();
        }
    }
}


void FrameBlockBox::dump(int depth)
{
    FrameBox::dump(depth);
    if (!hasBlockFlow()) {
        if (m_lineBoxes.size() && m_lineBoxes[0]->m_boxes.size()) {
            for (size_t i = 0; i < m_lineBoxes.size(); i ++) {
                puts("");
                for(int k = 0; k < depth + 1; k ++)
                    printf("  ");
                printf("LineBox(%g,%g,%g,%g)\n", m_lineBoxes[i]->m_frameRect.x(), m_lineBoxes[i]->m_frameRect.y()
                    , m_lineBoxes[i]->m_frameRect.width(), m_lineBoxes[i]->m_frameRect.height());

                const LineBox& lb = *m_lineBoxes[i];
                for (size_t j = 0; j < lb.m_boxes.size(); j ++) {
                    for(int k = 0; k < depth + 2; k ++)
                        printf("  ");
                    printf("%s", lb.m_boxes[j]->name());
                    lb.m_boxes[j]->dump(depth + 3);
                    if (j != lb.m_boxes.size() - 1)
                        puts("");
                }
            }
        }
    }
}


}
