#include "StarFishConfig.h"
#include "FrameBlockBox.h"
#include "FrameText.h"

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

class LineFormattingContext {
public:
    LineFormattingContext(FrameBlockBox& block, LayoutContext& ctx)
        : m_block(block)
        , m_layoutContext(ctx)
    {
        m_block.m_lineBoxes.clear();
        // m_block.m_lineBoxes.shrink_to_fit();
        m_block.m_lineBoxes.push_back(LineBox(&m_block));
        m_currentLine = 0;
        m_currentLineWidth = 0;
        m_layoutContext.setLastLineBox(&m_block.m_lineBoxes.back());
    }


    void breakLine()
    {
        m_block.m_lineBoxes.push_back(LineBox(&m_block));
        m_currentLine++;
        m_currentLineWidth = 0;
        m_layoutContext.setLastLineBox(&m_block.m_lineBoxes.back());
    }

    float m_currentLineWidth;
    size_t m_currentLine;
    FrameBlockBox& m_block;
    LayoutContext& m_layoutContext;
};


float FrameBlockBox::layoutInline(LayoutContext& ctx)
{
    float lineBoxX = paddingLeft() + borderLeft();
    float lineBoxY = paddingTop() + borderTop();
    float inlineContentWidth = contentWidth();
    LineFormattingContext lineFormattingContext(*this, ctx);

    // we dont need gc_allocator here. because frame-tree-item is referenced by its parent
    std::vector<Frame*> result;
    // NOTE this function collect only normal-flow child!
    collectInlineBox(result, this, this);

    for (unsigned i = 0; i < result.size(); i ++) {
        Frame* f = result[i];

        if (!f->isNormalFlow()) {
            m_lineBoxes[lineFormattingContext.m_currentLine].m_boxes.push_back(f->asFrameBox());
            f->setLayoutParent(&m_lineBoxes[lineFormattingContext.m_currentLine]);
            ctx.registerAbsolutePositionedFrames(f->asFrameBox());
            continue;
        }

        if (f->isFrameText()) {
            String* txt = f->asFrameText()->text();
            //fast path
            if (f == lastChild()) {
                if (txt->containsOnlyWhitespace()) {
                    continue;
                }
            }

            textDividerForLayout(txt, [&](size_t offset, size_t nextOffset, bool isWhiteSpace) {
                textAppendRetry:
                if (lineFormattingContext.m_currentLineWidth == 0 && isWhiteSpace) {
                    return;
                }
                if ((i + 1) == result.size() && offset != 0 && nextOffset == txt->length() && isWhiteSpace) {
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

                m_lineBoxes[lineFormattingContext.m_currentLine].m_boxes.push_back(new InlineTextBox(f->node(), f->style(), &m_lineBoxes[lineFormattingContext.m_currentLine], resultString));
                m_lineBoxes[lineFormattingContext.m_currentLine].m_boxes.back()->setWidth(textWidth);
                m_lineBoxes[lineFormattingContext.m_currentLine].m_boxes.back()->setHeight(f->style()->font()->metrics().m_fontHeight);
            });

        } else if (f->isFrameReplaced()) {
            FrameReplaced* r = f->asFrameReplaced();
            r->layout(ctx);

            insertReplacedBox:
            if ((r->width() + r->marginWidth()) <= (inlineContentWidth - lineFormattingContext.m_currentLineWidth) || lineFormattingContext.m_currentLineWidth == 0) {
                m_lineBoxes[lineFormattingContext.m_currentLine].m_boxes.push_back(new InlineReplacedBox(f->node(), f->style(), &m_lineBoxes[lineFormattingContext.m_currentLine], r));
                m_lineBoxes[lineFormattingContext.m_currentLine].m_boxes.back()->setMarginLeft(r->marginLeft());
                m_lineBoxes[lineFormattingContext.m_currentLine].m_boxes.back()->setMarginTop(r->marginTop());
                m_lineBoxes[lineFormattingContext.m_currentLine].m_boxes.back()->setMarginRight(r->marginRight());
                m_lineBoxes[lineFormattingContext.m_currentLine].m_boxes.back()->setMarginBottom(r->marginBottom());
                m_lineBoxes[lineFormattingContext.m_currentLine].m_boxes.back()->setWidth(r->width());
                m_lineBoxes[lineFormattingContext.m_currentLine].m_boxes.back()->setHeight(r->height());
                lineFormattingContext.m_currentLineWidth += (r->width() + r->marginWidth());
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
                // TODO consider margin
                float topToLineBox = ctx.lastLineBox()->absolutePoint(r).y();
                ascender = topToLineBox + ctx.lastLineBox()->m_ascender;
            } else {
                ascender = f->asFrameBox()->height();
            }

            insertBlockBox:
            if ((r->width() + r->marginWidth()) <= (inlineContentWidth - lineFormattingContext.m_currentLineWidth) || lineFormattingContext.m_currentLineWidth == 0) {
                m_lineBoxes[lineFormattingContext.m_currentLine].m_boxes.push_back(new InlineBlockBox(f->node(), f->style(), &m_lineBoxes[lineFormattingContext.m_currentLine], r, ascender));
                m_lineBoxes[lineFormattingContext.m_currentLine].m_boxes.back()->setMarginLeft(r->marginLeft());
                m_lineBoxes[lineFormattingContext.m_currentLine].m_boxes.back()->setMarginTop(r->marginTop());
                m_lineBoxes[lineFormattingContext.m_currentLine].m_boxes.back()->setMarginRight(r->marginRight());
                m_lineBoxes[lineFormattingContext.m_currentLine].m_boxes.back()->setMarginBottom(r->marginBottom());
                m_lineBoxes[lineFormattingContext.m_currentLine].m_boxes.back()->setWidth(r->width());
                m_lineBoxes[lineFormattingContext.m_currentLine].m_boxes.back()->setHeight(r->height());
                lineFormattingContext.m_currentLineWidth += (r->width() + r->marginWidth());
            } else {
                lineFormattingContext.breakLine();
                goto insertBlockBox;
            }
        } else if (f->isFrameLineBreak()) {
            lineFormattingContext.breakLine();
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    // position each line
    float contentHeight = 0;
    for (size_t i = 0; i < m_lineBoxes.size(); i ++) {
        LineBox& b = m_lineBoxes[i];

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
        float maxDecender = 0;
        float mostBiggestBoxHeight = 0;

        for (size_t j = 0; j < b.m_boxes.size(); j ++) {
            if (b.m_boxes[j]->isInlineBox()) {
                // normal flow
                InlineBox* ib = b.m_boxes[j]->asInlineBox();

                if (ib->isInlineTextBox()) {
                    maxAscender = std::max(ib->asInlineTextBox()->style()->font()->metrics().m_ascender, maxAscender);
                    maxDecender = std::min(ib->asInlineTextBox()->style()->font()->metrics().m_descender, maxDecender);
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
                        maxDecender = std::min(dec, maxDecender);
                    } else if (ib->isInlineBlockBox()) {
                        if (ib->asInlineBlockBox()->m_ascender == ib->height()) {
                            maxAscender = std::max(ib->asInlineBlockBox()->m_ascender + ib->marginHeight(), maxAscender);
                            maxDecender = std::min(0.f, maxDecender);
                        } else {
                            float dec = -(ib->height() - ib->asInlineBlockBox()->m_ascender) - ib->marginBottom();
                            maxAscender = std::max(ib->asInlineBlockBox()->m_ascender + ib->marginTop(), maxAscender);
                            maxDecender = std::min(dec, maxDecender);
                        }
                    } else {
                        STARFISH_RELEASE_ASSERT_NOT_REACHED();
                    }
                } else if (va == VerticalAlignValue::MiddleVAlignValue) {
                    float height = ib->height();
                    maxAscender = std::max(height/2 + ib->marginTop(), maxAscender);
                    maxDecender = std::min(-height/2 - ib->marginBottom(), maxDecender);
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
            while((maxAscender - maxDecender) != mostBiggestBoxHeight) {
                for (size_t j = 0; j < b.m_boxes.size(); j ++) {
                    InlineBox* ib = b.m_boxes[j]->asInlineBox();
                    VerticalAlignValue va = ib->style()->verticalAlign();
                    float height = ib->height() + ib->marginHeight();
                    if (height > maxAscender - maxDecender) {
                        if (va == VerticalAlignValue::TopVAlignValue) {
                            float dec = -height + maxAscender;
                            maxDecender = std::min(maxDecender, dec);
                        } else if (va == VerticalAlignValue::BottomVAlignValue) {
                            float asc = height + maxDecender;
                            maxAscender = std::max(maxAscender, asc);
                        }
                    }
                }
            }
        }
        b.m_ascender = maxAscender;
        b.m_decender = maxDecender;

        float height = maxAscender - maxDecender;
        for (size_t j = 0; j < b.m_boxes.size(); j ++) {
            if (b.m_boxes[j]->isInlineBox()) {
                InlineBox* ib = b.m_boxes[j]->asInlineBox();
                if (ib->isInlineTextBox()) {
                    ib->setY(height + maxDecender - ib->height() - ib->asInlineTextBox()->style()->font()->metrics().m_descender);
                } else {
                    VerticalAlignValue va = ib->style()->verticalAlign();
                    if (va == VerticalAlignValue::BaselineVAlignValue) {
                        if (ib->isInlineReplacedBox()) {
                            float asc = ib->marginTop() + ib->asInlineReplacedBox()->replacedBox()->borderTop() + ib->asInlineReplacedBox()->replacedBox()->paddingTop()
                                        + ib->asInlineReplacedBox()->replacedBox()->contentHeight();
                            ib->setY(height + maxDecender - asc + ib->marginTop());
                        } else if (ib->isInlineBlockBox()) {
                            if (ib->asInlineBlockBox()->m_ascender == ib->height()) {
                                float dec = 0;
                                ib->setY(height + maxDecender - ib->height() - dec - ib->marginTop());
                            } else {
                                float dec = -(ib->height() - ib->asInlineBlockBox()->m_ascender) - ib->marginBottom();
                                ib->setY(height + maxDecender - ib->height() - dec - ib->marginTop());
                            }
                        } else {
                            STARFISH_RELEASE_ASSERT_NOT_REACHED();
                        }
                    } else if (va == VerticalAlignValue::MiddleVAlignValue) {
                        float dec = -(ib->height() / 2);
                        ib->setY(height + maxDecender - ib->height() - dec - ib->marginTop());
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
        // we dont need gc_allocator here. because frame-tree-item is referenced by its parent
        std::vector<Frame*> result;
        // NOTE this function collect only normal-flow child!
        collectInlineBox(result, this, this);

        float currentLineWidth = 0;
        for (unsigned i = 0; i < result.size(); i ++) {
            Frame* f = result[i];

            if (!f->isNormalFlow())
                continue;

            if (f->isFrameText()) {
                String* s = f->asFrameText()->text();
                textDividerForLayout(s, [&](size_t offset, size_t nextOffset, bool isWhiteSpace){
                    if (currentLineWidth == 0 && isWhiteSpace) {
                        return;
                    }
                    if ((i + 1) == result.size() && offset != 0 && nextOffset == s->length() && isWhiteSpace) {
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
            LineBox& b = block->m_lineBoxes[i];
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
        ASSERT(stage == PaintingStackingContext);
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
    } else if(isPositionedElement()) {
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
                    LineBox& b = m_lineBoxes[i];
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
                    LineBox& b = m_lineBoxes[i];
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

void FrameBlockBox::dump(int depth)
{
    FrameBox::dump(depth);
    if (!hasBlockFlow()) {
        if (m_lineBoxes.size() && m_lineBoxes[0].m_boxes.size()) {
            for (size_t i = 0; i < m_lineBoxes.size(); i ++) {
                puts("");
                for(int k = 0; k < depth + 1; k ++)
                    printf("  ");
                printf("LineBox(%g,%g,%g,%g)\n", m_lineBoxes[i].m_frameRect.x(), m_lineBoxes[i].m_frameRect.y()
                    , m_lineBoxes[i].m_frameRect.width(), m_lineBoxes[i].m_frameRect.height());

                const LineBox& lb = m_lineBoxes[i];
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
