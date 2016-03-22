#include "StarFishConfig.h"
#include "FrameBlockBox.h"
#include "FrameText.h"
#include "FrameInline.h"

namespace StarFish {

LayoutUnit computeVerticalProperties(FrameBox* parentBox, ComputedStyle* parentStyle, LayoutUnit& ascenderInOut, LayoutUnit& descenderInOut, LineFormattingContext& ctx)
{
    LayoutUnit maxAscender = ascenderInOut;
    LayoutUnit maxDescender = descenderInOut;
    bool hasBoxOtherThanText = false;

    std::vector<FrameBox*, gc_allocator<FrameBox*> >* boxes;
    if (parentBox->isLineBox()) {
        boxes = &parentBox->asLineBox()->boxes();
        if (!boxes->size()) {
            maxAscender = parentStyle->font()->metrics().m_ascender;
            maxDescender = parentStyle->font()->metrics().m_descender;
        }
    } else {
        boxes = &parentBox->asInlineBox()->asInlineNonReplacedBox()->boxes();
    }

    LayoutUnit pascender = parentStyle->font()->metrics().m_ascender;
    LayoutUnit pdescender = parentStyle->font()->metrics().m_descender;
    bool hasNormalFlowContent = false;

    // 1. set relative y-pos from baseline
    for (size_t k = 0; k < boxes->size(); k ++) {
        FrameBox* f = boxes->at(k);
        if (f->isNormalFlow()) {
            hasNormalFlowContent = true;
        } else {
            // out of flow boxes
            f->asFrameBox()->setY(f->asFrameBox()->marginTop());
            continue;
        }
        VerticalAlignValue va = f->style()->verticalAlign();
        if (f->isInlineBox()) {
            InlineBox* ib = f->asFrameBox()->asInlineBox();
            // normal flow
            if (ib->isInlineTextBox()) {
                ib->setY(ib->asInlineTextBox()->style()->font()->metrics().m_ascender);
            } else {
                hasBoxOtherThanText = true;
                if (va == VerticalAlignValue::BaselineVAlignValue) {
                    LayoutUnit a = ib->asInlineNonReplacedBox()->ascender();
                    ib->setY(ib->asInlineNonReplacedBox()->ascender()+ib->marginTop());
                } else if (va == VerticalAlignValue::SubVAlignValue) {
                    ib->setY(pdescender + ib->asInlineNonReplacedBox()->ascender());
                } else if (va == VerticalAlignValue::SuperVAlignValue) {
                    // Placing a superscript is font and browser dependent.
                    // We place superscript above the baseline by 1/2 of ascender (following blink)
                    // (i.e, the baseline of superscript is aligned with 1/2 of the ascender)
                    ib->setY(pascender / 2 + ib->asInlineNonReplacedBox()->ascender());
                } else if (va == VerticalAlignValue::TopVAlignValue) {
                    ib->setY(ib->height() + ib->marginTop());
                } else if (va == VerticalAlignValue::BottomVAlignValue) {
                    ib->setY(pdescender + ib->height() + ib->marginBottom());
                } else if (va == VerticalAlignValue::MiddleVAlignValue) {
                    ib->setY(ib->height() / 2 + ib->marginTop());
                } else if (va == VerticalAlignValue::TextTopVAlignValue) {
                    ib->setY(pascender);
                } else if (va == VerticalAlignValue::TextBottomVAlignValue) {
                    ib->setY(pdescender + ib->height());
                } else if (va == VerticalAlignValue::NumericVAlignValue) {
                    Length len = f->style()->verticalAlignLength();
                    LayoutUnit height = ib->asInlineNonReplacedBox()->ascender() - ib->asInlineNonReplacedBox()->decender();
                    if (len.isPercent()) {
                        ib->setY(ib->asInlineNonReplacedBox()->ascender() + height * len.percent());
                    } else if (len.isFixed()) {
                        ib->setY(ib->asInlineNonReplacedBox()->ascender() + LayoutUnit::fromPixel(len.fixed()));
                    }
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
            }
        } else if (f->isFrameReplaced()) {
            // TODO use this code for when replaced content does not have content
            /*
            LayoutUnit asc = ib->marginTop() + ib->asInlineReplacedBox()->replacedBox()->borderTop() + ib->asInlineReplacedBox()->replacedBox()->paddingTop()
            + ib->asInlineReplacedBox()->replacedBox()->contentHeight();
            LayoutUnit dec = -(ib->asInlineReplacedBox()->replacedBox()->paddingBottom() + ib->asInlineReplacedBox()->replacedBox()->borderBottom()
            + ib->marginBottom());
            maxAscender = std::max(asc, maxAscender);
            maxDescender = std::min(dec, maxDescender);
            */

            hasBoxOtherThanText = true;
            if (va == VerticalAlignValue::BaselineVAlignValue) {
                f->setY(f->height() + f->marginHeight());
            } else if (va == VerticalAlignValue::TopVAlignValue) {
                // Nothing needs to be done
            } else if (va == VerticalAlignValue::BottomVAlignValue) {
                f->setY(pdescender + f->height() + f->marginTop() + f->marginBottom());
            } else if (va == VerticalAlignValue::TextTopVAlignValue) {
                f->setY(pascender);
            } else if (va == VerticalAlignValue::TextBottomVAlignValue) {
                f->setY(pdescender + f->height() + f->marginBottom());
            } else if (va == VerticalAlignValue::NumericVAlignValue) {
                Length len = f->style()->verticalAlignLength();
                if (len.isPercent()) {
                    f->setY(f->height() + f->height() * len.percent());
                } else if (len.isFixed()) {
                    f->setY(f->height() + LayoutUnit::fromPixel(len.fixed()));
                }
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        } else if (f->isFrameBlockBox() && f->style()->display() == InlineBlockDisplayValue) {
            hasBoxOtherThanText = true;
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    // 2. find max ascender and descender
    LayoutUnit maxAscenderSoFar = pascender;
    LayoutUnit maxDescenderSoFar = pdescender;
    // FIXME: step 2 can be reduced
    while (true) {
        LayoutUnit prevMaxAscenderSoFar = maxAscenderSoFar;
        LayoutUnit prevMaxDescenderSoFar = maxDescenderSoFar;

        for (size_t k = 0; k < boxes->size(); k ++) {
            FrameBox* box = boxes->at(k);
            VerticalAlignValue va = box->style()->verticalAlign();

            if (box->isInlineBox()) {
                if (box->asInlineBox()->isInlineTextBox()) {
                    maxAscenderSoFar = std::max(box->asInlineBox()->asInlineTextBox()->style()->font()->metrics().m_ascender, maxAscenderSoFar);
                    maxDescenderSoFar = std::min(box->asInlineBox()->asInlineTextBox()->style()->font()->metrics().m_descender, maxDescenderSoFar);
                } else {
                    InlineNonReplacedBox* rb = box->asInlineBox()->asInlineNonReplacedBox();
                    if (va == VerticalAlignValue::BaselineVAlignValue) {
                        maxAscenderSoFar = std::max(rb->ascender(), maxAscenderSoFar);
                        maxDescenderSoFar = std::min(rb->decender(), maxDescenderSoFar);
                    } else if (va == VerticalAlignValue::SubVAlignValue) {
                        maxDescenderSoFar = std::min(pdescender + rb->decender(), maxDescenderSoFar);
                    } else if (va == VerticalAlignValue::SuperVAlignValue) {
                        maxAscenderSoFar = std::max(pascender / 2 + rb->ascender(), maxAscenderSoFar);
                    } else if (va == VerticalAlignValue::TopVAlignValue) {
                        maxAscenderSoFar = std::max(rb->height() + rb->marginTop(), maxAscenderSoFar);
                    } else if (va == VerticalAlignValue::BottomVAlignValue) {
                        maxAscenderSoFar = std::max(rb->height() - maxDescenderSoFar, maxAscenderSoFar);
                    } else if (va == VerticalAlignValue::TextTopVAlignValue) {
                        maxDescenderSoFar = std::min(pascender - rb->height(), maxDescenderSoFar);
                    } else if (va == VerticalAlignValue::TextBottomVAlignValue) {
                        maxAscenderSoFar = std::max(pdescender + rb->height(), maxAscenderSoFar);
                    } else if (va == VerticalAlignValue::NumericVAlignValue) {
                        maxAscenderSoFar = std::max(rb->y(), maxAscenderSoFar);
                        maxDescenderSoFar = std::min(rb->y() - rb->height(), maxDescenderSoFar);
                    } else {
                        STARFISH_RELEASE_ASSERT_NOT_REACHED();
                    }
                }
            } else if (box->isFrameReplaced()) {
                if (va == VerticalAlignValue::BaselineVAlignValue) {
                    maxAscenderSoFar = std::max(box->height() + box->marginHeight(), maxAscenderSoFar);
                } else if (va == VerticalAlignValue::TopVAlignValue) {
                    maxAscenderSoFar = std::max(box->height() + box->marginTop(), maxAscenderSoFar);
                } else if (va == VerticalAlignValue::BottomVAlignValue) {
                    maxAscenderSoFar = std::max(maxDescenderSoFar + box->height() + box->marginTop(), maxAscenderSoFar);
                } else if (va == VerticalAlignValue::TextTopVAlignValue) {
                    maxDescenderSoFar = std::min(pascender - box->height(), maxDescenderSoFar);
                } else if (va == VerticalAlignValue::TextBottomVAlignValue) {
                    maxAscenderSoFar = std::max(pdescender + box->height(), maxAscenderSoFar);
                } else if (va == VerticalAlignValue::NumericVAlignValue) {
                    maxAscenderSoFar = std::max(box->y(), maxAscenderSoFar);
                    maxDescenderSoFar = std::max(box->y() - box->height(), maxDescenderSoFar);
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
            } else if (box->isFrameBlockBox() && box->style()->display() == InlineBlockDisplayValue) {
                hasBoxOtherThanText = true;
                LayoutUnit ascender = ctx.inlineBlockAscender(box->asFrameBlockBox());
                if (ascender == box->height()) {
                    maxAscenderSoFar = std::max(ascender + box->marginHeight(), maxAscenderSoFar);
                    maxDescenderSoFar = std::min(LayoutUnit(0), maxDescenderSoFar);
                } else {
                    LayoutUnit dec = -(box->height() - ascender) - box->marginBottom();
                    maxAscenderSoFar = std::max(ascender + box->marginTop(), maxAscenderSoFar);
                    maxDescenderSoFar = std::min(dec, maxDescenderSoFar);
                }
            }
        }

        if (prevMaxAscenderSoFar == maxAscenderSoFar
            && prevMaxDescenderSoFar == maxDescenderSoFar) {
            break;
        }
    }

    maxAscender = maxAscenderSoFar;
    maxDescender = maxDescenderSoFar;

    // 3. adjusting the line height
    if (!parentStyle->hasNormalLineHeight()) {
        LayoutUnit lineHeight = parentStyle->lineHeight().fixed();
        LayoutUnit ascenderShouldBe;
        LayoutUnit descenderShouldBe;

        if (lineHeight > parentStyle->font()->metrics().m_fontHeight) {
            LayoutUnit diff = (lineHeight - parentStyle->font()->metrics().m_fontHeight) / 2;
            ascenderShouldBe = parentStyle->font()->metrics().m_ascender + diff;
            descenderShouldBe = parentStyle->font()->metrics().m_descender - diff;
        } else {
            LayoutUnit diff = (lineHeight - parentStyle->font()->metrics().m_fontHeight) / 2;
            ascenderShouldBe = parentStyle->font()->metrics().m_ascender + diff;
            descenderShouldBe = parentStyle->font()->metrics().m_descender - diff;
            if (descenderShouldBe > 0) {
                ascenderShouldBe -= descenderShouldBe;
                descenderShouldBe = 0;
            }
            if (ascenderShouldBe < 0) {
                ascenderShouldBe = 0;
            }
        }

        if (maxAscender < ascenderShouldBe) {
            maxAscender = ascenderShouldBe;
        } else if (!hasBoxOtherThanText) {
            maxAscender = ascenderShouldBe;
        }
        maxDescender = descenderShouldBe;
    }

    ascenderInOut = maxAscender;
    descenderInOut = maxDescender;

    LayoutUnit height = maxAscender - maxDescender;
    for (size_t k = 0; k < boxes->size(); k ++) {
        FrameBox* f = boxes->at(k);

        if (!f->isFrameBlockBox() && f->style()->position() == PositionValue::RelativePositionValue)
            ctx.m_layoutContext.registerRelativePositionedFrames(f);

        if (f->isNormalFlow()) {
            if (f->isInlineBox() && f->asInlineBox()->isInlineTextBox()) {
                InlineBox* ib = f->asFrameBox()->asInlineBox();
                if (UNLIKELY(descenderInOut == 0)) {
                    if (height < ib->asInlineTextBox()->style()->font()->metrics().m_fontHeight)
                        ib->setY((height - ib->asInlineTextBox()->style()->font()->metrics().m_fontHeight + ib->asInlineTextBox()->style()->font()->metrics().m_descender) / 2);
                    else
                        ib->setY(height - ib->asInlineTextBox()->style()->font()->metrics().m_fontHeight - ib->asInlineTextBox()->style()->font()->metrics().m_descender);
                } else {
                    ib->setY(height + maxDescender - ib->height() - ib->asInlineTextBox()->style()->font()->metrics().m_descender);
                }
            } else {
                VerticalAlignValue va = f->style()->verticalAlign();
                if (va == VerticalAlignValue::BaselineVAlignValue) {
                    if (f->isInlineBox() && f->asInlineBox()->isInlineNonReplacedBox()) {
                        f->setY(height + maxDescender - f->height() - f->asInlineBox()->asInlineNonReplacedBox()->decender());
                    } else if (f->isFrameReplaced()) {
                        // TODO use this code for when replaced content does not have content
                        /*
                           LayoutUnit asc = ib->marginTop() + ib->asInlineReplacedBox()->replacedBox()->borderTop() + ib->asInlineReplacedBox()->replacedBox()->paddingTop()
                           + ib->asInlineReplacedBox()->replacedBox()->contentHeight();
                           ib->setY(height + maxDescender - asc + ib->marginTop());
                           */
                        f->setY(maxAscender - (f->y() - f->marginTop()));
                    } else {
                        STARFISH_ASSERT(f->isFrameBlockBox() && f->style()->display() == InlineBlockDisplayValue);
                        LayoutUnit ascender = ctx.inlineBlockAscender(f->asFrameBlockBox());
                        if (ascender == f->height()) {
                            LayoutUnit dec = f->marginBottom();
                            f->setY(height + maxDescender - f->height() - dec);
                        } else {
                            LayoutUnit dec = -(f->height() - ascender) + f->marginBottom();
                            f->setY(height + maxDescender - f->height() - dec + f->marginTop());
                        }
                    }
                // 4. convert a y pos relative to the baseline to a y pos relative to the top-left corner of the box
                } else if (va == VerticalAlignValue::MiddleVAlignValue) {
                    LayoutUnit dec = -(f->height() / 2);
                    f->setY(height + maxDescender - f->height() - dec - f->marginTop());
                } else if (va == VerticalAlignValue::TopVAlignValue) {
                    f->setY(f->marginTop());
                } else if (va == VerticalAlignValue::BottomVAlignValue) {
                    f->setY(height - f->height() - f->marginBottom());
                } else if (va == VerticalAlignValue::SubVAlignValue) {
                    f->setY(maxAscender - f->y());
                } else if (va == VerticalAlignValue::SuperVAlignValue) {
                    f->setY(maxAscender - f->y());
                } else if (va == VerticalAlignValue::TextTopVAlignValue) {
                    f->setY(maxAscender - f->y());
                } else if (va == VerticalAlignValue::TextBottomVAlignValue) {
                    f->setY(maxAscender - f->y());
                } else if (va == VerticalAlignValue::NumericVAlignValue) {
                    f->setY(maxAscender - f->y());
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
            }
        } else {
            // out of flow boxes
            STARFISH_ASSERT(!f->isNormalFlow());
        }
    }

    return ascenderInOut - descenderInOut;
}

FrameBox* findLastInlineBoxNonReplacedBoxCase(InlineNonReplacedBox* b)
{
    FrameBox* result = b;

    auto iter = b->boxes().begin();
    while (iter != b->boxes().end()) {
        FrameBox* f = *iter;
        if (f->asFrameBox()->isInlineBox()) {
            InlineBox* ib = f->asFrameBox()->asInlineBox();
            if (ib->isInlineNonReplacedBox()) {
                result = findLastInlineBoxNonReplacedBoxCase(ib->asInlineNonReplacedBox());
            } else {
                result = ib;
            }
        } else {
            result = *iter;
        }
        iter++;
    }

    return result;
}

FrameBox* findLastInlineBox(LineBox* lb)
{
    FrameBox* result = nullptr;
    for (size_t i = 0; i < lb->boxes().size(); i ++) {
        if (lb->boxes()[i]->isInlineBox()) {
            InlineBox* b = lb->boxes()[i]->asInlineBox();
            if (b->isInlineNonReplacedBox()) {
                result = findLastInlineBoxNonReplacedBoxCase(b->asInlineNonReplacedBox());
            } else {
                result = b;
            }
        } else if (lb->boxes()[i]->isNormalFlow()) {
            result = lb->boxes()[i];
        }
    }

    return result;
}

void LineFormattingContext::completeLastLine()
{
    LineBox* back = m_block.m_lineBoxes.back();
    FrameBox* last = findLastInlineBox(back);
    if (last && last->isInlineBox() && last->asInlineBox()->isInlineTextBox()) {
        if (last->asInlineBox()->asInlineTextBox()->text()->equals(String::spaceString)) {
            if (last->layoutParent()->asFrameBox()->isLineBox()) {
                std::vector<FrameBox*, gc_allocator<FrameBox*> >& boxes = last->layoutParent()->asFrameBox()->asLineBox()->boxes();
                boxes.erase(std::find(boxes.begin(), boxes.end(), last));
            } else {
                std::vector<FrameBox*, gc_allocator<FrameBox*> >& boxes = last->layoutParent()->asFrameBox()->asInlineBox()->asInlineNonReplacedBox()->boxes();
                boxes.erase(std::find(boxes.begin(), boxes.end(), last));
            }
        }
    }
}

void LineFormattingContext::breakLine(bool dueToBr)
{
    if (dueToBr == false)
        m_breakedLinesSet.insert(m_block.m_lineBoxes.size() - 1);

    LayoutUnit ascender;
    LayoutUnit descender;
    LineBox* back = m_block.m_lineBoxes.back();
    computeVerticalProperties(back, m_block.style(), ascender, descender, *this);
    back->m_ascender = ascender;
    back->m_descender = descender;
    back->setHeight(back->ascender() - back->decender());
    completeLastLine();

    m_block.m_lineBoxes.push_back(new LineBox(&m_block));
    m_block.m_lineBoxes.back()->setWidth(m_lineBoxWidth);
    m_currentLine++;
    m_currentLineWidth = 0;
}

template <typename fn>
void textDividerForLayout(String* txt, fn f)
{
    // TODO consider white-space
    unsigned offset = 0;
    while (true) {
        if (offset >= txt->length())
            break;
        bool isWhiteSpace = false;
        if (String::isSpaceOrNewline(txt->charAt(offset))) {
            isWhiteSpace = true;
        }

        // find next space
        unsigned nextOffset = offset + 1;
        if (isWhiteSpace) {
            while (nextOffset < txt->length() && String::isSpaceOrNewline((*txt)[nextOffset])) {
                nextOffset++;
            }
        } else {
            while (nextOffset < txt->length() && !String::isSpaceOrNewline((*txt)[nextOffset])) {
                // https://www.w3.org/TR/css-text-3/#soft-wrap-opportunity
                // TODO use icu for other language (or can we implement these things?)
                bool willBreak = (*txt)[nextOffset] == '-';
                nextOffset++;
                if (willBreak) {
                    break;
                }
            }
        }

        f(offset, nextOffset, isWhiteSpace);
        offset = nextOffset;
    }
}

void inlineBoxGenerator(Frame* origin, LayoutContext& ctx, LineFormattingContext& lineFormattingContext, LayoutUnit inlineContentWidth,
    std::function<void(FrameBox*)> gotInlineBoxCallback, std::function<void(bool)> lineBreakCallback, std::function<void(FrameInline*)> frameInlineCallback)
{
    Frame* f = origin->firstChild();
    while (f) {
        if (!f->isNormalFlow()) {
            LayoutUnit preferredWidth;
            if (f->asFrameBox()->shouldComputePreferredWidth()) {
                ComputePreferredWidthContext p(ctx, inlineContentWidth);
                f->asFrameBox()->computePreferredWidth(p);
                preferredWidth = p.result();
            } else {
                preferredWidth = f->style()->width().specifiedValue(inlineContentWidth);
            }

            if ((preferredWidth) > (inlineContentWidth - lineFormattingContext.m_currentLineWidth)) {
                lineBreakCallback(false);
            }
            f->asFrameBox()->setLayoutParent(lineFormattingContext.currentLine());
            lineFormattingContext.currentLine()->boxes().push_back(f->asFrameBox());
            ctx.registerAbsolutePositionedFrames(f->asFrameBox());
            f = f->next();
            continue;
        }

        if (f->isFrameText()) {
            String* txt = f->asFrameText()->text();
            textDividerForLayout(txt, [&](size_t offset, size_t nextOffset, bool isWhiteSpace) {
                textAppendRetry:
                if (isWhiteSpace) {
                    if (offset == 0 && f == origin->firstChild()) {
                        return;
                    } else if (nextOffset == txt->length() && f == origin->lastChild()) {
                        return;
                    } else if (lineFormattingContext.m_currentLineWidth == 0) {
                        return;
                    }
                }

                LayoutUnit textWidth;
                String* resultString;
                if (isWhiteSpace) {
                    resultString = String::spaceString;
                    textWidth = f->style()->font()->spaceWidth();
                } else {
                    String* ss = txt->substring(offset, nextOffset - offset);
                    resultString = ss;
                    textWidth = f->style()->font()->measureText(ss);
                }

                if (textWidth <= inlineContentWidth - lineFormattingContext.m_currentLineWidth || lineFormattingContext.m_currentLineWidth == 0) {

                } else {
                    if (isWhiteSpace)
                        return;
                    // try this at nextline
                    lineBreakCallback(false);
                    goto textAppendRetry;
                }
                InlineBox* ib = new InlineTextBox(f->node(), f->style(), nullptr, resultString, f->asFrameText());
                ib->setWidth(textWidth);
                ib->setHeight(f->style()->font()->metrics().m_fontHeight);
                lineFormattingContext.m_currentLineWidth += textWidth;
                gotInlineBoxCallback(ib);
            });

        } else if (f->isFrameReplaced()) {
            FrameReplaced* r = f->asFrameReplaced();
            r->layout(ctx);

        insertReplacedBox:
            if ((r->width() + r->marginWidth()) <= (inlineContentWidth - lineFormattingContext.m_currentLineWidth) || lineFormattingContext.m_currentLineWidth == 0) {
                lineFormattingContext.m_currentLineWidth += (r->width() + r->marginWidth());
                gotInlineBoxCallback(r);
            } else {
                lineBreakCallback(false);
                goto insertReplacedBox;
            }
        } else if (f->isFrameBlockBox()) {
            // inline-block
            FrameBlockBox* r = f->asFrameBlockBox();
            STARFISH_ASSERT(f->style()->display() == DisplayValue::InlineBlockDisplayValue);
            f->layout(ctx);

            LayoutUnit ascender = 0;
            if (ctx.lastLineBox() && r->isAncestorOf(ctx.lastLineBox()) && r->style()->overflow() == OverflowValue::VisibleOverflow) {
                LayoutUnit topToLineBox = ctx.lastLineBox()->absolutePointWithoutRelativePosition(r).y();
                ascender = topToLineBox + ctx.lastLineBox()->ascender();
            } else {
                ascender = f->asFrameBox()->height();
            }
            lineFormattingContext.registerInlineBlockAscender(ascender, r);

        insertBlockBox:
            if ((r->width() + r->marginWidth()) <= (inlineContentWidth - lineFormattingContext.m_currentLineWidth) || lineFormattingContext.m_currentLineWidth == 0) {
                lineFormattingContext.m_currentLineWidth += (r->width() + r->marginWidth());
                gotInlineBoxCallback(r);
            } else {
                lineBreakCallback(false);
                goto insertBlockBox;
            }
        } else if (f->isFrameLineBreak()) {
            lineBreakCallback(true);
        } else if (f->isFrameInline()) {
            frameInlineCallback(f->asFrameInline());
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        f = f->next();
    }
}

std::pair<LayoutUnit, LayoutRect> FrameBlockBox::layoutInline(LayoutContext& ctx)
{
    LayoutRect visibleRect(0, 0, 0, 0);
    LayoutUnit lineBoxX = paddingLeft() + borderLeft();
    LayoutUnit lineBoxY = paddingTop() + borderTop();
    LayoutUnit inlineContentWidth = contentWidth();
    LineFormattingContext lineFormattingContext(*this, ctx, inlineContentWidth);

    inlineBoxGenerator(this, ctx, lineFormattingContext, inlineContentWidth, [&](FrameBox* ib) {
        lineFormattingContext.currentLine()->boxes().push_back(ib);
        ib->setLayoutParent(lineFormattingContext.currentLine());
        lineFormattingContext.registerInlineContent();
    }, [&](bool dueToBr)
    {
        lineFormattingContext.breakLine(dueToBr);
    }, [&](FrameInline* f)
    {
        lineFormattingContext.registerInlineContent();
        InlineNonReplacedBox* inlineBox = new InlineNonReplacedBox(f->node(), f->node()->style(), nullptr, f->asFrameInline());
        InlineNonReplacedBox::layoutInline(inlineBox, ctx, this, &lineFormattingContext, nullptr, true);
    });

    LineBox* back = m_lineBoxes.back();
    LayoutUnit ascender;
    LayoutUnit descender;
    LayoutUnit minimumHeight;
    LayoutUnit h = computeVerticalProperties(back, style(), ascender, descender, lineFormattingContext);
    back->m_ascender = ascender;
    back->m_descender = descender;
    back->m_frameRect.setHeight(h);

    lineFormattingContext.completeLastLine();

    if (m_lineBoxes.size() == 1 && m_lineBoxes.back()->boxes().size() == 0) {
        m_lineBoxes.clear();
        m_lineBoxes.shrink_to_fit();
    }

    // position each line
    LayoutUnit contentHeight = 0;
    for (size_t i = 0; i < m_lineBoxes.size(); i++) {
        LineBox& b = *m_lineBoxes[i];
        STARFISH_ASSERT(b.isLineBox());

        b.m_frameRect.setX(lineBoxX);
        b.m_frameRect.setY(contentHeight + lineBoxY);

        // align boxes
        LayoutUnit x = 0;
        for (size_t k = 0; k < b.m_boxes.size(); k++) {
            FrameBox* childBox = b.m_boxes[k];
            childBox->setX(x + childBox->marginLeft());
            if (childBox->isNormalFlow())
                x += childBox->width() + childBox->marginWidth();
        }

        // text align
        if (style()->textAlign() == TextAlignValue::LeftTextAlignValue) {
        } else if (style()->textAlign() == TextAlignValue::RightTextAlignValue) {
            LayoutUnit diff = (inlineContentWidth - x);
            for (size_t k = 0; k < b.m_boxes.size(); k++) {
                FrameBox* childBox = b.m_boxes[k];
                childBox->moveX(diff);
            }
        } else if (style()->textAlign() == TextAlignValue::JustifyTextAlignValue) {
            // issue #145
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
            /*
            if (lineFormattingContext.isBreakedLineWithoutBR(i)) {
                LayoutUnit remainSpace = (inlineContentWidth - x);
                if (remainSpace > 0) {
                    size_t spaceBoxCnt = 0;
                    for (size_t j = 0; j < b.m_boxes.size(); j++) {
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
                        LayoutUnit moreWidthForSpace = remainSpace / spaceBoxCnt;
                        LayoutUnit diff = 0;
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
            */
        } else {
            STARFISH_ASSERT(style()->textAlign() == TextAlignValue::CenterTextAlignValue);
            LayoutUnit diff = (inlineContentWidth - x) / 2;
            if (diff > 0) {
                for (size_t k = 0; k < b.m_boxes.size(); k++) {
                    FrameBox* childBox = b.m_boxes[k];
                    childBox->moveX(diff);
                }
            }
        }

        for (size_t k = 0; k < b.m_boxes.size(); k++) {
            FrameBox* childBox = b.m_boxes[k];
            LayoutRect vr = childBox->absoluteRect(this);
            visibleRect.unite(vr);
        }

        contentHeight += b.m_frameRect.height();
    }

    return std::make_pair(contentHeight, visibleRect);
}

InlineNonReplacedBox* InlineNonReplacedBox::layoutInline(InlineNonReplacedBox* self, LayoutContext& ctx,
    FrameBlockBox* blockBox, LineFormattingContext* lfc, InlineNonReplacedBox* layoutParentBox, bool freshStart)
{
    LineFormattingContext& lineFormattingContext = *lfc;
    // just lay out children are horizontally.
    LayoutUnit inlineContentWidth = blockBox->contentWidth();

    if (freshStart)
        self->computeBorderMarginPadding(inlineContentWidth);
    LayoutUnit w = self->marginLeft() + self->borderLeft() + self->paddingLeft();
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
            lineFormattingContext.currentLine()->boxes().push_back(self);
            self->setLayoutParent(lineFormattingContext.currentLine());
        } else {
            layoutParentBox->boxes().push_back(self);
            self->setLayoutParent(layoutParentBox);
        }

        lineFormattingContext.registerInlineContent();
    }

    lineFormattingContext.m_currentLineWidth += w;
    self->setWidth(self->paddingLeft() + self->borderLeft());

    auto breakLine = [&]()
    {
        std::vector<InlineNonReplacedBox*> stack;

        FrameBox* addingUpWidth = nullptr;
        Frame* currentSelf = self;
        while (!currentSelf->asFrameBox()->isLineBox()) {
            stack.push_back(currentSelf->asFrameBox()->asInlineBox()->asInlineNonReplacedBox());

            if (addingUpWidth) {
                currentSelf->asFrameBox()->setWidth(currentSelf->asFrameBox()->width() + addingUpWidth->width() + addingUpWidth->marginWidth());
            }

            addingUpWidth = currentSelf->asFrameBox();
            currentSelf = currentSelf->layoutParent();
        }

        lineFormattingContext.breakLine();

        bool first = true;
        auto iter = stack.rbegin();
        InlineNonReplacedBox* last = nullptr;
        while (stack.rend() != iter) {
            InlineNonReplacedBox* origin = *iter;
            InlineNonReplacedBox* newBox = new InlineNonReplacedBox(origin->node(), origin->node()->style(), nullptr, origin->m_origin);

            if (first) {
                lineFormattingContext.currentLine()->boxes().push_back(newBox);
                newBox->setLayoutParent(lineFormattingContext.currentLine());
                first = false;
            } else {
                last->boxes().push_back(newBox);
                newBox->setLayoutParent(last);
            }
            last = newBox;

            // TODO consider rtl
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

    auto finishLayout = [&](bool lastNode = false)
    {
        InlineNonReplacedBox* selfForFinishLayout = self;
        while (selfForFinishLayout) {
            LayoutUnit end = lineFormattingContext.m_currentLineWidth;
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

            LayoutUnit ascender = std::max(selfForFinishLayout->style()->font()->metrics().m_ascender, blockBox->style()->font()->metrics().m_ascender);
            LayoutUnit descender = std::min(selfForFinishLayout->style()->font()->metrics().m_descender, blockBox->style()->font()->metrics().m_descender);

            computeVerticalProperties(selfForFinishLayout, selfForFinishLayout->style(), ascender, descender, lineFormattingContext);
            selfForFinishLayout->m_ascender = ascender;
            selfForFinishLayout->m_descender = descender;

            selfForFinishLayout->setContentHeight(ascender - descender);
            if (selfForFinishLayout->layoutParent()->asFrameBox()->isLineBox()) {
                break;
            }
            if (lastNode) {
                break;
            }
            selfForFinishLayout = selfForFinishLayout->layoutParent()->asFrameBox()->asInlineBox()->asInlineNonReplacedBox();
        }
    };

#define BREAK_LINE() \
    finishLayout();  \
    breakLine();

    inlineBoxGenerator(self->m_origin, ctx, lineFormattingContext, inlineContentWidth, [&](FrameBox* ib)
    {
        self->boxes().push_back(ib);
        ib->setLayoutParent(self);
        ib->setX(self->width() + ib->marginLeft());
        self->setWidth(self->width() + ib->width() + ib->marginWidth());
    }, [&](bool dueToBr)
    {
        // FIXME consider dueToBr
        BREAK_LINE();
    }, [&](FrameInline* f)
    {
        InlineNonReplacedBox* inlineBox = new InlineNonReplacedBox(f->node(), f->node()->style(), nullptr, f->asFrameInline());
        inlineBox->setX(self->width());
        InlineNonReplacedBox* result = InlineNonReplacedBox::layoutInline(inlineBox, ctx, blockBox, lfc, self, true);
        Frame* newSelfCandi = result->layoutParent();
        if (newSelfCandi->asFrameBox()->isInlineBox() && newSelfCandi->asFrameBox()->asInlineBox()->isInlineNonReplacedBox()) {
            InlineNonReplacedBox* newSelf = newSelfCandi->asFrameBox()->asInlineBox()->asInlineNonReplacedBox();
            if (self != newSelf) {
                self = newSelf;
            }
        }
        self->setWidth(self->width() + result->width() + result->marginWidth());
    });

    finishLayout(true);

    return self;
}

void FrameBlockBox::computePreferredWidth(ComputePreferredWidthContext& ctx)
{
    LayoutUnit remainWidth = ctx.lastKnownWidth();
    LayoutUnit minWidth = 0;

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

        LayoutUnit currentLineWidth = 0;
        std::function<void(Frame*)> computeInlineLayout = [&](Frame* f)
        {

            // current
            if (!f->isNormalFlow()) {
                return;
            }

            if (f->isFrameText()) {
                String* s = f->asFrameText()->text();
                textDividerForLayout(s, [&](size_t offset, size_t nextOffset, bool isWhiteSpace) {
                    if (isWhiteSpace) {
                        if (offset == 0 && f == f->parent()->firstChild()) {
                            return;
                        } else if (nextOffset == s->length() && f == f->parent()->lastChild()) {
                            return;
                        }
                    }

                    LayoutUnit w = 0;
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
                auto checkMBP = [&](Length l)
                {
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
                LayoutUnit w = f->asFrameReplaced()->intrinsicSize().width();

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

void FrameBlockBox::paintChildrenWith(Canvas* canvas, PaintingStage stage)
{
    if (hasBlockFlow()) {
        FrameBox::paintChildrenWith(canvas, stage);
    } else {
        for (size_t i = 0; i < m_lineBoxes.size(); i++) {
            canvas->save();
            LineBox& b = *m_lineBoxes[i];
            canvas->translate(b.frameRect().x(), b.frameRect().y());
            for (size_t k = 0; k < b.m_boxes.size(); k++) {
                FrameBox* childBox = b.m_boxes[k];
                canvas->save();
                canvas->translate(childBox->x(), childBox->y());
                childBox->paint(canvas, stage);
                canvas->restore();
            }
            canvas->restore();
        }
    }
}

void InlineTextBox::paint(Canvas* canvas, PaintingStage stage)
{
    if (stage == PaintingNormalFlowInline) {
        if (m_origin->textDecorationData()) {
            auto data = m_origin->textDecorationData();
            canvas->setNeedsLineThrough(data->m_hasLineThrough);
            canvas->setNeedsUnderline(data->m_hasUnderLine);
            canvas->setLineThroughColor(data->m_lineThroughColor);
            canvas->setUnderlineColor(data->m_underLineColor);
        }

        canvas->setFont(style()->font());
        canvas->setColor(style()->color());
        canvas->drawText(0, 0, m_text);
    }
}

void InlineNonReplacedBox::paintBackgroundAndBorders(Canvas* canvas)
{
    LayoutRect frameRectBack = m_frameRect;
    LayoutBoxSurroundData paddingBack = m_padding, borderBack = m_border, marginBack = m_margin;

    m_padding = m_orgPadding;
    m_margin = m_orgMargin;
    m_border = m_orgBorder;

    canvas->save();
    canvas->translate(LayoutUnit(0), -y());
    moveY(height());
    setContentHeight(style()->font()->metrics().m_ascender - style()->font()->metrics().m_descender);
    moveY(-contentHeight() - paddingTop() - borderTop());
    setHeight(contentHeight() + paddingHeight() + borderHeight());

    canvas->translate(LayoutUnit(0), y());
    FrameBox::paintBackgroundAndBorders(canvas);
    canvas->restore();

    m_frameRect = frameRectBack;
    m_padding = paddingBack;
    m_border = borderBack;
    m_margin = marginBack;
}

void InlineNonReplacedBox::paint(Canvas* canvas, PaintingStage stage)
{
    if (isEstablishesStackingContext()) {
        return;
    }
    // CHECK THIS at https://www.w3.org/TR/CSS2/zindex.html#stacking-defs
    if (stage == PaintingNormalFlowInline) {
        paintBackgroundAndBorders(canvas);
        paintChildrenWith(canvas, stage);
    }
}

Frame* InlineNonReplacedBox::hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage)
{
    if (isEstablishesStackingContext()) {
        return nullptr;
    }

    if (stage == HitTestStage::HitTestNormalFlowInline) {
        Frame* result = nullptr;
        HitTestStage s = HitTestStage::HitTestPositionedElements;

        while (s != HitTestStageEnd) {
            auto iter = boxes().rbegin();
            while (iter != boxes().rend()) {
                FrameBox* f = *iter;
                LayoutUnit cx = x - f->asFrameBox()->x();
                LayoutUnit cy = y - f->asFrameBox()->y();
                result = f->hitTest(cx, cy, s);
                if (result)
                    return result;
                iter++;
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
    auto iter = boxes().begin();
    while (iter != boxes().end()) {
        FrameBox* f = *iter;
        for (int k = 0; k < depth + 1; k++)
            printf("  ");
        printf("%s", f->name());
        f->dump(depth + 2);
        if (iter + 1 != boxes().end())
            puts("");
        iter++;
    }
}
}
