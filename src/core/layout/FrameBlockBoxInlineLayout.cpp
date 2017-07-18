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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/CharacterData.h"
#include "core/dom/Node.h"
#include "core/dom/HTMLTableElement.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameBlockBoxInlineLayout.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/FrameInline.h"
#include "core/layout/FrameTableBox.h"
#include "core/layout/StackingContext.h"
#include "core/modules/canvas/Canvas.h"
#include "core/util/LineBreakerIteratorPool.h"

namespace StarFish {

LayoutUnit FrameBox::lineHeight()
{
    LayoutUnit fontSize = style()->font()->metrics().m_ascender -
                          style()->font()->metrics().m_descender;

    if (!style()->hasNormalLineHeight()) {
        if (style()->lineHeight().isFixed()) {
            return style()->lineHeight().fixed();
        } else if (style()->lineHeight().isInheritableNumber()) {
            return fontSize * style()->lineHeight().number();
        } else {
            // Only Fixed | InheritableNumer possible here
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }
    return fontSize;
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
                maxAscenderSoFar = std::max(rb->ascender(), maxAscenderSoFar);
                maxDescenderSoFar = std::min(rb->decender(), maxDescenderSoFar);
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
                rb->setY(descender + rb->ascender());
                maxAscenderSoFar =
                    std::max(descender + rb->ascender(), maxAscenderSoFar);
                maxDescenderSoFar =
                    std::min(descender + rb->decender(), maxDescenderSoFar);
            } else if (va == VerticalAlignValue::SuperVAlignValue) {
                // Placing a superscript is font and browser dependent.
                // We place superscript above the baseline by 1/2 of
                // ascender (following blink)
                // (i.e, the baseline of superscript is aligned with 1/2 of
                // the ascender)
                rb->setY(ascender / 2 + rb->ascender());
                maxAscenderSoFar =
                    std::max(ascender / 2 + rb->ascender(), maxAscenderSoFar);
                maxDescenderSoFar =
                    std::min(ascender / 2 + rb->decender(), maxDescenderSoFar);
            } else if (va == VerticalAlignValue::TextTopVAlignValue) {
                maxDescenderSoFar =
                    std::min(ascender - rb->height(), maxDescenderSoFar);
            } else if (va == VerticalAlignValue::TextBottomVAlignValue) {
                maxAscenderSoFar =
                    std::max(descender + rb->height(), maxAscenderSoFar);
            } else if (va == VerticalAlignValue::NumericVAlignValue) {
                Length len = box->style()->verticalAlignLength();
                LayoutUnit y;
                if (len.isPercent()) {
                    y = rb->ascender() + box->lineHeight() * len.percent();
                } else if (len.isFixed()) {
                    y = rb->ascender() + LayoutUnit::fromPixel(len.fixed());
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
            LayoutUnit boxHeight = box->boxHeight();
            if (va == VerticalAlignValue::BaselineVAlignValue) {
                maxAscenderSoFar = std::max(boxHeight, maxAscenderSoFar);
            } else if (va == VerticalAlignValue::TopVAlignValue) {
                // NO WORK TO DO
            } else if (va == VerticalAlignValue::BottomVAlignValue) {
                // NO WORK TO DO
            } else if (va == VerticalAlignValue::MiddleVAlignValue) {
                LayoutUnit halfHeight = boxHeight / 2;
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
                box->setY(descender + boxHeight);
                maxAscenderSoFar =
                    std::max(descender + boxHeight, maxAscenderSoFar);
            } else if (va == VerticalAlignValue::SuperVAlignValue) {
                box->setY(ascender / 2 + boxHeight);
                maxAscenderSoFar =
                    std::max(ascender / 2 + boxHeight, maxAscenderSoFar);
            } else if (va == VerticalAlignValue::TextTopVAlignValue) {
                maxDescenderSoFar =
                    std::min(ascender - boxHeight, maxDescenderSoFar);
            } else if (va == VerticalAlignValue::TextBottomVAlignValue) {
                maxAscenderSoFar =
                    std::max(descender + boxHeight, maxAscenderSoFar);
            } else if (va == VerticalAlignValue::NumericVAlignValue) {
                Length len = box->style()->verticalAlignLength();
                LayoutUnit amount;
                if (len.isPercent()) {
                    amount = boxHeight + box->lineHeight() * len.percent();
                } else if (len.isFixed()) {
                    amount = boxHeight + LayoutUnit::fromPixel(len.fixed());
                }
                maxAscenderSoFar = std::max(amount, maxAscenderSoFar);
                maxDescenderSoFar =
                    std::min(amount - boxHeight, maxDescenderSoFar);
                box->setY(amount);
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        } else if (box->isFrameBlockBox() &&
                   ((box->style()->display() == InlineBlockDisplayValue) ||
                    (box->style()->display() == InlineTableDisplayValue))) {
            hasBoxOtherThanText = true;
            hasBoxOtherThanCollapsedInlineNonReplacedBox = true;
            LayoutUnit boxHeight = box->boxHeight();
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
                LayoutUnit halfHeight = boxHeight / 2;
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
                    std::min(ascender - boxHeight, maxDescenderSoFar);
            } else if (va == VerticalAlignValue::TextBottomVAlignValue) {
                maxAscenderSoFar =
                    std::max(descender + boxHeight, maxAscenderSoFar);
            } else if (va == VerticalAlignValue::NumericVAlignValue) {
                LayoutUnit ascender =
                    inlineBlockAscender(box->asFrameBlockBox());
                Length len = box->style()->verticalAlignLength();
                LayoutUnit amount;
                if (len.isPercent()) {
                    amount = ascender + box->lineHeight() * len.percent();
                } else if (len.isFixed()) {
                    amount = ascender + LayoutUnit::fromPixel(len.fixed());
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
                lineBox->setAscDescender(
                    parentStyle->font()->metrics().m_ascender,
                    parentStyle->font()->metrics().m_descender);
                return;
            }

            lineBox->setAscDescender(0, 0);
            return;
        }

        if (!hasBoxOtherThanCollapsedInlineNonReplacedBox) {
            lineBox->setAscDescender(0, 0);
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
                inrb->marginRight() == 0) {
                inrb->markCollapsed();
                inrb->setAscDescender(maxAscender, maxDescender);
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
                            f->asInlineNonReplacedBox()->decender());
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
                    STARFISH_ASSERT(
                        f->isFrameBlockBox() &&
                        ((f->style()->display() == InlineBlockDisplayValue) ||
                         (f->style()->display() == InlineTableDisplayValue)));
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

    parentBox->asInlineBoxLayoutParentBox()->setAscDescender(maxAscender,
                                                             maxDescender);
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

static bool hasIsolateBidiContent(InlineNonReplacedBox* box)
{
    if (box->style()->unicodeBidi() ==
        UnicodeBidiValue::IsolateUnicodeBidiValue) {
        return true;
    } else if (box->style()->unicodeBidi() ==
               UnicodeBidiValue::EmbedUnicodeBidiValue) {
        return true;
    }
    return false;
}

static FrameBox* fetchContentForResolveBidi(FrameBox* box)
{
    if (box->isInlineNonReplacedBox()) {
        InlineNonReplacedBox* b = box->asInlineNonReplacedBox();
        if (hasIsolateBidiContent(b)) {
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
            if (!hasIsolateBidiContent(inrb)) {
                splitInlineBoxes(inrb->boxes());
                // split every content for resolve bidi after
                auto& boxesToCopy = inrb->boxes();
                if (boxesToCopy.size()) {
                    boxes.erase(boxes.begin() + i);

                    size_t insertPos = i;

                    for (size_t j = 0; j < boxesToCopy.size(); j++) {
                        InlineNonReplacedBox* newBox =
                            new InlineNonReplacedBox(inrb, isFirstLineBox());
                        newBox->setX(boxesToCopy[j]->x() + inrb->x());
                        newBox->setY(inrb->y());
                        boxesToCopy[j]->setX(0);
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
        if (hasIsolateBidiContent(inrb)) {
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

void InlineBoxLayoutParentBox::setLeftMBPs()
{
    for (size_t i = 0; i < m_boxes.size(); i++) {
        FrameBox* box = m_boxes[i];
        if (box->isInlineNonReplacedBox()) {
            InlineNonReplacedBox* inrb = box->asInlineNonReplacedBox();
            if (!hasIsolateBidiContent(inrb)) {
                inrb->setLeftMBPs();

                if (inrb->style()->direction() == LtrDirectionValue) {
                    if (!inrb->isSetLeftMBP() &&
                        inrb->isProcessedStartingMBP()) {
                        inrb->setOrgLeftMBP();
                    } else {
                        inrb->unsetLeftMBP();
                    }
                } else {
                    if (!inrb->isSetLeftMBP() && inrb->isProcessedEndingMBP()) {
                        inrb->setOrgLeftMBP();
                    } else {
                        inrb->unsetLeftMBP();
                    }
                }
            }
        }
    }
}

void InlineBoxLayoutParentBox::setRightMBPs()
{
    for (size_t i = m_boxes.size() - 1; i != SIZE_MAX; i--) {
        FrameBox* box = m_boxes[i];
        if (box->isInlineNonReplacedBox()) {
            InlineNonReplacedBox* inrb = box->asInlineNonReplacedBox();
            if (!hasIsolateBidiContent(inrb)) {
                inrb->setRightMBPs();

                if (inrb->style()->direction() == LtrDirectionValue) {
                    if (!inrb->isSetRightMBP() &&
                        inrb->isProcessedEndingMBP()) {
                        inrb->setOrgRightMBP();
                    } else {
                        inrb->unsetRightMBP();
                    }
                } else {
                    if (!inrb->isSetRightMBP() &&
                        inrb->isProcessedStartingMBP()) {
                        inrb->setOrgRightMBP();
                    } else {
                        inrb->unsetRightMBP();
                    }
                }
            }
        }
    }
}

void LineFormattingContext::resolveBidi(DirectionValue parentDir,
                                        GCVector<FrameBox*>& boxes)
{
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
                bool shouldReplaceString = false;
                for (size_t j = sv.start(); j < sv.end(); j++) {
                    char32_t ch = sv.originalString()->charAt(j);
                    if (ch < 128) {
                        if (parenthesisMap[ch]) {
                            shouldReplaceString = true;
                            break;
                        }
                    }
                }

                if (shouldReplaceString) {
                    if (sv.originalString()
                            ->bufferAccessData()
                            .hasASCIIContent) {
                        ASCIIString str;
                        for (size_t j = sv.start(); j < sv.end(); j++) {
                            char32_t ch = sv.originalString()->charAt(j);
                            if (ch < 128) {
                                if (parenthesisMap[ch]) {
                                    ch = (char32_t)parenthesisMap[ch];
                                }
                            }
                            str += (char)ch;
                        }
                        itb->setText(new StringDataASCII(std::move(str)));
                    } else {
                        UTF32String str;
                        for (size_t j = sv.start(); j < sv.end(); j++) {
                            char32_t ch = sv.originalString()->charAt(j);
                            if (ch < 128) {
                                if (parenthesisMap[ch]) {
                                    ch = (char32_t)parenthesisMap[ch];
                                }
                            }
                            str += ch;
                        }
                        itb->setText(new StringDataUTF32(std::move(str)));
                    }
                }
            }
        }
    }

    m_currentLayoutParent->setLeftMBPs();
    m_currentLayoutParent->setRightMBPs();
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

LineFormattingContext::LineFormattingContext(FrameBlockBox* block,
                                             LayoutContext& ctx)
    : m_unprocessedStartingMBPWidth(0)
    , m_block(block)
    , m_layoutContext(ctx)
    , m_inlineBoxIndex(0)
{
    m_absPosition = block->absolutePoint(m_layoutContext.frameDocument());
    m_leftBoundary =
        m_absPosition.x() + block->paddingLeft() + block->borderLeft();
    m_rightBoundary = m_leftBoundary + block->contentWidth();
    m_lineBoxY = block->paddingTop() + block->borderTop();
    m_block->m_lineBoxes.clear();
    // m_block.m_lineBoxes.shrink_to_fit();
    resetLineBox();
    Length textIndent = block->style()->textIndent();
    m_textIndentWidth = textIndent.specifiedValue(block->boxWidth());
}

void LineFormattingContext::registerInlineContent()
{
    if (m_block->m_lineBoxes.size()) {
        LineBox* lb = m_block->m_lineBoxes.back();
        bool hasNormalFlowContent = false;
        for (size_t i = 0; i < lb->boxes().size(); i++) {
            if (lb->boxes()[i]->isNormalFlow()) {
                hasNormalFlowContent = true;
                break;
            }
        }
        if (hasNormalFlowContent) {
            m_layoutContext.registerYPositionPerVAInlineBlock(lb);
        }
    }
}

void InlineBoxLayoutParentBox::computeVisibleRect(StackingContext* sCtx,
                                                  LayoutLocation& loc)
{
    VisibleRectContext ctx(this, &loc);

    for (size_t i = 0; i < m_boxes.size(); i++) {
        m_boxes[i]->computeVisibleRect(sCtx, loc);
    }
}

FrameBox* InlineBoxLayoutParentBox::firstInlineBox()
{
    for (size_t i = 0; i < m_boxes.size(); i++) {
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
        return run.m_stringView.originalString() == String::spaceString;
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
        return run.m_stringView.originalString() == String::emptyString;
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
            ctx->m_currentLineWidth -= last->boxWidth();
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
                inrb->marginRight() == 0 &&
                inrb->isAbsolutePositionedBoxLayoutParent(ctx)) {
                iter++;
                continue;
            }
        }

        return false;
    }

    return true;
}

LayoutUnit InlineBoxLayoutParentBox::layoutInlineBoxes(LayoutUnit start)
{
    LayoutUnit x = start;
    for (size_t k = 0; k < m_boxes.size(); k++) {
        FrameBox* childBox = m_boxes[k];
        if (childBox->isNormalFlow()) {
            childBox->setX(x + childBox->marginLeft());
            x += childBox->boxWidth();
        } else if (childBox->isAbsolutePositioned()) {
            childBox->setX(x);
        }
    }

    return x;
}

void InlineBoxLayoutParentBox::registerRelativePositionedBoxes(
    LayoutContext& ctx)
{
    for (size_t k = 0; k < m_boxes.size(); k++) {
        FrameBox* childBox = m_boxes[k];
        STARFISH_ASSERT(childBox != nullptr);

        if (!childBox->isFrameBlockBox()) {
            if (childBox->style()->position() ==
                PositionValue::RelativePositionValue) {
                ctx.registerRelativePositionedBox(childBox, true);
            }

            if (childBox->isInlineNonReplacedBox()) {
                childBox->asInlineNonReplacedBox()
                    ->registerRelativePositionedBoxes(ctx);
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
            dontBreakLine(box, box->boxWidth())) {
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

        if (box->style()->originalDisplay() == BlockDisplayValue) {
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
    int hasFloat = HasNone;
    std::pair<LayoutUnit, LayoutUnit> boundaries =
        m_layoutContext.horizontalBoundaryBetweenFloatingBoxes(
            m_absPosition.y() + m_lineBoxY + yDiff, height, m_leftBoundary,
            m_rightBoundary);
    if (boundaries.first > m_leftBoundary) {
        hasFloat |= HasLeft;
        m_lineBoxX = boundaries.first - m_absPosition.x();
    } else {
        m_lineBoxX = m_leftBoundary - m_absPosition.x();
    }

    if (m_rightBoundary > boundaries.second) {
        hasFloat |= HasRight;
        m_lineBoxWidth = boundaries.second - m_leftBoundary;
    } else {
        m_lineBoxWidth = m_rightBoundary - m_leftBoundary;
    }
    m_lineBoxWidth -= (m_lineBoxX - m_leftBoundary + m_absPosition.x());

    lineBox->setX(m_lineBoxX);
    lineBox->setY(m_lineBoxY);
    lineBox->setWidth(m_lineBoxWidth);

    if (m_floatingBoxLayoutContexts.size() == 0) {
        m_floatingBoxLayoutContexts.emplace_back(hasFloat, yDiff, m_lineBoxX,
                                                 m_lineBoxWidth);
    } else {
        if (m_floatingBoxLayoutContexts[m_floatingBoxLayoutContexts.size() - 1]
                .m_y < yDiff) {
            m_floatingBoxLayoutContexts.emplace_back(
                hasFloat, yDiff, m_lineBoxX, m_lineBoxWidth);
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
        fbCtx.m_accumulatedLeftFloatingBoxWidth += box->boxWidth();
        if (fbCtx.m_y == 0 && box->boxWidth() > 0) {
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
        fbCtx.m_accumulatedRightFloatingBoxWidth += box->boxWidth();
        if (fbCtx.m_y == 0 && box->boxWidth() > 0) {
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

void LineFormattingContext::computeHorizontalProperties()
{
    LineBox* back = m_block->m_lineBoxes.back();

    resolveBidi(m_block->style()->direction(), back->boxes());

    back->setX(m_lineBoxX);
    back->setWidth(m_lineBoxWidth);
    LayoutUnit inlineBoxesWidth = back->layoutInlineBoxes(m_textIndentWidth);

    // text align
    if (m_block->style()->textAlign() == SideValue::LeftSideValue) {
    } else if (m_block->style()->textAlign() == SideValue::RightSideValue) {
        LayoutUnit diff = (m_lineBoxWidth - inlineBoxesWidth);
        for (size_t k = 0; k < back->m_boxes.size(); k++) {
            FrameBox* childBox = back->m_boxes[k];
            if (!childBox->isFloating()) {
                childBox->moveX(diff);
            }
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
        STARFISH_ASSERT(m_block->style()->textAlign() ==
                        SideValue::CenterSideValue);
        LayoutUnit diff = (m_lineBoxWidth - inlineBoxesWidth) / 2;
        if (diff > 0) {
            for (size_t k = 0; k < back->m_boxes.size(); k++) {
                FrameBox* childBox = back->m_boxes[k];
                if (!childBox->isFloating()) {
                    childBox->moveX(diff);
                }
            }
        }
    }
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
                if (!dontBreakLine(box, box->boxWidth())) {
                    break;
                }

                insertInlineBox(box);
            }
        }

        iter = m_pendingInlineBoxes.erase(iter);
        continue;
    }
}

void LineFormattingContext::resetLineBox()
{
    LineBox* lineBox = new LineBox(m_block);
    m_block->m_lineBoxes.push_back(lineBox);
    m_floatingBoxLayoutContexts.clear();
    layoutLineBox(0, 0);
    if (m_currentLineWidth > 0) {
        m_textIndentWidth = 0;
    }
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
    computeHorizontalProperties();
    registerInlineContent();

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

    LayoutUnit yDiff = distanceToNextLineBox(
        br, !isLastLine || m_pendingInlineBoxes.size() > 0);

    insertAbsolutePositionedBoxes();
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
    /*
    if (dueToBr == false) {
        m_breakedLinesSet.insert(m_block->m_lineBoxes.size() - 1);
    }
    */

    if (!skipFinishLine) {
        finishLineForLineBox(br, isLastLine);
    }

    resetLineBox();
    if (m_currentLayoutParent->isLineBox()) {
        m_currentLayoutParent = currentLine();
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
    return width <= (m_remainedWidth - m_currentLineWidth -
                     m_unprocessedStartingMBPWidth);
}

bool PreferredWidthContext::dontBreakLine(LayoutUnit width)
{
    return (!hasFloatingBoxAlreadyInLineBox() && m_currentLineWidth == 0) ||
           canInsertToLineBox(width);
}

bool LineFormattingContext::canInsertToLineBox(Frame* f, LayoutUnit width)
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
        bool ret = width <= remainingWidth;

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
bool LineFormattingContext::dontBreakLine(Frame* f, LayoutUnit width)
{
    return (!hasFloatingBoxAlreadyInLineBox(f) && m_currentLineWidth == 0) ||
           !f->shouldWrapLines() || canInsertToLineBox(f, width);
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

    if (dontBreakLine(m_word.boxes()[0], m_word.width())) {
        handleSoftHyphenate(false);
        auto& boxes = m_word.boxes();
        auto iter = boxes.begin();
        m_currentLayoutParent =
            boxes[0]->layoutParent()->asInlineBoxLayoutParentBox();

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
                if (m_currentLayoutParent->layoutParent()->isLineBox()) {
                    m_currentLayoutParent = currentLine();
                    break;
                } else {
                    m_currentLayoutParent =
                        m_currentLayoutParent->layoutParent()
                            ->asInlineNonReplacedBox();
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
                m_currentLayoutParent = box->asInlineBoxLayoutParentBox();
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
            m_block->node()->asElement()->hasPseudoElement(
                StyleResolver::PseudoElementFirstLine)) {
            m_word.unmarkFirstLine();
        }

        handleSoftHyphenate(true);

        auto& boxes = m_word.boxes();
        m_currentLayoutParent =
            boxes[0]->layoutParent()->asInlineBoxLayoutParentBox();
        breakLine(nullptr);
        boxes[0]->setLayoutParent(m_currentLayoutParent);
        insertWord(next);
    }
}

void LineFormattingContext::insertInlineBox(FrameBox* box)
{
    m_currentLayoutParent->insertInlineBox(box);
    if (m_currentLayoutParent->isInlineNonReplacedBox()) {
        InlineNonReplacedBox* self =
            m_currentLayoutParent->asInlineNonReplacedBox();
        self->setWidth(self->width() + box->boxWidth());
        self->processStartingMBP(this);
    }

    m_currentLineWidth += box->boxWidth();
    setIsWhiteSpaceAtLast(isCollapsibleWhiteSpace(box));
}

void LineFormattingContext::tryInsertInlineBox(FrameBox* box)
{
    if (isWord(box)) {
        m_word.concat(box);
        return;
    }

    STARFISH_ASSERT(m_word.isEmpty());

    if (isForcedNewLine(box)) {
        m_isPendingBreakLine = true;
    } else {
        if (!dontBreakLine(box, box->boxWidth())) {
            if (isCollapsibleWhiteSpace(box)) {
                m_isPendingBreakLine = true;
            } else {
                breakLine(nullptr);
                tryInsertInlineBox(box);
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
        dontBreakLine(box, box->boxWidth())) {
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
        InlineTextBox* ib =
            new InlineTextBox(f, TextRun(source, start, end, dir), isFirstLine);
        ib->setLayoutParent(m_currentLayoutParent);
        ib->setWidth(textWidth);
        ib->setHeight(f->style()->font()->metrics().m_fontHeight);
        tryInsertInlineBox(ib);
    } else {
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

                InlineTextBox* ib = new InlineTextBox(
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
                InlineTextBox* ib = new InlineTextBox(
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
            InlineTextBox* ib = new InlineTextBox(
                f, TextRun(srcTxt, offset, nextOffset, dir), isFirstLine);
            ib->setLayoutParent(m_currentLayoutParent);
            ib->setWidth(textWidth);
            ib->setHeight(f->style()->font()->metrics().m_fontHeight);
            tryInsertInlineBox(ib);
        }
    }
}

void LineFormattingContext::handleAbsoluteBox(FrameBox* box, bool canInsert,
                                              bool canRegister)
{
    if (canInsert) {
        if (canRegister) {
            registerAbsolutePositionedBox(box);
        }

        if (box->style()->originalDisplay() != BlockDisplayValue) {
            m_currentLayoutParent->insertInlineBox(box);
        }

        markAbsolutePositionedBoxLayoutParent(m_currentLayoutParent);
    } else {
        box->setLayoutParent(m_currentLayoutParent);
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
        !token.m_frameText->shouldPreserveWhiteSpaces() &&
        isWhiteSpaceAtLast()) {
        return;
    }

    generateInlineTextBox(token);

    // Consider direction for hyphen
    char32_t c = token.m_frameText->text()->charAt(token.m_end - 1);
    bool isHyphenAtLast = isSoftHyphen(c) || isHyphen(c);

    if (isHyphenAtLast) {
        insertWord(token.m_frameText);
        m_isSoftHyphenAtLast = isSoftHyphen(c);
    }
}

static void nextToken(std::vector<int32_t>::iterator& iter, int32_t& cur,
                      int32_t next)
{
    if (next == *iter) {
        iter++;
    }
    cur = next;
}

int utf32ToUtf16(char32_t i, char16_t* u);
size_t utf16ToUtf32(const char16_t* UTF16, const char16_t* bufferEnd,
                    char32_t& uc);

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
    if (data.hasASCIIContent) {
        while ((next = breaker->next()) != icu::BreakIterator::DONE) {
            locs.push_back(next);
        }
    } else {
        int32_t len = str.length();
        while ((next = breaker->next()) != icu::BreakIterator::DONE) {
            if (next == len) {
                locs.push_back(txt->length());
            } else {
                locs.push_back(str.getChar32Start(next));
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
        STARFISH_ASSERT(display == InlineBlockDisplayValue ||
                        display == InlineTableDisplayValue);
        // inline-block, inline-table
        ctx.m_layoutContext.pushInlineBlockBox(this);
        setLayoutParent(ctx.m_currentLayoutParent);
        layout(ctx.m_layoutContext, Frame::LayoutWantToResolve::ResolveAll);
        LayoutUnit ascender;

        if (display == InlineTableDisplayValue) {
            ascender = asFrameTableBox()->calBaseline();
        } else {
            std::pair<bool, LayoutUnit> p =
                ctx.m_layoutContext.registeredLastLineBoxYPosition(this);
            if (p.first &&
                style()->overflow() == OverflowValue::VisibleOverflow) {
                ascender = p.second;
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

    InlineNonReplacedBox* inlineBox =
        new InlineNonReplacedBox(this, ctx.isFirstLineBox());

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
        ctx.m_currentLayoutParent =
            ctx.m_currentLayoutParent->layoutParent()->asLineBox();

    } else {
        inlineBox->layoutInline(ctx);
        ctx.m_currentLayoutParent =
            ctx.m_currentLayoutParent->layoutParent()->asInlineNonReplacedBox();
    }
}

void LineFormattingContext::layoutInline(Frame* origin)
{
    Frame* f = origin->firstChild();
    while (f) {
        // Don't put any inline box leaving pending inline boxes ahead.
        STARFISH_ASSERT(m_pendingInlineBoxes.size() == 0);

        if (f->isAbsolutePositioned()) {
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
            if (f->style()->unicodeBidi() ==
                    UnicodeBidiValue::EmbedUnicodeBidiValue ||
                f->style()->unicodeBidi() ==
                    UnicodeBidiValue::IsolateUnicodeBidiValue) {
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

    DirectionValue currentDirection = direction;
    bool everMeetNonNeutralThing = false;
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
                        everMeetNonNeutralThing = true;
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
                    everMeetNonNeutralThing = true;
                    m_textRunsPerFrameText[f->asFrameText()].emplace_back(
                        run.m_stringView.originalString(),
                        run.m_stringView.start(), run.m_stringView.end(),
                        run.m_direction);
                    flushNeutral(run.m_direction == CharDirection::Ltr
                                     ? DirectionValue::LtrDirectionValue
                                     : DirectionValue::RtlDirectionValue);
                }
            }

        } else if (f->isFrameBox()) {
            if (everMeetNonNeutralThing) {
                putOffNeutral(f);
            } else {
                m_computedDirectionValuePerFrame[f] = direction;
            }
        } else if (f->isFrameInline()) {
            if (f->style()->unicodeBidi() ==
                UnicodeBidiValue::EmbedUnicodeBidiValue) {
                everMeetNonNeutralThing = true;
                flushNeutral(f->style()->direction());
            } else if (f->style()->unicodeBidi() ==
                       UnicodeBidiValue::IsolateUnicodeBidiValue) {
                if (everMeetNonNeutralThing) {
                    putOffNeutral(f);
                } else {
                    m_computedDirectionValuePerFrame[f] = direction;
                }
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        } else if (f->isFrameLineBreak()) {
            everMeetNonNeutralThing = true;
            flushNeutral(direction);
        }
    }

    flushNeutral(direction);
}

void InlineNonReplacedBox::setOrgLeftMBP()
{
    m_orgMargin.setLeft(marginLeft());
    m_orgBorder.setLeft(borderLeft());
    m_orgPadding.setLeft(paddingLeft());

    LayoutUnit w;
    moveX(marginLeft());
    LayoutUnit bp = borderLeft() + paddingLeft();
    w += marginLeft() + bp;
    if (boxes().size()) {
        STARFISH_ASSERT(boxes().size() == 1);
        boxes()[0]->moveX(bp);
    }
    setWidth(width() + bp);

    if (w > 0) {
        FrameBox* parent = layoutParent()->asFrameBox();
        while (parent->isInlineNonReplacedBox()) {
            parent->setWidth(parent->width() + w);
            parent = parent->layoutParent()->asFrameBox();
        }
    }

    markSetLeftMBP();
}

void InlineNonReplacedBox::setOrgRightMBP()
{
    m_orgMargin.setRight(marginRight());
    m_orgBorder.setRight(borderRight());
    m_orgPadding.setRight(paddingRight());

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

    markSetRightMBP();
}

void InlineNonReplacedBox::processStartingMBP(
    LineFormattingContext* lineFormattingContext)
{
    InlineNonReplacedBox* current = this;
    while (current) {
        if (!current->isProcessedStartingMBP()) {
            LayoutUnit unprocessedStartingMBPWidth =
                current->startingMBPWidth();
            lineFormattingContext->m_currentLineWidth +=
                unprocessedStartingMBPWidth;
            lineFormattingContext->m_unprocessedStartingMBPWidth -=
                unprocessedStartingMBPWidth;
            current->markProcessedStartingMBP();
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
    markProcessedEndingMBP();
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
        LayoutUnit w = current->width();
        if (current->isProcessedStartingMBP()) {
            if (current->style()->direction() == LtrDirectionValue) {
                w += current->borderLeft() + current->paddingLeft();
            } else {
                w += current->borderRight() + current->paddingRight();
            }
        }

        if (current->isProcessedEndingMBP()) {
            if (current->style()->direction() == LtrDirectionValue) {
                w += current->borderRight() + current->paddingRight();
            } else {
                w += current->borderLeft() + current->paddingLeft();
            }
        }
        current->setWidth(std::max(LayoutUnit(0), w));
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

    if (hasIsolateBidiContent(self)) {
        resolveBidi(self->style()->direction(), self->boxes());
    }
    self->layoutInlineBoxes(self->paddingLeft() + self->borderLeft());
}

void LineFormattingContext::breakLineForInlineNonReplacedBox(FrameLineBreak* br)
{
    InlineNonReplacedBox* self =
        m_currentLayoutParent->asInlineNonReplacedBox();

    if (self->boxes().size() == 0 && self->layoutParent()->isLineBox()) {
        auto& boxes = currentLine()->boxes();
        boxes.erase(std::find(boxes.begin(), boxes.end(), self));

        m_currentLayoutParent = currentLine();
        breakLineForLineBox(br, false, false);
        m_currentLayoutParent = self;

        currentLine()->insertInlineBox(self);
        return;
    }

    finishLineForInlineNonReplacedBox(br, false);

    InlineNonReplacedBox* newSelf = new InlineNonReplacedBox(self, false);
    FrameBox* parent = self->layoutParent()->asFrameBox();
    InlineNonReplacedBox* current = newSelf;
    while (parent->isInlineNonReplacedBox()) {
        InlineNonReplacedBox* newInrb =
            new InlineNonReplacedBox(parent->asInlineNonReplacedBox(), false);
        newInrb->insertInlineBox(current);
        current = newInrb;
        parent = parent->layoutParent()->asFrameBox();
    }

    m_currentLayoutParent = currentLine();
    breakLineForLineBox(br, false, false);
    m_currentLayoutParent = newSelf;

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

void LineFormattingContext::registerRelativePositionedBoxes()
{
    auto iter = m_block->m_lineBoxes.begin();

    while (iter != m_block->m_lineBoxes.end()) {
        LineBox* lineBox = *iter;
        STARFISH_ASSERT(lineBox != nullptr);

        if (lineBox->boxes().size() == 0) {
            if (lineBox->height() == 0) {
                iter = m_block->m_lineBoxes.erase(iter);
                continue;
            }
        }

        lineBox->registerRelativePositionedBoxes(m_layoutContext);
        iter++;
    }
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

    LayoutUnit top = paddingTop() + borderTop();
    LayoutUnit bottom = paddingBottom() + borderBottom();
    MarginInfo marginInfo(top, bottom, isEstablishesBlockFormattingContext() ||
                                           isFrameDocument(),
                          style()->height());
    ctx.setMarginInfo(this, &marginInfo);
    LineFormattingContext lineFormattingContext(this, ctx);

    // compute directions
    lineFormattingContext.computeDirection(this, style()->direction());

    lineFormattingContext.m_currentLayoutParent =
        lineFormattingContext.currentLine();

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

    lineFormattingContext.registerRelativePositionedBoxes();

    return lineFormattingContext.contentHeightForBlock();
}

void InlineNonReplacedBox::layoutInline(LineFormattingContext& ctx)
{
    LayoutUnit inlineContentWidth = ctx.m_block->contentWidth();

    computeBorderMarginPadding(inlineContentWidth);
    setTopBottomOrgMBP();
    unsetTopBottomMBP();
    ctx.m_unprocessedStartingMBPWidth += startingMBPWidth();

    ctx.m_currentLayoutParent = this;
    ctx.layoutInline(origin());

    if (!ctx.isWordProcessing()) {
        ctx.finishLineForInlineNonReplacedBox(nullptr, true);
    }
}

void PreferredWidthContext::handleTextToken(TextToken& token)
{
    if (m_isPendingWrapLine) {
        breakLine(true);
    }

    if (token.m_type != WordType::General) {
        updateCurrentLineWidthByWordWidth();
    }

    if (token.m_type == WordType::CollapsibleWhiteSpace &&
        !token.m_frameText->shouldPreserveWhiteSpaces() &&
        isWhiteSpaceAtLast()) {
        return;
    }

    LayoutUnit w = token.width();
    if (token.m_type != WordType::CollapsibleWhiteSpace) {
        if (token.m_type == WordType::General) {
            updatePreferredMinWidth(w);
        }
        w += m_unprocessedStartingMBPWidth;
    }

    setIsWhiteSpaceAtLast(token.m_type == WordType::CollapsibleWhiteSpace, w);
    updateCurrentLineWidth(token.m_frameText, w, token.m_type);
}

void PreferredWidthContext::updateUnprocessedStartingMBPWidth(Frame* f)
{
    m_unprocessedStartingMBPWidth += f->startingMBPWidth();
}

void PreferredWidthContext::updateCurrentLineWidth(Frame* f, LayoutUnit w,
                                                   WordType type)
{
    if (f->isFloating()) {
        if (canInsertFloatingBox(f)) {
            if (dontBreakLine(w)) {
                m_currentLineWidth += w;
            } else {
                breakLine(true);
                m_currentLineWidth = w;
            }
        } else {
            breakLine(false);
            m_hasFloat = HasNone;
            m_currentLineWidth = w;
        }
    } else if (type == WordType::ForcedNewline) {
        // In compute preferred width, even if trying break line here, it is
        // okay. Because we care only the longest width not the exact layout
        // result.
        breakLine(false);
    } else if (f->shouldWrapLines() && !f->isDirectDescendantOfTableCellBox()) {
        if (f->isFrameText() && type == WordType::General) {
            m_wordWidth += w;
        } else if (dontBreakLine(w)) {
            m_currentLineWidth += w;
        } else {
            if (type == WordType::CollapsibleWhiteSpace) {
                m_currentLineWidth += w;
                m_isPendingWrapLine = true;
            } else {
                breakLine(true);
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

LayoutUnit PreferredWidthContext::computeMinimumWidthDueToMBP(
    ComputedStyle* style)
{
    LayoutUnit minWidth;
    if (style->borderLeftWidth().isFixed()) {
        minWidth += style->borderLeftWidth().fixed();
    }
    if (style->borderRightWidth().isFixed()) {
        minWidth += style->borderRightWidth().fixed();
    }
    if (style->paddingLeft().isFixed()) {
        minWidth += style->paddingLeft().fixed();
    }
    if (style->paddingRight().isFixed()) {
        minWidth += style->paddingRight().fixed();
    }
    if (style->marginLeft().isFixed()) {
        minWidth += style->marginLeft().fixed();
    }
    if (style->marginRight().isFixed()) {
        minWidth += style->marginRight().fixed();
    }
    return minWidth;
}

LayoutUnit PreferredWidthContext::preferredWidthWithNewContext(Frame* f)
{
    LayoutUnit mbp =
        PreferredWidthContext::computeMinimumWidthDueToMBP(f->style());
    PreferredWidthContext newCtx(m_layoutContext, m_remainedWidth - mbp);
    f->computePreferredWidth(newCtx);

    return newCtx.preferredWidth() + mbp;
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
            updateCurrentLineWidthByWordWidth();

            LayoutUnit w = preferredWidthWithNewContext(f);

            if (f->isFloating()) {
                handleFloatingBox(f, w);
            } else {
                setIsWhiteSpaceAtLast(false, 0);
                updateCurrentLineWidth(f, w + m_unprocessedStartingMBPWidth);
            }
        } else {
            f->computePreferredWidth(*this);
        }

        f = f->next();
    }
}

String* FrameText::text()
{
    return node()->asCharacterData()->data();
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

    ctx.setCurrentLineWidth(ctx.currentLineWidth() + endingMBPWidth());
}

void FrameReplaced::computePreferredWidth(PreferredWidthContext& ctx)
{
    if (ctx.isPendingWrapLine()) {
        ctx.breakLine(true);
    }

    ctx.updateCurrentLineWidthByWordWidth();

    LayoutUnit parentContentWidth =
        ctx.layoutContext().blockContainer(this)->contentWidth();
    LayoutUnit intrinsicWidth, intrinsicHeight;
    bool hasAspectRatio;
    Length parentContentHeight;
    if (ctx.layoutContext().parentHasFixedHeight(this)) {
        parentContentHeight =
            Length(Length::Fixed, ctx.layoutContext().parentFixedHeight(this));
    } else {
        parentContentHeight = Length(Length::Auto);
    }
    computeIntrinsicSize(intrinsicWidth, intrinsicHeight, hasAspectRatio,
                         parentContentWidth, parentContentHeight);
    LayoutUnit w;

    if (style()->width().isAuto() && style()->height().isAuto()) {
        w = intrinsicWidth;
    } else if (style()->width().isSpecified()) {
        if (style()->width().isFixed()) {
            w = style()->width().fixed();
        } else {
            w = intrinsicWidth;
        }
    } else if (style()->height().isSpecified()) {
        if (style()->height().isFixed()) {
            LayoutUnit h = style()->height().fixed();
            if (hasAspectRatio) {
                w = h * (intrinsicWidth / intrinsicHeight);
            } else {
                w = intrinsicWidth;
            }
        } else {
            w = intrinsicWidth;
        }
    } else {
        w = intrinsicWidth;
    }

    LayoutUnit mbp =
        PreferredWidthContext::computeMinimumWidthDueToMBP(style());
    w += mbp;
    ctx.updatePreferredMinWidth(w);

    if (isFloating()) {
        ctx.handleFloatingBox(this, w);
    } else {
        ctx.setIsWhiteSpaceAtLast(false, 0);
        ctx.updateCurrentLineWidth(this, w + ctx.unprocessedStartingMBPWidth());
    }
}

void FrameLineBreak::computePreferredWidth(PreferredWidthContext& ctx)
{
    ctx.updateCurrentLineWidthByWordWidth();

    if (ctx.isPendingWrapLine()) {
        if (dontClear(ctx.hasFloat(), this)) {
            ctx.breakLine(true);
        }
    } else {
        ctx.breakLine(false);
    }
}

void FrameBlockBox::computePreferredWidth(PreferredWidthContext& ctx)
{
    if (!isNecessaryBlockBox()) {
        return;
    }

    if (ctx.isPendingWrapLine()) {
        ctx.breakLine(true);
    }

    ctx.updateCurrentLineWidthByWordWidth();

    LayoutUnit w;
    if (style()->width().isSpecified() && !isFrameTableCellBox()) {
        if (style()->width().isFixed()) {
            w = style()->width().fixed();
        } else {
            LayoutUnit mbp =
                PreferredWidthContext::computeMinimumWidthDueToMBP(style());
            LayoutUnit parentContentWidth =
                ctx.layoutContext().parentContentWidth(this);
            w = parentContentWidth * style()->width().percent() - mbp;
        }

        ctx.updatePreferredWidth(w);
    } else {
        if (hasBlockFlow()) {
            Frame* f = firstChild();
            while (f) {
                STARFISH_ASSERT(f->isNormalFlow());
                w = std::max(w, ctx.preferredWidthWithNewContext(f));

                f = f->next();
            }

            ctx.updatePreferredWidth(w);
        } else {
            Length textIndent = style()->textIndent();
            LayoutUnit textIndentWidth =
                textIndent.specifiedValue(ctx.remainedWidth());
            ctx.setCurrentLineWidth(textIndentWidth);
            ctx.setTextIndentWidth(textIndentWidth);
            ctx.computePreferredWidthInline(this);
        }
    }

    ctx.finishLine(false);
}

void FrameTableBox::computePreferredWidth(PreferredWidthContext& ctx)
{
    if (ctx.isPendingWrapLine()) {
        ctx.breakLine(true);
    }

    LayoutUnit borderSpacing = calculatedHorizontalBorderSpacing();

    LayoutUnit parentContentWidth =
        ctx.layoutContext().parentContentWidth(this);
    LayoutUnit tablePreferredWidth = 0;
    LayoutUnit tablePreferredMinWidth = 0;
    if (style()->width().isAuto()) {
        calCellWidth(ctx.layoutContext());
        tablePreferredWidth += borderSpacing;
        tablePreferredMinWidth += borderSpacing;

        for (auto& col : columnWidths()) {
            if (isCellWidthAuto(col.id)) {
                tablePreferredWidth += col.maxCellWidth;
                tablePreferredMinWidth += col.minCellWidth;
            } else {
                tablePreferredWidth += col.cellWidth;
                tablePreferredMinWidth += col.cellWidth;
            }

            tablePreferredWidth += borderSpacing;
            tablePreferredMinWidth += borderSpacing;
        }
    } else {
        if (style()->width().isFixed()) {
            tablePreferredWidth =
                LayoutUnit::fromPixel(style()->width().fixed());

            if (!isAnonymous() && node()->isHTMLTableElement()) {
                tablePreferredWidth -= borderWidth() + paddingWidth();
            }
            tablePreferredMinWidth = tablePreferredWidth;
        } else if (style()->width().isPercent()) {
            LayoutUnit tableWidth =
                parentContentWidth.toInt() * style()->width().percent();
            tableWidth -= borderWidth() + paddingWidth();
            tablePreferredWidth = tablePreferredMinWidth = tableWidth;
        }
    }

    if (node() && node()->isHTMLTableElement()) {
        LayoutUnit widthAttribute = widthFromAttribute(parentContentWidth);
        if (widthAttribute > 0) {
            widthAttribute -= borderWidth() + paddingWidth();
            tablePreferredWidth = tablePreferredMinWidth = widthAttribute;
        }
    }

    ctx.updatePreferredMinWidth(tablePreferredMinWidth);
    ctx.updateCurrentLineWidth(this, tablePreferredWidth +
                                         ctx.unprocessedStartingMBPWidth());
    ctx.finishLine(false);
}

void FrameBlockBox::paintChildrenWith(PaintingContext& ctx)
{
    if (hasBlockFlow()) {
        FrameBox::paintChildrenWith(ctx);
    } else {
        for (size_t i = 0; i < m_lineBoxes.size(); i++) {
            PaintingInlineStage s = PaintingInlineLevelElements;
            while (s != PaintingInlineStageEnd) {
                ctx.m_paintingInlineStage = s;
                ctx.m_canvas->save();
                LineBox& b = *m_lineBoxes[i];
                ctx.m_canvas->translate(b.frameRect().x(), b.frameRect().y());
                for (size_t k = 0; k < b.m_boxes.size(); k++) {
                    FrameBox* childBox = b.m_boxes[k];
                    ctx.m_canvas->save();
                    ctx.m_canvas->translate(childBox->x(), childBox->y());
                    childBox->paint(ctx);
                    ctx.m_canvas->restore();
                }
                ctx.m_canvas->restore();
                s = (PaintingInlineStage)(s + 1);
            }
        }
    }
}

FrameText* InlineTextBox::origin()
{
    return node()->frame()->asFrameText();
}

void InlineTextBox::paint(PaintingContext& ctx)
{
    if (ctx.m_paintingInlineStage == PaintingInlineLevelElements) {
        if (ctx.m_paintingStage == PaintingNormalFlowInline) {
            if (style()->visibility() ==
                VisibilityValue::HiddenVisibilityValue) {
                ctx.m_canvas->setVisible(false);
            } else {
                ctx.m_canvas->setVisible(true);
            }

            ctx.m_canvas->setFont(style()->font());
            ctx.m_canvas->setColor(style()->color());
            ctx.m_canvas->drawText(0, 0, contentWidth(), m_text);
        }
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
            frameBoxRareData()->m_padding = m_orgPadding;
            frameBoxRareData()->m_margin = m_orgMargin;
            frameBoxRareData()->m_border = m_orgBorder;
        } else {
            m_layoutParent = (Frame*)&fakeRareData;
            fakeRareData.m_padding = m_orgPadding;
            fakeRareData.m_margin = m_orgMargin;
            fakeRareData.m_border = m_orgBorder;
        }

        canvas->save();
        canvas->translate(LayoutUnit(0),
                          m_ascender - (style()->font()->metrics().m_ascender) -
                              borderTop() - paddingTop());
        setContentHeight(style()->font()->metrics().m_ascender -
                         style()->font()->metrics().m_descender);
        setHeight(contentHeight() + paddingHeight() + borderHeight());
        FrameBox::paintBackgroundAndBorders(canvas);
        canvas->restore();

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

void InlineNonReplacedBox::paint(PaintingContext& ctx)
{
    if (isEstablishesStackingContext()) {
        ctx.m_canvas->saveByFrame(this);
        return;
    }

    if (!isNormalFlow() || style()->display() == InlineBlockDisplayValue ||
        style()->display() == InlineTableDisplayValue) {
        ctx.m_canvas->resetTextDecorationData();
    } else {
        ctx.m_canvas->mergeTextDecorationData(style());
    }

    // CHECK THIS at https://www.w3.org/TR/CSS2/zindex.html#stacking-defs
    if (isPositioned()) {
        if (ctx.m_paintingStage == PaintingPositionedElements) {
            paintBackgroundAndBorders(ctx.m_canvas);
            PaintingStage s = PaintingStage::PaintingNormalFlowBlock;
            while (s != PaintingStageEnd) {
                ctx.m_paintingStage = s;
                paintChildrenWith(ctx);
                s = (PaintingStage)(s + 1);
            }
            ctx.m_paintingStage = PaintingPositionedElements;
        }
    } else if (isFloating()) {
        if (ctx.m_paintingStage == PaintingNonPositionedFloats) {
            paintBackgroundAndBorders(ctx.m_canvas);
            PaintingStage s = PaintingStage::PaintingNormalFlowBlock;
            while (s != PaintingStageEnd) {
                ctx.m_paintingStage = s;
                paintChildrenWith(ctx);
                s = (PaintingStage)(s + 1);
            }
            ctx.m_paintingStage = PaintingNonPositionedFloats;
        }
    } else if (ctx.m_paintingStage == PaintingNormalFlowInline &&
               ctx.m_paintingInlineStage == PaintingInlineLevelElements) {
        paintBackgroundAndBorders(ctx.m_canvas);
        paintChildrenWith(ctx);
    } else {
        paintChildrenWith(ctx);
    }
}

void InlineNonReplacedBox::paintChildrenWith(PaintingContext& ctx)
{
    auto iter = boxes().begin();
    while (iter != boxes().end()) {
        FrameBox* child = *iter;
        ctx.m_canvas->save();
        ctx.m_canvas->translate(child->asFrameBox()->x(),
                                child->asFrameBox()->y());
        child->paint(ctx);
        ctx.m_canvas->restore();
        iter++;
    }
}

Frame* InlineNonReplacedBox::hitTest(LayoutUnit x, LayoutUnit y,
                                     HitTestStage stage)
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
