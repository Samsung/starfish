#include "StarFishConfig.h"
#include "FrameBlockBox.h"
#include "FrameText.h"
#include "FrameInline.h"

namespace StarFish {

void computeVerticalProperties(FrameBox* parentBox, LayoutUnit& ascenderInOut,LayoutUnit& descenderInOut, LayoutContext& ctx)
{
    // TODO vertical align(length, text-top.., etc)
    bool hasSpecialCase = false;
    LayoutUnit maxAscender = ascenderInOut;
    LayoutUnit maxDescender = descenderInOut;
    LayoutUnit mostBiggestBoxHeight = 0;

    size_t k = 0;
    Frame* f = parentBox->firstChild();

#define NEXT_STOP_COMPUTE_VERTICAL() \
    if (parentBox->isLineBox()) { \
        k++; \
    } else { \
        if (!f) \
            break; \
        f = f->next(); \
    } \

    while (true) {
        if (parentBox->isLineBox()) {
            if (parentBox->asLineBox()->boxes().size() == k) {
                break;
            }
            f = parentBox->asLineBox()->boxes()[k];
        } else {
            if (!f)
                break;
        }

        if (f->asFrameBox()->isInlineBox()) {
            InlineBox* ib = f->asFrameBox()->asInlineBox();
            if (ib->isInlineBox()) {
                // normal flow
                if (ib->isInlineTextBox()) {
                    maxAscender = std::max(ib->asInlineTextBox()->style()->font()->metrics().m_ascender, maxAscender);
                    maxDescender = std::min(ib->asInlineTextBox()->style()->font()->metrics().m_descender, maxDescender);

                    NEXT_STOP_COMPUTE_VERTICAL();

                    continue;
                }
                VerticalAlignValue va = ib->style()->verticalAlign();
                if (va == VerticalAlignValue::BaselineVAlignValue) {
                    if (ib->isInlineReplacedBox()) {
                        // TODO use this code for when replaced content does not have content
                        /*
                        LayoutUnit asc = ib->marginTop() + ib->asInlineReplacedBox()->replacedBox()->borderTop() + ib->asInlineReplacedBox()->replacedBox()->paddingTop()
                                + ib->asInlineReplacedBox()->replacedBox()->contentHeight();
                        LayoutUnit dec = -(ib->asInlineReplacedBox()->replacedBox()->paddingBottom() + ib->asInlineReplacedBox()->replacedBox()->borderBottom()
                                + ib->marginBottom());
                        maxAscender = std::max(asc, maxAscender);
                        maxDescender = std::min(dec, maxDescender);
                        */
                        LayoutUnit asc = ib->height() + ib->marginHeight();
                        maxAscender = std::max(asc, maxAscender);
                    } else if (ib->isInlineBlockBox()) {
                        if (ib->asInlineBlockBox()->ascender() == ib->height()) {
                            maxAscender = std::max(ib->asInlineBlockBox()->ascender() + ib->marginHeight(), maxAscender);
                            maxDescender = std::min(LayoutUnit(0), maxDescender);
                        } else {
                            LayoutUnit dec = -(ib->height() - ib->asInlineBlockBox()->ascender()) - ib->marginBottom();
                            maxAscender = std::max(ib->asInlineBlockBox()->ascender() + ib->marginTop(), maxAscender);
                            maxDescender = std::min(dec, maxDescender);
                        }
                    } else {
                        maxAscender = std::max(ib->asInlineNonReplacedBox()->ascender(), maxAscender);
                        maxDescender = std::min(ib->asInlineNonReplacedBox()->decender(), maxDescender);
                    }
                } else if (va == VerticalAlignValue::MiddleVAlignValue) {
                    LayoutUnit height = ib->height();
                    maxAscender = std::max(height/2 + ib->marginTop(), maxAscender);
                    maxDescender = std::min(-height/2 - ib->marginBottom(), maxDescender);
                } else if (va == VerticalAlignValue::TopVAlignValue || va == VerticalAlignValue::BottomVAlignValue) {
                    hasSpecialCase = true;
                    LayoutUnit height = ib->height() + ib->marginHeight();
                    mostBiggestBoxHeight = std::max(mostBiggestBoxHeight, height);
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }

            } else {
                // out of flow boxes
                f->asFrameBox()->setY(f->asFrameBox()->marginTop());
            }
        }

        NEXT_STOP_COMPUTE_VERTICAL();

    }

    if (hasSpecialCase) {
        while((maxAscender - maxDescender) < mostBiggestBoxHeight) {
            f = parentBox->firstChild();
            k = 0;
            while (true) {
                if (parentBox->isLineBox()) {
                    if (parentBox->asLineBox()->boxes().size() == k) {
                        break;
                    }
                    f = parentBox->asLineBox()->boxes()[k];
                } else {
                    if (!f)
                        break;
                }
                InlineBox* ib = f->asFrameBox()->asInlineBox();
                VerticalAlignValue va = ib->style()->verticalAlign();
                LayoutUnit height = ib->height() + ib->marginHeight();
                if (height > maxAscender - maxDescender) {
                    if (va == VerticalAlignValue::TopVAlignValue) {
                        LayoutUnit dec = -height + maxAscender;
                        maxDescender = std::min(maxDescender, dec);
                    } else if (va == VerticalAlignValue::BottomVAlignValue) {
                        LayoutUnit asc = height + maxDescender;
                        maxAscender = std::max(maxAscender, asc);
                    }
                }

                NEXT_STOP_COMPUTE_VERTICAL();
            }
        }
    }

    ascenderInOut = maxAscender;
    descenderInOut = maxDescender;

    LayoutUnit height = maxAscender - maxDescender;
    f = parentBox->firstChild();
    k = 0;
    while (true) {
        if (parentBox->isLineBox()) {
            if (parentBox->asLineBox()->boxes().size() == k) {
                break;
            }
            f = parentBox->asLineBox()->boxes()[k];
        } else {
            if (!f)
                break;
        }

        if (f->asFrameBox()->isInlineBox()) {
            InlineBox* ib = f->asFrameBox()->asInlineBox();
            if (ib->isInlineTextBox()) {
                ib->setY(height + maxDescender - ib->height() - ib->asInlineTextBox()->style()->font()->metrics().m_descender);
            } else {
                VerticalAlignValue va = ib->style()->verticalAlign();
                if (va == VerticalAlignValue::BaselineVAlignValue) {
                    if (ib->isInlineReplacedBox()) {
                        // TODO use this code for when replaced content does not have content
                        /*
                        LayoutUnit asc = ib->marginTop() + ib->asInlineReplacedBox()->replacedBox()->borderTop() + ib->asInlineReplacedBox()->replacedBox()->paddingTop()
                                    + ib->asInlineReplacedBox()->replacedBox()->contentHeight();
                        ib->setY(height + maxDescender - asc + ib->marginTop());
                        */
                        LayoutUnit asc = ib->height() + ib->marginHeight();
                        ib->setY(height + maxDescender - asc + ib->marginTop());
                    } else if (ib->isInlineBlockBox()) {
                        if (ib->asInlineBlockBox()->ascender() == ib->height()) {
                            LayoutUnit dec = 0;
                            ib->setY(height + maxDescender - ib->height() - dec - ib->marginTop());
                        } else {
                            LayoutUnit dec = -(ib->height() - ib->asInlineBlockBox()->ascender()) - ib->marginBottom();
                            ib->setY(height + maxDescender - ib->height() - dec - ib->marginTop());
                        }
                    } else {
                        STARFISH_ASSERT(ib->isInlineNonReplacedBox());
                        ib->setY(height + maxDescender - ib->height() - ib->asInlineNonReplacedBox()->decender());
                    }
                } else if (va == VerticalAlignValue::MiddleVAlignValue) {
                    LayoutUnit dec = -(ib->height() / 2);
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

        NEXT_STOP_COMPUTE_VERTICAL();
    }
}

template <typename fn>
void textDividerForLayout(String* txt, fn f)
{
    // TODO consider white-space
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
        } else {
            while(nextOffset < txt->length() && !String::isSpaceOrNewline((*txt)[nextOffset])) {
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
        std::function<void (InlineBox*)> gotInlineBoxCallback, std::function<void (bool)> lineBreakCallback, std::function<void (FrameInline*)> frameInlineCallback)
{
    Frame* f = origin->firstChild();
    while(f) {
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

                } else {
                    if (isWhiteSpace)
                        return;
                    // try this at nextline
                    lineBreakCallback(false);
                    goto textAppendRetry;
                }
                InlineBox* ib = new InlineTextBox(f->node(), f->style(), nullptr, resultString);
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
                InlineBox* ib = new InlineReplacedBox(f->node(), f->style(), nullptr, r);
                ib->setMarginLeft(r->marginLeft());
                ib->setMarginTop(r->marginTop());
                ib->setMarginRight(r->marginRight());
                ib->setMarginBottom(r->marginBottom());
                ib->setWidth(r->width());
                ib->setHeight(r->height());
                lineFormattingContext.m_currentLineWidth += (ib->width() + ib->marginWidth());
                gotInlineBoxCallback(ib);
            } else {
                lineBreakCallback(false);
                goto insertReplacedBox;
            }
        } else if (f->isFrameBlockBox()) {
            // inline-block
            FrameBlockBox* r = f->asFrameBlockBox();
            f->layout(ctx);

            LayoutUnit ascender = 0;
            if (ctx.lastLineBox() && r->isAncestorOf(ctx.lastLineBox())) {
                LayoutUnit topToLineBox = ctx.lastLineBox()->absolutePoint(r).y();
                ascender = topToLineBox + ctx.lastLineBox()->ascender();
                if (ascender > f->asFrameBox()->height()) {
                    ascender = f->asFrameBox()->height();
                }
            } else {
                ascender = f->asFrameBox()->height();
            }

            insertBlockBox:
            if ((r->width() + r->marginWidth()) <= (inlineContentWidth - lineFormattingContext.m_currentLineWidth) || lineFormattingContext.m_currentLineWidth == 0) {
                InlineBox* ib = new InlineBlockBox(f->node(), f->style(), nullptr, r, ascender);
                ib->setMarginLeft(r->marginLeft());
                ib->setMarginTop(r->marginTop());
                ib->setMarginRight(r->marginRight());
                ib->setMarginBottom(r->marginBottom());
                ib->setWidth(r->width());
                ib->setHeight(r->height());
                lineFormattingContext.m_currentLineWidth += (ib->width() + ib->marginWidth());
                gotInlineBoxCallback(ib);
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

LayoutUnit FrameBlockBox::layoutInline(LayoutContext& ctx)
{
    LayoutUnit lineBoxX = paddingLeft() + borderLeft();
    LayoutUnit lineBoxY = paddingTop() + borderTop();
    LayoutUnit inlineContentWidth = contentWidth();
    LineFormattingContext lineFormattingContext(*this, ctx);

    inlineBoxGenerator(this, ctx, lineFormattingContext, inlineContentWidth, [&](InlineBox* ib) {
        lineFormattingContext.currentLine()->boxes().push_back(ib);
        ib->setParent(lineFormattingContext.currentLine());
        lineFormattingContext.registerInlineContent();
    }, [&](bool dueToBr) {
        std::vector<FrameBox*, gc_allocator<FrameBox*>>& vector = lineFormattingContext.currentLine()->boxes();
        auto iter = vector.rbegin();
        size_t i = vector.size();
        while (vector.rend() != iter) {
            i--;
            if ((*iter)->isInlineBox()) {
                InlineBox* ib = (*iter)->asInlineBox();
                if (ib->isInlineTextBox()) {
                    if (ib->asInlineTextBox()->text()->equals(String::spaceString)) {
                        vector.erase(i + vector.begin());
                    }
                }
                break;
            }
            iter ++;
        }

        lineFormattingContext.breakLine(dueToBr);
    }, [&](FrameInline* f) {
        lineFormattingContext.registerInlineContent();
        InlineNonReplacedBox* inlineBox = new InlineNonReplacedBox(f->node(), f->node()->style(), nullptr, f->asFrameInline());
        InlineNonReplacedBox::layoutInline(inlineBox, ctx, this, &lineFormattingContext, nullptr, true);
    });

    // position each line
    LayoutUnit contentHeight = 0;
    for (size_t i = 0; i < m_lineBoxes.size(); i ++) {
        LineBox& b = *m_lineBoxes[i];
        STARFISH_ASSERT(b.isLineBox());

        b.m_frameRect.setX(lineBoxX);
        b.m_frameRect.setY(contentHeight + lineBoxY);

        // align boxes
        LayoutUnit x = 0;
        for (size_t k = 0; k < b.m_boxes.size(); k ++) {
            FrameBox* childBox = b.m_boxes[k];
            childBox->setX(x + childBox->marginLeft());
            if (childBox->isNormalFlow())
                x += childBox->width() + childBox->marginWidth();
        }

        // text align
        if (style()->textAlign() == TextAlignValue::LeftTextAlignValue) {
        } else if (style()->textAlign() == TextAlignValue::RightTextAlignValue) {
            LayoutUnit diff = (inlineContentWidth - x);
            for (size_t k = 0; k < b.m_boxes.size(); k ++) {
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
            for (size_t k = 0; k < b.m_boxes.size(); k ++) {
                FrameBox* childBox = b.m_boxes[k];
                childBox->moveX(diff);
            }
        }

        LayoutUnit ascender = 0;
        LayoutUnit descender = 0;
        computeVerticalProperties(&b, ascender, descender, ctx);
        b.m_ascender = ascender;
        b.m_descender = descender;
        b.m_frameRect.setWidth(inlineContentWidth);
        b.m_frameRect.setHeight(ascender - descender);
        contentHeight += b.m_frameRect.height();
    }

    return contentHeight;
}

InlineNonReplacedBox* InlineNonReplacedBox::layoutInline(InlineNonReplacedBox* self, LayoutContext& ctx,
        FrameBlockBox* blockBox, LineFormattingContext* lfc, FrameBox* layoutParentBox, bool freshStart)
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
                lineFormattingContext.currentLine()->boxes().push_back(newBox);
                newBox->setParent(lineFormattingContext.currentLine());
                first = false;
            } else {
                last->appendChild(newBox);
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

    auto finishLayout = [&](bool lastNode = false) {
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

            LayoutUnit ascender = selfForFinishLayout->style()->font()->metrics().m_ascender;
            LayoutUnit descender = selfForFinishLayout->style()->font()->metrics().m_descender;

            computeVerticalProperties(selfForFinishLayout, ascender, descender, ctx);
            selfForFinishLayout->m_ascender = ascender;
            selfForFinishLayout->m_descender = descender;

            selfForFinishLayout->setContentHeight(ascender - descender);
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


    inlineBoxGenerator(self->m_origin, ctx, lineFormattingContext, inlineContentWidth, [&](InlineBox* ib) {
        STARFISH_ASSERT(ib->isInlineBox());
        self->appendChild(ib);
        ib->setX(self->width() + ib->marginLeft());
        self->setWidth(self->width() + ib->width() + ib->marginWidth());
    }, [&](bool dueToBr) {
        // FIXME consider dueToBr
        BREAK_LINE();
    }, [&](FrameInline* f) {
        InlineNonReplacedBox* inlineBox = new InlineNonReplacedBox(f->node(), f->node()->style(), nullptr, f->asFrameInline());
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
        std::function<void(Frame*)> computeInlineLayout = [&](Frame* f) {

            // current
            if (!f->isNormalFlow()) {
                return;
            }

            if (f->isFrameText()) {
                String* s = f->asFrameText()->text();
                textDividerForLayout(s, [&](size_t offset, size_t nextOffset, bool isWhiteSpace){
                    if (isWhiteSpace) {
                        if (offset == 0 && f == f->parent()->firstChild()) {
                            return;
                        } else if(nextOffset == s->length() && f == f->parent()->lastChild()) {
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
            for (size_t k = 0; k < b.m_boxes.size(); k ++) {
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

Frame* InlineBlockBox::hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage)
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

void InlineNonReplacedBox::paint(Canvas* canvas, PaintingStage stage)
{
    // CHECK THIS at https://www.w3.org/TR/CSS2/zindex.html#stacking-defs
    if (stage == PaintingNormalFlowInline) {
        LayoutRect frameRectBack = m_frameRect;
        LayoutBoxSurroundData paddingBack = m_padding, borderBack = m_border, marginBack = m_margin;

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

Frame* InlineNonReplacedBox::hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage)
{
    if (stage == HitTestStage::HitTestNormalFlowInline) {
        Frame* result = nullptr;
        HitTestStage s = HitTestStage::HitTestPositionedElements;

        while (s != HitTestStageEnd) {
            Frame* f = lastChild();
            while (f) {
                LayoutUnit cx = x - f->asFrameBox()->x();
                LayoutUnit cy = y - f->asFrameBox()->y();
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

}
