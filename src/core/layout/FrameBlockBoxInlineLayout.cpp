/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/CharacterData.h"
#include "core/dom/Node.h"
#include "core/dom/HTMLTableElement.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameBlockBoxInlineLayout.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/FrameFlexibleBox.h"
#include "core/layout/FrameInline.h"
#include "core/layout/FrameTableBox.h"
#include "core/layout/StackingContext.h"
#include "core/modules/canvas/Canvas.h"
#include "core/util/LineBreakerIteratorPool.h"
#include "core/modules/canvas/ShadowBlur.h"
#include "core/modules/canvas/image/NativeImageData.h"

namespace StarFish {

LayoutUnit Frame::lineHeight()
{
    LayoutUnit fontSize = style()->font()->metrics().m_ascender -
                          style()->font()->metrics().m_descender;

    if (!style()->hasNormalLineHeight()) {
        Length lineHeight = style()->lineHeight();
        if (lineHeight.isSpecified()) {
            return lineHeight.specifiedValue(fontSize, this);
        } else if (lineHeight.isInheritableNumber()) {
            return fontSize * lineHeight.inheritableNumber();
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }
    return fontSize;
}

void* LineBox::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(LineBox)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(LineBox, m_node));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(LineBox, m_layoutParent));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(LineBox, m_boxes));
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(LineBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void LineFormattingContext::computeVerticalProperties(FrameBox* parentBox,
                                                      bool dueToBr)
{
    LayoutUnit ascender;
    LayoutUnit descender;
    ComputedStyle* parentStyle;
    bool hasBoxOtherThanText = false;
    bool hasBoxOtherThanCollapsedInlineNonReplacedBox = false;
    bool hasNormalFlowChild = false;

    GCVector<FrameBox*>& boxes =
        parentBox->asInlineBoxLayoutParentBox()->boxes();
    if (parentBox->isLineBox()) {
        parentStyle =
            m_block->style(m_block, m_block->style(), isFirstLineBox());
    } else {
        parentStyle = parentBox->style();
    }

    ascender = parentStyle->font()->metrics().m_ascender;
    descender = parentStyle->font()->metrics().m_descender;

    // 1. set relative y-pos from baseline (only if it needs)
    // 2. find max ascender and descender
    LayoutUnit maxAscenderSoFar = 0;
    LayoutUnit maxDescenderSoFar = intMaxForLayoutUnit;
    for (size_t k = 0; k < boxes.size(); k++) {
        FrameBox* box = boxes[k];
        if (!box->isNormalFlow()) {
            continue;
        } else {
            hasNormalFlowChild = true;
        }
        VerticalAlignValue va = box->style()->verticalAlign();
        if (box->isInlineTextBox()) {
            hasBoxOtherThanCollapsedInlineNonReplacedBox = true;
        } else if (box->isInlineNonReplacedBox()) {
            hasBoxOtherThanText = true;
            InlineNonReplacedBox* rb = box->asInlineNonReplacedBox();
            if (!rb->isCollapsed()) {
                hasBoxOtherThanCollapsedInlineNonReplacedBox = true;
            }

            if (va == VerticalAlignValue::BaselineVAlignValue) {
                maxAscenderSoFar = std::max(LineFormattingContext::ascender(rb),
                                            maxAscenderSoFar);
                maxDescenderSoFar = std::min(
                    LineFormattingContext::descender(rb), maxDescenderSoFar);
            } else if (va == VerticalAlignValue::TopVAlignValue) {
                // NO WORK TO DO
            } else if (va == VerticalAlignValue::BottomVAlignValue) {
                // NO WORK TO DO
            } else if (va == VerticalAlignValue::MiddleVAlignValue) {
                LayoutUnit halfHeight = rb->height() / 2;
                LayoutUnit halfXHeight =
                    (parentStyle->font()->metrics().m_xheightRate *
                     parentStyle->font()->size()) /
                    2;
                rb->setY(halfHeight + halfXHeight);
                maxAscenderSoFar =
                    std::max(halfHeight + halfXHeight, maxAscenderSoFar);
                maxDescenderSoFar = std::min(-1 * (halfHeight - halfXHeight),
                                             maxDescenderSoFar);
            } else if (va == VerticalAlignValue::SubVAlignValue) {
                rb->setY(descender + LineFormattingContext::ascender(rb));
                maxAscenderSoFar =
                    std::max(descender + LineFormattingContext::ascender(rb),
                             maxAscenderSoFar);
                maxDescenderSoFar =
                    std::min(descender + LineFormattingContext::descender(rb),
                             maxDescenderSoFar);
            } else if (va == VerticalAlignValue::SuperVAlignValue) {
                // Placing a superscript is font and browser dependent.
                // We place superscript above the baseline by 1/2 of
                // ascender (following blink)
                // (i.e, the baseline of superscript is aligned with 1/2 of
                // the ascender)
                rb->setY(ascender / 2 + LineFormattingContext::ascender(rb));
                maxAscenderSoFar =
                    std::max(ascender / 2 + LineFormattingContext::ascender(rb),
                             maxAscenderSoFar);
                maxDescenderSoFar = std::min(
                    ascender / 2 + LineFormattingContext::descender(rb),
                    maxDescenderSoFar);
            } else if (va == VerticalAlignValue::TextTopVAlignValue) {
                maxDescenderSoFar =
                    std::min(ascender - rb->height(), maxDescenderSoFar);
            } else if (va == VerticalAlignValue::TextBottomVAlignValue) {
                maxAscenderSoFar =
                    std::max(descender + rb->height(), maxAscenderSoFar);
            } else if (va == VerticalAlignValue::NumericVAlignValue) {
                Length len = box->style()->verticalAlignLength();
                LayoutUnit y;
                if (len.isSpecified()) {
                    y = LineFormattingContext::ascender(rb) +
                        len.specifiedValue(box->lineHeight(), box);
                }
                maxAscenderSoFar = std::max(y, maxAscenderSoFar);
                maxDescenderSoFar =
                    std::min(y - rb->height(), maxDescenderSoFar);
                rb->setY(y);
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        } else if (box->isFrameReplaced()) {
            hasBoxOtherThanText = true;
            hasBoxOtherThanCollapsedInlineNonReplacedBox = true;
            LayoutUnit outerHeight = box->outerHeight();
            if (va == VerticalAlignValue::BaselineVAlignValue) {
                maxAscenderSoFar = std::max(outerHeight, maxAscenderSoFar);
            } else if (va == VerticalAlignValue::TopVAlignValue) {
                // NO WORK TO DO
            } else if (va == VerticalAlignValue::BottomVAlignValue) {
                // NO WORK TO DO
            } else if (va == VerticalAlignValue::MiddleVAlignValue) {
                LayoutUnit halfHeight = outerHeight / 2;
                LayoutUnit halfXHeight =
                    (parentStyle->font()->metrics().m_xheightRate *
                     parentStyle->font()->size()) /
                    2;
                box->setY(halfHeight + halfXHeight);
                maxAscenderSoFar =
                    std::max(halfHeight + halfXHeight, maxAscenderSoFar);
                maxDescenderSoFar = std::min(-1 * (halfHeight - halfXHeight),
                                             maxDescenderSoFar);
            } else if (va == VerticalAlignValue::SubVAlignValue) {
                box->setY(descender + outerHeight);
                maxAscenderSoFar =
                    std::max(descender + outerHeight, maxAscenderSoFar);
            } else if (va == VerticalAlignValue::SuperVAlignValue) {
                box->setY(ascender / 2 + outerHeight);
                maxAscenderSoFar =
                    std::max(ascender / 2 + outerHeight, maxAscenderSoFar);
            } else if (va == VerticalAlignValue::TextTopVAlignValue) {
                maxDescenderSoFar =
                    std::min(ascender - outerHeight, maxDescenderSoFar);
            } else if (va == VerticalAlignValue::TextBottomVAlignValue) {
                maxAscenderSoFar =
                    std::max(descender + outerHeight, maxAscenderSoFar);
            } else if (va == VerticalAlignValue::NumericVAlignValue) {
                Length len = box->style()->verticalAlignLength();
                LayoutUnit amount;
                if (len.isSpecified()) {
                    amount = outerHeight +
                             len.specifiedValue(box->lineHeight(), box);
                }
                maxAscenderSoFar = std::max(amount, maxAscenderSoFar);
                maxDescenderSoFar =
                    std::min(amount - outerHeight, maxDescenderSoFar);
                box->setY(amount);
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        } else if (box->isFrameBlockBox() && box->isInlineLevel()) {
            hasBoxOtherThanText = true;
            hasBoxOtherThanCollapsedInlineNonReplacedBox = true;
            LayoutUnit outerHeight = box->outerHeight();
            if (va == VerticalAlignValue::BaselineVAlignValue) {
                LayoutUnit ascender =
                    inlineBlockAscender(box->asFrameBlockBox());
                if (ascender == box->height()) {
                    maxAscenderSoFar = std::max(ascender + box->marginHeight(),
                                                maxAscenderSoFar);
                    maxDescenderSoFar =
                        std::min(LayoutUnit(0), maxDescenderSoFar);
                } else {
                    LayoutUnit descender =
                        -(box->height() - ascender) - box->marginBottom();
                    maxAscenderSoFar =
                        std::max(ascender + box->marginTop(), maxAscenderSoFar);
                    maxDescenderSoFar = std::min(descender, maxDescenderSoFar);
                }
            } else if (va == VerticalAlignValue::TopVAlignValue) {
                // NO WORK TO DO
            } else if (va == VerticalAlignValue::BottomVAlignValue) {
                // NO WORK TO DO
            } else if (va == VerticalAlignValue::MiddleVAlignValue) {
                LayoutUnit halfHeight = outerHeight / 2;
                LayoutUnit halfXHeight =
                    (parentStyle->font()->metrics().m_xheightRate *
                     parentStyle->font()->size()) /
                    2;
                box->setY(halfHeight + halfXHeight);
                maxAscenderSoFar =
                    std::max(halfHeight + halfXHeight, maxAscenderSoFar);
                maxDescenderSoFar = std::min(-1 * (halfHeight - halfXHeight),
                                             maxDescenderSoFar);
            } else if (va == VerticalAlignValue::SubVAlignValue) {
                // TODO : Need Implement Here
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            } else if (va == VerticalAlignValue::SuperVAlignValue) {
                // TODO : Need Implement Here
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            } else if (va == VerticalAlignValue::TextTopVAlignValue) {
                maxDescenderSoFar =
                    std::min(ascender - outerHeight, maxDescenderSoFar);
            } else if (va == VerticalAlignValue::TextBottomVAlignValue) {
                maxAscenderSoFar =
                    std::max(descender + outerHeight, maxAscenderSoFar);
            } else if (va == VerticalAlignValue::NumericVAlignValue) {
                LayoutUnit ascender =
                    inlineBlockAscender(box->asFrameBlockBox());
                Length len = box->style()->verticalAlignLength();
                LayoutUnit amount;
                if (len.isSpecified()) {
                    amount =
                        ascender + len.specifiedValue(box->lineHeight(), box);
                }
                maxAscenderSoFar = std::max(amount, maxAscenderSoFar);
                maxDescenderSoFar =
                    std::min(amount - ascender, maxDescenderSoFar);
                box->setY(amount);
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }
    }

    if (parentBox->isLineBox()) {
        LineBox* lineBox = parentBox->asLineBox();
        if (!hasNormalFlowChild) {
            if (dueToBr) {
                LineFormattingContext::setAscDescender(
                    lineBox, parentStyle->font()->metrics().m_ascender,
                    parentStyle->font()->metrics().m_descender);
                return;
            }

            LineFormattingContext::setAscDescender(lineBox, 0, 0);
            return;
        }

        if (!hasBoxOtherThanCollapsedInlineNonReplacedBox) {
            LineFormattingContext::setAscDescender(lineBox, 0, 0);
            return;
        }
    }

    // Consider parent's font ascender/descender
    LayoutUnit maxAscender = std::max(maxAscenderSoFar, ascender);
    LayoutUnit maxDescender = std::min(maxDescenderSoFar, descender);

    // If maxDescenderSoFar is initial value(=intMaxForLayoutUnit), set it to 0.
    maxDescenderSoFar = maxDescenderSoFar.toInt() == intMaxForLayoutUnit
                            ? LayoutUnit(0)
                            : maxDescenderSoFar;

    // 3. adjusting the line height
    if (!parentStyle->hasNormalLineHeight()) {
        LayoutUnit lineHeight;
        if (parentBox->isLineBox()) {
            lineHeight = m_block->lineHeight();
        } else {
            lineHeight = parentBox->lineHeight();
        }
        LayoutUnit diff =
            (lineHeight - parentStyle->font()->metrics().m_fontHeight) / 2;
        LayoutUnit ascenderShouldBe =
            parentStyle->font()->metrics().m_ascender + diff;
        LayoutUnit descenderShouldBe =
            parentStyle->font()->metrics().m_descender - diff;

        if (hasBoxOtherThanText) {
            maxAscender = std::max(maxAscenderSoFar, ascenderShouldBe);
            maxDescender = std::min(maxDescenderSoFar, descenderShouldBe);
        } else {
            // Only Text Children
            maxAscender = ascenderShouldBe;
            maxDescender = descenderShouldBe;
        }
    }

    // VA: top/bottom
    for (size_t k = 0; k < boxes.size(); k++) {
        FrameBox* box = boxes[k];
        VerticalAlignValue va = box->style()->verticalAlign();

        // Ignore non normalFlow
        if (!box->isNormalFlow()) {
            continue;
        }

        // Ignore inlineBox's marginHeight
        LayoutUnit marginHeight;
        if (!box->isInlineBox()) {
            marginHeight = box->marginHeight();
        }

        // Update maxAsc & maxDes
        if (!box->isInlineTextBox()) {
            if (va == VerticalAlignValue::TopVAlignValue) {
                maxDescender = std::min(
                    maxAscender - (box->height() + marginHeight), maxDescender);
            } else if (va == VerticalAlignValue::BottomVAlignValue) {
                maxAscender = std::max(
                    maxDescender + (box->height() + marginHeight), maxAscender);
            }
        }
    }

    if (parentBox->isInlineNonReplacedBox()) {
        InlineNonReplacedBox* inrb = parentBox->asInlineNonReplacedBox();
        if (!hasNormalFlowChild) {
            if (inrb->width() == 0 && inrb->marginLeft() == 0 &&
                inrb->marginRight() == 0 && inrb->paddingLeft() == 0 &&
                inrb->paddingRight() == 0 && inrb->borderLeft() == 0 &&
                inrb->borderRight() == 0) {
                inrb->markCollapsed();
                LineFormattingContext::setAscDescender(inrb, maxAscender,
                                                       maxDescender);
                return;
            }
        }
    }

    LayoutUnit height = maxAscender - maxDescender;
    for (size_t k = 0; k < boxes.size(); k++) {
        FrameBox* f = boxes[k];

        if (!f->isNormalFlow()) {
            continue;
        }

        if (f->isInlineTextBox()) {
            /*
            InlineTextBox* ib = f->asInlineTextBox();
            if (UNLIKELY(descenderInOut == 0)) {
                if (height <
                    ib->asInlineTextBox()->style()->font()->metrics()
                    .m_fontHeight)
                {
                    ib->setY((height -
                        ib->asInlineTextBox()->style()->font()->metrics()
                            .m_fontHeight
                    +
                    ib->asInlineTextBox()->style()->font()->metrics()
                            .m_descender) /
                    2);
                } else {
                    ib->setY(height -
                        ib->asInlineTextBox()->style()->font()->metrics()
                        .m_fontHeight
                    -
                    ib->asInlineTextBox()->style()->font()->metrics()
                        .m_descender);
                }
            } else {
                ib->setY(height + maxDescender - ib->height() -
                ib->asInlineTextBox()->style()->font()->metrics().m_descender);
            } */
            InlineTextBox* ib = f->asInlineTextBox();
            ib->setY(maxAscender - ib->style()->font()->metrics().m_ascender);
        } else {
            VerticalAlignValue va = f->style()->verticalAlign();
            LayoutUnit marginTop, marginRight, marginBottom, marginLeft;
            if (!f->isInlineBox()) {
                marginTop = f->marginTop();
                marginRight = f->marginRight();
                marginBottom = f->marginBottom();
                marginLeft = f->marginLeft();
            }
            if (va == VerticalAlignValue::BaselineVAlignValue) {
                if (f->isInlineNonReplacedBox()) {
                    f->setY(height + maxDescender - f->height() -
                            LineFormattingContext::descender(
                                f->asInlineNonReplacedBox()));
                } else if (f->isFrameReplaced()) {
                    // TODO use this code for when replaced content does not
                    // have content
                    /*
                    LayoutUnit asc = ib->marginTop() +
                    ib->asInlineReplacedBox()->replacedBox()->borderTop()
                    +
                    ib->asInlineReplacedBox()->replacedBox()->paddingTop()
                    +
                    ib->asInlineReplacedBox()->replacedBox()->contentHeight();
                    ib->setY(height + maxDescender - asc +
                    ib->marginTop()); */
                    f->setY(maxAscender - f->height() - marginBottom);
                } else {
                    STARFISH_ASSERT(f->isFrameBlockBox() && f->isInlineLevel());
                    LayoutUnit ascender =
                        inlineBlockAscender(f->asFrameBlockBox());
                    if (ascender == f->height()) {
                        f->setY(maxAscender - ascender - marginBottom);
                    } else {
                        f->setY(maxAscender - ascender);
                    }
                }
                // 4. convert a y pos relative to the baseline to a y pos
                // relative to the top-left corner of the box
            } else if (va == VerticalAlignValue::TopVAlignValue) {
                f->setY(marginTop);
            } else if (va == VerticalAlignValue::BottomVAlignValue) {
                f->setY(height - f->height() - marginBottom);
            } else if (va == VerticalAlignValue::MiddleVAlignValue) {
                f->setY(maxAscender - f->y() + marginTop);
            } else if (va == VerticalAlignValue::SubVAlignValue) {
                f->setY(maxAscender - f->y() + marginTop);
            } else if (va == VerticalAlignValue::SuperVAlignValue) {
                // pascender / 2 + ib->asInlineNonReplacedBox()->ascender()
                f->setY(maxAscender - f->y() + marginTop);
            } else if (va == VerticalAlignValue::TextTopVAlignValue) {
                f->setY(maxAscender - ascender + marginTop);
            } else if (va == VerticalAlignValue::TextBottomVAlignValue) {
                f->setY(maxAscender - descender - f->height() - marginBottom);
            } else if (va == VerticalAlignValue::NumericVAlignValue) {
                f->setY(maxAscender - f->y() + marginTop);
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }
    }

    LineFormattingContext::setAscDescender(
        parentBox->asInlineBoxLayoutParentBox(), maxAscender, maxDescender);
}

static char charDirection(char32_t c)
{
    auto property = u_getIntPropertyValue(c, UCHAR_BIDI_CLASS);
    if ((property == U_RIGHT_TO_LEFT) || (property == U_RIGHT_TO_LEFT_ARABIC) ||
        (property == U_RIGHT_TO_LEFT_EMBEDDING) ||
        (property == U_RIGHT_TO_LEFT_OVERRIDE)) {
        return 0;
    } else if ((property == U_LEFT_TO_RIGHT) ||
               (property == U_LEFT_TO_RIGHT_EMBEDDING) ||
               (property == U_LEFT_TO_RIGHT_OVERRIDE)) {
        return 1;
    } else {
        return 2;
    }
}

static CharDirection charDirFromICUDir(UBiDiDirection dir)
{
    if (dir == UBIDI_LTR) {
        return CharDirection::Ltr;
    } else if (dir == UBIDI_RTL) {
        return CharDirection::Rtl;
    } else if (dir == UBIDI_NEUTRAL) {
        return CharDirection::Neutral;
    } else {
        STARFISH_ASSERT(dir == UBIDI_MIXED);
        return CharDirection::Mixed;
    }
}

static UBiDiDirection getTextDir(const StringView& sv, size_t start, size_t end)
{
    UBiDiDirection dir;
    StringView(sv.string(), start, end)
        .peekUTF16Buffer(
            [](const char16_t* buf, size_t len, void* data) -> size_t {
                *((UBiDiDirection*)data) =
                    ubidi_getBaseDirection((const UChar*)buf, len);
                return 0;
            },
            &dir);
    return dir;
}

static bool startsWithNewlineChar(const StringView& sv)
{
    return String::isNewline(sv.originalString()->charAt(sv.start()));
}

static bool isHyphen(char32_t d)
{
    return d == 0x2010 || d == '-';
}

static bool isSoftHyphen(char32_t d)
{
    return d == 0x00AD;
}

static bool isNumberChar(char32_t d)
{
    if (('0' <= d && d <= '9') ||
        (0x0660 <= d && d <= 0x0669)       // 0 to 9 in Arabic
        || (0x06F0 <= d && d <= 0x06F9)) { // 0 to 9 in Extended Arabic
        return true;
    } else {
        return false;
    }
}

static bool isNumber(String* text, size_t start, size_t end)
{
    for (unsigned i = start; i < end; i++) {
        if (isNumberChar(text->charAt(i))) {
        } else {
            return false;
        }
    }
    return true;
}

static bool isNumber(const TextRun& run)
{
    const StringView& sv = run.m_stringView;
    return isNumber(sv.originalString(), sv.start(), sv.end());
}

static bool isWhiteSpace(const TextRun& run)
{
    const StringView& sv = run.m_stringView;
    return sv.originalString()->containsOnlyWhitespace(sv.start(), sv.end());
}

static bool isWord(FrameBox* box)
{
    if (box->isInlineTextBox()) {
        const TextRun& tr = box->asInlineTextBox()->textRun();
        return !isWhiteSpace(tr);
    }

    return false;
}

static bool hasIsolateBidiContent(ComputedStyle* style)
{
    if (style->unicodeBidi() == UnicodeBidiValue::IsolateUnicodeBidiValue) {
        return true;
    } else if (style->unicodeBidi() ==
               UnicodeBidiValue::EmbedUnicodeBidiValue) {
        return true;
    }
    return false;
}

static FrameBox* fetchContentForResolveBidi(FrameBox* box)
{
    if (box->isInlineNonReplacedBox()) {
        InlineNonReplacedBox* b = box->asInlineNonReplacedBox();
        if (hasIsolateBidiContent(b->style())) {
            return b;
        }
        if (b->boxes().size()) {
            STARFISH_ASSERT(b->boxes().size() == 1);
            return fetchContentForResolveBidi(b->boxes()[0]);
        }
    }

    return box;
}

void LineFormattingContext::splitInlineBoxes(GCVector<FrameBox*>& boxes)
{
    for (size_t i = 0; i < boxes.size(); i++) {
        FrameBox* box = boxes[i];
        if (box->isInlineNonReplacedBox()) {
            InlineNonReplacedBox* inrb = box->asInlineNonReplacedBox();
            if (!(hasIsolateBidiContent(inrb->style()))) {
                splitInlineBoxes(inrb->boxes());
                // split every content for resolve bidi after
                auto& boxesToCopy = inrb->boxes();
                if (boxesToCopy.size()) {
                    boxes.erase(boxes.begin() + i);

                    size_t insertPos = i;

                    for (size_t j = 0; j < boxesToCopy.size(); j++) {
                        InlineNonReplacedBox* newBox = new InlineNonReplacedBox(
                            this, inrb, isFirstLineBox());
                        newBox->setY(inrb->y());
                        newBox->insertInlineBox(boxesToCopy[j]);
                        if (boxesToCopy[j]->isNormalFlow()) {
                            newBox->setWidth(boxesToCopy[j]->width());
                            newBox->setHeight(inrb->height());
                        } else {
                            STARFISH_ASSERT(
                                boxesToCopy[j]->isAbsolutePositioned());
                            markAbsolutePositionedBoxLayoutParent(newBox);
                        }

                        setInlineBoxIndex(newBox,
                                          inlineBoxIndex(boxesToCopy[j]));
                        newBox->setLayoutParent(inrb->layoutParent());
                        boxes.insert(boxes.begin() + insertPos++, newBox);
                    }

                    i += (boxesToCopy.size() - 1);
                } else {
                    inrb->setWidth(0);
                }
            }
        }
    }
}

CharDirection LineFormattingContext::contentDir(FrameBox* box)
{
    if (box->isInlineTextBox()) {
        return box->asInlineTextBox()->charDirection();
    } else if (box->isInlineNonReplacedBox()) {
        InlineNonReplacedBox* inrb = box->asInlineNonReplacedBox();
        if (hasIsolateBidiContent(inrb->style())) {
            // when unicode-bidi property is isolate, we should return
            // neutral direction
            return CharDirection::Neutral;
        } else {
            const auto& boxes = inrb->boxes();
            for (size_t i = 0; i < boxes.size(); i++) {
                CharDirection dir = contentDir(boxes[i]);
                if (dir == CharDirection::Ltr) {
                    return CharDirection::Ltr;
                } else if (dir == CharDirection::Rtl) {
                    return CharDirection::Rtl;
                } else { // neutral
                }
            }
            return CharDirection::Neutral;
        }
    } else if (box->isFrameBox()) {
        auto iter = m_computedDirectionValuePerFrame.find(box);
        if (iter != m_computedDirectionValuePerFrame.end()) {
            return (iter->second == DirectionValue::LtrDirectionValue)
                       ? CharDirection::Ltr
                       : CharDirection::Rtl;
        } else {
            return CharDirection::Neutral;
        }
    } else {
        return CharDirection::Neutral;
    }
}

void InlineBoxLayoutParentBox::setLeftMBPs(LineFormattingContext* ctx)
{
    for (size_t i = 0; i < m_boxes.size(); i++) {
        FrameBox* box = m_boxes[i];
        if (box->isInlineNonReplacedBox()) {
            InlineNonReplacedBox* inrb = box->asInlineNonReplacedBox();
            inrb->setLeftMBPs(ctx);

            if (inrb->style()->direction() == LtrDirectionValue) {
                if (!ctx->isSetLeftMBP(inrb) &&
                    ctx->isProcessedStartingMBP(inrb)) {
                    inrb->setOrgLeftMBP(ctx);
                } else {
                    inrb->unsetLeftMBP();
                }
            } else {
                if (!ctx->isSetLeftMBP(inrb) &&
                    ctx->isProcessedEndingMBP(inrb)) {
                    inrb->setOrgLeftMBP(ctx);
                } else {
                    inrb->unsetLeftMBP();
                }
            }
        }
    }
}

void InlineBoxLayoutParentBox::setRightMBPs(LineFormattingContext* ctx)
{
    for (size_t i = m_boxes.size() - 1; i != SIZE_MAX; i--) {
        FrameBox* box = m_boxes[i];
        if (box->isInlineNonReplacedBox()) {
            InlineNonReplacedBox* inrb = box->asInlineNonReplacedBox();
            inrb->setRightMBPs(ctx);

            if (inrb->style()->direction() == LtrDirectionValue) {
                if (!ctx->isSetRightMBP(inrb) &&
                    ctx->isProcessedEndingMBP(inrb)) {
                    inrb->setOrgRightMBP(ctx);
                } else {
                    inrb->unsetRightMBP();
                }
            } else {
                if (!ctx->isSetRightMBP(inrb) &&
                    ctx->isProcessedStartingMBP(inrb)) {
                    inrb->setOrgRightMBP(ctx);
                } else {
                    inrb->unsetRightMBP();
                }
            }
        }
    }
}

void InlineBoxLayoutParentBox::saveChildrenVerticalPositions(
    std::vector<LayoutUnit>& vPositions)
{
    vPositions.reserve(m_boxes.size());
    for (size_t i = 0; i < m_boxes.size(); i++) {
        vPositions.push_back(m_boxes[i]->y());
        m_boxes[i]->setY(0);
    }
}

void InlineBoxLayoutParentBox::restoreChildrenVerticalPositions(
    std::vector<LayoutUnit>& vPositions)
{
    for (size_t i = 0; i < m_boxes.size(); i++) {
        m_boxes[i]->setY(vPositions[i]);
    }
}

void InlineBoxLayoutParentBox::quickInlineLayout(LineFormattingContext* ctx)
{
    for (size_t i = 0; i < m_boxes.size(); i++) {
        FrameBox* box = m_boxes[i];
        if (box->isInlineTextBox()) {
            continue;
        } else if (box->isInlineNonReplacedBox()) {
            InlineNonReplacedBox* inrb = box->asInlineNonReplacedBox();
            inrb->quickInlineLayout(ctx);
            if (ctx->isLastLineBox()) {
                std::vector<LayoutUnit> vPositions;
                inrb->saveChildrenVerticalPositions(vPositions);
                ctx->computeVerticalProperties(inrb, false);
                inrb->setContentHeight(
                    inrb->style()->font()->metrics().m_ascender -
                    inrb->style()->font()->metrics().m_descender);
                inrb->restoreChildrenVerticalPositions(vPositions);
            }
        } else {
            if (box->isAbsolutePositioned()) {
                ctx->m_layoutContext.registerAbsolutePositionedBox(box);
            } else if (box->isFloating()) {
                box->layout(ctx->m_layoutContext,
                            LayoutWantToResolve::ResolveAll);
            } else if (box->isFrameBlockBox()) {
                ctx->m_layoutContext.pushInlineBlockBox(box->asFrameBlockBox());
                box->layout(ctx->m_layoutContext,
                            LayoutWantToResolve::ResolveAll);
                LayoutUnit ascender;
                DisplayValue display = box->style()->display();

                if (display == InlineTableDisplayValue) {
                    ascender = box->asFrameTableBox()->calBaseline(
                        ctx->m_layoutContext);
                } else {
                    Nullable<LayoutUnit> p =
                        ctx->m_layoutContext.lineBoxAscender(
                            box->asFrameBlockBox());
                    if (p.hasValue() &&
                        box->appliedOverflowX() == VisibleOverflow) {
                        ascender = p.getValue();
                    } else {
                        ascender = height();
                    }
                }

                ctx->m_layoutContext.popInlineBlockBox();
                ctx->registerInlineBlockAscender(ascender,
                                                 box->asFrameBlockBox());
            } else {
                if (box->isEstablishesBlockFormattingContext()) {
                    box->layout(ctx->m_layoutContext,
                                LayoutWantToResolve::ResolveAll);
                } else {
                    box->computePaintingFlags(ctx->m_layoutContext,
                                              LayoutWantToResolve::ResolveAll);
                    box->quickLayout(ctx->m_layoutContext);
                }
            }
        }
    }
}

void LineFormattingContext::resolveBidi(DirectionValue parentDir,
                                        GCVector<FrameBox*>& boxes)
{
    STARFISH_ASSERT(
        (m_currentLayoutParent->isInlineNonReplacedBox() &&
         hasIsolateBidiContent(
             m_currentLayoutParent->asInlineNonReplacedBox()->style())) ||
        m_currentLanguageDirection.isMixed());
    splitInlineBoxes(boxes);

    if (parentDir == DirectionValue::RtlDirectionValue) {
        // find inline Text Boxes has only number for
        // <LTR> <Number> <RTL> case
        // Number following LTR Text should be LTR
        // <LTR> <Number> <RTL> -> <LTR> <Number-LTR not neutral> <RTL>
        for (size_t i = 0; i < boxes.size(); i++) {
            FrameBox* box = fetchContentForResolveBidi(boxes[i]);
            if (box->isInlineTextBox()) {
                InlineTextBox* itb = box->asInlineTextBox();
                if (isNumber(itb->textRun())) {
                    for (size_t j = i - 1; j != SIZE_MAX; j--) {
                        FrameBox* box2 = fetchContentForResolveBidi(boxes[j]);
                        if (box2->isInlineTextBox()) {
                            InlineTextBox* itb2 = box2->asInlineTextBox();
                            if (itb2->charDirection() == CharDirection::Ltr) {
                                itb->setCharDirection(CharDirection::Ltr);
                                break;
                            } else if (itb2->charDirection() ==
                                       CharDirection::Rtl) {
                                break;
                            }
                        }
                    }
                }
            }
        }
    } else {
        // find inline Text Boxes has only number for
        // <RTL> <Number> <LTR> case
        // Number following RTL Text should be RTL
        // <RTL> <Number> <LTR> -> <RTL> <Number-RTL not neutral> <LTR>
        for (size_t i = 0; i < boxes.size(); i++) {
            FrameBox* box = fetchContentForResolveBidi(boxes[i]);
            if (box->isInlineTextBox()) {
                InlineTextBox* itb = box->asInlineTextBox();
                if (isNumber(itb->textRun())) {
                    for (size_t j = i - 1; j != SIZE_MAX; j--) {
                        FrameBox* box2 = fetchContentForResolveBidi(boxes[j]);
                        if (box2->isInlineTextBox()) {
                            InlineTextBox* itb2 = box2->asInlineTextBox();
                            if (itb2->charDirection() == CharDirection::Rtl) {
                                itb->setCharDirection(CharDirection::Rtl);
                                break;
                            } else if (itb2->charDirection() ==
                                       CharDirection::Ltr) {
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

#ifndef NDEBUG
    for (size_t i = 0; i < boxes.size(); i++) {
        FrameBox* box = fetchContentForResolveBidi(boxes[i]);
        if (box->isInlineTextBox()) {
            InlineTextBox* itb = box->asInlineTextBox();
            STARFISH_ASSERT(itb->charDirection() != CharDirection::Mixed);
        }
    }
#endif

    if (parentDir == DirectionValue::LtrDirectionValue) {
        GCVector<FrameBox*> oldBoxes = std::move(boxes);
        boxes.reserve(oldBoxes.size());
        std::vector<FrameBox*> rtlStorage; // use normal allocator because
                                           // oldBoxes has strong reference

        CharDirection currentDirection = CharDirection::Ltr;
        for (size_t i = 0; i < oldBoxes.size(); i++) {
            CharDirection currentDirectionBefore = currentDirection;
            FrameBox* box = fetchContentForResolveBidi(oldBoxes[i]);
            CharDirection dir = contentDir(box);
            if (dir == CharDirection::Neutral) {
                CharDirection nextDir = CharDirection::Ltr;
                for (size_t j = i + 1; j < oldBoxes.size(); j++) {
                    nextDir =
                        contentDir(fetchContentForResolveBidi(oldBoxes[j]));
                    if (nextDir != CharDirection::Neutral) {
                        break;
                    }
                }

                if (currentDirection == CharDirection::Rtl &&
                    nextDir == CharDirection::Neutral) {
                    currentDirection = CharDirection::Ltr;
                } else if (currentDirection == CharDirection::Rtl &&
                           nextDir == CharDirection::Ltr) {
                    currentDirection = CharDirection::Ltr;
                }

                if (box->isInlineTextBox()) {
                    InlineTextBox* itb = box->asInlineTextBox();
                    if (!isWhiteSpace(itb->textRun())) {
                        if (isNumber(itb->textRun())) {
                            currentDirection = currentDirectionBefore;
                        }
                        itb->setCharDirection(currentDirection);
                    }
                }
            } else {
                currentDirection = dir;
            }

            if (currentDirection == CharDirection::Ltr) {
                if (rtlStorage.size()) {
                    boxes.insert(boxes.end(), rtlStorage.begin(),
                                 rtlStorage.end());
                    rtlStorage.clear();
                }
                boxes.push_back(oldBoxes[i]);
            } else {
                rtlStorage.insert(rtlStorage.begin(), oldBoxes[i]);
            }
        }

        if (rtlStorage.size()) {
            boxes.insert(boxes.end(), rtlStorage.begin(), rtlStorage.end());
            rtlStorage.clear();
        }
    } else {
        GCVector<FrameBox*> oldBoxes = std::move(boxes);
        boxes.reserve(oldBoxes.size());
        std::vector<FrameBox*> ltrStorage; // use normal allocator because
                                           // oldBoxes has strong reference

        CharDirection currentDirection = CharDirection::Rtl;
        for (size_t i = 0; i < oldBoxes.size(); i++) {
            CharDirection currentDirectionBefore = currentDirection;
            FrameBox* box = fetchContentForResolveBidi(oldBoxes[i]);
            CharDirection dir = contentDir(box);
            if (dir == CharDirection::Neutral) {
                CharDirection nextDir = CharDirection::Rtl;
                for (size_t j = i + 1; j < oldBoxes.size(); j++) {
                    nextDir =
                        contentDir(fetchContentForResolveBidi(oldBoxes[j]));
                    if (nextDir != CharDirection::Neutral) {
                        break;
                    }
                }

                if (currentDirection == CharDirection::Ltr &&
                    nextDir == CharDirection::Neutral) {
                    currentDirection = CharDirection::Rtl;
                } else if (currentDirection == CharDirection::Ltr &&
                           nextDir == CharDirection::Rtl) {
                    currentDirection = CharDirection::Rtl;
                }

                if (box->isInlineTextBox()) {
                    InlineTextBox* itb = box->asInlineTextBox();
                    if (!isWhiteSpace(itb->textRun())) {
                        if (isNumber(itb->textRun())) {
                            currentDirection = currentDirectionBefore;
                        }
                        itb->setCharDirection(currentDirection);
                    }
                }
            } else {
                currentDirection = dir;
            }

            if (currentDirection == CharDirection::Rtl) {
                if (ltrStorage.size()) {
                    boxes.insert(boxes.begin(), ltrStorage.begin(),
                                 ltrStorage.end());
                    ltrStorage.clear();
                }
                boxes.insert(boxes.begin(), oldBoxes[i]);
            } else {
                ltrStorage.push_back(oldBoxes[i]);
            }
        }

        if (ltrStorage.size()) {
            boxes.insert(boxes.begin(), ltrStorage.begin(), ltrStorage.end());
            ltrStorage.clear();
        }
    }
    // reverse parenthesis chars
    static const char parenthesisMap[] = {
        0, 0, 0,  0,  0, 0,  0, 0, 0, 0,   0, 0,   0, 0, 0, 0,  0, 0,  0,
        0, 0, 0,  0,  0, 0,  0, 0, 0, 0,   0, 0,   0, 0, 0, 0,  0, 0,  0,
        0, 0, 41, 40, 0, 0,  0, 0, 0, 0,   0, 0,   0, 0, 0, 0,  0, 0,  0,
        0, 0, 0,  62, 0, 60, 0, 0, 0, 0,   0, 0,   0, 0, 0, 0,  0, 0,  0,
        0, 0, 0,  0,  0, 0,  0, 0, 0, 0,   0, 0,   0, 0, 0, 93, 0, 91, 0,
        0, 0, 0,  0,  0, 0,  0, 0, 0, 0,   0, 0,   0, 0, 0, 0,  0, 0,  0,
        0, 0, 0,  0,  0, 0,  0, 0, 0, 125, 0, 123, 0, 0
    };

    for (size_t i = 0; i < boxes.size(); i++) {
        FrameBox* box = fetchContentForResolveBidi(boxes[i]);
        if (box->isInlineTextBox()) {
            InlineTextBox* itb = box->asInlineTextBox();
            if (itb->charDirection() == CharDirection::Rtl) {
                const StringView& sv = itb->textRun().m_stringView;
                auto data = sv.bufferAccessData();
                bool shouldReplaceString = false;
                for (size_t j = 0; j < data.length; j++) {
                    char32_t ch = data.charAt(j);
                    if (ch < 128) {
                        if (parenthesisMap[ch]) {
                            shouldReplaceString = true;
                            break;
                        }
                    }
                }

                if (shouldReplaceString) {
                    StringBuilder builder;
                    for (size_t j = 0; j < data.length; j++) {
                        char32_t ch = data.charAt(j);
                        if (ch < 128) {
                            if (parenthesisMap[ch]) {
                                ch = (char32_t)parenthesisMap[ch];
                            }
                        }
                        builder.appendChar(ch);
                    }
                    itb->setText(builder.finalize());
                }
            }
        }
    }
}

static void removeBoxFromLine(FrameBox* box)
{
    if (box->layoutParent()->isLineBox()) {
        auto& boxes = box->layoutParent()->asLineBox()->boxes();
        boxes.erase(std::find(boxes.begin(), boxes.end(), box));
    } else {
        InlineNonReplacedBox* parent =
            box->layoutParent()->asInlineNonReplacedBox();
        auto& boxes = parent->boxes();
        while (true) {
            parent->setWidth(parent->width() - box->width());
            if (parent->layoutParent()->isLineBox()) {
                break;
            }
            parent = parent->layoutParent()->asInlineNonReplacedBox();
        }
        boxes.erase(std::find(boxes.begin(), boxes.end(), box));
    }
}

static void collectUselessInlineBoxes(LayoutContext& ctx, FrameBox* f)
{
    auto& boxes = f->asInlineBoxLayoutParentBox()->boxes();
    for (size_t i = 0; i < boxes.size(); i++) {
        if (boxes[i]->isInlineTextBox()) {
            ctx.pushIntoInlineTextBoxPool(boxes[i]->asInlineTextBox());
        } else if (boxes[i]->isInlineBoxLayoutParentBox()) {
            collectUselessInlineBoxes(ctx, boxes[i]);
        }
    }

    if (f->isInlineNonReplacedBox()) {
        f->asInlineNonReplacedBox()->boxes().clear();
        ctx.pushIntoInlineNonReplacedBoxPool(f->asInlineNonReplacedBox());
    }
}

LineFormattingContext::LineFormattingContext(FrameBlockBox* block,
                                             LayoutContext& ctx, bool forQuick)
    : m_textIndentWidth(0)
    , m_unprocessedStartingMBPWidth(0)
    , m_block(block)
    , m_layoutContext(ctx)
    , m_isLastLineBox(false)
    , m_isFirstLineCandidate(false)
    , m_inlineBoxIndex(0)
    , m_lastLineHasFloatValue(0)
{
    if (forQuick) {
        return;
    }

    m_absPosition = block->absolutePoint(m_layoutContext.frameDocument());
    m_leftBoundary =
        m_absPosition.x() + block->paddingLeft() + block->borderLeft();
    m_rightBoundary = m_leftBoundary + block->contentWidth();
    m_lineBoxY = block->paddingTop() + block->borderTop();

    for (size_t i = 0; i < m_block->m_lineBoxes.size(); i++) {
        collectUselessInlineBoxes(ctx, m_block->m_lineBoxes[i]);
    }

    m_block->m_lineBoxes.clear();
    // m_block.m_lineBoxes.shrink_to_fit();
    resetLineBox();
    if (!block->isAnonymous() || ctx.checkIfThisIsFirstLineCandidate(block)) {
        m_isFirstLineCandidate = true;
        Length textIndent = block->style()->textIndent();
        FrameBox* cb = containingBlock(block);
        m_textIndentWidth =
            textIndent.specifiedValue(cb->contentWidth(), block);
    }
}

void LineFormattingContext::registerInlineContent(FrameLineBreak* br)
{
    LineBox* back = currentLine();
    bool hasNormalFlowContent = br != nullptr;
    for (size_t i = 0; i < back->boxes().size(); i++) {
        if (back->boxes()[i]->isNormalFlow()) {
            hasNormalFlowContent = true;
            break;
        }
    }
    if (hasNormalFlowContent) {
        if (m_isFirstLineCandidate) {
            m_layoutContext.registerFirstLineCandidate(m_block);
            m_isFirstLineCandidate = false;
        }
        m_layoutContext.registerLineBoxAscender(m_block, back, ascender(back));
    }
}

void* InlineBoxLayoutParentBox::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(InlineBoxLayoutParentBox)] = { 0 };
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(InlineBoxLayoutParentBox, m_node));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(InlineBoxLayoutParentBox, m_layoutParent));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(InlineBoxLayoutParentBox, m_boxes));
        descr = GC_make_descriptor(obj_bitmap,
                                   GC_WORD_LEN(InlineBoxLayoutParentBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void InlineBoxLayoutParentBox::computeVisibleRect(
    Frame::ComputeVisibleRectContext& ctx)
{
    FrameBox::ComputeVisibleRectContextFragment f(ctx, this);
    tryUniteVisibleRect(ctx);
    for (size_t i = 0; i < m_boxes.size(); i++) {
        m_boxes[i]->computeVisibleRect(ctx);
    }
}

FrameBox* InlineBoxLayoutParentBox::firstInlineBox()
{
    for (size_t i = 0; i < m_boxes.size(); i++) {
        if (m_boxes[i]->isInlineTextBox()) {
            return m_boxes[i];
        } else if (m_boxes[i]->isInlineNonReplacedBox()) {
            FrameBox* r =
                m_boxes[i]->asInlineNonReplacedBox()->firstInlineBox();
            if (r) {
                return r;
            }
        } else if (m_boxes[i]->isNormalFlow()) {
            return m_boxes[i];
        }
    }

    return nullptr;
}

FrameBox* InlineBoxLayoutParentBox::lastInlineBox()
{
    for (size_t i = m_boxes.size() - 1; i != SIZE_MAX; i--) {
        if (m_boxes[i]->isInlineTextBox()) {
            return m_boxes[i];
        } else if (m_boxes[i]->isInlineNonReplacedBox()) {
            FrameBox* r = m_boxes[i]->asInlineNonReplacedBox()->lastInlineBox();
            if (r) {
                return r;
            }
        } else if (m_boxes[i]->isNormalFlow()) {
            return m_boxes[i];
        }
    }

    return nullptr;
}

static bool isCollapsibleWhiteSpace(FrameBox* box)
{
    if (!box) {
        return true;
    }

    if (box->isInlineTextBox()) {
        const TextRun& run = box->asInlineTextBox()->textRun();
        return run.m_stringView.originalString()->equals(String::spaceString);
    }

    return false;
}

static bool isForcedNewLine(FrameBox* box)
{
    if (!box) {
        return true;
    }

    if (box->isInlineTextBox()) {
        const TextRun& run = box->asInlineTextBox()->textRun();
        return run.m_stringView.originalString()->equals(String::emptyString);
    }

    return false;
}

void InlineBoxLayoutParentBox::removeDanglingSpace(LineFormattingContext* ctx)
{
    FrameBox* last = lastInlineBox();
    while (last) {
        if (isCollapsibleWhiteSpace(last)) {
            // Ignore last whitespace when wrapping lines.
            removeBoxFromLine(last);
            ctx->m_currentLineWidth -= last->outerWidth();
        } else {
            break;
        }
        last = lastInlineBox();
    }
}

bool InlineBoxLayoutParentBox::isAbsolutePositionedBoxLayoutParent(
    LineFormattingContext* ctx)
{
    if (ctx->absolutePositionedBoxLayoutParentCnt(this) == 0) {
        auto iter = m_boxes.begin();

        while (iter != m_boxes.end()) {
            FrameBox* box = *iter;
            if (box->isInlineNonReplacedBox()) {
                if (!box->asInlineNonReplacedBox()
                         ->isAbsolutePositionedBoxLayoutParent(ctx)) {
                    return false;
                }
            }

            iter++;
        }

        return true;
    }

    return false;
}

bool InlineBoxLayoutParentBox::containOnlyEmptyInlineNonReplacedBoxes(
    LineFormattingContext* ctx)
{
    if (m_boxes.size() == 0) {
        return true;
    }

    auto iter = m_boxes.rbegin();

    while (iter != m_boxes.rend()) {
        FrameBox* last = *iter;
        if (last->isInlineNonReplacedBox()) {
            InlineNonReplacedBox* inrb = last->asInlineNonReplacedBox();
            if (inrb->width() == 0 && inrb->marginLeft() == 0 &&
                inrb->marginRight() == 0 && inrb->paddingLeft() == 0 &&
                inrb->paddingRight() == 0 && inrb->borderLeft() == 0 &&
                inrb->borderRight() == 0 &&
                inrb->isAbsolutePositionedBoxLayoutParent(ctx)) {
                iter++;
                continue;
            }
        }

        return false;
    }

    return true;
}

void InlineBoxLayoutParentBox::layoutInlineBoxes(LineFormattingContext* ctx,
                                                 LayoutUnit start)
{
    LayoutUnit x = start;
    for (size_t k = 0; k < m_boxes.size(); k++) {
        FrameBox* childBox = m_boxes[k];
        if (childBox->isNormalFlow()) {
            childBox->setX(x + childBox->marginLeft());
            if (childBox->isInlineNonReplacedBox()) {
                childBox->asInlineNonReplacedBox()->layoutInlineBoxes(
                    ctx, childBox->borderLeft() + childBox->paddingLeft());
            }

            x += (childBox->outerWidth() + ctx->wordSpacing(childBox, this));
        } else if (childBox->isAbsolutePositioned()) {
            childBox->setX(x);
        }
    }
}

void InlineBoxLayoutParentBox::coordinateVerticalProperties(
    LineFormattingContext* ctx, LayoutUnit yOffset)
{
    for (size_t i = 0; i < m_boxes.size(); i++) {
        FrameBox* box = m_boxes[i];
        if (box->isInlineNonReplacedBox()) {
            LayoutUnit newYOffset =
                ctx->ascender(box->asInlineNonReplacedBox()) -
                box->style()->font()->metrics().m_ascender - box->borderTop() -
                box->paddingTop();
            box->moveY(-yOffset + newYOffset);
            box->setContentHeight(box->style()->font()->metrics().m_ascender -
                                  box->style()->font()->metrics().m_descender);
            box->asInlineBoxLayoutParentBox()->coordinateVerticalProperties(
                ctx, newYOffset);
        } else {
            box->moveY(-yOffset);
        }
    }
}

void InlineBoxLayoutParentBox::registerRelativePositionedBoxesAndMarkPaintFlag(
    LayoutContext& ctx)
{
    for (size_t k = 0; k < m_boxes.size(); k++) {
        FrameBox* childBox = m_boxes[k];
        STARFISH_ASSERT(childBox != nullptr);
        childBox->markSeenNormalFlowInline();

        if (!childBox->isEstablishesStackingContext()) {
            if (childBox->isInlineNonReplacedBox()) {
                seenInlineBox(PaintingInlineStage::PaintingInlineBox);
            } else if (childBox->isInlineTextBox()) {
                seenInlineBox(PaintingInlineStage::PaintingInlineBox);
            } else if (childBox->isAtomicInlineLevel() ||
                       childBox->isFloating()) {
                seenInlineBox(PaintingInlineStage::
                                  PaintingAtomicInlineBoxButInlineReplaced);
            } else if (childBox->isFrameReplaced()) {
                seenInlineBox(PaintingInlineStage::PaintingInlineReplaced);
            } else {
                seenInlineBox(PaintingInlineStage::PaintingInlineBox);
            }
        }

        if (!childBox->isFrameBlockBox()) {
            if (childBox->style()->position() ==
                PositionValue::RelativePositionValue) {
                ctx.registerRelativePositionedBox(childBox, true);
            }

            if (childBox->isInlineNonReplacedBox()) {
                childBox->asInlineNonReplacedBox()
                    ->registerRelativePositionedBoxesAndMarkPaintFlag(ctx);
            }
        }
    }
}

void InlineBoxLayoutParentBox::moveToNewLineBox(LineFormattingContext* ctx,
                                                FrameBox* box, LineBox* lineBox)
{
    LayoutLocation loc = box->absolutePoint(lineBox);
    m_boxes.erase(std::find(m_boxes.begin(), m_boxes.end(), box));
    box->setX(loc.x());
    box->setY(0);
    ctx->unMarkAbsolutePositionedBoxLayoutParent(this);
    lineBox->insertInlineBox(box);
    ctx->markAbsolutePositionedBoxLayoutParent(lineBox);
}

void InlineBoxLayoutParentBox::mergeInlineTextBoxes(LineFormattingContext* ctx)
{
    auto& boxes = this->boxes();
    auto it = boxes.begin();
    InlineTextBox* first = nullptr;
    if (isLineBox() || !hasIsolateBidiContent(style())) {
        if (ctx->m_languageDirections[ctx->m_block].isMixed()) {
            return;
        }
    } else {
        if (ctx->m_languageDirections[asInlineNonReplacedBox()->origin()]
                .isMixed()) {
            return;
        }
    }

    if (isLineBox()) {
        if (ctx->m_block->style()->wordSpacing() != Length(Length::Fixed, 0)) {
            return;
        }
    } else {
        if (this->style()->wordSpacing() != Length(Length::Fixed, 0)) {
            return;
        }
    }

    LayoutUnit totalWidth;
    StringBuilder builder;
    while (it != boxes.end()) {
        FrameBox* box = *it;

        if (box->isInlineTextBox()) {
            InlineTextBox* textBox = box->asInlineTextBox();
            totalWidth += textBox->width();
            StringView sv = textBox->text();
            builder.appendString(sv);
            if (first) {
                ctx->m_layoutContext.pushIntoInlineTextBoxPool(
                    (*it)->asInlineTextBox());
                it = boxes.erase(it);
                continue;
            } else {
                first = textBox;
            }
        } else {
            if (first) {
                first->setText(builder.finalizeToStringView());
                first->setWidth(totalWidth);
                builder.clear();
                first = nullptr;
                totalWidth = 0;
            }
            if (box->isInlineNonReplacedBox()) {
                box->asInlineNonReplacedBox()->mergeInlineTextBoxes(ctx);
            }
        }
        it++;
    }

    if (first) {
        first->setText(builder.finalizeToStringView());
        first->setWidth(totalWidth);
    }
}

void InlineBoxLayoutParentBox::paintInlineContent(Canvas* canvas,
                                                  PaintingInlineStage stage,
                                                  LayoutUnit dx, LayoutUnit dy)
{
    if (stage == PaintingInlineBox && !m_flags.m_seenNormalFlowInlineBox) {
        return;
    } else if (stage == PaintingAtomicInlineBoxButInlineReplaced &&
               !m_flags.m_seenNormalFlowInlineBlockBox) {
        return;
    } else if (stage == PaintingInlineReplaced &&
               !m_flags.m_seenNormalFlowInlineReplaced) {
        return;
    }

    for (size_t k = 0; k < m_boxes.size(); k++) {
        FrameBox* childBox = m_boxes[k];
        if (childBox->isEstablishesStackingContext()) {
            continue;
        }

        LayoutUnit cdx = childBox->x();
        LayoutUnit cdy = childBox->y();
        dx += cdx;
        dy += cdy;

        if (childBox->isInlineNonReplacedBox()) {
            canvas->save();

            if (childBox->shouldResetTextDecoration()) {
                canvas->resetTextDecorationData();
            } else {
                canvas->mergeTextDecorationData(childBox->style());
            }

            bool overflowApplied = childBox->shouldApplyOverflow();

            if (childBox->style()->visibility() ==
                VisibilityValue::HiddenVisibilityValue) {
                canvas->setVisible(false);
            } else {
                canvas->setVisible(true);
            }

            if (overflowApplied) {
                canvas->save();
                auto clipRect =
                    childBox->makeRect(BoxValue::PaddingBoxBoxValue);
                clipRect.setX(clipRect.x() + dx);
                clipRect.setY(clipRect.y() + dy);
                canvas->clip(clipRect);
                const LayoutRect rect(0, 0, childBox->width(),
                                      childBox->height());
                childBox->applyBorderRadiusClippingIfNeeds(canvas, rect);
            }

            childBox->paintInlineContent(canvas, stage, dx, dy);

            if (overflowApplied) {
                canvas->restore();
            }

            canvas->restore();
        } else if (childBox->isInlineTextBox()) {
            childBox->paintInlineContent(canvas, stage, dx, dy);
        } else if (childBox->isFrameReplaced()) {
            if (stage == PaintingAtomicInlineBoxButInlineReplaced &&
                childBox->isFloating()) {
                PaintingContext ctx(canvas);
                ctx.m_paintingStage = PaintingNonPositionedFloats;
                canvas->translate(dx, dy);
                childBox->paintContent(ctx);
                canvas->translate(-dx, -dy);
            } else if (stage == PaintingAtomicInlineBoxButInlineReplaced &&
                       childBox->isAtomicInlineLevel()) { // inline-block
                PaintingContext ctx(canvas);
                ctx.m_paintingStage = PaintingNormalFlowInline;
                canvas->translate(dx, dy);
                childBox->paintContent(ctx);
                canvas->translate(-dx, -dy);
            } else if (stage == PaintingInlineReplaced) {
                PaintingContext ctx(canvas);
                ctx.m_paintingStage = PaintingNormalFlowInline;
                canvas->translate(dx, dy);
                childBox->paintContent(ctx);
                canvas->translate(-dx, -dy);
            }

        } else if (childBox->isAtomicInlineLevel() || childBox->isFloating()) {
            if (stage == PaintingAtomicInlineBoxButInlineReplaced) {
                STARFISH_ASSERT(childBox->isFrameBlockBox());
                canvas->save();

                canvas->translate(dx, dy);
                PaintingContext ctx(canvas);
                childBox->paintBackgroundAndBorders(canvas);

                if (childBox->shouldResetTextDecoration()) {
                    canvas->resetTextDecorationData();
                } else {
                    canvas->mergeTextDecorationData(childBox->style());
                }

                bool overflowApplied = childBox->shouldApplyOverflow();

                if (childBox->style()->visibility() ==
                    VisibilityValue::HiddenVisibilityValue) {
                    canvas->setVisible(false);
                } else {
                    canvas->setVisible(true);
                }

                STARFISH_ASSERT(!isPositioned());

                if (overflowApplied) {
                    canvas->save();
                    canvas->clip(
                        childBox->makeRect(BoxValue::PaddingBoxBoxValue));
                    const LayoutRect rect(0, 0, childBox->width(),
                                          childBox->height());
                    childBox->applyBorderRadiusClippingIfNeeds(canvas, rect);
                    canvas->translate(
                        -childBox->asFrameBlockBox()->scrollLeft(),
                        -childBox->asFrameBlockBox()->scrollTop());
                }

                while (ctx.m_paintingStage != PaintingStageEnd) {
                    childBox->paintChildrenWith(ctx);
                    ctx.m_paintingStage =
                        (PaintingStage)(ctx.m_paintingStage + 1);
                }

                if (overflowApplied) {
                    canvas->restore();
                    if (childBox->node() && childBox->node()->isElement()) {
                        if (childBox->node()->asElement()->hasRareMembers() &&
                            childBox->node()
                                ->asElement()
                                ->rareMembers()
                                ->m_scrolling) {
                            childBox->node()
                                ->asElement()
                                ->rareMembers()
                                ->m_scrolling->paintScrollbars(
                                    canvas, childBox->asFrameBlockBox(),
                                    childBox->appliedOverflowX(),
                                    childBox->appliedOverflowY());
                        }
                    }
                }

                canvas->restore();
            }
        } else {
            childBox->paintInlineContent(canvas, stage, dx, dy);
        }

        dx -= cdx;
        dy -= cdy;
    }
}

void LineFormattingContext::markInlineBoxIndex(FrameBox* box)
{
    setInlineBoxIndex(box, m_inlineBoxIndex);
    m_inlineBoxIndex++;
}

static bool dontClear(int hasFloat, Frame* f)
{
    return (f->style()->clear() == LeftClearValue &&
            (hasFloat & HasLeft) == 0) ||
           (f->style()->clear() == RightClearValue &&
            (hasFloat & HasRight) == 0) ||
           (f->style()->clear() == BothClearValue && hasFloat == HasNone);
}

bool LineFormattingContext::canInsertFloatingBox(FrameBox* f,
                                                 bool allowPendingFloatingBox)
{
    if (!allowPendingFloatingBox) {
        if (m_pendingFloatingBoxes.size() > 0) {
            return false;
        }
    }

    FloatingBoxLayoutContext* fbCtx =
        &m_floatingBoxLayoutContexts[m_floatingBoxLayoutContexts.size() - 1];

    if (f->style()->clear() == NoneClearValue ||
        dontClear(fbCtx->m_hasFloat, f)) {
    } else {
        makeFloatingBoxLayoutContextDueToClearIfNeeds(f);
    }

    LineBox* lineBox = currentLine();
    fbCtx =
        &m_floatingBoxLayoutContexts[m_floatingBoxLayoutContexts.size() - 1];

    return m_absPosition.y() + lineBox->y() + fbCtx->m_y >=
           m_layoutContext.lastTopLoc();
}

void LineFormattingContext::makeFloatingBoxLayoutContext(LayoutUnit yDiff)
{
    LayoutUnit oldLineBoxX = m_lineBoxX;
    LayoutUnit oldLineBoxWidth = m_lineBoxWidth;
    layoutLineBox(yDiff, 0);
    m_lineBoxX = oldLineBoxX;
    m_lineBoxWidth = oldLineBoxWidth;
    currentLine()->setX(m_lineBoxX);
    currentLine()->setWidth(m_lineBoxWidth);
}

void LineFormattingContext::makeFloatingBoxLayoutContextDueToClearIfNeeds(
    FrameBox* box)
{
    LayoutUnit clearedDistanceToFloatBottom =
        m_layoutContext.clearedDistanceToFloatBottom(
            m_absPosition.y() + m_lineBoxY, box->style()->clear());

    if (clearedDistanceToFloatBottom == 0) {
        return;
    }

    makeFloatingBoxLayoutContext(clearedDistanceToFloatBottom);
}

void LineFormattingContext::insertPendingFloatingBoxes()
{
    bool onlyAllowBeforeCurrentLine = m_pendingInlineBoxes.size() > 0;

    auto iter = m_pendingFloatingBoxes.begin();
    while (iter != m_pendingFloatingBoxes.end()) {
        FrameBox* box = *iter;
        if ((m_pendingFloatingBoxNumsBeforeCurrentLine > 0 ||
             !onlyAllowBeforeCurrentLine) &&
            canInsertFloatingBox(box, true) &&
            dontBreakLine(box, box->outerWidth())) {
            insertFloatingBoxAndReLayoutLineBoxIfNeeds(box);

            iter = m_pendingFloatingBoxes.erase(iter);

            m_pendingFloatingBoxNumsBeforeCurrentLine--;
            if (m_pendingFloatingBoxNumsBeforeCurrentLine == SIZE_MAX) {
                m_pendingFloatingBoxNumsBeforeCurrentLine = 0;
            }

            continue;
        }

        break;
    }
}

void LineFormattingContext::insertAbsolutePositionedBoxes()
{
    LineBox* lineBox = currentLine();
    DirectionValue dir = m_block->style()->direction();
    FrameBox* firstInlineBox = lineBox->firstInlineBox();
    size_t nextInlineBoxIndex = SIZE_MAX;
    if (m_pendingInlineBoxes.size() > 0) {
        nextInlineBoxIndex = inlineBoxIndex((*m_pendingInlineBoxes.begin()));
    }

    auto iter = m_absolutePositionedBoxes.begin();

    while (iter != m_absolutePositionedBoxes.end()) {
        FrameBox* box = *iter;

        if (nextInlineBoxIndex != SIZE_MAX) {
            if (nextInlineBoxIndex < inlineBoxIndex(box)) {
                // Absolute positioned box can be located on the same line box
                // if it appeared earlier than the first inline box of
                // pending inline boxes.
                break;
            }
        }

        if (box->isBlockLevel(true)) {
            if (dir == DirectionValue::LtrDirectionValue) {
                box->setX(m_leftBoundary - m_absPosition.x() - m_lineBoxX);
            } else {
                box->setX(m_rightBoundary - m_absPosition.x() - m_lineBoxX);
            }

            if (firstInlineBox) {
                if (inlineBoxIndex(firstInlineBox) < inlineBoxIndex(box)) {
                    box->setY(lineBox->height());
                } else {
                    box->setY(0);
                }
            } else {
                box->setY(0);
            }
            lineBox->insertInlineBox(box);
            markAbsolutePositionedBoxLayoutParent(lineBox);
        } else {
            box->layoutParent()->asInlineBoxLayoutParentBox()->moveToNewLineBox(
                this, box, lineBox);
        }

        iter = m_absolutePositionedBoxes.erase(iter);
    }
}

void LineFormattingContext::layoutLineBox(LayoutUnit yDiff, LayoutUnit height)
{
    LineBox* lineBox = currentLine();
    m_lastLineHasFloatValue = HasNone;
    std::pair<LayoutUnit, LayoutUnit> boundaries =
        m_layoutContext.horizontalBoundaryBetweenFloatingBoxes(
            m_absPosition.y() + m_lineBoxY + yDiff, height, m_leftBoundary,
            m_rightBoundary);
    if (boundaries.first > m_leftBoundary) {
        m_lastLineHasFloatValue |= HasLeft;
        m_lineBoxX = boundaries.first - m_absPosition.x();
    } else {
        m_lineBoxX = m_leftBoundary - m_absPosition.x();
    }

    if (m_rightBoundary > boundaries.second) {
        m_lastLineHasFloatValue |= HasRight;
        m_lineBoxWidth = boundaries.second - m_leftBoundary;
    } else {
        m_lineBoxWidth = m_rightBoundary - m_leftBoundary;
    }
    m_lineBoxWidth -= (m_lineBoxX - m_leftBoundary + m_absPosition.x());

    lineBox->setX(m_lineBoxX);
    lineBox->setY(m_lineBoxY);
    lineBox->setWidth(m_lineBoxWidth);

    if (m_floatingBoxLayoutContexts.size() == 0) {
        m_floatingBoxLayoutContexts.emplace_back(m_lastLineHasFloatValue, yDiff,
                                                 m_lineBoxX, m_lineBoxWidth);
    } else {
        if (m_floatingBoxLayoutContexts[m_floatingBoxLayoutContexts.size() - 1]
                .m_y < yDiff) {
            m_floatingBoxLayoutContexts.emplace_back(
                m_lastLineHasFloatValue, yDiff, m_lineBoxX, m_lineBoxWidth);
        }
    }
}

void LineFormattingContext::insertFloatingBoxAndReLayoutLineBoxIfNeeds(
    FrameBox* box)
{
    if (box->height() == 0 && box->marginHeight() == 0) {
        return;
    }

    FloatingBoxLayoutContext& fbCtx = *m_floatingBoxLayoutContexts.rbegin();
    if (box->style()->floating() == LeftFloatValue) {
        fbCtx.m_hasFloat |= HasLeft;
        LayoutUnit lastAccumulatedLeftFloatingBoxWidth =
            fbCtx.m_accumulatedLeftFloatingBoxWidth;
        LayoutUnit oldLineBoxX = m_lineBoxX;
        fbCtx.m_accumulatedLeftFloatingBoxWidth += box->outerWidth();
        if (fbCtx.m_y == 0 && box->outerWidth() > 0) {
            m_lineBoxX = fbCtx.m_originalLineBoxX +
                         fbCtx.m_accumulatedLeftFloatingBoxWidth;
            m_lineBoxWidth = fbCtx.m_originalLineBoxWidth -
                             fbCtx.m_accumulatedLeftFloatingBoxWidth -
                             fbCtx.m_accumulatedRightFloatingBoxWidth;
        }

        box->setX(fbCtx.m_originalLineBoxX +
                  lastAccumulatedLeftFloatingBoxWidth + box->marginLeft() -
                  m_lineBoxX);

        if (oldLineBoxX != m_lineBoxX) {
            reCacheFloatingBoxes(oldLineBoxX - m_lineBoxX);
        }
    } else {
        fbCtx.m_hasFloat |= HasRight;
        fbCtx.m_accumulatedRightFloatingBoxWidth += box->outerWidth();
        if (fbCtx.m_y == 0 && box->outerWidth() > 0) {
            m_lineBoxWidth = fbCtx.m_originalLineBoxWidth -
                             fbCtx.m_accumulatedLeftFloatingBoxWidth -
                             fbCtx.m_accumulatedRightFloatingBoxWidth;
        }

        LayoutUnit rightFloatX = fbCtx.m_originalLineBoxX +
                                 fbCtx.m_originalLineBoxWidth -
                                 fbCtx.m_accumulatedRightFloatingBoxWidth;
        LayoutUnit marginLeft = std::max(LayoutUnit(0), box->marginLeft());
        LayoutUnit marginRight = box->marginRight();
        if (marginRight < 0) {
            marginLeft = std::max(marginLeft, -marginRight - box->width());
        }

        box->setX(rightFloatX + marginLeft - m_lineBoxX);
    }

    box->setY(fbCtx.m_y + box->marginTop());
    currentLine()->insertInlineBox(box);
    m_layoutContext.registerFloatingBox(box);
}

LayoutUnit LineFormattingContext::offsetApplyingTextAlign()
{
    TextAlignValue textAlign = m_block->style()->textAlign();
    DirectionValue direction = m_block->style()->direction();
    LayoutUnit offset;

    if (textAlign == LeftTextAlignValue ||
        (textAlign == StartTextAlignValue && direction == LtrDirectionValue) ||
        (textAlign == EndTextAlignValue && direction == RtlDirectionValue)) {
        if (direction == LtrDirectionValue) {
            offset = m_textIndentWidth;
        } else {
            if (m_lineBoxWidth - m_currentLineWidth > m_textIndentWidth) {
                offset = 0;
            } else {
                offset =
                    (m_lineBoxWidth - m_currentLineWidth - m_textIndentWidth);
            }
        }
    } else if (textAlign == TextAlignValue::RightTextAlignValue ||
               (textAlign == StartTextAlignValue &&
                direction == RtlDirectionValue) ||
               (textAlign == EndTextAlignValue &&
                direction == LtrDirectionValue)) {
        if (direction == LtrDirectionValue) {
            if (m_lineBoxWidth - m_currentLineWidth > m_textIndentWidth) {
                offset = (m_lineBoxWidth - m_currentLineWidth);
            } else {
                offset = m_textIndentWidth;
            }
        } else {
            offset = (m_lineBoxWidth - m_currentLineWidth - m_textIndentWidth);
        }
        /*
         * justify: No supported value
        } else if (m_block.style()->textAlign() ==
                   SideValue::JustifySideValue) {
            // issue #145
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
            if (lineFormattingContext.isBreakedLineWithoutBR(i)) {
                LayoutUnit remainSpace = (inlineContentWidth - x);
                if (remainSpace > 0) {
                    size_t spaceBoxCnt = 0;
                    for (size_t j = 0; j < back->m_boxes.size(); j++) {
                        FrameBox* box = back->m_boxes[j];
                        if (box->isInlineTextBox()) {
                            String* str = box->asInlineTextBox()->text();
                            if (str->equals(str)) {
                                spaceBoxCnt++;
                            }
                        }
                    }

                    if (spaceBoxCnt) {
                        LayoutUnit moreWidthForSpace = remainSpace /
                                                       spaceBoxCnt;
                        LayoutUnit diff = 0;
                        for (size_t j = 0; j < back->m_boxes.size(); j ++) {
                            FrameBox* box = back->m_boxes[j];
                            box->moveX(diff);
                            if (box->isInlineTextBox()) {
                                String* str = x->asInlineTextBox()->text();
                                if (str->equals(str)) {
                                    diff += moreWidthForSpace;
                                }
                            }
                        }
                    }
                }
            }
            */
    } else {
        STARFISH_ASSERT(textAlign == TextAlignValue::CenterTextAlignValue);
        if (direction == LtrDirectionValue) {
            if (m_lineBoxWidth - m_currentLineWidth > m_textIndentWidth) {
                offset =
                    (m_lineBoxWidth - m_currentLineWidth + m_textIndentWidth) /
                    2;
            } else {
                offset = m_textIndentWidth;
            }
        } else {
            if (m_lineBoxWidth - m_currentLineWidth > m_textIndentWidth) {
                offset =
                    (m_lineBoxWidth - m_currentLineWidth - m_textIndentWidth) /
                    2;
            } else {
                offset =
                    (m_lineBoxWidth - m_currentLineWidth - m_textIndentWidth);
            }
        }
    }

    return offset;
}

void LineFormattingContext::computeHorizontalProperties()
{
    LineBox* back = m_block->m_lineBoxes.back();
    DirectionValue direction = m_block->style()->direction();

    if (m_currentLanguageDirection.isMixed()) {
        resolveBidi(direction, back->boxes());
    }
    m_currentLayoutParent->setLeftMBPs(this);
    m_currentLayoutParent->setRightMBPs(this);

    back->setX(m_lineBoxX);
    back->setWidth(m_lineBoxWidth);
    LayoutUnit offset = offsetApplyingTextAlign();
    back->layoutInlineBoxes(this, offset);
}

LayoutUnit LineFormattingContext::distanceToNextLineBox(FrameLineBreak* br,
                                                        bool hasMoreInlineBoxes)
{
    LineBox* lineBox = currentLine();
    STARFISH_ASSERT(lineBox != nullptr);
    LayoutUnit distance = lineBox->height();
    FloatingBoxLayoutContext& fbCtx = (*m_floatingBoxLayoutContexts.rbegin());

    if (fbCtx.m_hasFloat != HasNone) {
        if (br != nullptr) {
            distance =
                std::max(distance, m_layoutContext.clearedDistanceToFloatBottom(
                                       m_absPosition.y() + m_lineBoxY,
                                       br->style()->clear()));
            lineBox->setHeight(distance);
        } else {
            if (m_currentLineWidth == 0) {
                distance = m_layoutContext.nextDistanceToFloatBottom(
                    m_absPosition.y() + m_lineBoxY, distance);
            }
        }
    }

    if (!hasMoreInlineBoxes && distance == 0 &&
        m_pendingFloatingBoxes.size() > 0) {
        // if there are only pending float box and the height which y diff for
        // the next linebox is zero, then it goes infinite loop. It happens
        // because 2 things. First, the rule that float box can't be inserted
        // into the line box when the last top position of float box whose
        // direction is the same with the direction of float box we are trying
        // to insert. Second, this kind of float box can't be detected by using
        // current y position of line box and its height.
        LayoutUnit curYPos = m_absPosition.y() + m_lineBoxY;
        LayoutUnit yToPos = m_layoutContext.lastTopLoc();
        distance = yToPos - curYPos;
    }

    return distance;
}

template <typename Iter>
void LineFormattingContext::sortInlineBoxes(Iter& iter)
{
    std::sort(iter.begin(), iter.end(), [&](FrameBox* a, FrameBox* b) {
        return inlineBoxIndex(a) > inlineBoxIndex(b);
    });
}

void LineFormattingContext::removeAllInlineBoxes()
{
    LineBox* lineBox = currentLine();
    auto iter = lineBox->boxes().begin();

    bool hasAlreadyPendingInlineBoxes = (m_pendingInlineBoxes.size() > 0);
    size_t index = 0;

    while (iter != lineBox->boxes().end()) {
        FrameBox* box = *iter;
        if (box->isFloating()) {
            iter++;
            continue;
        } else if (box->isAbsolutePositioned()) {
            unMarkAbsolutePositionedBoxLayoutParent(lineBox);
        }

        // already its x, y positions are somehow determined, so we have to
        // reset the values.
        box->setX(0);
        box->setY(0);
        if (!hasAlreadyPendingInlineBoxes) {
            m_pendingInlineBoxes.push_back(box);
        } else {
            m_pendingInlineBoxes.insert(m_pendingInlineBoxes.begin() + index,
                                        box);
            index++;
        }

        iter = lineBox->boxes().erase(iter);
    }

    if (m_block->style()->direction() == RtlDirectionValue) {
        sortInlineBoxes(m_pendingInlineBoxes);
        sortInlineBoxes(lineBox->boxes());
    }

#ifndef NDEBUG
    iter = lineBox->boxes().begin();
    // LineBox should only contain floating boxes.
    STARFISH_ASSERT(
        std::all_of(lineBox->boxes().cbegin(), lineBox->boxes().cend(),
                    [](FrameBox* box) { return box->isFloating(); }));

    // Pending inline boxes should be put by the order as they were initially
    // inserted into the line box.
    size_t lastInlineboxIndex = SIZE_MAX;
    STARFISH_ASSERT(std::all_of(m_pendingInlineBoxes.begin(),
                                m_pendingInlineBoxes.end(), [&](FrameBox* box) {
                                    if (lastInlineboxIndex != SIZE_MAX) {
                                        return inlineBoxIndex(box) >
                                               lastInlineboxIndex;
                                    }
                                    lastInlineboxIndex = inlineBoxIndex(box);
                                    return true;
                                }));
#endif
}

void LineFormattingContext::insertPendingInlineBoxes()
{
    auto iter = m_pendingInlineBoxes.begin();

    while (iter != m_pendingInlineBoxes.end()) {
        FrameBox* box = *iter;
        STARFISH_ASSERT(box->isNormalFlow() || box->isAbsolutePositioned());

        if (!(m_currentLineWidth == 0 && isCollapsibleWhiteSpace(box))) {
            // TODO: consider if continuous box can form a word, these
            // should be on the same line
            if (box->isAbsolutePositioned()) {
                handleAbsoluteBox(box, true, false);
            } else {
                if (!dontBreakLine(box, box->outerWidth())) {
                    if (!m_canConcatWord && isWord(box)) {
                        InlineTextBox* box2 =
                            splitInlineTextBox(box->asInlineTextBox());
                        insertInlineBox(box2);
                    }
                    break;
                }

                insertInlineBox(box);
            }
        }

        iter = m_pendingInlineBoxes.erase(iter);
        continue;
    }
}

void LineFormattingContext::updateCurrentLayoutParent(Frame* parent)
{
    m_currentLayoutParent = parent->asInlineBoxLayoutParentBox();
    if (m_currentLayoutParent->isLineBox() ||
        !hasIsolateBidiContent(m_currentLayoutParent->style())) {
        m_currentLanguageDirection = m_languageDirections[m_block];
    } else {
        m_currentLanguageDirection =
            m_languageDirections[m_currentLayoutParent->asInlineNonReplacedBox()
                                     ->origin()];
    }

    if (m_currentLayoutParent->isLineBox()) {
        m_canConcatWord =
            m_block->style()->wordWrap() != BreakWordWordWrapValue;
    } else {
        m_canConcatWord = m_currentLayoutParent->style()->wordWrap() !=
                          BreakWordWordWrapValue;
    }
}

void LineFormattingContext::resetLineBox()
{
    LineBox* lineBox = new LineBox(m_block);
    m_block->m_lineBoxes.push_back(lineBox);
    m_floatingBoxLayoutContexts.clear();
    layoutLineBox(0, 0);
    m_currentLineWidth = 0;
    if (m_pendingInlineBoxes.size() == 0) {
        m_pendingFloatingBoxNumsBeforeCurrentLine =
            m_pendingFloatingBoxes.size();
    }
    m_isPendingBreakLine = false;
    m_isSoftHyphenAtLast = false;
    m_isWhiteSpaceAtLast = true;
    m_floatingBoxesSizeBeforeCurrentLine = m_layoutContext.floatingBoxesSize();
}

void LineFormattingContext::reCacheFloatingBoxes(LayoutUnit xDiff)
{
    LineBox* back = currentLine();

    back->setX(m_lineBoxX);

    for (size_t i = 0; i < back->boxes().size(); i++) {
        FrameBox* childBox = back->boxes()[i];
        if (childBox->isNormalFlow()) {
            continue;
        } else if (childBox->isAbsolutePositioned()) {
            continue;
        } else {
            STARFISH_ASSERT(childBox->isFloating());
            childBox->moveX(xDiff);
        }
    }

    m_layoutContext.reCacheFloatingBoxes(m_floatingBoxesSizeBeforeCurrentLine);
}

bool LineFormattingContext::isAnyOfInlineBoxesCollidedWithFloatingBoxes()
{
    LineBox* lineBox = currentLine();
    auto iter = lineBox->boxes().begin();

    while (iter != lineBox->boxes().end()) {
        if ((*iter)->isFloating()) {
            iter++;
            continue;
        }

        FrameBox* box = *iter;

        // Check boundary as if inline boxes are not vertical aligned yet, so we
        // don't consider box->y() here.
        if (m_layoutContext.isCollidedWithFloatingBoxes(
                LayoutLocation(m_absPosition.x() + m_lineBoxX + box->x(),
                               m_absPosition.y() + m_lineBoxY),
                box, m_leftBoundary, m_rightBoundary)) {
            return true;
        }

        iter++;
    }

    return false;
}

void LineFormattingContext::finishLineForLineBox(FrameLineBreak* br,
                                                 bool isLastLine)
{
    LineBox* back = currentLine();
    computeVerticalProperties(back, br != nullptr);
    LayoutUnit height = back->height();

    FloatingBoxLayoutContext& fbCtx = *m_floatingBoxLayoutContexts.begin();
    if (fbCtx.m_hasFloat != HasNone && m_pendingFloatingBoxes.size() > 0 &&
        height > 0) {
        LayoutUnit nextDistanceToFloatBottom =
            m_layoutContext.nextDistanceToFloatBottom(
                m_absPosition.y() + m_lineBoxY, 0);
        FloatingBoxLayoutContext* lastFbCtx =
            &(*m_floatingBoxLayoutContexts.rbegin());
        if (nextDistanceToFloatBottom != 0 &&
            height > nextDistanceToFloatBottom &&
            nextDistanceToFloatBottom > lastFbCtx->m_y) {
            makeFloatingBoxLayoutContext(nextDistanceToFloatBottom);
            insertPendingFloatingBoxes();
            removeAllInlineBoxes();
            m_currentLineWidth = 0;
            insertPendingInlineBoxes();

            finishLineForLineBox(br, isLastLine);
            return;
        }
    }

    back->removeDanglingSpace(this);
    // Should check if there has enough space for pending block box due to
    // removing white space from above function `removeDanglingSpaceFromLine`
    insertPendingFloatingBoxes();
    back->coordinateVerticalProperties(this, 0);
    computeHorizontalProperties();
    registerInlineContent(br);

    if (m_layoutContext.isCollidedWithFloatingBoxes(
            LayoutLocation(m_absPosition.x() + m_lineBoxX,
                           m_absPosition.y() + m_lineBoxY),
            back, m_leftBoundary, m_rightBoundary)) {
        if (isAnyOfInlineBoxesCollidedWithFloatingBoxes()) {
            removeAllInlineBoxes();
            LayoutUnit oldLineBoxX = m_lineBoxX;
            layoutLineBox(0, height);
            if (oldLineBoxX != m_lineBoxX) {
                reCacheFloatingBoxes(oldLineBoxX - m_lineBoxX);
            }
            m_currentLineWidth = 0;
            insertPendingInlineBoxes();

            finishLineForLineBox(br, isLastLine);
            return;
        }
    }

    insertAbsolutePositionedBoxes();
#ifdef STARFISH_ENABLE_TEST
    if (!g_enablePixelTest) {
        back->mergeInlineTextBoxes(this);
    }
#else
    back->mergeInlineTextBoxes(this);
#endif

    LayoutUnit yDiff = distanceToNextLineBox(
        br, !isLastLine || m_pendingInlineBoxes.size() > 0);
    m_lineBoxY += yDiff;

    if (isLastLine) {
        STARFISH_ASSERT(m_absolutePositionedBoxes.size() == 0 ||
                        m_pendingInlineBoxes.size() > 0);
        if (m_pendingInlineBoxes.size() > 0 ||
            m_pendingFloatingBoxes.size() > 0) {
            breakLineForLineBox(nullptr, isLastLine, true);
        }
    }
}

void LineFormattingContext::breakLineForLineBox(FrameLineBreak* br,
                                                bool isLastLine,
                                                bool skipFinishLine)
{
    if (!skipFinishLine) {
        finishLineForLineBox(br, isLastLine);
    }

    if (m_currentLineWidth > 0 || br != nullptr) {
        m_textIndentWidth = 0;
    }
    resetLineBox();
    if (m_currentLayoutParent->isLineBox()) {
        updateCurrentLayoutParent(currentLine());
    }

    insertPendingFloatingBoxes();
    insertPendingInlineBoxes();

    if (m_pendingInlineBoxes.size() > 0) {
        breakLineForLineBox(nullptr, isLastLine, false);
    } else if (isLastLine) {
        finishLineForLineBox(nullptr, isLastLine);
    }
}

bool PreferredWidthContext::canInsertFloatingBox(Frame* f)
{
    return (f->style()->clear() == NoneClearValue || dontClear(m_hasFloat, f));
}

bool PreferredWidthContext::canInsertToLineBox(LayoutUnit width)
{
    LayoutUnit remainingWidth =
        (m_remainingWidth - m_currentLineWidth - m_unprocessedStartingMBPWidth -
         m_textIndentWidth);
    return width <= remainingWidth;
}

bool PreferredWidthContext::dontBreakLine(LayoutUnit width)
{
    return (!hasFloatingBoxAlreadyInLineBox() && m_currentLineWidth == 0) ||
           canInsertToLineBox(width);
}

bool LineFormattingContext::canInsertToLineBox(FrameBox* f, LayoutUnit width)
{
    if (f->isFloating()) {
        FloatingBoxLayoutContext& fbCtx = *m_floatingBoxLayoutContexts.rbegin();
        LayoutUnit remainedWidth = fbCtx.m_originalLineBoxWidth -
                                   fbCtx.m_accumulatedLeftFloatingBoxWidth -
                                   fbCtx.m_accumulatedRightFloatingBoxWidth;
        if (fbCtx.m_y == 0) {
            return width <= (remainedWidth - m_currentLineWidth -
                             m_unprocessedStartingMBPWidth);
        } else {
            return width <= remainedWidth;
        }
    } else {
        LayoutUnit remainingWidth =
            (m_lineBoxWidth - m_currentLineWidth -
             m_unprocessedStartingMBPWidth - m_textIndentWidth);
        LayoutUnit ws = wordSpacing(f, m_currentLayoutParent);
        bool ret = (width + ws) <= remainingWidth;

        if (!ret && f->isInlineTextBox()) {
            InlineTextBox* itb = f->asInlineTextBox();
            if (isWhiteSpace(itb->textRun())) {
                String* str = itb->textRun().m_stringView.originalString();
                if (str != String::spaceString) {
                    // NonCollapsibleWhiteSpace
                    itb->setWidth(itb->width() - (width - remainingWidth));
                    return true;
                }
            }
        }

        return ret;
    }
}

bool LineFormattingContext::hasFloatingBoxAlreadyInLineBox(Frame* f)
{
    if (f->isFloating()) {
        FloatingBoxLayoutContext& fbCtx =
            (*m_floatingBoxLayoutContexts.rbegin());
        return fbCtx.m_hasFloat != HasNone;
    } else {
        FloatingBoxLayoutContext& fbCtx =
            (*m_floatingBoxLayoutContexts.begin());
        return fbCtx.m_hasFloat != HasNone;
    }
}

// TODO: when concatenating word, we should check if word to concatenate
// is shouldWrapLines.
bool LineFormattingContext::dontBreakLine(FrameBox* box, LayoutUnit width)
{
    bool wrapLine = box->shouldWrapLines();
    if (box->isFloating()) {
        wrapLine = true;
    }
    return (!hasFloatingBoxAlreadyInLineBox(box) && m_currentLineWidth == 0 &&
            !(isWord(box) && !m_canConcatWord)) ||
           !wrapLine || canInsertToLineBox(box, width);
}

void LineFormattingContext::handleSoftHyphenate(bool hyphenateOnLine)
{
    if (!m_isSoftHyphenAtLast) {
        return;
    }

    // It can behave different depending on the language.
    // Please refer to http://unicode.org/reports/tr14/#SoftHyphen
    InlineBoxLayoutParentBox* parent;
    if (isWordProcessing()) {
        parent = (*m_word.boxes().begin())
                     ->layoutParent()
                     ->asInlineBoxLayoutParentBox();
    } else {
        parent = m_currentLayoutParent;
    }
    InlineTextBox* itb = (*parent->boxes().rbegin())->asInlineTextBox();
    const StringView& sv = itb->textRun().m_stringView;
    size_t start = sv.start();
    size_t end = sv.end();
    STARFISH_ASSERT(isSoftHyphen(sv.originalString()->charAt(end - 1)));
    StringBuilder builder;
    builder.appendSubString(sv.originalString(), start, end - 1);
    if (hyphenateOnLine) {
        builder.appendChar((char32_t)0x2010);
    }
    String* newStr = builder.finalize();
    itb->setText(newStr);
    LayoutUnit width = itb->width();
    itb->setWidth(itb->style()->font()->measureText(
        itb->asInlineTextBox()->textRun().m_stringView));
    itb->setHeight(itb->style()->font()->metrics().m_fontHeight);
    m_currentLineWidth += itb->width() - width;
    m_isSoftHyphenAtLast = false;
}

void LineFormattingContext::insertWord(Frame* next)
{
    if (!isWordProcessing()) {
        handleSoftHyphenate(false);
        return;
    }

    STARFISH_ASSERT(m_canConcatWord);
    if (dontBreakLine(m_word.boxes()[0], m_word.width())) {
        handleSoftHyphenate(false);
        auto& boxes = m_word.boxes();
        auto iter = boxes.begin();
        updateCurrentLayoutParent(boxes[0]->layoutParent());

        while (true) {
            FrameBox* box = nullptr;
            if (iter != boxes.end()) {
                box = *iter;
            }

            while (m_currentLayoutParent->isInlineNonReplacedBox()) {
                if (iter != boxes.end()) {
                    InlineBoxLayoutParentBox* layoutParent =
                        box->layoutParent()->asInlineBoxLayoutParentBox();
                    if (!layoutParent->isLineBox() &&
                        layoutParent->asInlineNonReplacedBox()->origin() ==
                            m_currentLayoutParent->asInlineNonReplacedBox()
                                ->origin()) {
                        break;
                    }
                } else if (next) {
                    Frame* parent = next->parent();
                    if (parent->isFrameInline() &&
                        parent->asFrameInline() ==
                            m_currentLayoutParent->asInlineNonReplacedBox()
                                ->origin()) {
                        break;
                    }
                }

                finishLineForInlineNonReplacedBox(nullptr, true);
                updateCurrentLayoutParent(
                    m_currentLayoutParent->layoutParent());
                if (m_currentLayoutParent->isLineBox()) {
                    break;
                }
            }

            if (!box) {
                break;
            }

            if (box->isInlineNonReplacedBox()) {
                m_currentLayoutParent->insertInlineBox(box);
                if (m_currentLayoutParent->isLineBox()) {
                    markInlineBoxIndex(box);
                }
                updateCurrentLayoutParent(box);
            } else {
                if (box->isAbsolutePositioned()) {
                    handleAbsoluteBox(box, true, true);
                } else {
                    insertInlineBox(box);
                    markInlineBoxIndex(box);
                }
            }

            iter++;
        }

        m_word.clear();
    } else {
        if (isFirstLineBox() && m_block->node() &&
            m_block->node()->style()->seenPseudoElement(
                StyleResolver::PseudoElementFirstLine)) {
            m_word.unmarkFirstLine();
        }

        handleSoftHyphenate(true);

        auto& boxes = m_word.boxes();
        updateCurrentLayoutParent(boxes[0]->layoutParent());
        breakLine(nullptr);
        boxes[0]->setLayoutParent(m_currentLayoutParent);
        insertWord(next);
    }
}

LayoutUnit LineFormattingContext::wordSpacing(
    FrameBox* box, InlineBoxLayoutParentBox* parentBox)
{
    if (box->isInlineTextBox() &&
        (isWhiteSpace(box->asInlineTextBox()->textRun()) ||
         String::isNBSP(box->asInlineTextBox()->text().charAt(0)))) {
        if (parentBox->isLineBox()) {
            return m_block->style()->wordSpacing().specifiedValue(LayoutUnit(),
                                                                  m_block);
        } else {
            return parentBox->style()->wordSpacing().specifiedValue(
                LayoutUnit(), parentBox);
        }
    }
    return 0;
}

LayoutUnit PreferredWidthContext::wordSpacing(const TextToken& token)
{
    int cnt = 0;
    for (size_t i = token.m_start; i < token.m_end; i++) {
        if (String::isSpace(token.m_frameText->text()->charAt(i)) ||
            String::isNBSP(token.m_frameText->text()->charAt(i))) {
            cnt++;
        }
    }

    return cnt *
           token.m_frameText->parent()->style()->wordSpacing().specifiedValue(
               LayoutUnit(), token.m_frameText->parent());
}

void LineFormattingContext::insertInlineBox(FrameBox* box)
{
    m_currentLayoutParent->insertInlineBox(box);
    if (m_currentLayoutParent->isInlineNonReplacedBox()) {
        InlineNonReplacedBox* self =
            m_currentLayoutParent->asInlineNonReplacedBox();
        self->setWidth(self->width() + box->outerWidth());
        self->processStartingMBP(this);
    }

    m_currentLineWidth +=
        (box->outerWidth() + wordSpacing(box, m_currentLayoutParent));
    setIsWhiteSpaceAtLast(isCollapsibleWhiteSpace(box));
}

InlineTextBox* LineFormattingContext::splitInlineTextBox(InlineTextBox* textBox)
{
    TextRun run = textBox->textRun();
    size_t start = run.m_stringView.start();
    size_t end = run.m_stringView.end();
    StringView* sv = new StringView(run.m_stringView);
    InlineTextBox* ret = new InlineTextBox(textBox);
    ret->setLayoutParent(m_currentLayoutParent);
    ret->setHeight(ret->style()->font()->metrics().m_fontHeight);
    bool splitted = false;
    size_t splittedIndex;
    LayoutUnit remainingWidth =
        (m_lineBoxWidth - m_currentLineWidth - m_unprocessedStartingMBPWidth -
         m_textIndentWidth);
    float ratio;
    if (remainingWidth > 0) {
        ratio = remainingWidth / textBox->width();
    } else {
        ratio = 0;
    }
    splittedIndex =
        std::max((size_t)((end - start) * ratio) + start, start + 1);
    for (; splittedIndex > start; splittedIndex--) {
        sv->setEnd(splittedIndex);
        ret->setText(sv);
        ret->setWidth(ret->style()->font()->measureText(sv));
        if (ret->width() < remainingWidth) {
            splitted = true;
            break;
        }
    }

    StringView* sv2 = new StringView(run.m_stringView);
    if (splitted) {
        sv2->setStart(splittedIndex);
    } else {
        sv2->setStart(start + 1);
    }
    textBox->setText(sv2);
    textBox->setWidth(textBox->style()->font()->measureText(sv2));

    return ret;
}

void LineFormattingContext::tryInsertInlineBox(FrameBox* box)
{
    if (m_canConcatWord && isWord(box)) {
        m_word.concat(box);
        return;
    }

    STARFISH_ASSERT(m_word.isEmpty());

    if (isForcedNewLine(box)) {
        if (box->shouldPreserveWhiteSpaces() && box->shouldWrapLines()) {
            if (m_isPendingBreakLine) {
                breakLine(nullptr);
            } else {
                if (m_currentLayoutParent->boxes().size() == 0) {
                    m_isPendingBreakLine = true;
                } else {
                    breakLine(nullptr);
                }
            }
        } else {
            m_isPendingBreakLine = true;
        }
    } else {
        if (!dontBreakLine(box, box->outerWidth())) {
            if (isCollapsibleWhiteSpace(box)) {
                m_isPendingBreakLine = true;
            } else {
                if (!m_canConcatWord && isWord(box) &&
                    box->asInlineTextBox()->text().length() > 1) {
                    InlineTextBox* box2 =
                        splitInlineTextBox(box->asInlineTextBox());
                    insertInlineBox(box2);
                }
                breakLine(nullptr);

                if (!canInsertToLineBox(box, box->outerWidth()) &&
                    m_lastLineHasFloatValue == HasFloat::HasNone) {
                    insertInlineBox(box);
                    markInlineBoxIndex(box);
                } else {
                    tryInsertInlineBox(box);
                }
            }
            return;
        }
    }

    insertInlineBox(box);
    markInlineBoxIndex(box);
}

void LineFormattingContext::tryInsertFloatingBox(FrameBox* box)
{
    STARFISH_ASSERT(m_word.isEmpty());

    if (canInsertFloatingBox(box, false) &&
        dontBreakLine(box, box->outerWidth())) {
        insertFloatingBoxAndReLayoutLineBoxIfNeeds(box);
    } else {
        m_pendingFloatingBoxes.push_back(box);
    }
}

void LineFormattingContext::registerAbsolutePositionedBox(FrameBox* box)
{
    m_layoutContext.registerAbsolutePositionedBox(box);
    markInlineBoxIndex(box);

    m_absolutePositionedBoxes.push_back(box);
}

void LineFormattingContext::breakLine(FrameLineBreak* br)
{
    if (m_currentLayoutParent->isLineBox()) {
        breakLineForLineBox(br, false, false);
    } else {
        breakLineForInlineNonReplacedBox(br);
    }
}

void* LineFormattingContext::allocateInlineTextBox()
{
    if (m_layoutContext.hasItemInInlineTextBoxPool()) {
        return m_layoutContext.takeFromInlineTextBoxPool();
    } else {
        return InlineTextBox::operator new(sizeof(InlineTextBox));
    }
}

void* LineFormattingContext::allocateInlineNonReplacedBox()
{
    if (m_layoutContext.hasItemInInlineNonReplacedBoxPool()) {
        return m_layoutContext.takeFromInlineNonReplacedBoxPool();
    } else {
        return InlineNonReplacedBox::operator new(sizeof(InlineNonReplacedBox));
    }
}

void LineFormattingContext::generateInlineTextBox(TextToken& token)
{
    FrameText* f = token.m_frameText;
    String* srcTxt = f->text();
    size_t offset = token.m_start;
    size_t nextOffset = token.m_end;
    LayoutUnit textWidth = token.width();
    ComputedStyle* style = token.style();
    bool isFirstLine = token.isFirstLine();
    const std::vector<TextRun>& runs = m_textRunsPerFrameText[f];

    if (token.isWhiteSpace()) {
        CharDirection dir = CharDirection::Ltr;
        for (size_t i = 0; i < runs.size(); i++) {
            if (runs[i].m_stringView.start() <= offset &&
                offset + 1 <= runs[i].m_stringView.end()) {
                dir = runs[i].m_direction;
                break;
            }
        }
        String* source = String::spaceString;
        size_t start = 0, end = 1;
        if (token.m_type == WordType::NonCollapsibleWhiteSpace) {
            source = srcTxt;
            start = offset;
            end = nextOffset;
        } else if (token.m_type == WordType::ForcedNewline) {
            source = String::emptyString;
            start = end = 0;
        }
        InlineTextBox* ib = new (allocateInlineTextBox())
            InlineTextBox(f, TextRun(source, start, end, dir), isFirstLine);
        ib->setLayoutParent(m_currentLayoutParent);
        ib->setWidth(textWidth);
        ib->setHeight(f->style()->font()->metrics().m_fontHeight);
        tryInsertInlineBox(ib);
    } else if (m_currentLanguageDirection.isMixed()) {
        size_t startPos = offset;
        size_t endPos = nextOffset;

        std::set<size_t> splitPosition;

        for (size_t i = 0; i < runs.size(); i++) {
            TextRun r = runs[i];
            if (offset < r.m_stringView.start() &&
                r.m_stringView.start() < nextOffset) {
                splitPosition.insert(r.m_stringView.start());
            }
            if (offset < r.m_stringView.end() &&
                r.m_stringView.end() < nextOffset) {
                splitPosition.insert(r.m_stringView.end());
            }
        }

        if (splitPosition.size()) {
            size_t start = offset;
            size_t end = SIZE_MAX;
            auto iter = splitPosition.begin();
            while (iter != splitPosition.end()) {
                end = *iter;

                CharDirection dir = CharDirection::Ltr;
                for (size_t i = 0; i < runs.size(); i++) {
                    if (runs[i].m_stringView.start() <= start &&
                        end <= runs[i].m_stringView.end()) {
                        dir = runs[i].m_direction;
                        break;
                    }
                }

                InlineTextBox* ib = new (allocateInlineTextBox()) InlineTextBox(
                    f, TextRun(srcTxt, start, end, dir), isFirstLine);
                ib->setLayoutParent(m_currentLayoutParent);
                ib->setWidth(style->font()->measureText(
                    ib->asInlineTextBox()->textRun().m_stringView));
                ib->setHeight(f->style()->font()->metrics().m_fontHeight);
                tryInsertInlineBox(ib);
                start = end;
                iter++;
            }
            STARFISH_ASSERT(end != SIZE_MAX);
            if (end != nextOffset) {
                CharDirection dir = CharDirection::Ltr;
                for (size_t i = 0; i < runs.size(); i++) {
                    if (runs[i].m_stringView.start() <= end &&
                        nextOffset <= runs[i].m_stringView.end()) {
                        dir = runs[i].m_direction;
                        break;
                    }
                }
                InlineTextBox* ib = new (allocateInlineTextBox()) InlineTextBox(
                    f, TextRun(srcTxt, end, nextOffset, dir), isFirstLine);
                ib->setLayoutParent(m_currentLayoutParent);
                ib->setWidth(style->font()->measureText(
                    ib->asInlineTextBox()->textRun().m_stringView));
                ib->setHeight(f->style()->font()->metrics().m_fontHeight);
                tryInsertInlineBox(ib);
            }
        } else {
            CharDirection dir = CharDirection::Ltr;
            for (size_t i = 0; i < runs.size(); i++) {
                if (runs[i].m_stringView.start() <= offset &&
                    nextOffset <= runs[i].m_stringView.end()) {
                    dir = runs[i].m_direction;
                    break;
                }
            }
            InlineTextBox* ib = new (allocateInlineTextBox()) InlineTextBox(
                f, TextRun(srcTxt, offset, nextOffset, dir), isFirstLine);
            ib->setLayoutParent(m_currentLayoutParent);
            ib->setWidth(textWidth);
            ib->setHeight(f->style()->font()->metrics().m_fontHeight);
            tryInsertInlineBox(ib);
        }
    } else {
        CharDirection dir = CharDirection::Ltr;
        if (m_currentLanguageDirection.isRtlOnly()) {
            dir = CharDirection::Rtl;
        }
        InlineTextBox* ib = new (allocateInlineTextBox()) InlineTextBox(
            f, TextRun(srcTxt, offset, nextOffset, dir), isFirstLine);
        ib->setLayoutParent(m_currentLayoutParent);
        ib->setWidth(textWidth);
        ib->setHeight(f->style()->font()->metrics().m_fontHeight);
        tryInsertInlineBox(ib);
    }
}

void LineFormattingContext::handleAbsoluteBox(FrameBox* box, bool canInsert,
                                              bool canRegister)
{
    if (canInsert) {
        if (canRegister) {
            registerAbsolutePositionedBox(box);
        }

        if (!box->isBlockLevel(true)) {
            m_currentLayoutParent->insertInlineBox(box);
        }

        markAbsolutePositionedBoxLayoutParent(m_currentLayoutParent);
    } else {
        box->setLayoutParent(m_currentLayoutParent);
        STARFISH_ASSERT(m_canConcatWord);
        m_word.concat(box);
    }
}

void LineFormattingContext::handleTextToken(TextToken& token)
{
    if (m_isPendingBreakLine) {
        breakLine(nullptr);
    }

    if (token.m_type != WordType::General) {
        insertWord(token.m_frameText);
    }

    if (token.m_type == WordType::CollapsibleWhiteSpace &&
        isWhiteSpaceAtLast()) {
        return;
    }

    std::vector<TextToken> tokens;
    size_t cur = token.m_start;
    size_t end = token.m_end;
    while (cur < end) {
        if (String::isNBSP(token.m_frameText->text()->charAt(cur))) {
            TextToken t = TextToken(token.m_frameText, cur, cur + 1,
                                    token.m_type, token.m_isFirstLine);
            tokens.push_back(t);
            cur++;
        } else {
            size_t offset = cur + 1;
            while (offset < end &&
                   !String::isNBSP(token.m_frameText->text()->charAt(offset))) {
                offset++;
            }
            TextToken t = TextToken(token.m_frameText, cur, offset,
                                    token.m_type, token.m_isFirstLine);
            tokens.push_back(t);
            cur = offset;
        }
    }

    auto iter = tokens.begin();

    while (iter != tokens.end()) {
        TextToken& t = *iter;
        generateInlineTextBox(t);

        // Consider direction for hyphen
        char32_t c = t.m_frameText->text()->charAt(t.m_end - 1);
        bool isHyphenAtLast = isSoftHyphen(c) || isHyphen(c);

        if (isHyphenAtLast) {
            insertWord(t.m_frameText);
            m_isSoftHyphenAtLast = isSoftHyphen(c);
        }

        iter++;
    }
}

static void nextToken(std::vector<int32_t>::iterator& iter, int32_t& cur,
                      int32_t next)
{
    if (next >= *iter) {
        iter++;
    }
    cur = next;
}

template <typename T>
size_t utf16ToUtf32(const T* UTF16, const T* bufferEnd, char32_t& uc);

template <typename Context>
static void tokenizeText(StarFish* sf, FrameText* f, Context& ctx)
{
    // TODO : Consider direction
    String* txt = f->text();

    bool collapseSpace = !f->shouldPreserveWhiteSpaces();
    bool collapseNewline = f->shouldIgnoreNewlineChar();

    auto breaker = sf->lineBreakIteratorPool()->get(
        icu::Locale::getUS(), LineBreakIteratorModeUAX14, false);
    std::vector<int32_t> locs;

    icu::UnicodeString str = txt->toUnicodeString();
    breaker->setText(str);

    int32_t cur = 0;
    int32_t next = 0;

    StringBufferAccessData data = txt->bufferAccessData();
    if (data.bufferDataKind == StringBufferAccessData::ASCIIData) {
        while ((next = breaker->next()) != icu::BreakIterator::DONE) {
            locs.push_back(next);
        }
    } else {
        int32_t len = str.length();
        int32_t prev = 0;
        int32_t offset = 0;
        const UChar* buffer = str.getBuffer();
        // printf("diff : %d\n", (int)(txt->length() - len));

        while ((next = breaker->next()) != icu::BreakIterator::DONE) {
            if (next == len) {
                locs.push_back(txt->length());
            } else {
                while (prev < next) {
                    char32_t t;
                    size_t required =
                        utf16ToUtf32(buffer + prev, buffer + len, t);
                    offset -= (required - 1);
                    prev += required;
                }
                locs.push_back(next + offset);
                // printf("loc : %d\n", (int)locs.back());
                STARFISH_ASSERT(prev == next);
            }
        }
    }

    cur = 0;
    next = 0;

    auto iter = locs.begin();
    WordType type = WordType::CollapsibleWhiteSpace;

    while (iter != locs.end()) {
        next = *iter;

        if (!collapseNewline && String::isNewline(txt->charAt(cur))) {
            type = WordType::ForcedNewline;
        } else if (isSeparator(txt->charAt(cur))) {
            // Mostly white-spaces in text are collapsed.
            // But the text in <pre> or depending on CSS white-space property,
            // user agent should preserve white-spaces in text.

            int32_t offset = cur + 1;

            while (offset < next && isSeparator(txt->charAt(offset))) {
                if (!collapseNewline &&
                    String::isNewline(txt->charAt(offset))) {
                    break;
                }
                offset++;
            }

            next = offset;

            if (collapseSpace) {
                if (type == WordType::CollapsibleWhiteSpace && cur != 0) {
                    nextToken(iter, cur, next);
                    continue;
                }
                type = WordType::CollapsibleWhiteSpace;
            } else {
                type = WordType::NonCollapsibleWhiteSpace;
            }
        } else {
            int32_t offset = cur + 1;

            while (offset < next && !isSeparator(txt->charAt(offset))) {
                offset++;
            }

            next = offset;
            type = WordType::General;
        }

        TextToken token = TextToken(f, cur, next, type, ctx.isFirstLineBox());
        ctx.handleTextToken(token);
        nextToken(iter, cur, next);
    }
}

void* FrameTextRareData::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FrameTextRareData)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTextRareData, m_node));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTextRareData, m_text));
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FrameTextRareData));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

FrameText::FrameText(Node* node, ComputedStyle* style)
    : Frame(node, style)
{
    m_flags.m_isFrameText = true;
}

void FrameText::setText(String* text)
{
    STARFISH_ASSERT(text);
    if (hasRareData()) {
        frameTextRareData()->m_text = text;
    }
}

bool FrameText::isFrameInlineOrEmptyText(Frame* f)
{
    if (f->isFrameInline()) {
        return true;
    }

    if (!f->isFrameText()) {
        return false;
    }

    return f->asFrameText()->text()->isEmpty();
}

char32_t FrameText::previousChar()
{
    Frame* prevText = previousInPreOrder();
    for (; prevText; prevText = prevText->previousInPreOrder()) {
        if (!isFrameInlineOrEmptyText(prevText)) {
            break;
        }
    }

    char32_t prev = ' ';
    if (prevText && prevText->isFrameText()) {
        String* str = prevText->asFrameText()->text();
        size_t len = str->length();
        prev = str->charAt(len - 1);
    }

    return prev;
}

void FrameText::transformText(String* text)
{
    STARFISH_ASSERT(text);

    if (!hasRareData()) {
        m_node = (Node*)new FrameTextRareData(m_node);
    }

    if (style()->textTransform() == CapitalizeTextTransformValue) {
        setText(makeCapitalized(text, previousChar()));
    } else if (style()->textTransform() == UppercaseTextTransformValue) {
        setText(text->toUpper());
    } else if (style()->textTransform() == LowercaseTextTransformValue) {
        setText(text->toLower());
    }
}

String* FrameText::makeCapitalized(String* txt, char32_t prev)
{
    auto breaker = node()->starFish()->lineBreakIteratorPool()->get(
        icu::Locale::getUS(), LineBreakIteratorModeUAX14, false);
    std::vector<int32_t> locs;

    icu::UnicodeString str = txt->toUnicodeString();
    breaker->setText(str);

    int32_t cur = 0;
    int32_t next = 0;

    StringBufferAccessData data = txt->bufferAccessData();
    if (data.bufferDataKind == StringBufferAccessData::ASCIIData) {
        while ((next = breaker->next()) != icu::BreakIterator::DONE) {
            locs.push_back(next);
        }
    } else {
        int32_t len = str.length();
        int32_t prev = 0;
        int32_t offset = 0;
        const UChar* buffer = str.getBuffer();
        // printf("diff : %d\n", (int)(txt->length() - len));

        while ((next = breaker->next()) != icu::BreakIterator::DONE) {
            if (next == len) {
                locs.push_back(txt->length());
            } else {
                while (prev < next) {
                    char32_t t;
                    size_t required =
                        utf16ToUtf32(buffer + prev, buffer + len, t);
                    offset -= (required - 1);
                    prev += required;
                }
                locs.push_back(next + offset);
                // printf("loc : %d\n", (int)locs.back());
                STARFISH_ASSERT(prev == next);
            }
        }
    }

    cur = 0;
    next = 0;
    auto iter = locs.begin();

    StringBuilder sb;
    char32_t c = txt->charAt(cur);
    while (iter != locs.end()) {
        next = *iter;
        if (String::isNewline(c)) {
        } else if (isSeparator(c) || String::isNBSP(c) ||
                   String::isPunctuation(c)) {
            int32_t offset = cur + 1;

            c = txt->charAt(offset);
            while (offset < next && (isSeparator(c) || String::isNBSP(c) ||
                                     String::isPunctuation(c))) {
                if (String::isNewline(c)) {
                    break;
                }
                c = txt->charAt(++offset);
            }
            next = offset;
        } else {
            int32_t offset;
            if (isSeparator(prev) || String::isNBSP(prev) ||
                String::isPunctuation(prev)) {
                c = txt->charAt(cur);
                sb.appendChar((char32_t)u_totitle(c));
                offset = ++cur;
            } else {
                offset = cur;
                prev = ' ';
            }

            c = txt->charAt(offset);
            while (offset < next &&
                   !(isSeparator(c) || String::isNBSP(c) ||
                     String::isPunctuation(c))) {
                c = txt->charAt(++offset);
            }

            next = offset;
        }

        sb.appendSubString(txt, cur, next);
        nextToken(iter, cur, next);
        c = txt->charAt(cur);
    }

    return sb.finalize();
}

void FrameText::layoutInline(LineFormattingContext& ctx)
{
    // split the text into tokens using the ICU divider, and for each
    // token, execute the following function
    tokenizeText(ctx.m_layoutContext.starFish(), this, ctx);
}

void FrameReplaced::layoutInline(LineFormattingContext& ctx)
{
    if (ctx.m_isPendingBreakLine) {
        ctx.breakLine(nullptr);
    }

    ctx.insertWord(this);

    setLayoutParent(ctx.m_currentLayoutParent);
    layout(ctx.m_layoutContext, Frame::LayoutWantToResolve::ResolveAll);

    if (isFloating()) {
        ctx.tryInsertFloatingBox(this);
    } else {
        ctx.tryInsertInlineBox(this);
    }
}

void FrameBlockBox::layoutInline(LineFormattingContext& ctx)
{
    if (ctx.m_isPendingBreakLine) {
        ctx.breakLine(nullptr);
    }

    ctx.insertWord(this);

    if (isFloating()) {
        setLayoutParent(ctx.m_currentLayoutParent);
        layout(ctx.m_layoutContext, Frame::LayoutWantToResolve::ResolveAll);

        ctx.tryInsertFloatingBox(this);
    } else {
        DisplayValue display = style()->display();
        STARFISH_ASSERT(isInlineLevel());
        // inline-block, inline-table, inline-flex
        ctx.m_layoutContext.pushInlineBlockBox(this);
        setLayoutParent(ctx.m_currentLayoutParent);
        layout(ctx.m_layoutContext, Frame::LayoutWantToResolve::ResolveAll);
        LayoutUnit ascender;

        if (display == InlineTableDisplayValue) {
            ascender = asFrameTableBox()->calBaseline(ctx.m_layoutContext);
        } else {
            Nullable<LayoutUnit> p = ctx.m_layoutContext.lineBoxAscender(this);
            if (p.hasValue() && appliedOverflowX() == VisibleOverflow) {
                ascender = p.getValue();
            } else {
                ascender = height();
            }
        }

        ctx.m_layoutContext.popInlineBlockBox();
        ctx.registerInlineBlockAscender(ascender, this);
        ctx.tryInsertInlineBox(this);
    }
}

void FrameLineBreak::layoutInline(LineFormattingContext& ctx)
{
    ctx.insertWord(this);

    if (ctx.m_isPendingBreakLine) {
        FloatingBoxLayoutContext& fbCtx =
            *ctx.m_floatingBoxLayoutContexts.begin();
        if (dontClear(fbCtx.m_hasFloat, this)) {
            ctx.breakLine(this);
        }
    } else {
        ctx.breakLine(this);
    }
}

void FrameInline::layoutInline(LineFormattingContext& ctx)
{
    // There are different policies between browsers. In chrome, soft hyphen
    // isn't visible when FrameInline comes next, not in fire-fox, though.
    // Here we follow the policy of chrome.
    ctx.handleSoftHyphenate(false);
    if (style()->wordWrap() == BreakWordWordWrapValue) {
        ctx.insertWord(this);
    }

    InlineNonReplacedBox* inlineBox = new (ctx.allocateInlineNonReplacedBox())
        InlineNonReplacedBox(&ctx, this, ctx.isFirstLineBox());

    if (ctx.isWordProcessing()) {
        inlineBox->setLayoutParent(ctx.m_currentLayoutParent);
        ctx.m_word.concat(inlineBox);
    } else {
        ctx.m_currentLayoutParent->insertInlineBox(inlineBox);
    }

    if (ctx.m_currentLayoutParent->isLineBox()) {
        if (!ctx.isWordProcessing()) {
            ctx.markInlineBoxIndex(inlineBox);
        }
        inlineBox->layoutInline(ctx);
        STARFISH_ASSERT(ctx.isWordProcessing() ||
                        ctx.m_unprocessedStartingMBPWidth == 0);
    } else {
        inlineBox->layoutInline(ctx);
    }

    ctx.updateCurrentLayoutParent(ctx.m_currentLayoutParent->layoutParent());
}

void LineFormattingContext::layoutInline(Frame* origin)
{
    Frame* f = origin->firstChild();
    while (f) {
        // Don't put any inline box leaving pending inline boxes ahead.
        STARFISH_ASSERT(m_pendingInlineBoxes.size() == 0);

        if (f->isAbsolutePositioned() && f->isFrameBox()) {
            if (m_isPendingBreakLine) {
                breakLine(nullptr);
            }

            handleAbsoluteBox(f->asFrameBox(), m_word.isEmpty(), true);
        } else {
            f->layoutInline(*this);
        }

        f = f->next();
    }
}

void LineFormattingContext::collectComputeDirectionsCandidate(
    Frame* parent, std::vector<Frame*>& frames)
{
    for (Frame* f = parent->firstChild(); f; f = f->next()) {
        if (!f->isNormalFlow()) {
            continue;
        }
        if (f->isFrameInline()) {
            if (hasIsolateBidiContent(f->style())) {
                frames.push_back(f);
                computeDirection(f, f->style()->direction());
            } else {
                collectComputeDirectionsCandidate(f, frames);
            }
        } else {
            frames.push_back(f);
        }
    }
}

static void breakTextRun(std::vector<TextRun>& runs, size_t breakRunIndex,
                         size_t breakTextIndex)
{
    TextRun target = runs[breakRunIndex];
    runs.erase(runs.begin() + breakRunIndex);

    size_t start = target.m_stringView.start();
    size_t end = breakTextIndex + 1;
    runs.insert(runs.begin() + breakRunIndex++,
                TextRun(target.m_stringView.originalString(), start, end,
                        charDirFromICUDir(
                            getTextDir(target.m_stringView, start, end))));

    start = breakTextIndex + 1;
    end = target.m_stringView.end();
    runs.insert(runs.begin() + breakRunIndex,
                TextRun(target.m_stringView.originalString(), start, end,
                        charDirFromICUDir(
                            getTextDir(target.m_stringView, start, end))));
}

static void textBidiResolver(FrameText* frameText,
                             DirectionValue directionValue,
                             std::vector<TextRun>& runs)
{
    UBiDi* bidi = ubidi_open();
    UTF16StringDataNonGCStd str = frameText->text()->toUTF16NonGCString();
    UErrorCode err = (UErrorCode)0;
    ubidi_setPara(bidi, (const UChar*)str.data(), str.length(),
                  directionValue == DirectionValue::LtrDirectionValue
                      ? UBIDI_DEFAULT_LTR
                      : UBIDI_DEFAULT_RTL,
                  NULL, &err);
    STARFISH_ASSERT(U_SUCCESS(err));
    size_t total = ubidi_countRuns(bidi, &err);
    STARFISH_ASSERT(U_SUCCESS(err));
    if (total == 1) {
        UBiDiDirection dir = getTextDir(
            StringView(frameText->text(), 0, frameText->text()->length()), 0,
            frameText->text()->length());
        runs.emplace_back(frameText->text(), 0, frameText->text()->length(),
                          charDirFromICUDir(dir));
    } else {
        int32_t start = 0;
        int32_t end;
        size_t utf32Pos = 0;
        for (size_t i = 0; i < total; i++) {
            ubidi_getLogicalRun(bidi, start, &end, NULL);
            UBiDiDirection dir = ubidi_getBaseDirection(
                (const UChar*)str.data() + start, end - start);
            size_t utf32Len = 0;

            /* U16_NEXT post-increments */
            for (size_t i = start; i < (size_t)end;) {
                char32_t c;
                U16_NEXT((const UChar*)str.data(), i, (size_t)end, c);
                utf32Len++;
            }

            runs.emplace_back(frameText->text(), utf32Pos, utf32Pos + utf32Len,
                              charDirFromICUDir(dir));
            utf32Pos += utf32Len;

            start = end;
        }
    }
    ubidi_close(bidi);

    for (size_t i = 0; i < runs.size(); i++) {
        if (isNumber(runs[i])) {
            runs[i].m_direction = CharDirection::Neutral;
            continue;
        }

        // check it has leading neutral chars
        char32_t first = runs[i].m_stringView.originalString()->charAt(
            runs[i].m_stringView.start());
        if ((runs[i].m_stringView.end() - runs[i].m_stringView.start()) > 1 &&
            charDirection(first) == 2 && !isNumberChar(first)) {
            breakTextRun(runs, i, runs[i].m_stringView.start());
            i--;
            continue;
        }

        // check it has trailing neutral chars
        if (runs[i].m_stringView.end() - runs[i].m_stringView.start() > 1) {
            size_t lastPos = runs[i].m_stringView.end() - 1;
            char32_t last =
                runs[i].m_stringView.originalString()->charAt(lastPos);
            if (charDirection(last) == 2 && !isNumberChar(last)) {
                breakTextRun(runs, i, lastPos - 1);
                i--;
                continue;
            }
        }
    }
}

void LineFormattingContext::computeDirection(Frame* parent,
                                             DirectionValue direction)
{
    std::vector<Frame*> frames;
    collectComputeDirectionsCandidate(parent, frames);
    LanguageDirection ld;
    ld.markDirection(direction);

    DirectionValue currentDirection = direction;
    bool everMetNonNeutralThing = false;
    std::vector<Frame*> pendingNeutralFrames;
    std::vector<std::pair<TextRun, FrameText*>> pendingTextRuns;
    DirectionValue directionWhenNeutralMeets = direction;

    auto putOffNeutral = [&](Frame* f) {
        if (pendingNeutralFrames.size() == 0) {
            directionWhenNeutralMeets = currentDirection;
        }
        pendingNeutralFrames.push_back(f);
    };

    auto flushNeutral = [&](DirectionValue dir) {
        DirectionValue result;
        if (direction == LtrDirectionValue) {
            if (directionWhenNeutralMeets == RtlDirectionValue &&
                dir == RtlDirectionValue) {
                result = RtlDirectionValue;
            } else {
                result = LtrDirectionValue;
            }
        } else {
            if (directionWhenNeutralMeets == LtrDirectionValue &&
                dir == LtrDirectionValue) {
                result = LtrDirectionValue;
            } else {
                result = RtlDirectionValue;
            }
        }

        for (size_t i = 0; i < pendingNeutralFrames.size(); i++) {
            m_computedDirectionValuePerFrame[pendingNeutralFrames[i]] = result;
        }

        CharDirection ch =
            (result == DirectionValue::LtrDirectionValue ? CharDirection::Ltr
                                                         : CharDirection::Rtl);
        for (size_t i = 0; i < pendingTextRuns.size(); i++) {
            TextRun run = pendingTextRuns[i].first;
            FrameText* origin = pendingTextRuns[i].second;
            TextRun newRun(run.m_stringView.originalString(),
                           run.m_stringView.start(), run.m_stringView.end(),
                           ch);
            m_textRunsPerFrameText[origin].emplace_back(newRun);
        }

        pendingNeutralFrames.clear();
        pendingTextRuns.clear();
        currentDirection = dir;
    };

    for (size_t i = 0; i < frames.size(); i++) {
        Frame* f = frames[i];
        if (f->isFrameText()) {
            String* txt = f->asFrameText()->text();
            std::vector<TextRun> runs;
            textBidiResolver(f->asFrameText(), direction, runs);

            for (size_t i = 0; i < runs.size(); i++) {
                TextRun run = runs[i];
                if (run.m_direction == CharDirection::Neutral) {
                    if (isNumber(run)) {
                        continue;
                    }
                    // When shouldIgnoreNewlineChar() is true, ignore
                    // newline characters, otherwise flush pending neutrals
                    //
                    // NOTE: textBidiResolver() guarantees that
                    // single TextRun with newline-char has length of 1.
                    if (!f->asFrameText()->shouldIgnoreNewlineChar() &&
                        startsWithNewlineChar(run.m_stringView)) {
                        STARFISH_ASSERT(run.m_stringView.length() == 1);
                        everMetNonNeutralThing = true;
                        flushNeutral(direction);
                        continue;
                    }
                    if (pendingTextRuns.size() == 0) {
                        directionWhenNeutralMeets = currentDirection;
                    }
                    pendingTextRuns.push_back(
                        std::make_pair(run, f->asFrameText()));
                } else {
                    STARFISH_ASSERT(run.m_direction == CharDirection::Ltr ||
                                    run.m_direction == CharDirection::Rtl);
                    everMetNonNeutralThing = true;
                    m_textRunsPerFrameText[f->asFrameText()].emplace_back(
                        run.m_stringView.originalString(),
                        run.m_stringView.start(), run.m_stringView.end(),
                        run.m_direction);

                    ld.markDirection(run.m_direction);

                    flushNeutral(run.m_direction == CharDirection::Ltr
                                     ? DirectionValue::LtrDirectionValue
                                     : DirectionValue::RtlDirectionValue);
                }
            }

        } else if (f->isFrameBox()) {
            if (everMetNonNeutralThing) {
                putOffNeutral(f);
            } else {
                m_computedDirectionValuePerFrame[f] = direction;
            }
        } else if (f->isFrameInline()) {
            if (f->style()->unicodeBidi() ==
                UnicodeBidiValue::EmbedUnicodeBidiValue) {
                everMetNonNeutralThing = true;
                flushNeutral(f->style()->direction());
            } else if (f->style()->unicodeBidi() ==
                       UnicodeBidiValue::IsolateUnicodeBidiValue) {
                if (everMetNonNeutralThing) {
                    putOffNeutral(f);
                } else {
                    m_computedDirectionValuePerFrame[f] = direction;
                }
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        } else if (f->isFrameLineBreak()) {
            everMetNonNeutralThing = true;
            flushNeutral(direction);
        }
    }

    flushNeutral(direction);
    m_languageDirections.emplace(parent, ld);
}

void* InlineNonReplacedBox::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(InlineNonReplacedBox)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(InlineNonReplacedBox, m_node));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(InlineNonReplacedBox, m_layoutParent));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(InlineNonReplacedBox, m_boxes));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(InlineNonReplacedBox, m_origin));
        descr =
            GC_make_descriptor(obj_bitmap, GC_WORD_LEN(InlineNonReplacedBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

InlineNonReplacedBox::InlineNonReplacedBox(LineFormattingContext* ctx,
                                           InlineNonReplacedBox* inlineBox,
                                           bool isFirstLine)
    : InlineNonReplacedBox(inlineBox, inlineBox->origin(), isFirstLine)
{
    ctx->m_inlineNonReplacedBoxMBPStatus[this] =
        ctx->m_inlineNonReplacedBoxMBPStatus[inlineBox];
    ctx->setAscDescender(this, ctx->ascender(inlineBox),
                         ctx->descender(inlineBox));

    if (inlineBox->hasRareData()) {
        ensureFrameBoxRareData()->m_margin =
            inlineBox->frameBoxRareData()->m_margin;
        ensureFrameBoxRareData()->m_border =
            inlineBox->frameBoxRareData()->m_border;
        ensureFrameBoxRareData()->m_padding =
            inlineBox->frameBoxRareData()->m_padding;
    }

    if (inlineBox->rareData() &&
        (inlineBox->rareData()->m_orgBorder.hasNonZeroEdge() ||
         inlineBox->rareData()->m_orgPadding.hasNonZeroEdge() ||
         inlineBox->rareData()->m_orgMargin.hasNonZeroEdge())) {
        m_rareData = new InlineNonReplacedBoxRareData(m_origin);
        m_rareData->m_orgMargin = inlineBox->m_rareData->m_orgMargin;
        m_rareData->m_orgBorder = inlineBox->m_rareData->m_orgBorder;
        m_rareData->m_orgPadding = inlineBox->m_rareData->m_orgPadding;
    }
}

InlineNonReplacedBox::InlineNonReplacedBox(LineFormattingContext* ctx,
                                           FrameInline* frame, bool isFirstLine)
    : InlineNonReplacedBox(frame, frame, isFirstLine)
{
    ctx->m_inlineNonReplacedBoxMBPStatus[this] =
        adoptRef(new InlineNonReplacedBoxMBPStatusHolder());
}

void InlineNonReplacedBox::setOrgLeftMBP(
    LineFormattingContext* lineFormattingContext)
{
    if (rareData() || marginLeft() || borderLeft() || paddingLeft()) {
        if (!rareData()) {
            m_rareData = new InlineNonReplacedBoxRareData(m_origin);
        }
        m_rareData->m_orgMargin.setLeft(marginLeft());
        m_rareData->m_orgBorder.setLeft(borderLeft());
        m_rareData->m_orgPadding.setLeft(paddingLeft());
    }

    LayoutUnit w;
    moveX(marginLeft());
    LayoutUnit bp = borderLeft() + paddingLeft();
    w += marginLeft() + bp;
    setWidth(width() + bp);

    if (w > 0) {
        FrameBox* parent = layoutParent()->asFrameBox();
        while (parent->isInlineNonReplacedBox()) {
            parent->setWidth(parent->width() + w);
            parent = parent->layoutParent()->asFrameBox();
        }
    }

    lineFormattingContext->markSetLeftMBP(this);
}

void InlineNonReplacedBox::setOrgRightMBP(
    LineFormattingContext* lineFormattingContext)
{
    if (rareData() || marginRight() || borderRight() || paddingRight()) {
        if (!rareData()) {
            m_rareData = new InlineNonReplacedBoxRareData(m_origin);
        }
        m_rareData->m_orgMargin.setRight(marginRight());
        m_rareData->m_orgBorder.setRight(borderRight());
        m_rareData->m_orgPadding.setRight(paddingRight());
    }

    LayoutUnit w = marginRight();
    LayoutUnit bp = borderRight() + paddingRight();
    w += bp;
    setWidth(width() + bp);

    if (w > 0) {
        FrameBox* parent = layoutParent()->asFrameBox();
        while (parent->isInlineNonReplacedBox()) {
            parent->setWidth(parent->width() + w);
            parent = parent->layoutParent()->asFrameBox();
        }
    }

    lineFormattingContext->markSetRightMBP(this);
}

void InlineNonReplacedBox::processStartingMBP(
    LineFormattingContext* lineFormattingContext)
{
    InlineNonReplacedBox* current = this;
    while (current) {
        if (!lineFormattingContext->isProcessedStartingMBP(current)) {
            LayoutUnit unprocessedStartingMBPWidth =
                current->startingMBPWidth();
            lineFormattingContext->m_currentLineWidth +=
                unprocessedStartingMBPWidth;
            lineFormattingContext->m_unprocessedStartingMBPWidth -=
                unprocessedStartingMBPWidth;
            lineFormattingContext->markProcessedStartingMBP(current);
        } else {
            break;
        }

        if (current->layoutParent()->isLineBox()) {
            break;
        }

        current = current->layoutParent()->asInlineNonReplacedBox();
    }
}

void InlineNonReplacedBox::processEndingMBP(
    LineFormattingContext* lineFormattingContext)
{
    LayoutUnit unprocessedEndingMBP = endingMBPWidth();
    lineFormattingContext->m_currentLineWidth += unprocessedEndingMBP;
    lineFormattingContext->markProcessedEndingMBP(this);
}

void LineFormattingContext::finishLineForInlineNonReplacedBox(
    FrameLineBreak* br, bool isLastNode)
{
    InlineNonReplacedBox* self =
        m_currentLayoutParent->asInlineNonReplacedBox();

    if (isLastNode) {
        self->processStartingMBP(this);
        self->processEndingMBP(this);
        handleSoftHyphenate(false);
    }

    InlineNonReplacedBox* current = self;
    InlineBoxLayoutParentBox* parent =
        current->layoutParent()->asInlineBoxLayoutParentBox();
    while (current) {
        parent->setWidth(parent->width() + current->width());

        computeVerticalProperties(current, br);

        if (parent->isLineBox()) {
            break;
        }

        if (isLastNode) {
            break;
        }

        current = parent->asInlineNonReplacedBox();
        parent = current->layoutParent()->asInlineBoxLayoutParentBox();
    }

    if (hasIsolateBidiContent(self->style())) {
        resolveBidi(self->style()->direction(), self->boxes());
    }
}

void LineFormattingContext::breakLineForInlineNonReplacedBox(FrameLineBreak* br)
{
    InlineNonReplacedBox* self =
        m_currentLayoutParent->asInlineNonReplacedBox();

    if (self->boxes().size() == 0 && self->layoutParent()->isLineBox()) {
        auto& boxes = currentLine()->boxes();
        boxes.erase(std::find(boxes.begin(), boxes.end(), self));

        updateCurrentLayoutParent(currentLine());
        breakLineForLineBox(br, false, false);
        updateCurrentLayoutParent(self);

        currentLine()->insertInlineBox(self);
        return;
    }

    finishLineForInlineNonReplacedBox(br, false);

    InlineNonReplacedBox* newSelf = new (allocateInlineNonReplacedBox())
        InlineNonReplacedBox(this, self, false);
    FrameBox* parent = self->layoutParent()->asFrameBox();
    InlineNonReplacedBox* current = newSelf;
    while (parent->isInlineNonReplacedBox()) {
        InlineNonReplacedBox* newInrb = new (allocateInlineNonReplacedBox())
            InlineNonReplacedBox(this, parent->asInlineNonReplacedBox(), false);
        newInrb->insertInlineBox(current);
        current = newInrb;
        parent = parent->layoutParent()->asFrameBox();
    }

    updateCurrentLayoutParent(currentLine());
    breakLineForLineBox(br, false, false);
    updateCurrentLayoutParent(newSelf);

    currentLine()->insertInlineBox(current);
    markInlineBoxIndex(current);
}

bool LineFormattingContext::removeLastLineBoxIfNeeds()
{
    LineBox* back = currentLine();

    back->removeDanglingSpace(this);
    if (back->containOnlyEmptyInlineNonReplacedBoxes(this)) {
        if (m_pendingFloatingBoxes.size() == 0 &&
            m_pendingInlineBoxes.size() == 0 &&
            absolutePositionedBoxLayoutParentCnt(back) == 0) {
            m_block->m_lineBoxes.erase(m_block->m_lineBoxes.end() - 1);
            return true;
        } else {
            back->boxes().clear();
        }
    }

    return false;
}

LayoutUnit LineFormattingContext::contentHeightForBlock()
{
    LayoutUnit top = m_block->paddingTop() + m_block->borderTop();
    LayoutUnit bottom;
    LayoutUnit height;

    auto riter = m_block->m_lineBoxes.rbegin();
    while (riter != m_block->m_lineBoxes.rend()) {
        LineBox* lineBox = *riter;
        STARFISH_ASSERT(lineBox != nullptr);

        if (lineBox->height() != 0) {
            bottom = lineBox->y() + lineBox->height();
            break;
        }

        riter++;
    }

    if (m_block->isEstablishesBlockFormattingContext()) {
        bottom = std::max(bottom, m_layoutContext.clearedDistanceToFloatBottom(
                                      m_absPosition.y(), BothClearValue));
    }

    if (bottom > 0) {
        return bottom - top;
    }

    return LayoutUnit(0);
}

LayoutUnit FrameBlockBox::layoutInline(LayoutContext& ctx)
{
    if (!isNecessaryBlockBox()) {
        return LayoutUnit(0);
    }

    LineFormattingContext lineFormattingContext(this, ctx, false);

    // compute directions
    lineFormattingContext.computeDirection(this, style()->direction());

    lineFormattingContext.updateCurrentLayoutParent(
        lineFormattingContext.currentLine());

    lineFormattingContext.layoutInline(this);

    if (lineFormattingContext.isWordProcessing()) {
        lineFormattingContext.insertWord(nullptr);
    }
    bool skipFinishLine = lineFormattingContext.removeLastLineBoxIfNeeds();
    if (!skipFinishLine) {
        lineFormattingContext.finishLineForLineBox(nullptr, true);
    }

    STARFISH_ASSERT(lineFormattingContext.m_pendingFloatingBoxes.size() == 0 &&
                    lineFormattingContext.m_pendingInlineBoxes.size() == 0 &&
                    lineFormattingContext.m_absolutePositionedBoxes.size() ==
                        0 &&
                    lineFormattingContext.m_word.isEmpty());

    registerRelativePositionedBoxesAndMarkPaintFlag(ctx);

    return lineFormattingContext.contentHeightForBlock();
}

void FrameBlockBox::registerRelativePositionedBoxesAndMarkPaintFlag(
    LayoutContext& ctx)
{
    auto iter = m_lineBoxes.begin();

    while (iter != m_lineBoxes.end()) {
        LineBox* lineBox = *iter;
        STARFISH_ASSERT(lineBox != nullptr);

        if (lineBox->boxes().size() == 0) {
            if (lineBox->height() == 0) {
                iter = m_lineBoxes.erase(iter);
                continue;
            }
        }

        lineBox->markSeenNormalFlowInline();
        lineBox->registerRelativePositionedBoxesAndMarkPaintFlag(ctx);
        iter++;
    }
}

void InlineNonReplacedBox::layoutInline(LineFormattingContext& ctx)
{
    LayoutUnit inlineContentWidth = ctx.m_block->contentWidth();

    computeBorderMarginPadding(ctx.m_layoutContext, inlineContentWidth);
    setTopBottomOrgMBP();
    ctx.m_unprocessedStartingMBPWidth += startingMBPWidth();

    ctx.updateCurrentLayoutParent(this);
    ctx.layoutInline(origin());

    if (!ctx.isWordProcessing()) {
        ctx.finishLineForInlineNonReplacedBox(nullptr, true);
    }
}

void PreferredWidthContext::computePreferredWidth()
{
    PreferredWidthKey key(m_remainingWidth, m_frame);
    auto it = m_layoutContext.preferredWidthInfo(key);
    if (it.hasValue()) {
        updatePreferredWidth(it.getValue().m_preferredWidth);
        updatePreferredMinWidth(it.getValue().m_preferredMinWidth);
        return;
    }

    m_frame->computePreferredWidth(*this);

    PreferredWidthValue value(preferredWidth(), preferredMinWidth());
    m_layoutContext.registerPreferredWidthInfo(key, value);
}

void PreferredWidthContext::handleTextToken(TextToken& token)
{
    if (m_isPendingWrapLine) {
        breakLine(true, false);
    }

    if (token.m_type != WordType::General) {
        updateCurrentLineWidthByWordWidth();
    }

    if (token.m_type == WordType::CollapsibleWhiteSpace &&
        isWhiteSpaceAtLast()) {
        return;
    }

    LayoutUnit w = token.width() + wordSpacing(token);
    if (token.m_type != WordType::CollapsibleWhiteSpace) {
        if (token.m_type == WordType::General) {
            w = widthAppliedByTextIndent(w);
            updatePreferredMinWidth(w);
        }
        w += m_unprocessedStartingMBPWidth;
    }

    setIsWhiteSpaceAtLast(token.m_type == WordType::CollapsibleWhiteSpace, w);
    updateCurrentLineWidth(token.m_frameText, w, token.m_type);
}

void PreferredWidthContext::updateUnprocessedStartingMBPWidth(Frame* f)
{
    m_unprocessedStartingMBPWidth += startingMBPWidth(f);
}

void PreferredWidthContext::updateCurrentLineWidth(Frame* f, LayoutUnit w,
                                                   WordType type)
{
    if (f->isFloating()) {
        if (canInsertFloatingBox(f)) {
            if (dontBreakLine(w)) {
                m_currentLineWidth += w;
            } else {
                breakLine(true, false);
                m_currentLineWidth = w;
            }
        } else {
            breakLine(false, false);
            m_hasFloat = HasNone;
            m_currentLineWidth = w;
        }
    } else if (type == WordType::ForcedNewline) {
        // In compute preferred width, even if trying break line here, it is
        // okay. Because we care only the longest width not the exact layout
        // result.
        breakLine(false, false);
    } else if (f->shouldWrapLines() && !f->isDirectDescendantOfTableCellBox()) {
        if ((!m_hasAppliedTextIndent || hasFloatingBoxAlreadyInLineBox()) &&
            f->isFrameText() && type == WordType::General) {
            m_wordWidth += w;
        } else if (dontBreakLine(w)) {
            m_currentLineWidth += w;
        } else {
            if (type == WordType::CollapsibleWhiteSpace) {
                m_currentLineWidth += w;
                m_isPendingWrapLine = true;
            } else {
                breakLine(true, false);
                if (type == WordType::General) {
                    m_currentLineWidth = w;
                }
            }
        }
    } else {
        m_currentLineWidth += w;
    }

    if (type == WordType::General && !f->isFloating()) {
        m_unprocessedStartingMBPWidth = 0;
    }
}

void PreferredWidthContext::handleFloatingBox(Frame* f, LayoutUnit w)
{
    updateCurrentLineWidth(f, w);

    if (f->style()->floating() == LeftFloatValue) {
        m_hasFloat |= HasLeft;
    } else {
        m_hasFloat |= HasRight;
    }
}

LayoutUnit PreferredWidthContext::leftMBPWidth(Frame* f)
{
    LayoutUnit width;
    LayoutUnit unused;
    BorderData border = f->style()->border();
    Length borderLeftWidth = border.left().width();
    Length paddingLeft = f->style()->padding().left();
    Length marginLeft = f->style()->margin().left();

    if (borderLeftWidth.isDefinite(false)) {
        width += borderLeftWidth.specifiedValue(unused, f);
    }

    if (paddingLeft.isDefinite(false)) {
        width += paddingLeft.specifiedValue(unused, f);
    }

    if (marginLeft.isDefinite(false)) {
        width += marginLeft.specifiedValue(unused, f);
    }

    return width;
}

LayoutUnit PreferredWidthContext::rightMBPWidth(Frame* f)
{
    LayoutUnit width;
    LayoutUnit unused;
    BorderData border = f->style()->border();
    LengthData padding = f->style()->padding();
    LengthData margin = f->style()->margin();
    Length borderRightWidth = border.right().width();
    Length paddingRight = padding.right();
    Length marginRight = margin.right();

    if (borderRightWidth.isDefinite(false)) {
        width += borderRightWidth.specifiedValue(unused, f);
    }

    if (paddingRight.isDefinite(false)) {
        width += paddingRight.specifiedValue(unused, f);
    }

    if (marginRight.isDefinite(false)) {
        width += marginRight.specifiedValue(unused, f);
    }

    return width;
}

LayoutUnit PreferredWidthContext::startingMBPWidth(Frame* f)
{
    LayoutUnit w;
    if (f->style()->direction() == LtrDirectionValue) {
        w = leftMBPWidth(f);
    } else {
        w = rightMBPWidth(f);
    }
    return w;
}

LayoutUnit PreferredWidthContext::endingMBPWidth(Frame* f)
{
    LayoutUnit w;
    if (f->style()->direction() == LtrDirectionValue) {
        w = rightMBPWidth(f);
    } else {
        w = leftMBPWidth(f);
    }
    return w;
}

LayoutUnit PreferredWidthContext::mbpWidth(Frame* f)
{
    return leftMBPWidth(f) + rightMBPWidth(f);
}

std::pair<LayoutUnit, LayoutUnit>
PreferredWidthContext::preferredWidthsWithNewContext(Frame* f)
{
    LayoutUnit mbpWidth = this->mbpWidth(f);
    PreferredWidthContext newCtx(m_layoutContext, mainContext(), f, m_owner,
                                 m_remainingWidth - mbpWidth);
    newCtx.computePreferredWidth();

    if (f->style()->clear() & ClearValue::LeftClearValue) {
        mainContext().m_floatLeftWidth = 0;
    }
    if (f->style()->clear() & ClearValue::RightClearValue) {
        mainContext().m_floatRightWidth = 0;
    }

    if (f->style()->floating() & FloatValue::LeftFloatValue) {
        mainContext().m_floatLeftWidth += (newCtx.preferredWidth() + mbpWidth);
    } else if (f->style()->floating() & FloatValue::RightFloatValue) {
        mainContext().m_floatRightWidth += (newCtx.preferredWidth() + mbpWidth);
    }

    return std::make_pair(newCtx.preferredWidth() + mbpWidth,
                          newCtx.preferredMinWidth() + mbpWidth);
}

void PreferredWidthContext::computePreferredWidthInline(Frame* parent)
{
    Frame* f = parent->firstChild();
    while (f) {
        if (f->isAbsolutePositioned()) {
            f = f->next();
            continue;
        }

        if (f->isFrameBlockBox()) {
            if (isPendingWrapLine()) {
                breakLine(true, false);
            }

            updateCurrentLineWidthByWordWidth();

            auto widths = preferredWidthsWithNewContext(f);
            LayoutUnit pMinWidth = widths.second;
            LayoutUnit pWidth = widths.first;

            if (f->isFloating()) {
                updatePreferredMinWidth(pMinWidth);
                handleFloatingBox(f, pWidth);
            } else {
                LayoutUnit oldTextIndent = m_textIndentWidth;
                pMinWidth = widthAppliedByTextIndent(pMinWidth);
                updatePreferredMinWidth(pMinWidth);
                m_textIndentWidth = oldTextIndent;
                pWidth = widthAppliedByTextIndent(pWidth);
                setIsWhiteSpaceAtLast(false, 0);
                updateCurrentLineWidth(f,
                                       pWidth + m_unprocessedStartingMBPWidth);
            }
        } else {
            f->computePreferredWidth(*this);
        }

        f = f->next();
    }
}

bool FrameText::isSelfCollapsingBlock(LayoutContext& ctx)
{
    return text()->containsOnlyWhitespace();
}

void FrameText::computePreferredWidth(PreferredWidthContext& ctx)
{
    tokenizeText(ctx.layoutContext().starFish(), this, ctx);
}

void FrameInline::computePreferredWidth(PreferredWidthContext& ctx)
{
    ctx.updateUnprocessedStartingMBPWidth(this);

    ctx.computePreferredWidthInline(this);

    if (ctx.unprocessedStartingMBPWidth() > 0) {
        ctx.updateCurrentLineWidth(this, ctx.unprocessedStartingMBPWidth());
    }

    ctx.setCurrentLineWidth(ctx.currentLineWidth() + ctx.endingMBPWidth(this));
}

void FrameReplaced::computePreferredWidth(PreferredWidthContext& ctx)
{
    if (ctx.isPendingWrapLine()) {
        ctx.breakLine(true, false);
    }

    ctx.updateCurrentLineWidthByWordWidth();

    Length width = style()->width();
    Length height = style()->height();
    BoxSizingValue boxSizing = style()->boxSizing();
    LayoutUnit intrinsicWidth, intrinsicHeight, w, h;
    FrameBox* cb = containingBlock(this);
    MBPRestorer restorer(this);
    computeBorderMarginPadding(ctx.layoutContext(), cb->contentWidth());
    LayoutUnit parentContentWidth, parentContentHeight;
    Length parentHeightLength;
    bool parentHasFixedHeight;
    bool hasAspectRatio;

    parentContentWidth = intMaxForLayoutUnit;
    parentHasFixedHeight = ctx.layoutContext().parentHasFixedHeight(this);

    if (parentHasFixedHeight) {
        parentContentHeight = ctx.layoutContext().parentFixedHeight(this);
        parentHeightLength = Length(Length::Fixed, parentContentHeight);
    } else {
        parentHeightLength = Length(Length::Auto);
    }

    computeIntrinsicSize(ctx.layoutContext(), intrinsicWidth, intrinsicHeight,
                         hasAspectRatio, parentContentWidth,
                         parentHeightLength);

    if (width.isDefinite(false)) {
        LayoutUnit unused;
        w = width.specifiedValue(unused, this);
        w = minMaxWidthAppliedIfNeeds(ctx.layoutContext(), w, unused, true);
    } else {
        w = intrinsicWidth;
        h = intrinsicHeight;

        if (height.isDefinite(parentHasFixedHeight)) {
            h = height.specifiedValue(parentContentHeight, this);
            h = contentHeightApplyingBoxSizing(h);

            if (hasAspectRatio) {
                w = h * (intrinsicWidth / intrinsicHeight);
            }
        }

        auto widthAndHeight = minMaxWidthAndHeightAppliedIfNeeds(
            ctx.layoutContext(), w, h, parentContentWidth, parentContentHeight,
            hasAspectRatio, parentHasFixedHeight);

        w = widthAndHeight.first;
    }

    if (boxSizing == BorderBoxBoxSizingValue) {
        w += marginWidth();
    } else {
        w += mbpWidth();
    }

    if (isFloating()) {
        ctx.updatePreferredMinWidth(w);
        ctx.handleFloatingBox(this, w);
    } else {
        w = ctx.widthAppliedByTextIndent(w);
        ctx.updatePreferredMinWidth(w);
        ctx.setIsWhiteSpaceAtLast(false, 0);
        ctx.updateCurrentLineWidth(this, w + ctx.unprocessedStartingMBPWidth());
    }
}

void FrameLineBreak::computePreferredWidth(PreferredWidthContext& ctx)
{
    ctx.updateCurrentLineWidthByWordWidth();

    if (ctx.isPendingWrapLine()) {
        if (dontClear(ctx.hasFloat(), this)) {
            ctx.breakLine(true, true);
        }
    } else {
        ctx.breakLine(false, true);
    }
}

void FrameBlockBox::computePreferredWidth(PreferredWidthContext& ctx)
{
    if (!isNecessaryBlockBox()) {
        return;
    }

    FrameBox* cb = containingBlock(this);
    MBPRestorer restorer(this);
    computeBorderMarginPadding(ctx.layoutContext(), cb->contentWidth());
    Length width = style()->width();

    if (width.isDefinite(false) && this != ctx.owner()) {
        LayoutUnit unused;
        LayoutUnit w = width.specifiedValue(unused, this);
        w = contentWidthApplyingBoxSizing(w);
        ctx.updatePreferredMinWidth(w);
        ctx.updatePreferredWidth(w);
    } else {
        if (hasBlockFlow()) {
            Frame* f = firstChild();
            LayoutUnit w;
            while (f) {
                auto widths =
                    ctx.preferredWidthsWithNewContext(f->asFrameBox());

                if (f->isEstablishesBlockFormattingContext()) {
                    if (f->isNormalFlow()) {
                        if (f->style()->clear() & ClearValue::LeftClearValue) {
                            ctx.mainContext().m_floatLeftWidth = 0;
                        }
                        if (f->style()->clear() & ClearValue::RightClearValue) {
                            ctx.mainContext().m_floatRightWidth = 0;
                        }
                        w = std::max(w,
                                     widths.first +
                                         ctx.mainContext().m_floatLeftWidth +
                                         ctx.mainContext().m_floatRightWidth);
                    } else {
                        w = std::max(w, widths.first);
                    }
                } else {
                    if (!f->isAnonymous() && f->isNormalFlow()) {
                        ctx.mainContext().m_floatLeftWidth = 0;
                        ctx.mainContext().m_floatRightWidth = 0;
                    }
                    w = std::max(w, widths.first);
                }

                ctx.updatePreferredMinWidth(widths.second);
                f = f->next();
            }
            ctx.updatePreferredWidth(w);
        } else {
            bool isFirstLineCandidate =
                !isAnonymous() ||
                ctx.layoutContext().checkIfThisIsFirstLineCandidate(this);
            if (isFirstLineCandidate) {
                Length textIndent = style()->textIndent();
                LayoutUnit textIndentWidth =
                    textIndent.specifiedValue(ctx.remainingWidth(), this);
                ctx.setTextIndentWidth(textIndentWidth);
            }
            ctx.computePreferredWidthInline(this);
            if (isFirstLineCandidate) {
                if (ctx.hasAppliedTextIndent()) {
                    ctx.layoutContext().registerFirstLineCandidate(this);
                }
            }
            ctx.finishLine(false);
        }
    }

    LayoutUnit unused;
    LayoutUnit w = minMaxWidthAppliedIfNeeds(
        ctx.layoutContext(), ctx.preferredWidth(), unused, true);
    ctx.updatePreferredWidth(w);
    w = minMaxWidthAppliedIfNeeds(ctx.layoutContext(), ctx.preferredMinWidth(),
                                  unused, true);
    ctx.updatePreferredMinWidth(w);
}

void FrameFlexibleBox::computePreferredWidth(PreferredWidthContext& ctx)
{
    if (!isNecessaryBlockBox()) {
        return;
    }

    FrameBox* cb = containingBlock(this);
    MBPRestorer restorer(this);
    computeBorderMarginPadding(ctx.layoutContext(), cb->contentWidth());
    Length width = style()->width();

    if (width.isDefinite(false)) {
        LayoutUnit unused;
        LayoutUnit w = width.specifiedValue(unused, this);
        w = contentWidthApplyingBoxSizing(w);
        ctx.updatePreferredMinWidth(w);
        ctx.updatePreferredWidth(w);
    } else {
        FlexFormattingContext fCtx(ctx.layoutContext(), this,
                                   ctx.remainingWidth());
        Frame* f = firstChild();
        // TODO: Implement following
        // https://www.w3.org/TR/css-flexbox-1/#intrinsic-sizes
        if (fCtx.isMainAxisInInlineAxis()) {
            float maxContentFlexGrowFraction = 0;
            float maxContentFlexShrinkFraction = 0;
            Frame* f = firstChild();
            while (f) {
                if (!f->isFlexItem()) {
                    f = f->next();
                    continue;
                }

                FrameBox* flexItem = f->asFrameBox();
                LayoutUnit mbpWidth = ctx.mbpWidth(f);
                LayoutUnit basisSize = fCtx.basisSize(flexItem);
                LayoutUnit outerBasisSize = basisSize + mbpWidth;
                LayoutUnit maxContentContributeSize =
                    ctx.preferredWidthsWithNewContext(f->asFrameBox()).first;
                Length width = flexItem->style()->width();
                LayoutUnit unused;
                if (width.isDefinite(false)) {
                    maxContentContributeSize = std::max(
                        maxContentContributeSize,
                        LayoutUnit(width.specifiedValue(unused, flexItem)));
                }
                if (flexItem->style()->flexGrow() != 0) {
                    maxContentContributeSize =
                        std::min(maxContentContributeSize, basisSize);
                }
                if (flexItem->style()->flexShrink() == 0) {
                    maxContentContributeSize =
                        std::max(maxContentContributeSize, basisSize);
                }
                maxContentContributeSize = flexItem->minMaxWidthAppliedIfNeeds(
                    ctx.layoutContext(), maxContentContributeSize, unused,
                    false);
                LayoutUnit diff = maxContentContributeSize - outerBasisSize;
                if (diff > 0) {
                    if (f->style()->flexGrow() > 0) {
                        maxContentFlexGrowFraction =
                            std::max(maxContentFlexGrowFraction,
                                     std::floor(diff / f->style()->flexGrow()));
                    }
                } else if (diff < 0) {
                    if (f->style()->flexShrink() > 0) {
                        maxContentFlexShrinkFraction = std::max(
                            maxContentFlexShrinkFraction,
                            std::floor(-diff /
                                       (f->style()->flexShrink() * basisSize)));
                    }
                }

                f = f->next();
            }

            f = firstChild();
            LayoutUnit w;
            while (f) {
                if (!f->isFlexItem()) {
                    f = f->next();
                    continue;
                }

                FrameBox* flexItem = f->asFrameBox();
                LayoutUnit basisSize = fCtx.basisSize(flexItem);
                LayoutUnit mbpWidth = flexItem->mbpWidth();
                LayoutUnit unused;
                LayoutUnit itemWidth = basisSize;
                if (maxContentFlexGrowFraction > maxContentFlexShrinkFraction) {
                    itemWidth =
                        f->style()->flexGrow() * maxContentFlexGrowFraction;
                } else if (maxContentFlexShrinkFraction >
                           maxContentFlexGrowFraction) {
                    itemWidth = f->style()->flexShrink() * basisSize *
                                -maxContentFlexShrinkFraction;
                }
                itemWidth = flexItem->minMaxWidthAppliedIfNeeds(
                    ctx.layoutContext(), itemWidth, unused, false);
                w += itemWidth;
                w += mbpWidth;
                f = f->next();
            }
            ctx.updatePreferredWidth(w);
        } else {
            Frame* f = firstChild();
            LayoutUnit w;
            while (f) {
                if (!f->isFlexItem()) {
                    f = f->next();
                    continue;
                }

                auto widths =
                    ctx.preferredWidthsWithNewContext(f->asFrameBox());
                w = std::max(w, widths.first);
                ctx.updatePreferredMinWidth(widths.second);
                f = f->next();
            }
            ctx.updatePreferredWidth(w);
        }
    }

    LayoutUnit unused;
    LayoutUnit w = minMaxWidthAppliedIfNeeds(
        ctx.layoutContext(), ctx.preferredWidth(), unused, true);
    ctx.updatePreferredWidth(w);
    w = minMaxWidthAppliedIfNeeds(ctx.layoutContext(), ctx.preferredMinWidth(),
                                  unused, true);
    ctx.updatePreferredMinWidth(w);
}

void FrameTableBox::computePreferredWidth(PreferredWidthContext& ctx)
{
    LayoutUnit unused;
    LayoutUnit borderSpacing =
        style()->horizontalBorderSpacing().specifiedValue(unused, this);

    LayoutUnit tablePreferredWidth = 0;
    LayoutUnit tablePreferredMinWidth = 0;
    Length width = style()->width();
    if (width.isDefinite(false)) {
        LayoutUnit unused;
        tablePreferredWidth = width.specifiedValue(unused, this);
        if (!isAnonymous() && node()->isHTMLTableElement()) {
            tablePreferredWidth -= borderWidth() + paddingWidth();
        }
        tablePreferredMinWidth = tablePreferredWidth;
    } else {
        calCellWidth(ctx.layoutContext());
        tablePreferredWidth += borderSpacing;
        tablePreferredMinWidth += borderSpacing;
        for (auto& col : columnWidths()) {
            tablePreferredWidth += col.maxCellWidth;
            tablePreferredMinWidth += col.minCellWidth;
            tablePreferredWidth += borderSpacing;
            tablePreferredMinWidth += borderSpacing;
        }
    }

    LayoutUnit oldTextIndentWidth = ctx.textIndentWidth();
    tablePreferredMinWidth =
        ctx.widthAppliedByTextIndent(tablePreferredMinWidth);
    ctx.updatePreferredMinWidth(tablePreferredMinWidth);
    ctx.setTextIndentWidth(oldTextIndentWidth);
    tablePreferredWidth = ctx.widthAppliedByTextIndent(tablePreferredWidth);
    ctx.updatePreferredWidth(tablePreferredWidth);
}

void FrameBlockBox::paintChildrenWith(PaintingContext& ctx)
{
    if (hasBlockFlow()) {
        FrameBox::paintChildrenWith(ctx);
    } else {
        if (ctx.m_paintingStage == PaintingNormalFlowInline) {
            paintInlineContent(ctx.m_canvas);
        }
    }
}

void FrameBlockBox::paintInlineContent(Canvas* canvas)
{
    PaintingInlineStage stage = PaintingInlineBox;

    LayoutUnit dx, dy;
    while (stage != PaintingInlineStageEnd) {
        for (size_t i = 0; i < m_lineBoxes.size(); i++) {
            LineBox& b = *m_lineBoxes[i];
            LayoutUnit ldx = b.frameRect().x();
            LayoutUnit ldy = b.frameRect().y();
            dx += ldx;
            dy += ldy;
            b.paintInlineContent(canvas, stage, dx, dy);
            dx -= ldx;
            dy -= ldy;
        }
        stage = (PaintingInlineStage)(stage + 1);
    }
}

void FrameBlockBox::paintContent(PaintingContext& ctx)
{
    if (canSkipPaintingStage(ctx)) {
        return;
    }

    ctx.m_canvas->save();

    if (shouldResetTextDecoration()) {
        ctx.m_canvas->resetTextDecorationData();
    } else {
        ctx.m_canvas->mergeTextDecorationData(style());
    }

    bool overflowApplied = shouldApplyOverflow();

    if (style()->visibility() == VisibilityValue::HiddenVisibilityValue) {
        ctx.m_canvas->setVisible(false);
    } else {
        ctx.m_canvas->setVisible(true);
    }

    STARFISH_ASSERT(!isPositioned());

    if (isFlexItem()) {
        if (ctx.m_paintingStage == PaintingNormalFlowInline) {
            paintBackgroundAndBorders(ctx.m_canvas);
        }
    } else if (isFloating()) {
        if (ctx.m_paintingStage == PaintingNonPositionedFloats) {
            paintBackgroundAndBorders(ctx.m_canvas);
        }
    } else {
        if (ctx.m_paintingStage == PaintingNormalFlowBlock) {
            paintBackgroundAndBorders(ctx.m_canvas);
        }
    }

    if (overflowApplied) {
        ctx.m_canvas->save();
        ctx.m_canvas->clip(makeRect(BoxValue::PaddingBoxBoxValue));
        const LayoutRect rect(0, 0, width(), height());
        applyBorderRadiusClippingIfNeeds(ctx.m_canvas, rect);
        ctx.m_canvas->translate(-scrollLeft(), -scrollTop());
    }

    if (isFlexItem()) {
        if (ctx.m_paintingStage == PaintingNormalFlowInline) {
            PaintingContext ctx2(ctx.m_canvas);
            while (ctx2.m_paintingStage != PaintingStageEnd) {
                paintChildrenWith(ctx2);
                ctx2.m_paintingStage =
                    (PaintingStage)(ctx2.m_paintingStage + 1);
            }
            paintOutline(ctx.m_canvas);
        }
    } else if (isFloating()) {
        if (ctx.m_paintingStage == PaintingNonPositionedFloats) {
            paintChildrenWith(ctx);
            paintOutline(ctx.m_canvas);
        } else {
            paintChildrenWith(ctx);
        }
    } else if (!hasBlockFlow()) {
        if (ctx.m_paintingStage == PaintingNormalFlowInline) {
            paintInlineContent(ctx.m_canvas);
            paintOutline(ctx.m_canvas);
        }
    } else {
        if (ctx.m_paintingStage == PaintingNormalFlowBlock) {
            paintChildrenWith(ctx);
            paintOutline(ctx.m_canvas);
        } else {
            paintChildrenWith(ctx);
        }
    }

    if (overflowApplied) {
        ctx.m_canvas->restore();
    }

    if (overflowApplied && ctx.m_paintingStage == PaintingNormalFlowInline) {
        if (node() && node()->isElement()) {
            if (node()->asElement()->hasRareMembers() &&
                node()->asElement()->rareMembers()->m_scrolling) {
                node()
                    ->asElement()
                    ->rareMembers()
                    ->m_scrolling->paintScrollbars(ctx.m_canvas, this,
                                                   appliedOverflowX(),
                                                   appliedOverflowY());
            }
        }
    }

    ctx.m_canvas->restore();
}

void* InlineTextBox::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(InlineTextBox)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(InlineTextBox, m_node));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(InlineTextBox, m_layoutParent));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(InlineTextBox, m_text));
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(InlineTextBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

FrameText* InlineTextBox::origin()
{
    return node()->frame()->asFrameText();
}

void InlineTextBox::paintInlineContent(Canvas* canvas,
                                       PaintingInlineStage stage, LayoutUnit dx,
                                       LayoutUnit dy)
{
    if (stage == PaintingInlineStage::PaintingInlineBox) {
        ComputedStyle* s = style();
        if (s->visibility() == VisibilityValue::HiddenVisibilityValue) {
            return;
        } else {
            canvas->setVisible(true);
        }

        canvas->setFont(s->font());
        canvas->setColor(s->color());

        bool hasShadow = s->textShadow().size() ? true : false;

        if (hasShadow) {
            canvas->save();
            size_t width = (size_t)contentWidth().ceil();
            size_t height = (size_t)(ceil(s->font()->metrics().m_fontHeight));
            canvas->translate(dx, dy);

            CanvasShadowDataList list =
                s->textShadow().toCanvasShadowDataList(this);
            for (auto shadow = list.rbegin(); shadow != list.rend(); shadow++) {
                float radiusOffset = 0.0f;
                if (shadow->radius()) {
                    radiusOffset = shadow->radius();
                    radiusOffset =
                        std::min(ShadowBlur::RADIUS_LIMIT, radiusOffset);
                    radiusOffset *= 2;
                }
                NativeImageData* nativeImage = NativeImageData::create(
                    width + ceil(radiusOffset), height + ceil(radiusOffset));
                Canvas* cv = Canvas::createGenericCanvas(node()->starFish(),
                                                         nativeImage);
                cv->clearColor(Unit::Color(0, 0, 0, 0));
                cv->setFont(s->font());
                auto tdc = canvas->textDecorationData();
                if (shadow->hasColor()) {
                    cv->setColor(shadow->color());
                    tdc.setUnderLineColor(shadow->color());
                    tdc.setLineThroughColor(shadow->color());
                } else {
                    cv->setColor(s->color());
                }
                cv->setTextDecorationData(tdc);
                cv->translate(ceil(radiusOffset / 2), ceil(radiusOffset / 2));
                cv->drawText(0, 0, contentWidth(), text());

                ShadowBlur sb(nativeImage->data(), nativeImage->width(),
                              nativeImage->height(), nativeImage->stride());
                sb.process(shadow->radius());

                delete cv;

                Unit::Rect rect(0, 0, nativeImage->width(),
                                nativeImage->height());
                float offset = ceil(radiusOffset / 2);
                canvas->translate(-offset + shadow->offsetX(),
                                  -offset + shadow->offsetY());
                canvas->drawImage(nativeImage, rect);
                canvas->translate(offset - shadow->offsetX(),
                                  offset - shadow->offsetY());
            }
            list.clear();

            canvas->restore();
        }

        canvas->drawText(dx, dy, contentWidth(), text());
    }
}

void InlineNonReplacedBox::paintBackgroundAndBorders(Canvas* canvas)
{
    if (!isCollapsed()) {
        LayoutRect frameRectBack = m_frameRect;
        LayoutBoxSurroundData paddingBack, borderBack, marginBack;
        FrameBoxRareData fakeRareData(layoutParent());
        bool hasRareDataBefore = hasRareData();
        if (hasRareDataBefore) {
            paddingBack = frameBoxRareData()->m_padding;
            borderBack = frameBoxRareData()->m_border;
            marginBack = frameBoxRareData()->m_margin;
            if (rareData()) {
                frameBoxRareData()->m_padding = m_rareData->m_orgPadding;
                frameBoxRareData()->m_margin = m_rareData->m_orgMargin;
                frameBoxRareData()->m_border = m_rareData->m_orgBorder;
            } else {
                frameBoxRareData()->m_padding = LayoutBoxSurroundData();
                frameBoxRareData()->m_margin = LayoutBoxSurroundData();
                frameBoxRareData()->m_border = LayoutBoxSurroundData();
            }
        } else {
            m_layoutParent = (Frame*)&fakeRareData;
            if (rareData()) {
                fakeRareData.m_padding = m_rareData->m_orgPadding;
                fakeRareData.m_margin = m_rareData->m_orgMargin;
                fakeRareData.m_border = m_rareData->m_orgBorder;
            } else {
                fakeRareData.m_padding = LayoutBoxSurroundData();
                fakeRareData.m_margin = LayoutBoxSurroundData();
                fakeRareData.m_border = LayoutBoxSurroundData();
            }
        }

        FrameBox::paintBackgroundAndBorders(canvas);
        FrameBox::paintOutline(canvas);

        m_frameRect = frameRectBack;
        if (hasRareDataBefore) {
            frameBoxRareData()->m_padding = paddingBack;
            frameBoxRareData()->m_border = borderBack;
            frameBoxRareData()->m_margin = marginBack;
        } else {
            m_layoutParent = fakeRareData.m_layoutParent;
        }
    }
}

void InlineNonReplacedBox::paintStackingContextContent(Canvas* canvas)
{
    PaintingInlineStage stage = PaintingInlineBox;

    while (stage != PaintingInlineStageEnd) {
        InlineBoxLayoutParentBox::paintInlineContent(canvas, stage, 0, 0);
        stage = (PaintingInlineStage)(stage + 1);
    }
}

void InlineNonReplacedBox::paintInlineContent(Canvas* canvas,
                                              PaintingInlineStage stage,
                                              LayoutUnit dx, LayoutUnit dy)
{
    if (isEstablishesStackingContext()) {
        return;
    }

    if (stage == PaintingInlineBox) {
        canvas->translate(dx, dy);
        paintBackgroundAndBorders(canvas);
        canvas->translate(-dx, -dy);
    }

    InlineBoxLayoutParentBox::paintInlineContent(canvas, stage, dx, dy);
}

void InlineNonReplacedBox::paintChildrenWith(PaintingContext& ctx)
{
    if (ctx.m_paintingStage == PaintingNormalFlowInline) {
        PaintingInlineStage stage = PaintingInlineBox;

        while (stage != PaintingInlineStageEnd) {
            paintInlineContent(ctx.m_canvas, stage, 0, 0);
            stage = (PaintingInlineStage)(stage + 1);
        }
    }
}

Frame* InlineTextBox::hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage)
{
    if (stage == HitTestStage::HitTestNormalFlowInline) {
        if (style()->visibility() == VisibilityValue::HiddenVisibilityValue) {
            return nullptr;
        }
        return FrameBox::hitTest(x, y, stage);
    }
    return nullptr;
}

Frame* InlineNonReplacedBox::hitTest(LayoutUnit x, LayoutUnit y,
                                     HitTestStage stage)
{
    if (isEstablishesStackingContext()) {
        return nullptr;
    }

    if (style()->visibility() == VisibilityValue::HiddenVisibilityValue) {
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
                if (result) {
                    return result;
                }
                iter++;
            }
            s = (HitTestStage)(s + 1);
        }
    }
    return nullptr;
}
#ifdef STARFISH_ENABLE_TEST
void InlineNonReplacedBox::dump(int depth)
{
    FrameBox::dump(depth);

    printf(" origin %p", m_origin);

    puts("");
    auto iter = boxes().begin();
    while (iter != boxes().end()) {
        FrameBox* f = *iter;
        for (int k = 0; k < depth + 1; k++) {
            printf("  ");
        }
        f->dump(depth + 2);
        if (iter + 1 != boxes().end()) {
            puts("");
        }
        iter++;
    }
}
#endif
}
