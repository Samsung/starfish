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

#include "StarFishConfig.h"
#include "FrameBlockBox.h"
#include "FrameText.h"
#include "FrameInline.h"

namespace StarFish {

static LayoutUnit computeLineHeight(ComputedStyle* style)
{
    LayoutUnit fontSize = style->font()->metrics().m_ascender - style->font()->metrics().m_descender;

    if (!style->hasNormalLineHeight()) {
        if (style->lineHeight().isFixed()) {
            return style->lineHeight().fixed();
        } else if (style->lineHeight().isInheritableNumber()) {
            return fontSize * style->lineHeight().number();
        } else {
            // Only Fixed | InheritableNumer possible here
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }
    return fontSize;
}

static LayoutUnit computeVerticalProperties(FrameBox* parentBox, ComputedStyle* parentStyle, LayoutUnit& ascenderInOut, LayoutUnit& descenderInOut, LineFormattingContext& ctx, bool dueToBr, bool isLineBox)
{
    LayoutUnit maxAscender = ascenderInOut;
    LayoutUnit maxDescender = descenderInOut;
    bool hasBoxOtherThanText = false;
    bool hasNormalFlowChild = false;

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

    if (isLineBox) {
        if (!boxes->size()) {
            if (dueToBr) {
                ascenderInOut = maxAscender;
                descenderInOut = maxDescender;
                return ascenderInOut - descenderInOut;
            }
            ascenderInOut = 0;
            descenderInOut = 0;
            return 0;
        }

        bool hasOnlyCollapsedInlineNonReplacedBox = true;
        for (size_t i = 0; i < boxes->size(); i ++) {
            if (boxes->at(i)->isInlineBox() && boxes->at(i)->asInlineBox()->isInlineNonReplacedBox() && boxes->at(i)->asInlineBox()->asInlineNonReplacedBox()->isCollapsed()) {

            } else {
                hasOnlyCollapsedInlineNonReplacedBox = false;
                break;
            }
        }

        if (hasOnlyCollapsedInlineNonReplacedBox) {
            ascenderInOut = 0;
            descenderInOut = 0;
            return 0;
        }
    }

    LayoutUnit pascender = parentStyle->font()->metrics().m_ascender;
    LayoutUnit pdescender = parentStyle->font()->metrics().m_descender;

    // 1. set relative y-pos from baseline (only if it needs)
    // 2. find max ascender and descender
    LayoutUnit maxAscenderSoFar = 0;
    LayoutUnit maxDescenderSoFar = intMaxForLayoutUnit;
    for (size_t k = 0; k < boxes->size(); k ++) {
        FrameBox* box = boxes->at(k);
        if (!box->isNormalFlow()) {
            continue;
        } else {
            hasNormalFlowChild = true;
        }
        VerticalAlignValue va = box->style()->verticalAlign();
        if (box->isInlineBox()) {
            if (box->asInlineBox()->isInlineTextBox()) {
            } else {
                hasBoxOtherThanText = true;
                InlineNonReplacedBox* rb = box->asInlineBox()->asInlineNonReplacedBox();
                if (va == VerticalAlignValue::BaselineVAlignValue) {
                    maxAscenderSoFar = std::max(rb->ascender(), maxAscenderSoFar);
                    maxDescenderSoFar = std::min(rb->decender(), maxDescenderSoFar);
                } else if (va == VerticalAlignValue::TopVAlignValue) {
                    // NO WORK TO DO
                } else if (va == VerticalAlignValue::BottomVAlignValue) {
                    // NO WORK TO DO
                } else if (va == VerticalAlignValue::MiddleVAlignValue) {
                    LayoutUnit halfHeight = rb->height() / 2;
                    LayoutUnit halfXHeight = (parentStyle->font()->metrics().m_xheightRate * parentStyle->font()->size()) / 2;
                    rb->setY(halfHeight + halfXHeight);
                    maxAscenderSoFar = std::max(halfHeight + halfXHeight, maxAscenderSoFar);
                    maxDescenderSoFar = std::min(-1 * (halfHeight - halfXHeight), maxDescenderSoFar);
                } else if (va == VerticalAlignValue::SubVAlignValue) {
                    rb->setY(pdescender + rb->ascender());
                    maxAscenderSoFar = std::max(pdescender + rb->ascender(), maxAscenderSoFar);
                    maxDescenderSoFar = std::min(pdescender + rb->decender(), maxDescenderSoFar);
                } else if (va == VerticalAlignValue::SuperVAlignValue) {
                    // Placing a superscript is font and browser dependent.
                    // We place superscript above the baseline by 1/2 of ascender (following blink)
                    // (i.e, the baseline of superscript is aligned with 1/2 of the ascender)
                    rb->setY(pascender / 2 + rb->ascender());
                    maxAscenderSoFar = std::max(pascender / 2 + rb->ascender(), maxAscenderSoFar);
                    maxDescenderSoFar = std::min(pascender / 2 + rb->decender(), maxDescenderSoFar);
                } else if (va == VerticalAlignValue::TextTopVAlignValue) {
                    maxDescenderSoFar = std::min(pascender - rb->height(), maxDescenderSoFar);
                } else if (va == VerticalAlignValue::TextBottomVAlignValue) {
                    maxAscenderSoFar = std::max(pdescender + rb->height(), maxAscenderSoFar);
                } else if (va == VerticalAlignValue::NumericVAlignValue) {
                    Length len = box->style()->verticalAlignLength();
                    LayoutUnit y;
                    if (len.isPercent()) {
                        y = rb->ascender() + computeLineHeight(box->style()) * len.percent();
                    } else if (len.isFixed()) {
                        y = rb->ascender() + LayoutUnit::fromPixel(len.fixed());
                    }
                    maxAscenderSoFar = std::max(y, maxAscenderSoFar);
                    maxDescenderSoFar = std::min(y - rb->height(), maxDescenderSoFar);
                    rb->setY(y);
                }  else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
            }
        } else if (box->isFrameReplaced()) {
            hasBoxOtherThanText = true;
            LayoutUnit boxHeight = box->height() + box->marginHeight();
            if (va == VerticalAlignValue::BaselineVAlignValue) {
                maxAscenderSoFar = std::max(boxHeight, maxAscenderSoFar);
            } else if (va == VerticalAlignValue::TopVAlignValue) {
                // NO WORK TO DO
            } else if (va == VerticalAlignValue::BottomVAlignValue) {
                // NO WORK TO DO
            } else if (va == VerticalAlignValue::MiddleVAlignValue) {
                LayoutUnit halfHeight = boxHeight / 2;
                LayoutUnit halfXHeight = (parentStyle->font()->metrics().m_xheightRate * parentStyle->font()->size()) / 2;
                box->setY(halfHeight + halfXHeight);
                maxAscenderSoFar = std::max(halfHeight + halfXHeight, maxAscenderSoFar);
                maxDescenderSoFar = std::min(-1 * (halfHeight - halfXHeight), maxDescenderSoFar);
            } else if (va == VerticalAlignValue::SubVAlignValue) {
                box->setY(pdescender + boxHeight);
                maxAscenderSoFar = std::max(pdescender + boxHeight, maxAscenderSoFar);
            } else if (va == VerticalAlignValue::SuperVAlignValue) {
                box->setY(pascender / 2 + boxHeight);
                maxAscenderSoFar = std::max(pascender / 2 + boxHeight, maxAscenderSoFar);
            } else if (va == VerticalAlignValue::TextTopVAlignValue) {
                maxDescenderSoFar = std::min(pascender - boxHeight, maxDescenderSoFar);
            } else if (va == VerticalAlignValue::TextBottomVAlignValue) {
                maxAscenderSoFar = std::max(pdescender + boxHeight, maxAscenderSoFar);
            } else if (va == VerticalAlignValue::NumericVAlignValue) {
                Length len = box->style()->verticalAlignLength();
                LayoutUnit amount;
                if (len.isPercent()) {
                    amount = boxHeight + computeLineHeight(box->style()) * len.percent();
                } else if (len.isFixed()) {
                    amount = boxHeight + LayoutUnit::fromPixel(len.fixed());
                }
                maxAscenderSoFar = std::max(amount, maxAscenderSoFar);
                maxDescenderSoFar = std::min(amount - boxHeight, maxDescenderSoFar);
                box->setY(amount);
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        } else if (box->isFrameBlockBox() && box->style()->display() == InlineBlockDisplayValue) {
            hasBoxOtherThanText = true;
            LayoutUnit boxHeight = box->height() + box->marginHeight();
            if (va == VerticalAlignValue::BaselineVAlignValue) {
                LayoutUnit ascender = ctx.inlineBlockAscender(box->asFrameBlockBox());
                if (ascender == box->height()) {
                    maxAscenderSoFar = std::max(ascender + box->marginHeight(), maxAscenderSoFar);
                    maxDescenderSoFar = std::min(LayoutUnit(0), maxDescenderSoFar);
                } else {
                    LayoutUnit descender = -(box->height() - ascender) - box->marginBottom();
                    maxAscenderSoFar = std::max(ascender + box->marginTop(), maxAscenderSoFar);
                    maxDescenderSoFar = std::min(descender, maxDescenderSoFar);
                }
            } else if (va == VerticalAlignValue::TopVAlignValue) {
                // NO WORK TO DO
            } else if (va == VerticalAlignValue::BottomVAlignValue) {
                // NO WORK TO DO
            } else if (va == VerticalAlignValue::MiddleVAlignValue) {
                LayoutUnit halfHeight = boxHeight / 2;
                LayoutUnit halfXHeight = (parentStyle->font()->metrics().m_xheightRate * parentStyle->font()->size()) / 2;
                box->setY(halfHeight + halfXHeight);
                maxAscenderSoFar = std::max(halfHeight + halfXHeight, maxAscenderSoFar);
                maxDescenderSoFar = std::min(-1 * (halfHeight - halfXHeight), maxDescenderSoFar);
            } else if (va == VerticalAlignValue::SubVAlignValue) {
                // TODO : Need Implement Here
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            } else if (va == VerticalAlignValue::SuperVAlignValue) {
                // TODO : Need Implement Here
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            } else if (va == VerticalAlignValue::TextTopVAlignValue) {
                maxDescenderSoFar = std::min(pascender - boxHeight, maxDescenderSoFar);
            } else if (va == VerticalAlignValue::TextBottomVAlignValue) {
                maxAscenderSoFar = std::max(pdescender + boxHeight, maxAscenderSoFar);
            } else if (va == VerticalAlignValue::NumericVAlignValue) {
                LayoutUnit ascender = ctx.inlineBlockAscender(box->asFrameBlockBox());
                Length len = box->style()->verticalAlignLength();
                LayoutUnit amount;
                if (len.isPercent()) {
                    amount = ascender + computeLineHeight(box->style()) * len.percent();
                } else if (len.isFixed()) {
                    amount = ascender + LayoutUnit::fromPixel(len.fixed());
                }
                maxAscenderSoFar = std::max(amount, maxAscenderSoFar);
                maxDescenderSoFar = std::min(amount - ascender, maxDescenderSoFar);
                box->setY(amount);
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }
    }

    if (!isLineBox && !hasNormalFlowChild && parentBox->width() == 0 && parentBox->marginLeft() == 0 && parentBox->marginRight() == 0 && parentBox->style()->hasNormalLineHeight()) {
        parentBox->asInlineBox()->asInlineNonReplacedBox()->markCollapsed();
        ascenderInOut = pascender;
        descenderInOut = pdescender;
        return pascender - pdescender;
    }

    if (!hasNormalFlowChild && boxes->size() != 0) {
        ascenderInOut = descenderInOut = 0;
        return 0;
    }

    // Consider parent's font ascender/descender
    maxAscender = std::max(maxAscenderSoFar, pascender);
    maxDescender = std::min(maxDescenderSoFar, pdescender);

    // If maxDescenderSoFar is initial value(=intMaxForLayoutUnit), set it to 0.
    maxDescenderSoFar = maxDescenderSoFar.toInt() == intMaxForLayoutUnit ? LayoutUnit(0) : maxDescenderSoFar;

    // 3. adjusting the line height
    if (!parentStyle->hasNormalLineHeight()) {
        LayoutUnit lineHeight = computeLineHeight(parentStyle);
        LayoutUnit diff = (lineHeight - parentStyle->font()->metrics().m_fontHeight) / 2;
        LayoutUnit ascenderShouldBe = parentStyle->font()->metrics().m_ascender + diff;
        LayoutUnit descenderShouldBe = parentStyle->font()->metrics().m_descender - diff;

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
    for (size_t k = 0; k < boxes->size(); k ++) {
        FrameBox* box = boxes->at(k);
        VerticalAlignValue va = box->style()->verticalAlign();

        // Ignore non normalFlow
        if (!box->isNormalFlow())
            continue;

        // Ignore inlineBox's marginHeight
        LayoutUnit marginHeight;
        if (!box->isInlineBox())
            marginHeight = box->marginHeight();

        // Update maxAsc & maxDes
        if (!(box->isInlineBox() && box->asInlineBox()->isInlineTextBox())) {
            if (va == VerticalAlignValue::TopVAlignValue) {
                maxDescender = std::min(maxAscender - (box->height() + marginHeight), maxDescender);
            } else if (va == VerticalAlignValue::BottomVAlignValue) {
                maxAscender = std::max(maxDescender + (box->height() + marginHeight), maxAscender);
            }
        }
    }

    ascenderInOut = maxAscender;
    descenderInOut = maxDescender;
    LayoutUnit height = maxAscender - maxDescender;
    for (size_t k = 0; k < boxes->size(); k ++) {
        FrameBox* f = boxes->at(k);

        if (f->isNormalFlow()) {
            if (f->isInlineBox() && f->asInlineBox()->isInlineTextBox()) {
                // InlineBox* ib = f->asFrameBox()->asInlineBox();
                // if (UNLIKELY(descenderInOut == 0)) {
                //     if (height < ib->asInlineTextBox()->style()->font()->metrics().m_fontHeight)
                //         ib->setY((height - ib->asInlineTextBox()->style()->font()->metrics().m_fontHeight + ib->asInlineTextBox()->style()->font()->metrics().m_descender) / 2);
                //     else
                //         ib->setY(height - ib->asInlineTextBox()->style()->font()->metrics().m_fontHeight - ib->asInlineTextBox()->style()->font()->metrics().m_descender);
                // } else {
                //     ib->setY(height + maxDescender - ib->height() - ib->asInlineTextBox()->style()->font()->metrics().m_descender);
                // }
                InlineTextBox* ib = f->asFrameBox()->asInlineBox()->asInlineTextBox();
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
                    if (f->isInlineBox() && f->asInlineBox()->isInlineNonReplacedBox()) {
                        f->setY(height + maxDescender - f->height() - f->asInlineBox()->asInlineNonReplacedBox()->decender());
                    } else if (f->isFrameReplaced()) {
                        // TODO use this code for when replaced content does not have content
                        /*
                           LayoutUnit asc = ib->marginTop() + ib->asInlineReplacedBox()->replacedBox()->borderTop() + ib->asInlineReplacedBox()->replacedBox()->paddingTop()
                           + ib->asInlineReplacedBox()->replacedBox()->contentHeight();
                           ib->setY(height + maxDescender - asc + ib->marginTop());
                           */
                        f->setY(maxAscender - f->height() - marginBottom);
                    } else {
                        STARFISH_ASSERT(f->isFrameBlockBox() && f->style()->display() == InlineBlockDisplayValue);
                        LayoutUnit ascender = ctx.inlineBlockAscender(f->asFrameBlockBox());
                        if (ascender == f->height()) {
                            f->setY(maxAscender - ascender - marginBottom);
                        } else {
                            f->setY(maxAscender - ascender);
                        }
                    }
                // 4. convert a y pos relative to the baseline to a y pos relative to the top-left corner of the box
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
                    f->setY(maxAscender - pascender + marginTop);
                } else if (va == VerticalAlignValue::TextBottomVAlignValue) {
                    f->setY(maxAscender - pdescender - f->height() - marginBottom);
                } else if (va == VerticalAlignValue::NumericVAlignValue) {
                    f->setY(maxAscender - f->y() + marginTop);
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

static FrameBox* findLastInlineBoxNonReplacedBoxCase(InlineNonReplacedBox* b)
{
    FrameBox* result = nullptr;

    auto iter = b->boxes().begin();
    while (iter != b->boxes().end()) {
        FrameBox* f = *iter;
        if (f->asFrameBox()->isInlineBox()) {
            InlineBox* ib = f->asFrameBox()->asInlineBox();
            if (ib->isInlineNonReplacedBox()) {
                auto r = findLastInlineBoxNonReplacedBoxCase(ib->asInlineNonReplacedBox());
                if (r) {
                    result = r;
                }
            } else {
                result = ib;
            }
        } else if (f->isNormalFlow()) {
            result = *iter;
        }
        iter++;
    }

    return result;
}

static FrameBox* findLastInlineBox(LineBox* lb)
{
    FrameBox* result = nullptr;
    for (size_t i = 0; i < lb->boxes().size(); i ++) {
        if (lb->boxes()[i]->isInlineBox()) {
            InlineBox* b = lb->boxes()[i]->asInlineBox();
            if (b->isInlineNonReplacedBox()) {
                auto r = findLastInlineBoxNonReplacedBoxCase(b->asInlineNonReplacedBox());
                if (r) {
                    result = r;
                }
            } else {
                result = b;
            }
        } else if (lb->boxes()[i]->isNormalFlow()) {
            result = lb->boxes()[i];
        }
    }

    return result;
}

static char charDirection(char32_t c)
{
    auto property = u_getIntPropertyValue(c, UCHAR_BIDI_CLASS);
    if ((property == U_RIGHT_TO_LEFT) || (property == U_RIGHT_TO_LEFT_ARABIC) || (property == U_RIGHT_TO_LEFT_EMBEDDING) || (property == U_RIGHT_TO_LEFT_OVERRIDE)) {
        return 0;
    } else if ((property == U_LEFT_TO_RIGHT) || (property == U_LEFT_TO_RIGHT_EMBEDDING) || (property == U_LEFT_TO_RIGHT_OVERRIDE)) {
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

static UBiDiDirection getTextDir(String* text)
{
    UTF16String str = text->toUTF16String();
    UBiDiDirection dir = ubidi_getBaseDirection((const UChar*)str.data(), str.length());
    return dir;
}

static bool isNumberChar(char32_t d)
{
    if (('0' <= d && d <= '9')
        || (0x0660 <= d && d <= 0x0669) // 0 to 9 in Arabic
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

static bool hasIsolateBidiContent(InlineNonReplacedBox* box)
{
    // TODO add isolate
    if (box->style()->unicodeBidi() == UnicodeBidiValue::EmbedUnicodeBidiValue) {
        return true;
    }
    return false;
}

static FrameBox* fetchContentForResolveBidi(FrameBox* box)
{
    if (box->isInlineBox()) {
        if (box->asInlineBox()->isInlineNonReplacedBox()) {
            InlineNonReplacedBox* b = box->asInlineBox()->asInlineNonReplacedBox();
            if (hasIsolateBidiContent(b)) {
                return b;
            }
            if (b->boxes().size()) {
                STARFISH_ASSERT(b->boxes().size() == 1);
                return fetchContentForResolveBidi(b->boxes().at(0));
            }
            return b;
        } else {
            return box;
        }
    } else {
        return box;
    }
}

void splitInlineBoxesAndMarkDirectionForResolveBidi(LineFormattingContext& ctx, DirectionValue parentDir, std::vector<FrameBox*, gc_allocator<FrameBox*>>& boxes)
{
    for (size_t i = 0; i < boxes.size(); i ++) {
        FrameBox* box = boxes[i];
        if (box->isInlineBox()) {
            InlineBox* ib = box->asInlineBox();
            if (ib->isInlineTextBox()) {
            } else if (ib->isInlineNonReplacedBox()) {
                if (!hasIsolateBidiContent(ib->asInlineNonReplacedBox())) {
                    splitInlineBoxesAndMarkDirectionForResolveBidi(ctx, parentDir, ib->asInlineNonReplacedBox()->boxes());
                    // split every content for resolve bidi after
                    InlineNonReplacedBox* inrb = ib->asInlineNonReplacedBox();
                    STARFISH_ASSERT(ctx.m_dataForRestoreLeftRightOfMBPAfterResolveBidiLinePerLine.find(inrb->origin()) == ctx.m_dataForRestoreLeftRightOfMBPAfterResolveBidiLinePerLine.end());

                    DataForRestoreLeftRightOfMBPAfterResolveBidiLinePerLine mbpData;

                    mbpData.m_margin = inrb->m_margin;
                    mbpData.m_border = inrb->m_border;
                    mbpData.m_padding = inrb->m_padding;
                    mbpData.m_orgMargin = inrb->m_orgMargin;
                    mbpData.m_orgBorder = inrb->m_orgBorder;
                    mbpData.m_orgPadding = inrb->m_orgPadding;

                    ctx.m_dataForRestoreLeftRightOfMBPAfterResolveBidiLinePerLine[inrb->origin()] = mbpData;

                    std::vector<FrameBox*, gc_allocator<FrameBox*>>& boxesNeedsCopy = inrb->boxes();
                    if (boxesNeedsCopy.size()) {
                        FrameBox* parent = ib->layoutParent()->asFrameBox();
                        LayoutUnit x = box->x();

                        STARFISH_ASSERT(boxesNeedsCopy.size());
                        boxes.erase(boxes.begin() + i);

                        size_t insertPos = i;

                        for (size_t j = 0; j < boxesNeedsCopy.size(); j++) {
                            InlineNonReplacedBox* newBox = new InlineNonReplacedBox(ib->node(), ib->style(), nullptr, inrb->origin());
                            newBox->setX(boxesNeedsCopy[j]->x() + ib->x());
                            newBox->setY(ib->y());
                            boxesNeedsCopy[j]->setX(0);
                            boxesNeedsCopy[j]->setLayoutParent(newBox);
                            newBox->boxes().push_back(boxesNeedsCopy[j]);
                            newBox->setLayoutParent(parent);
                            if (boxesNeedsCopy[j]->isNormalFlow()) {
                                newBox->setWidth(boxesNeedsCopy[j]->width());
                            }
                            newBox->setHeight(ib->height());
                            newBox->setAscender(inrb->ascender());
                            newBox->setDecender(inrb->decender());
                            newBox->m_orgMargin = inrb->m_orgMargin;
                            newBox->m_orgMargin.setLeft(0);
                            newBox->m_orgMargin.setRight(0);
                            newBox->m_orgBorder = inrb->m_orgBorder;
                            newBox->m_orgBorder.setLeft(0);
                            newBox->m_orgBorder.setRight(0);
                            newBox->m_orgPadding = inrb->m_orgPadding;
                            newBox->m_orgPadding.setLeft(0);
                            newBox->m_orgPadding.setRight(0);
                            boxes.insert(boxes.begin() + insertPos++, newBox);
                        }

                        i += (boxesNeedsCopy.size() - 1);
                    } else {
                        inrb->setWidth(0);
                        inrb->setMarginLeft(0);
                        inrb->setBorderLeft(0);
                        inrb->setPaddingLeft(0);
                        inrb->setMarginRight(0);
                        inrb->setBorderRight(0);
                        inrb->setPaddingRight(0);
                        inrb->m_margin.setLeft(0);
                        inrb->m_border.setLeft(0);
                        inrb->m_padding.setLeft(0);
                        inrb->m_margin.setRight(0);
                        inrb->m_border.setRight(0);
                        inrb->m_padding.setRight(0);
                    }
                }
            }
        }
    }
}

static CharDirection contentDir(FrameBox* box, LineFormattingContext& ctx)
{
    if (box->isInlineBox()) {
        InlineBox* ib = box->asInlineBox();
        if (ib->isInlineTextBox()) {
            return ib->asInlineTextBox()->charDirection();
        } else if (ib->isInlineNonReplacedBox()) {
            InlineNonReplacedBox* b = ib->asInlineNonReplacedBox();
            if (hasIsolateBidiContent(b)) {
                // TODO
                // when unicode-bidi property is isolate, we should return netural direction
                return b->style()->direction() == DirectionValue::LtrDirectionValue ? CharDirection::Ltr : CharDirection::Rtl;
            } else {
                const std::vector<FrameBox*, gc_allocator<FrameBox*>>& boxes = b->boxes();
                for (size_t i = 0; i < boxes.size(); i ++) {
                    CharDirection dir = contentDir(boxes[i], ctx);
                    if (dir == CharDirection::Ltr) {
                        return CharDirection::Ltr;
                    } else if (dir == CharDirection::Rtl) {
                        return CharDirection::Rtl;
                    } else { // netural
                    }
                }
                return CharDirection::Neutral;
            }
        } else {
            return CharDirection::Neutral;
        }
    } else if (box->isFrameBox()) {
        auto iter = ctx.m_computedDirectonValuePerFrameBox.find(box);
        if (iter != ctx.m_computedDirectonValuePerFrameBox.end()) {
            return (iter->second == DirectionValue::LtrDirectionValue) ? CharDirection::Ltr : CharDirection::Rtl;
        } else {
            return CharDirection::Neutral;
        }
    } else {
        return CharDirection::Neutral;
    }
}

void reassignLeftRightMBPOfInlineNonReplacedBoxPreProcess(LineFormattingContext& ctx, std::vector<FrameBox*, gc_allocator<FrameBox*>>& boxes)
{
    for (size_t i = 0; i < boxes.size(); i ++) {
        FrameBox* box = boxes[i];
        if (box->isInlineBox()) {
            InlineBox* ib = box->asInlineBox();
            if (ib->isInlineTextBox()) {
            } else if (ib->isInlineNonReplacedBox()) {
                if (!hasIsolateBidiContent(ib->asInlineNonReplacedBox())) {
                    reassignLeftRightMBPOfInlineNonReplacedBoxPreProcess(ctx, ib->asInlineNonReplacedBox()->boxes());

                    // find last box
                    FrameInline* origin = ib->asInlineNonReplacedBox()->origin();
                    ctx.m_checkLastInlineNonReplacedPerLine[origin] = ib->asInlineNonReplacedBox();
                }
            }
        }
    }
}

void reassignLeftRightMBPOfInlineNonReplacedBox(LineFormattingContext& ctx, std::vector<FrameBox*, gc_allocator<FrameBox*>>& boxes)
{
    for (size_t i = 0; i < boxes.size(); i ++) {
        FrameBox* box = boxes[i];
        if (box->isInlineBox()) {
            InlineBox* ib = box->asInlineBox();
            if (ib->isInlineTextBox()) {
            } else if (ib->isInlineNonReplacedBox()) {
                if (!hasIsolateBidiContent(ib->asInlineNonReplacedBox())) {
                    reassignLeftRightMBPOfInlineNonReplacedBox(ctx, ib->asInlineNonReplacedBox()->boxes());

                    // restore left
                    FrameInline* origin = ib->asInlineNonReplacedBox()->origin();
                    auto iter = ctx.m_dataForRestoreLeftRightOfMBPAfterResolveBidiLinePerLine.find(origin);

                    if (!iter->second.m_isFirstEdgeProcessed) {
                        ib->asInlineNonReplacedBox()->m_orgPadding.setLeft(iter->second.m_orgPadding.left());
                        ib->asInlineNonReplacedBox()->m_orgBorder.setLeft(iter->second.m_orgBorder.left());
                        ib->asInlineNonReplacedBox()->m_orgMargin.setLeft(iter->second.m_orgMargin.left());

                        ib->setMarginLeft(iter->second.m_margin.left());
                        ib->setBorderLeft(iter->second.m_border.left());
                        ib->setPaddingLeft(iter->second.m_padding.left());

                        LayoutUnit widthNeedToPropagateToParent;
                        widthNeedToPropagateToParent = iter->second.m_margin.left();
                        ib->moveX(widthNeedToPropagateToParent);
                        LayoutUnit bp = iter->second.m_border.left() + iter->second.m_padding.left();
                        widthNeedToPropagateToParent += bp;
                        if (ib->asInlineNonReplacedBox()->boxes().size()) {
                            STARFISH_ASSERT(ib->asInlineNonReplacedBox()->boxes().size() == 1);
                            ib->asInlineNonReplacedBox()->boxes()[0]->moveX(bp);
                        }
                        ib->setWidth(ib->width() + bp);

                        if (widthNeedToPropagateToParent) {
                            FrameBox* parent = ib->layoutParent()->asFrameBox();
                            while (parent->isInlineBox() && parent->asInlineBox()->isInlineNonReplacedBox()) {
                                parent->setWidth(parent->width() + widthNeedToPropagateToParent);
                                parent = parent->layoutParent()->asFrameBox();
                            }
                        }
                        iter->second.m_isFirstEdgeProcessed = true;
                    }

                    // restore right
                    if (ctx.m_checkLastInlineNonReplacedPerLine[origin] == ib) {
                        ib->asInlineNonReplacedBox()->m_orgPadding.setRight(iter->second.m_orgPadding.right());
                        ib->asInlineNonReplacedBox()->m_orgBorder.setRight(iter->second.m_orgBorder.right());
                        ib->asInlineNonReplacedBox()->m_orgMargin.setRight(iter->second.m_orgMargin.right());

                        ib->setMarginRight(iter->second.m_margin.right());
                        ib->setBorderRight(iter->second.m_border.right());
                        ib->setPaddingRight(iter->second.m_padding.right());

                        LayoutUnit widthNeedToPropagateToParent;
                        widthNeedToPropagateToParent = iter->second.m_margin.right();
                        LayoutUnit bp = iter->second.m_border.right() + iter->second.m_padding.right();
                        widthNeedToPropagateToParent += bp;
                        ib->setWidth(ib->width() + bp);

                        if (widthNeedToPropagateToParent) {
                            FrameBox* parent = ib->layoutParent()->asFrameBox();
                            while (parent->isInlineBox() && parent->asInlineBox()->isInlineNonReplacedBox()) {
                                parent->setWidth(parent->width() + widthNeedToPropagateToParent);
                                parent = parent->layoutParent()->asFrameBox();
                            }
                        }
                    }
                }
            }
        }
    }
}

static void resolveBidi(LineFormattingContext& ctx, DirectionValue parentDir, std::vector<FrameBox*, gc_allocator<FrameBox*>>& boxes)
{
    splitInlineBoxesAndMarkDirectionForResolveBidi(ctx, parentDir, boxes);

    if (parentDir == DirectionValue::RtlDirectionValue) {
        // find inline Text Boxes has only number for
        // <LTR> <Number> <RTL> case
        // Number following LTR Text should be LTR
        // <LTR> <Number> <RTL> -> <LTR> <Number-LTR not netural> <RTL>
        for (size_t i = 0; i < boxes.size(); i ++) {
            FrameBox* box = fetchContentForResolveBidi(boxes[i]);
            if (box->isInlineBox()) {
                InlineBox* ib = box->asInlineBox();
                if (ib->isInlineTextBox()) {
                    if (isNumber(ib->asInlineTextBox()->text(), 0, ib->asInlineTextBox()->text()->length())) {
                        for (size_t j = i - 1; j != SIZE_MAX; j--) {
                            FrameBox* box2 = fetchContentForResolveBidi(boxes[j]);
                            if (box2->isInlineBox()) {
                                InlineBox* ib2 = box2->asInlineBox();
                                if (ib2->isInlineTextBox()) {
                                    if (ib2->asInlineTextBox()->charDirection() == CharDirection::Ltr) {
                                        ib->asInlineTextBox()->setCharDirection(CharDirection::Ltr);
                                        break;
                                    } else if (ib2->asInlineTextBox()->charDirection() == CharDirection::Rtl) {
                                        break;
                                    }
                                }
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
        // <RTL> <Number> <LTR> -> <RTL> <Number-RTL not netural> <LTR>
        for (size_t i = 0; i < boxes.size(); i ++) {
            FrameBox* box = fetchContentForResolveBidi(boxes[i]);
            if (box->isInlineBox()) {
                InlineBox* ib = box->asInlineBox();
                if (ib->isInlineTextBox()) {
                    if (isNumber(ib->asInlineTextBox()->text(), 0, ib->asInlineTextBox()->text()->length())) {
                        for (size_t j = i - 1; j != SIZE_MAX; j--) {
                            FrameBox* box2 = fetchContentForResolveBidi(boxes[j]);
                            if (box2->isInlineBox()) {
                                InlineBox* ib2 = box2->asInlineBox();
                                if (ib2->isInlineTextBox()) {
                                    if (ib2->asInlineTextBox()->charDirection() == CharDirection::Rtl) {
                                        ib->asInlineTextBox()->setCharDirection(CharDirection::Rtl);
                                        break;
                                    } else if (ib2->asInlineTextBox()->charDirection() == CharDirection::Ltr) {
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

#ifndef NDEBUG
    for (size_t i = 0; i < boxes.size(); i ++) {
        FrameBox* box = fetchContentForResolveBidi(boxes[i]);
        if (box->isInlineBox()) {
            InlineBox* ib = box->asInlineBox();
            if (ib->isInlineTextBox()) {
                InlineTextBox* tb = ib->asInlineTextBox();
                STARFISH_ASSERT(tb->charDirection() != CharDirection::Mixed);
            }
        }
    }
#endif

    if (parentDir == DirectionValue::LtrDirectionValue) {
        std::vector<FrameBox*, gc_allocator<FrameBox*>> oldBoxes = std::move(boxes);
        std::vector<FrameBox*> rtlStorage; // use normal allocator because oldBoxes has strong reference

        CharDirection currentDirection = CharDirection::Ltr;
        for (size_t i = 0; i < oldBoxes.size(); i ++) {
            CharDirection currentDirectionBefore = currentDirection;
            FrameBox* box = fetchContentForResolveBidi(oldBoxes[i]);
            CharDirection dir = contentDir(box, ctx);
            if (dir == CharDirection::Neutral) {
                CharDirection nextDir = CharDirection::Ltr;
                for (size_t j = i + 1; j < oldBoxes.size(); j ++) {
                    nextDir = contentDir(fetchContentForResolveBidi(oldBoxes[j]), ctx);
                    if (nextDir != CharDirection::Neutral) {
                        break;
                    }
                }

                if (currentDirection == CharDirection::Rtl && nextDir == CharDirection::Neutral) {
                    currentDirection = CharDirection::Ltr;
                } else if (currentDirection == CharDirection::Rtl && nextDir == CharDirection::Ltr) {
                    currentDirection = CharDirection::Ltr;
                }

                if (box->isInlineBox()) {
                    InlineBox* ib = box->asInlineBox();
                    if (ib->isInlineTextBox()) {
                        String* text = ib->asInlineTextBox()->text();
                        if (text->containsOnlyWhitespace()) {
                        } else {
                            if (isNumber(text, 0, text->length())) {
                                currentDirection = currentDirectionBefore;
                            }
                            ib->asInlineTextBox()->setCharDirection(currentDirection);
                        }
                    }
                }
            } else {
                currentDirection = dir;
            }

            if (currentDirection == CharDirection::Ltr) {
                if (rtlStorage.size()) {
                    boxes.insert(boxes.end(), rtlStorage.begin(), rtlStorage.end());
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
        std::vector<FrameBox*, gc_allocator<FrameBox*>> oldBoxes = std::move(boxes);
        std::vector<FrameBox*> ltrStorage; // use normal allocator because oldBoxes has strong reference

        CharDirection currentDirection = CharDirection::Rtl;
        for (size_t i = 0; i < oldBoxes.size(); i ++) {
            CharDirection currentDirectionBefore = currentDirection;
            FrameBox* box = fetchContentForResolveBidi(oldBoxes[i]);
            CharDirection dir = contentDir(box, ctx);
            if (dir == CharDirection::Neutral) {
                CharDirection nextDir = CharDirection::Rtl;
                for (size_t j = i + 1; j < oldBoxes.size(); j ++) {
                    nextDir = contentDir(fetchContentForResolveBidi(oldBoxes[j]), ctx);
                    if (nextDir != CharDirection::Neutral) {
                        break;
                    }
                }

                if (currentDirection == CharDirection::Ltr && nextDir == CharDirection::Neutral) {
                    currentDirection = CharDirection::Rtl;
                } else if (currentDirection == CharDirection::Ltr && nextDir == CharDirection::Rtl) {
                    currentDirection = CharDirection::Rtl;
                }

                if (box->isInlineBox()) {
                    InlineBox* ib = box->asInlineBox();
                    if (ib->isInlineTextBox()) {
                        String* text = ib->asInlineTextBox()->text();
                        if (text->containsOnlyWhitespace()) {
                        } else {
                            if (isNumber(text, 0, text->length())) {
                                currentDirection = currentDirectionBefore;
                            }
                            ib->asInlineTextBox()->setCharDirection(currentDirection);
                        }
                    }
                }
            } else {
                currentDirection = dir;
            }

            if (currentDirection == CharDirection::Rtl) {
                if (ltrStorage.size()) {
                    boxes.insert(boxes.begin(), ltrStorage.begin(), ltrStorage.end());
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
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        41, 40, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        62, 0, 60, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 93, 0, 91, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 123, 0, 125, 0, 0
    };

    for (size_t i = 0; i < boxes.size(); i ++) {
        FrameBox* box = fetchContentForResolveBidi(boxes[i]);
        if (box->isInlineBox()) {
            if (box->asInlineBox()->isInlineTextBox() && box->asInlineBox()->asInlineTextBox()->charDirection() == CharDirection::Rtl) {
                InlineTextBox* b = box->asInlineBox()->asInlineTextBox();
                String* t = b->text();
                bool shouldReplaceString = false;
                for (size_t j = 0; j < t->length(); j++) {
                    char32_t ch = t->charAt(j);
                    if (ch < 128) {
                        if (parenthesisMap[ch]) {
                            shouldReplaceString = true;
                            break;
                        }
                    }
                }

                if (shouldReplaceString) {
                    if (t->isASCIIString()) {
                        ASCIIString str;
                        for (size_t j = 0; j < t->length(); j++) {
                            char32_t ch = t->charAt(j);
                            if (ch < 128) {
                                if (parenthesisMap[ch]) {
                                    ch = (char32_t) parenthesisMap[ch];
                                }
                            }
                            str += (char)ch;
                        }
                        t = new StringDataASCII(std::move(str));
                    } else {
                        UTF32String str;
                        for (size_t j = 0; j < t->length(); j++) {
                            char32_t ch = t->charAt(j);
                            if (ch < 128) {
                                if (parenthesisMap[ch]) {
                                    ch = (char32_t) parenthesisMap[ch];
                                }
                            }
                            str += ch;
                        }
                        t = new StringDataUTF32(std::move(str));
                    }
                    b->setText(t);
                }
            }
        }
    }


    // reassign left, right mbp for inlineNonReplacedBox
    reassignLeftRightMBPOfInlineNonReplacedBoxPreProcess(ctx, boxes);
    reassignLeftRightMBPOfInlineNonReplacedBox(ctx, boxes);

    ctx.m_dataForRestoreLeftRightOfMBPAfterResolveBidiLinePerLine.clear();
    ctx.m_checkLastInlineNonReplacedPerLine.clear();
}

static void removeBoxFromLine(FrameBox* box)
{
    if (box->layoutParent()->asFrameBox()->isLineBox()) {
        std::vector<FrameBox*, gc_allocator<FrameBox*> >& boxes = box->layoutParent()->asFrameBox()->asLineBox()->boxes();
        boxes.erase(std::find(boxes.begin(), boxes.end(), box));
    } else {
        std::vector<FrameBox*, gc_allocator<FrameBox*> >& boxes = box->layoutParent()->asFrameBox()->asInlineBox()->asInlineNonReplacedBox()->boxes();
        auto self = box->layoutParent()->asFrameBox()->asInlineBox()->asInlineNonReplacedBox();
        LayoutUnit w = box->asInlineBox()->asInlineTextBox()->width();
        while (true) {
            self->setWidth(self->width() - w);
            if (self->layoutParent()->asFrameBox()->isLineBox())
                break;
            self = self->layoutParent()->asFrameBox()->asInlineBox()->asInlineNonReplacedBox();
        }
        boxes.erase(std::find(boxes.begin(), boxes.end(), box));
    }
}

void LineFormattingContext::registerInlineContent()
{
    if (m_block.m_lineBoxes.size()) {
        LineBox* lb = m_block.m_lineBoxes.back();
        bool hasNormalFlowContent = false;
        for (size_t i = 0; i < lb->boxes().size(); i ++) {
            if (lb->boxes()[i]->isNormalFlow()) {
                hasNormalFlowContent = true;
                break;
            }
        }
        if (hasNormalFlowContent)
            m_layoutContext.registerYPositionForVerticalAlignInlineBlock(lb);
    }
}

void LineFormattingContext::completeLastLine()
{
    LineBox* back = m_block.m_lineBoxes.back();
    FrameBox* last = nullptr;
    while ((last = findLastInlineBox(back), last) && last->isInlineBox() && last->asInlineBox()->isInlineTextBox()) {
        if (last->asInlineBox()->asInlineTextBox()->text()->equals(String::spaceString)) {
            removeBoxFromLine(last);
        } else {
            break;
        }
    }

    resolveBidi(*this, m_block.style()->direction(), back->boxes());

    LayoutUnit x;
    for (size_t k = 0; k < back->m_boxes.size(); k++) {
        FrameBox* childBox = back->m_boxes[k];
        if (childBox->isNormalFlow()) {
            childBox->setX(x + childBox->marginLeft());
            x += childBox->width() + childBox->marginWidth();
        } else {
            childBox->setX(x);
        }
    }

    // text align
    if (m_block.style()->textAlign() == SideValue::LeftSideValue) {
    } else if (m_block.style()->textAlign() == SideValue::RightSideValue) {
        LayoutUnit diff = (m_lineBoxWidth - x);
        for (size_t k = 0; k < back->m_boxes.size(); k++) {
            FrameBox* childBox = back->m_boxes[k];
            childBox->moveX(diff);
        }
    /*
     * justify: No supported value
    } else if (m_block.style()->textAlign() == SideValue::JustifySideValue) {
        // issue #145
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        if (lineFormattingContext.isBreakedLineWithoutBR(i)) {
            LayoutUnit remainSpace = (inlineContentWidth - x);
            if (remainSpace > 0) {
                size_t spaceBoxCnt = 0;
                for (size_t j = 0; j < back->m_boxes.size(); j++) {
                    FrameBox* box = back->m_boxes[j];
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
                    for (size_t j = 0; j < back->m_boxes.size(); j ++) {
                        FrameBox* box = back->m_boxes[j];
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
        STARFISH_ASSERT(m_block.style()->textAlign() == SideValue::CenterSideValue);
        LayoutUnit diff = (m_lineBoxWidth - x) / 2;
        if (diff > 0) {
            for (size_t k = 0; k < back->m_boxes.size(); k++) {
                FrameBox* childBox = back->m_boxes[k];
                childBox->moveX(diff);
            }
        }
    }

    DirectionValue dir = m_block.style()->direction();
    for (size_t i = 0; i < m_absolutePositionedBoxes.size(); i ++) {
        FrameBox* box = m_absolutePositionedBoxes[i].first;
        if (box->style()->originalDisplay() == BlockDisplayValue) {
            back->boxes().push_back(box);
            box->setLayoutParent(back);
            if (dir == DirectionValue::LtrDirectionValue)
                box->setX(0);
            else
                box->setX(back->width());
            if (m_absolutePositionedBoxes[i].second)
                box->setY(back->height());
            else
                box->setY(0);
        } else {
            box->setY(0);
            FrameBox* parent = box->layoutParent()->asFrameBox();
            if (parent->isLineBox()) {
                STARFISH_ASSERT(parent == back);
                std::vector<FrameBox*, gc_allocator<FrameBox*> >& boxes = parent->asLineBox()->boxes();
                boxes.erase(std::find(boxes.begin(), boxes.end(), box));
            } else {
                std::vector<FrameBox*, gc_allocator<FrameBox*> >& boxes = parent->asInlineBox()->asInlineNonReplacedBox()->boxes();
                auto pos = box->absolutePoint(back);
                boxes.erase(std::find(boxes.begin(), boxes.end(), box));
                box->setX(pos.x());
            }
            back->boxes().push_back(box);
            box->setLayoutParent(back);
        }
    }

    m_absolutePositionedBoxes.clear();

    registerInlineContent();
}

void LineFormattingContext::breakLine(bool dueToBr, bool isInLineBox)
{
    if (dueToBr == false)
        m_breakedLinesSet.insert(m_block.m_lineBoxes.size() - 1);

    LayoutUnit ascender;
    LayoutUnit descender;
    LineBox* back = m_block.m_lineBoxes.back();
    computeVerticalProperties(back, m_block.style(), ascender, descender, *this, dueToBr, isInLineBox);
    back->m_ascender = ascender;
    back->m_descender = descender;
    back->setHeight(back->ascender() - back->decender());
    completeLastLine();
    m_lineBoxY += back->height();

    m_block.m_lineBoxes.push_back(new LineBox(&m_block));
    m_block.m_lineBoxes.back()->setX(m_lineBoxX);
    m_block.m_lineBoxes.back()->setY(m_lineBoxY);
    m_block.m_lineBoxes.back()->setWidth(m_lineBoxWidth);
    m_currentLine++;
    m_currentLineWidth = 0;
    m_shouldLineBreakForabsolutePositionedBlock = false;
}

template <typename fn>
void textDividerForLayout(StarFish* sf, String* txt, fn f)
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
            f(txt, offset, nextOffset, isWhiteSpace, true);
        } else {
            size_t start = offset;
            while (nextOffset < txt->length() && !String::isSpaceOrNewline((*txt)[nextOffset])) {
                nextOffset++;
            }

            auto breaker = sf->lineBreaker();
            breaker->setText(txt->toUnicodeString(start, nextOffset));
            int32_t c, prev = 0;
            while ((c = breaker->next()) != icu::BreakIterator::DONE) {
                f(txt, prev + start, c + start, isWhiteSpace, (prev + start != start));
                prev = c;
            }
        }
        offset = nextOffset;
    }
}

void inlineBoxGenerator(FrameBox* layoutParent, Frame* origin, LayoutContext& ctx, LineFormattingContext& lineFormattingContext, LayoutUnit inlineContentWidth, LayoutUnit& unprocessedStartingWidth, LayoutUnit& unprocessedEndingWidth,
    std::function<void(FrameBox*)> gotInlineBoxCallback, std::function<void(bool)> lineBreakCallback, std::function<void(FrameInline*)> frameInlineCallback, std::function<void(FrameBox*)> absBoxCallback)
{
    Frame* f = origin->lastChild();
    Frame* lastContent = nullptr;

    while (f) {
        if (f->isFrameText()) {
            String* txt = f->asFrameText()->text();
            if (!txt->containsOnlyWhitespace()) {
                lastContent = f;
                break;
            }
        } else if (f->isFrameBlockBox() || f->isFrameReplaced()) {
            lastContent = f;
            break;
        }

        f = f->previous();
    }

    f = origin->firstChild();
    while (f) {
        if (!f->isNormalFlow()) {
            ctx.registerAbsolutePositionedFrames(f->asFrameBox());

            lineFormattingContext.m_absolutePositionedBoxes.push_back(std::make_pair(f->asFrameBox(), lineFormattingContext.m_shouldLineBreakForabsolutePositionedBlock));
            absBoxCallback(f->asFrameBox());
            f = f->next();
            continue;
        }

        if (f->isFrameText()) {
            String* txt = f->asFrameText()->text();
            textDividerForLayout(ctx.starFish(), txt, [&](String* srcTxt, size_t offset, size_t nextOffset, bool isWhiteSpace, bool canBreak) {
                textAppendRetry:
                if (isWhiteSpace) {
                    if (offset == 0) {
                        FrameBox* last = findLastInlineBox(lineFormattingContext.currentLine());
                        if (!last)
                            return;
                        if (last && last->isInlineBox() && last->asInlineBox()->isInlineTextBox()) {
                            String* str = last->asInlineBox()->asInlineTextBox()->text();
                            if (str->containsOnlyWhitespace()) {
                                return;
                            }
                        }
                    } else if (nextOffset == srcTxt->length() && f == origin->lastChild()) {
                        if (origin->isFrameInline()) {

                        } else {
                            return;
                        }
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
                    String* ss = srcTxt->substring(offset, nextOffset - offset);
                    resultString = ss;
                    textWidth = f->style()->font()->measureText(ss);
                }

                /*
                bool couldNotBreak = false;
                if (!isWhiteSpace && !canBreak) {
                    FrameBox* last = findLastInlineBox(lineFormattingContext.currentLine());
                    if (last && last->isInlineBox() && last->asInlineBox()->isInlineTextBox()) {
                        String* str = last->asInlineBox()->asInlineTextBox()->text();
                        if (!str->containsOnlyWhitespace()) {
                            couldNotBreak = true;
                        }
                    }
                }
                */

                LayoutUnit unprocessedWidth;

                if (lastContent == f && srcTxt->substring(nextOffset, srcTxt->length() - nextOffset)->containsOnlyWhitespace()) {
                    unprocessedWidth = unprocessedEndingWidth;
                } else {
                    unprocessedWidth = unprocessedStartingWidth;
                }

                if (/*couldNotBreak || */(lineFormattingContext.m_currentLineWidth == 0) || textWidth <= (inlineContentWidth - lineFormattingContext.m_currentLineWidth - unprocessedWidth)) {

                } else {
                    // try this at nextline
                    lineBreakCallback(false);
                    goto textAppendRetry;
                }
                lineFormattingContext.m_shouldLineBreakForabsolutePositionedBlock |= !isWhiteSpace;

                const std::vector<TextRun>& runs = lineFormattingContext.m_textRunsPerFrameText[f->asFrameText()];
                if (isWhiteSpace) {
                    CharDirection dir = CharDirection::Ltr;
                    for (size_t i = 0; i < runs.size(); i ++) {
                        if (offset <= runs[i].m_startPosition && runs[i].m_endPosition <= nextOffset) {
                            dir = runs[i].m_direction;
                            break;
                        }
                    }
                    InlineBox* ib = new InlineTextBox(f->node(), f->style(), nullptr, resultString, TextRun(f->asFrameText(), offset, nextOffset, dir));
                    ib->setWidth(textWidth);
                    ib->setHeight(f->style()->font()->metrics().m_fontHeight);
                    gotInlineBoxCallback(ib);
                } else {
                    size_t startPos = offset;
                    size_t endPos = nextOffset;

                    std::set<size_t> splitPosition;

                    for (size_t i = 0; i < runs.size(); i ++) {
                        TextRun r = runs[i];
                        if (offset < r.m_startPosition && r.m_startPosition < nextOffset) {
                            splitPosition.insert(r.m_startPosition);
                        }
                        if (offset < r.m_endPosition && r.m_endPosition < nextOffset) {
                            splitPosition.insert(r.m_endPosition);
                        }
                    }

                    if (splitPosition.size()) {
                        size_t start = offset;
                        size_t end;
                        auto iter = splitPosition.begin();
                        while (iter != splitPosition.end()) {
                            end = *iter;

                            CharDirection dir = CharDirection::Ltr;
                            for (size_t i = 0; i < runs.size(); i ++) {
                                if (runs[i].m_startPosition <= start && end <= runs[i].m_endPosition) {
                                    dir = runs[i].m_direction;
                                    break;
                                }
                            }

                            String* newText = f->asFrameText()->text()->substring(start, end-start);
                            InlineBox* ib = new InlineTextBox(f->node(), f->style(), nullptr, newText, TextRun(f->asFrameText(), start, end, dir));
                            ib->setWidth(f->style()->font()->measureText(newText));
                            ib->setHeight(f->style()->font()->metrics().m_fontHeight);
                            gotInlineBoxCallback(ib);
                            start = end;
                            iter++;
                        }
                        if (end != nextOffset) {
                            CharDirection dir = CharDirection::Ltr;
                            for (size_t i = 0; i < runs.size(); i ++) {
                                if (runs[i].m_startPosition <= end && nextOffset <= runs[i].m_endPosition) {
                                    dir = runs[i].m_direction;
                                    break;
                                }
                            }
                            String* newText = f->asFrameText()->text()->substring(end, nextOffset - end);
                            InlineBox* ib = new InlineTextBox(f->node(), f->style(), nullptr, newText, TextRun(f->asFrameText(), end, nextOffset, dir));
                            ib->setWidth(f->style()->font()->measureText(newText));
                            ib->setHeight(f->style()->font()->metrics().m_fontHeight);
                            gotInlineBoxCallback(ib);
                        }
                    } else {
                        CharDirection dir = CharDirection::Ltr;
                        for (size_t i = 0; i < runs.size(); i ++) {
                            if (runs[i].m_startPosition <= offset && nextOffset <= runs[i].m_endPosition) {
                                dir = runs[i].m_direction;
                                break;
                            }
                        }
                        InlineBox* ib = new InlineTextBox(f->node(), f->style(), nullptr, resultString, TextRun(f->asFrameText(), offset, nextOffset, dir));
                        ib->setWidth(textWidth);
                        ib->setHeight(f->style()->font()->metrics().m_fontHeight);
                        gotInlineBoxCallback(ib);
                    }
                }

                lineFormattingContext.m_currentLineWidth += textWidth;
            });

        } else if (f->isFrameReplaced()) {
            lineFormattingContext.m_shouldLineBreakForabsolutePositionedBlock = true;
            FrameReplaced* r = f->asFrameReplaced();
            LayoutUnit unprocessedWidth;
            if (lastContent == f) {
                unprocessedWidth = unprocessedEndingWidth;
            } else {
                unprocessedWidth = unprocessedStartingWidth;
            }

            r->layout(ctx, Frame::LayoutWantToResolve::ResolveAll);
        insertReplacedBox:
            if ((r->width() + r->marginWidth()) <= (inlineContentWidth - lineFormattingContext.m_currentLineWidth - unprocessedWidth) || lineFormattingContext.m_currentLineWidth == 0) {
                lineFormattingContext.m_currentLineWidth += (r->width() + r->marginWidth());
                gotInlineBoxCallback(r);
            } else {
                lineBreakCallback(false);
                goto insertReplacedBox;
            }
        } else if (f->isFrameBlockBox()) {
            lineFormattingContext.m_shouldLineBreakForabsolutePositionedBlock = true;
            // inline-block
            FrameBlockBox* r = f->asFrameBlockBox();
            LayoutUnit unprocessedWidth;
            if (lastContent == f) {
                unprocessedWidth = unprocessedEndingWidth;
            } else {
                unprocessedWidth = unprocessedStartingWidth;
            }


            ctx.pushInlineBlockBox(r);
            STARFISH_ASSERT(f->style()->display() == DisplayValue::InlineBlockDisplayValue);
            f->setLayoutParent(layoutParent);
            f->layout(ctx, Frame::LayoutWantToResolve::ResolveAll);

            LayoutUnit ascender;

            std::pair<bool, LayoutUnit> p = ctx.readRegisteredLastLineBoxYPos(r);
            if (p.first && r->style()->overflow() == OverflowValue::VisibleOverflow) {
                ascender = p.second;
            } else {
                ascender = f->asFrameBox()->height();
            }
            ctx.popInlineBlockBox();
            lineFormattingContext.registerInlineBlockAscender(ascender, r);

        insertBlockBox:
            if ((r->width() + r->marginWidth()) <= (inlineContentWidth - lineFormattingContext.m_currentLineWidth - unprocessedWidth) || lineFormattingContext.m_currentLineWidth == 0) {
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

static void registerRelativePositionInlineBoxes(LayoutContext& ctx, std::vector<FrameBox*, gc_allocator<FrameBox*>>& boxes)
{
    for (size_t k = 0; k < boxes.size(); k++) {
        FrameBox* childBox = boxes[k];
        if (!childBox->isFrameBlockBox()) {
            if (childBox->style()->position() == PositionValue::RelativePositionValue) {
                ctx.registerRelativePositionedFrames(childBox, true);
            }
            if (childBox->isInlineBox() && childBox->asInlineBox()->isInlineNonReplacedBox()) {
                registerRelativePositionInlineBoxes(ctx, childBox->asInlineBox()->asInlineNonReplacedBox()->boxes());
            }
        }
    }
}

static void computeDirection(LineFormattingContext& ctx, Frame* parent, DirectionValue directionValue);
static void collectComputeDirectionsCandidate(LineFormattingContext& ctx, Frame* parent, std::vector<Frame*>& frames)
{

    for (Frame* f = parent->firstChild(); f; f = f->next()) {
        if (!f->isNormalFlow()) {
            continue;
        }
        if (f->isFrameInline()) {
            // TODO add isolate
            if (f->style()->unicodeBidi() == UnicodeBidiValue::EmbedUnicodeBidiValue) {
                frames.push_back(f);
                computeDirection(ctx, f, f->style()->direction());
            } else {
                collectComputeDirectionsCandidate(ctx, f, frames);
            }
        } else {
            frames.push_back(f);
        }
    }
}

static void breakTextRun(std::vector<TextRun>& runs, size_t breakRunIndex, size_t breakTextIndex)
{
    TextRun target = runs[breakRunIndex];
    runs.erase(runs.begin() + breakRunIndex);

    size_t start = target.m_startPosition;
    size_t end = breakTextIndex + 1;
    String* txt = target.m_frameText->text()->substring(start, end - start);
    runs.insert(runs.begin() + breakRunIndex++, TextRun(target.m_frameText, start, end, charDirFromICUDir(getTextDir(txt))));

    start = breakTextIndex + 1;
    end = target.m_endPosition;
    txt = target.m_frameText->text()->substring(start, end - start);
    runs.insert(runs.begin() + breakRunIndex, TextRun(target.m_frameText, start, end, charDirFromICUDir(getTextDir(txt))));
}

static std::vector<TextRun> textBidiResolver(FrameText* frameText, DirectionValue directionValue)
{
    std::vector<TextRun> result;
    UBiDi* bidi = ubidi_open();
    UTF16String str = frameText->text()->toUTF16String();
    UErrorCode err = (UErrorCode)0;
    ubidi_setPara(bidi, (const UChar*)str.data(), str.length(), directionValue == DirectionValue::LtrDirectionValue ? UBIDI_DEFAULT_LTR : UBIDI_DEFAULT_RTL, NULL, &err);
    STARFISH_ASSERT(U_SUCCESS(err));
    size_t total = ubidi_countRuns(bidi, &err);
    STARFISH_ASSERT(U_SUCCESS(err));
    if (total == 1) {
        UBiDiDirection dir = getTextDir(frameText->text());
        result.push_back(TextRun(frameText, 0, frameText->text()->length(), charDirFromICUDir(dir)));
    } else {
        int32_t start = 0;
        int32_t end;
        size_t utf32Pos = 0;
        for (size_t i = 0; i < total; i ++) {
            ubidi_getLogicalRun(bidi, start, &end, NULL);
            UBiDiDirection dir = ubidi_getBaseDirection((const UChar*)str.data() + start, end - start);
            size_t utf32Len = 0;

            for (size_t i = start; i < (size_t)end; /* U16_NEXT post-increments */) {
                char32_t c;
                U16_NEXT((const UChar*)str.data(), i, (size_t)end, c);
                utf32Len++;
            }

            result.push_back(TextRun(frameText, utf32Pos, utf32Pos + utf32Len, charDirFromICUDir(dir)));
            utf32Pos += utf32Len;

            start = end;
        }
    }
    ubidi_close(bidi);

    for (size_t i = 0; i < result.size(); i ++) {
        if (isNumber(result[i].m_frameText->text(), result[i].m_startPosition, result[i].m_endPosition)) {
            result[i].m_direction = CharDirection::Neutral;
            continue;
        }

        // check it has leading netural chars
        char32_t first = result[i].m_frameText->text()->charAt(result[i].m_startPosition);
        if ((result[i].m_endPosition - result[i].m_startPosition) > 1 && charDirection(first) == 2 && !isNumberChar(first)) {
            breakTextRun(result, i, result[i].m_startPosition);
            i--;
            continue;
        }

        // check it has trailing neutral chars
        if (result[i].m_endPosition - result[i].m_startPosition > 1) {
            size_t lastPos = result[i].m_endPosition - 1;
            char32_t last = result[i].m_frameText->text()->charAt(lastPos);
            if (charDirection(last) == 2 && !isNumberChar(last)) {
                breakTextRun(result, i, lastPos - 1);
                i--;
                continue;
            }
        }
    }

    return result;
}


static void computeDirection(LineFormattingContext& ctx, Frame* parent, DirectionValue directionValue)
{
    std::vector<Frame*> frames;
    collectComputeDirectionsCandidate(ctx, parent, frames);

    DirectionValue currentDirection = directionValue;
    bool everMeetNonNeutralThing = false;
    std::vector<Frame*> putOffNeutralFrames;
    std::vector<TextRun> putOffTextRuns;
    DirectionValue directionValueWhenNeturalMeets = directionValue;

    auto putOffNeutral = [&](Frame* f)
    {
        if (putOffNeutralFrames.size() == 0) {
            directionValueWhenNeturalMeets = currentDirection;
        }
        putOffNeutralFrames.push_back(f);
    };

    auto flushNeurtal = [&](DirectionValue dir)
    {
        DirectionValue result;
        if (directionValue == LtrDirectionValue) {
            if (directionValueWhenNeturalMeets == RtlDirectionValue && dir == RtlDirectionValue) {
                result = RtlDirectionValue;
            } else {
                result = LtrDirectionValue;
            }
        } else {
            if (directionValueWhenNeturalMeets == LtrDirectionValue && dir == LtrDirectionValue) {
                result = LtrDirectionValue;
            } else {
                result = RtlDirectionValue;
            }
        }

        for (size_t i = 0; i < putOffNeutralFrames.size(); i ++) {
            ctx.m_computedDirectonValuePerFrameBox[putOffNeutralFrames[i]->asFrameBox()] = result;
        }

        CharDirection ch = (result == DirectionValue::LtrDirectionValue ? CharDirection::Ltr : CharDirection::Rtl);
        for (size_t i = 0; i < putOffTextRuns.size(); i ++) {
            TextRun run = putOffTextRuns[i];
            ctx.m_textRunsPerFrameText[run.m_frameText].push_back(TextRun(run.m_frameText, run.m_startPosition, run.m_endPosition, ch));
        }

        putOffNeutralFrames.clear();
        putOffTextRuns.clear();
        currentDirection = dir;
    };

    for (size_t i = 0; i < frames.size(); i ++) {
        Frame* f = frames[i];
        if (f->isFrameText()) {
            String* txt = f->asFrameText()->text();
            std::vector<TextRun> runs = textBidiResolver(f->asFrameText(), directionValue);

            for (size_t i = 0; i < runs.size(); i ++) {
                TextRun run = runs[i];
                if (run.m_direction == CharDirection::Neutral) {
                    if (isNumber(run.m_frameText->text(), run.m_startPosition, run.m_endPosition)) {
                        continue;
                    }
                    if (putOffTextRuns.size() == 0) {
                        directionValueWhenNeturalMeets = currentDirection;
                    }
                    putOffTextRuns.push_back(run);
                } else {
                    STARFISH_ASSERT(run.m_direction == CharDirection::Ltr || run.m_direction == CharDirection::Rtl);
                    everMeetNonNeutralThing = true;
                    ctx.m_textRunsPerFrameText[f->asFrameText()].push_back(TextRun(f->asFrameText(), run.m_startPosition, run.m_endPosition, run.m_direction));
                    flushNeurtal(run.m_direction == CharDirection::Ltr ? DirectionValue::LtrDirectionValue : DirectionValue::RtlDirectionValue);
                }
            }

        } else if (f->isFrameBox()) {
            if (everMeetNonNeutralThing) {
                putOffNeutral(f);
            } else {
                ctx.m_computedDirectonValuePerFrameBox[f->asFrameBox()] = directionValue;
            }
        } else if (f->isFrameInline()) {
            STARFISH_ASSERT(f->style()->unicodeBidi() == UnicodeBidiValue::EmbedUnicodeBidiValue);
            everMeetNonNeutralThing = true;
            flushNeurtal(f->style()->direction());
        } else if (f->isFrameLineBreak()) {
            everMeetNonNeutralThing = true;
            flushNeurtal(directionValue);
        }
    }

    flushNeurtal(directionValue);
}

LayoutUnit FrameBlockBox::layoutInline(LayoutContext& ctx)
{
    if (!isNecessaryBlockBox()) {
        return 0;
    }

    LayoutRect visibleRect(0, 0, 0, 0);
    LayoutUnit inlineContentWidth = contentWidth();
    LineFormattingContext lineFormattingContext(*this, ctx, paddingLeft() + borderLeft(), paddingTop() + borderTop(), inlineContentWidth);
    LayoutUnit unused;

    // compute directions
    computeDirection(lineFormattingContext, this, style()->direction());

    inlineBoxGenerator(this, this, ctx, lineFormattingContext, inlineContentWidth, unused, unused, [&](FrameBox* ib) {
        lineFormattingContext.currentLine()->boxes().push_back(ib);
        ib->setLayoutParent(lineFormattingContext.currentLine());
    }, [&](bool dueToBr)
    {
        lineFormattingContext.breakLine(dueToBr, true);
    }, [&](FrameInline* f)
    {
        InlineNonReplacedBox* inlineBox = new InlineNonReplacedBox(f->node(), f->node()->style(), nullptr, f->asFrameInline());
        InlineNonReplacedBox::layoutInline(inlineBox, ctx, this, &lineFormattingContext, nullptr, true);

        if (inlineBox->m_boxes.size() == 0) {
            if (inlineBox->style()->direction() == LtrDirectionValue) {
                inlineBox->setWidth(inlineBox->width() + inlineBox->borderLeft() + inlineBox->paddingLeft());
            } else {
                inlineBox->setWidth(inlineBox->width() + inlineBox->borderRight() + inlineBox->paddingRight());
            }
        }

        if (lineFormattingContext.currentLine()->boxes().size() == 1) {
            FrameBox* box = lineFormattingContext.currentLine()->boxes().back();
            if (box->isInlineBox() && box->asInlineBox()->isInlineNonReplacedBox()) {
                InlineNonReplacedBox* lastInlineBox = box->asInlineBox()->asInlineNonReplacedBox();
                if (lastInlineBox->m_boxes.size() == 1) {
                    FrameBox* box = lastInlineBox->m_boxes.back();
                    if (box->isInlineBox() && box->asInlineBox()->isInlineTextBox()) {
                        String* text = box->asInlineBox()->asInlineTextBox()->text();
                        if (text->containsOnlyWhitespace()) {
                            lineFormattingContext.currentLine()->boxes().erase(lineFormattingContext.currentLine()->boxes().end() - 1);
                            lineFormattingContext.m_currentLineWidth = 0;
                        }
                    }
                }
            }
        }
    }, [&](FrameBox* box)
    {
        if (box->style()->originalDisplay() != BlockDisplayValue) {
            box->setLayoutParent(lineFormattingContext.currentLine());
            lineFormattingContext.currentLine()->boxes().push_back(box);
        }
    });

    LineBox* back = m_lineBoxes.back();
    LayoutUnit ascender;
    LayoutUnit descender;
    computeVerticalProperties(back, style(), ascender, descender, lineFormattingContext, false, true);
    back->m_ascender = ascender;
    back->m_descender = descender;
    back->m_frameRect.setHeight(ascender - descender);

    lineFormattingContext.completeLastLine();

    if (m_lineBoxes.size() && m_lineBoxes.back()->boxes().size() == 0) {
        m_lineBoxes.erase(m_lineBoxes.end() - 1);
    }

    size_t p = m_lineBoxes.size();
    while (p--) {
        LineBox* lineBox = m_lineBoxes[p];
        if (lineBox->boxes().size() == 1 && lineBox->boxes().at(0)->isInlineBox() && lineBox->boxes().at(0)->asInlineBox()->isInlineNonReplacedBox() && lineBox->boxes().at(0)->width() == 0 && lineBox->boxes().at(0)->marginLeft() == 0 && lineBox->boxes().at(0)->marginRight() == 0) {
            m_lineBoxes.erase(m_lineBoxes.begin() + p);
        }
    }

    LayoutUnit contentHeight = 0;
    for (size_t i = 0; i < m_lineBoxes.size(); i++) {
        LineBox& b = *m_lineBoxes[i];
        STARFISH_ASSERT(b.isLineBox());

        // register position: relative boxes
        for (size_t k = 0; k < b.m_boxes.size(); k++) {
            FrameBox* childBox = b.m_boxes[k];
            if (!childBox->isFrameBlockBox()) {
                if (childBox->style()->position() == PositionValue::RelativePositionValue) {
                    ctx.registerRelativePositionedFrames(childBox, true);
                }

                if (childBox->isInlineBox() && childBox->asInlineBox()->isInlineNonReplacedBox()) {
                    registerRelativePositionInlineBoxes(ctx, childBox->asInlineBox()->asInlineNonReplacedBox()->boxes());
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

    return contentHeight;
}

struct InlineNonReplacedBoxMBPStore {
    LayoutBoxSurroundData m_orgPadding, m_orgBorder, m_orgMargin;
};

InlineNonReplacedBox* InlineNonReplacedBox::layoutInline(InlineNonReplacedBox* self, LayoutContext& ctx,
    FrameBlockBox* blockBox, LineFormattingContext* lfc, InlineNonReplacedBox* layoutParentBox, bool freshStart)
{
    LineFormattingContext& lineFormattingContext = *lfc;
    // just lay out children are horizontally.
    LayoutUnit inlineContentWidth = blockBox->contentWidth();

    if (freshStart)
        self->computeBorderMarginPadding(inlineContentWidth);
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

        if (!layoutParentBox) {
            lineFormattingContext.currentLine()->boxes().push_back(self);
            self->setLayoutParent(lineFormattingContext.currentLine());
        } else {
            layoutParentBox->boxes().push_back(self);
            self->setLayoutParent(layoutParentBox);
        }
    }

    if (layoutParentBox) {
        LayoutUnit w = self->marginLeft() + self->borderLeft() + self->paddingLeft();
        lineFormattingContext.m_currentLineWidth += w;
        self->setWidth(self->paddingLeft() + self->borderLeft());
    }

    auto breakLine = [&]()
    {
        std::vector<std::pair<InlineNonReplacedBox*, InlineNonReplacedBoxMBPStore> > stack;

        FrameBox* addingUpWidth = nullptr;
        Frame* currentSelf = self;
        while (!currentSelf->asFrameBox()->isLineBox()) {
            InlineNonReplacedBoxMBPStore store;
            store.m_orgMargin = currentSelf->asFrameBox()->asInlineBox()->asInlineNonReplacedBox()->m_orgMargin;
            store.m_orgBorder = currentSelf->asFrameBox()->asInlineBox()->asInlineNonReplacedBox()->m_orgBorder;
            store.m_orgPadding = currentSelf->asFrameBox()->asInlineBox()->asInlineNonReplacedBox()->m_orgPadding;

            stack.push_back(std::make_pair(currentSelf->asFrameBox()->asInlineBox()->asInlineNonReplacedBox(), store));

            if (addingUpWidth) {
                currentSelf->asFrameBox()->setWidth(currentSelf->asFrameBox()->width() + addingUpWidth->width() + addingUpWidth->marginWidth());
            }

            addingUpWidth = currentSelf->asFrameBox();
            currentSelf = currentSelf->layoutParent();
        }

        auto iter = stack.rbegin();
        while (stack.rend() != iter) {
            InlineNonReplacedBox* origin = iter->first;

            if (origin->style()->direction() == DirectionValue::LtrDirectionValue) {
                origin->m_margin.setRight(0);
                origin->m_border.setRight(0);
                origin->m_padding.setRight(0);

                origin->m_orgMargin.setRight(0);
                origin->m_orgBorder.setRight(0);
                origin->m_orgPadding.setRight(0);
            } else {
                origin->m_margin.setLeft(0);
                origin->m_border.setLeft(0);
                origin->m_padding.setLeft(0);

                origin->m_orgMargin.setLeft(0);
                origin->m_orgBorder.setLeft(0);
                origin->m_orgPadding.setLeft(0);
            }

            iter++;
        }

        lineFormattingContext.breakLine(false, false);

        bool first = true;
        iter = stack.rbegin();
        InlineNonReplacedBox* last = nullptr;
        while (stack.rend() != iter) {
            InlineNonReplacedBox* origin = iter->first;
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

            if (origin->style()->direction() == DirectionValue::LtrDirectionValue) {
                newBox->m_margin.setRight(iter->second.m_orgMargin.right());
                newBox->m_border.setRight(iter->second.m_orgBorder.right());
                newBox->m_padding.setRight(iter->second.m_orgPadding.right());

                newBox->m_orgMargin.setRight(iter->second.m_orgMargin.right());
                newBox->m_orgBorder.setRight(iter->second.m_orgBorder.right());
                newBox->m_orgPadding.setRight(iter->second.m_orgPadding.right());
            } else {
                newBox->m_margin.setLeft(iter->second.m_orgMargin.left());
                newBox->m_border.setLeft(iter->second.m_orgBorder.left());
                newBox->m_padding.setLeft(iter->second.m_orgPadding.left());

                newBox->m_orgMargin.setLeft(iter->second.m_orgMargin.left());
                newBox->m_orgBorder.setLeft(iter->second.m_orgBorder.left());
                newBox->m_orgPadding.setLeft(iter->second.m_orgPadding.left());
            }

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

    auto finishLayout = [&](bool lastNode = false, bool dueToBr = false)
    {
        if (hasIsolateBidiContent(self)) {
            resolveBidi(lineFormattingContext, self->style()->direction(), self->boxes());
        }
        LayoutUnit x = self->paddingLeft() + self->borderLeft();
        for (size_t k = 0; k < self->m_boxes.size(); k++) {
            FrameBox* childBox = self->m_boxes[k];
            childBox->setX(x + childBox->marginLeft());
            if (childBox->isNormalFlow())
                x += childBox->width() + childBox->marginWidth();
        }
        InlineNonReplacedBox* selfForFinishLayout = self;
        while (selfForFinishLayout) {
            LayoutUnit end = lineFormattingContext.m_currentLineWidth;

            if (selfForFinishLayout->style()->direction() == DirectionValue::LtrDirectionValue) {
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
            } else {
                if (end + selfForFinishLayout->paddingLeft() + selfForFinishLayout->borderLeft() + selfForFinishLayout->marginLeft() > inlineContentWidth) {
                    selfForFinishLayout->m_margin.setLeft(0);
                    selfForFinishLayout->m_border.setLeft(0);
                    selfForFinishLayout->m_padding.setLeft(0);
                    if (lastNode) {
                        selfForFinishLayout->m_orgMargin.setLeft(0);
                        selfForFinishLayout->m_orgBorder.setLeft(0);
                        selfForFinishLayout->m_orgPadding.setLeft(0);
                    }
                }
                lineFormattingContext.m_currentLineWidth += selfForFinishLayout->paddingLeft() + selfForFinishLayout->borderLeft() + selfForFinishLayout->marginLeft();
            }

            selfForFinishLayout->setWidth(selfForFinishLayout->width() + selfForFinishLayout->paddingWidth() + selfForFinishLayout->borderWidth());
            if (selfForFinishLayout->width() < 0) {
                selfForFinishLayout->setWidth(0);
            }

            LayoutUnit ascender = std::max(selfForFinishLayout->style()->font()->metrics().m_ascender, blockBox->style()->font()->metrics().m_ascender);
            LayoutUnit descender = std::min(selfForFinishLayout->style()->font()->metrics().m_descender, blockBox->style()->font()->metrics().m_descender);

            computeVerticalProperties(selfForFinishLayout, selfForFinishLayout->style(), ascender, descender, lineFormattingContext, dueToBr, false);
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
    LayoutUnit unprocessedStartingWidth;
    LayoutUnit unprocessedEndingWidth;
    if (!layoutParentBox) {
        if (self->style()->direction() == LtrDirectionValue) {
            unprocessedStartingWidth = self->marginLeft() + self->borderLeft() + self->paddingLeft();
            unprocessedEndingWidth = self->marginRight() + self->borderRight() + self->paddingRight();
        } else {
            unprocessedStartingWidth = self->marginRight() + self->borderRight() + self->paddingRight();
            unprocessedEndingWidth = self->marginLeft() + self->borderLeft() + self->paddingLeft();
        }
    }
    inlineBoxGenerator(self, self->m_origin, ctx, lineFormattingContext, inlineContentWidth, unprocessedStartingWidth, unprocessedEndingWidth, [&](FrameBox* ib)
    {
        self->boxes().push_back(ib);
        if (self->style()->direction() == LtrDirectionValue) {
            if (unprocessedStartingWidth) {
                self->setWidth(self->width() + self->borderLeft() + self->paddingLeft());
                lineFormattingContext.m_currentLineWidth += unprocessedStartingWidth;
                unprocessedStartingWidth = 0;
            }
        } else {
            if (unprocessedStartingWidth) {
                self->setWidth(self->width() + self->borderRight() + self->paddingRight());
                lineFormattingContext.m_currentLineWidth += unprocessedStartingWidth;
                unprocessedStartingWidth = 0;
            }
        }


        ib->setLayoutParent(self);
        if (self->style()->direction() == LtrDirectionValue) {
            ib->setX(self->width() + ib->marginLeft());
        } else {
            ib->setX(self->width() + ib->marginRight());
        }
        self->setWidth(self->width() + ib->width() + ib->marginWidth());
    }, [&](bool dueToBr)
    {
        if (!self->boxes().size() && !layoutParentBox) {
            lineFormattingContext.currentLine()->boxes().erase(std::find(lineFormattingContext.currentLine()->boxes().begin(), lineFormattingContext.currentLine()->boxes().end(), self));
            lineFormattingContext.breakLine(dueToBr, false);
            lineFormattingContext.currentLine()->boxes().push_back(self);
            self->setLayoutParent(lineFormattingContext.currentLine());
        } else {
            finishLayout(dueToBr);
            breakLine();
        }
    }, [&](FrameInline* f)
    {
        LayoutUnit extra;
        if (self->style()->direction() == LtrDirectionValue) {
            if (unprocessedStartingWidth) {
                lineFormattingContext.m_currentLineWidth += unprocessedStartingWidth;
                extra = unprocessedStartingWidth;
                unprocessedStartingWidth = 0;
            }
        } else {
            if (unprocessedStartingWidth) {
                lineFormattingContext.m_currentLineWidth += unprocessedStartingWidth;
                extra = unprocessedStartingWidth;
                unprocessedStartingWidth = 0;
            }
        }
        InlineNonReplacedBox* inlineBox = new InlineNonReplacedBox(f->node(), f->node()->style(), nullptr, f->asFrameInline());
        self->setWidth(self->width() + extra);
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
    }, [&](FrameBox* box)
    {
        if (box->style()->originalDisplay() != BlockDisplayValue) {
            box->setLayoutParent(self);
            self->boxes().push_back(box);
        }
    });

    finishLayout(true, false);

    return self;
}

void FrameBlockBox::computePreferredWidth(ComputePreferredWidthContext& ctx)
{
    LayoutUnit remainWidth = ctx.lastKnownWidth();
    LayoutUnit minWidth;

    if (!isNecessaryBlockBox()) {
        return;
    }

    if (hasBlockFlow()) {
        if (style()->width().isFixed()) {
            minWidth += style()->width().fixed();
            ctx.setMinimumWidth(minWidth);
            ctx.setResult(minWidth);
        } else {
            Frame* child = firstChild();
            while (child) {
                if (child->isNormalFlow()) {
                    LayoutUnit mbp = ComputePreferredWidthContext::computeMinimumWidthDueToMBP(child->style());
                    ComputePreferredWidthContext newCtx(ctx.layoutContext(), ctx.lastKnownWidth() - mbp, 0);
                    child->computePreferredWidth(newCtx);
                    ctx.setResult(newCtx.result() + mbp);
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
                textDividerForLayout(ctx.layoutContext().starFish(), s, [&](String* srcTxt, size_t offset, size_t nextOffset, bool isWhiteSpace, bool canBreak) {
                    if (isWhiteSpace) {
                        if (offset == 0) {
                            if (f == f->parent()->firstChild()) {
                                return;
                            } else if (currentLineWidth == 0) {
                                return;
                            } else if (ctx.isWhiteSpaceAtLast()) {
                                return;
                            }
                        }

                        if (nextOffset == srcTxt->length() && f == f->parent()->lastChild())
                            return;
                    }

                    LayoutUnit w = 0;
                    if (isWhiteSpace) {
                        w = f->style()->font()->spaceWidth();
                    } else {
                        w = f->style()->font()->measureText(srcTxt->substring(offset, nextOffset - offset));
                    }

                    ctx.setMinimumWidth(w);

                    if (currentLineWidth + w < remainWidth) {
                        currentLineWidth += w;
                    } else {
                        // CHECK THIS
                        // ctx.setResult(currentLineWidth);
                        ctx.setResult(remainWidth);
                        currentLineWidth = 0;
                    }

                    if (w > remainWidth) {
                        ctx.setResult(remainWidth);
                        currentLineWidth = 0;
                    }

                    ctx.setIsWhiteSpaceAtLast(isWhiteSpace);
                });
            } else if (f->isFrameBlockBox()) {
                LayoutUnit mbp = ComputePreferredWidthContext::computeMinimumWidthDueToMBP(f->style());
                ComputePreferredWidthContext newCtx(ctx.layoutContext(), remainWidth - mbp, 0);
                f->computePreferredWidth(newCtx);
                LayoutUnit w = newCtx.result() + mbp;
                ctx.setResult(w);

                if (currentLineWidth + w < remainWidth) {
                    currentLineWidth += w;
                } else {
                    ctx.setResult(currentLineWidth);
                    currentLineWidth = w;
                }

                if (currentLineWidth > remainWidth) {
                    // linebreaks
                    ctx.setResult(remainWidth);
                    currentLineWidth = 0;
                }

                ctx.setIsWhiteSpaceAtLast(false);
            } else if (f->isFrameLineBreak()) {
                // linebreaks
                ctx.setResult(currentLineWidth);
                currentLineWidth = 0;

                ctx.setIsWhiteSpaceAtLast(false);
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

                ctx.setIsWhiteSpaceAtLast(false);
            } else {
                STARFISH_ASSERT(f->isFrameReplaced());

                LayoutUnit mbp = ComputePreferredWidthContext::computeMinimumWidthDueToMBP(f->style());
                ComputePreferredWidthContext newCtx(ctx.layoutContext(), remainWidth - mbp, 0);
                f->computePreferredWidth(newCtx);
                LayoutUnit w = newCtx.result() + mbp;
                ctx.setResult(w);

                if (currentLineWidth + w < remainWidth) {
                    currentLineWidth += w;
                } else {
                    ctx.setResult(currentLineWidth);
                    currentLineWidth = w;
                }

                if (currentLineWidth > remainWidth) {
                    // linebreaks
                    ctx.setResult(remainWidth);
                    currentLineWidth = 0;
                }

                ctx.setIsWhiteSpaceAtLast(false);
            }
            if (!f->isFrameBlockBox()) {
                Frame* c = f->firstChild();
                while (c) {
                    computeInlineLayout(c);
                    c = c->next();
                }
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

void FrameBlockBox::paintChildrenWith(PaintingContext& ctx)
{
    if (hasBlockFlow()) {
        FrameBox::paintChildrenWith(ctx);
    } else {
        PaintingInlineStage last = ctx.m_paintingInlineStage;

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

        ctx.m_paintingInlineStage = last;
    }
}

void InlineTextBox::paint(PaintingContext& ctx)
{
    if (ctx.m_paintingStage == PaintingNormalFlowInline && ctx.m_paintingInlineStage == PaintingInlineLevelElements) {
        if (style()->visibility() == VisibilityValue::HiddenVisibilityValue) {
            ctx.m_canvas->setVisible(false);
        } else {
            ctx.m_canvas->setVisible(true);
        }

        if (m_textRun.m_frameText->textDecorationData()) {
            auto data = m_textRun.m_frameText->textDecorationData();
            ctx.m_canvas->setNeedsLineThrough(data->m_hasLineThrough);
            ctx.m_canvas->setNeedsUnderline(data->m_hasUnderLine);
            ctx.m_canvas->setLineThroughColor(data->m_lineThroughColor);
            ctx.m_canvas->setUnderlineColor(data->m_underLineColor);
        }

        ctx.m_canvas->setFont(style()->font());
        ctx.m_canvas->setColor(style()->color());
        ctx.m_canvas->drawText(0, 0, m_text);
    }
}

void InlineNonReplacedBox::paintBackgroundAndBorders(Canvas* canvas)
{
    if (!isCollapsed()) {
        LayoutRect frameRectBack = m_frameRect;
        LayoutBoxSurroundData paddingBack = m_padding, borderBack = m_border, marginBack = m_margin;

        m_padding = m_orgPadding;
        m_margin = m_orgMargin;
        m_border = m_orgBorder;

        canvas->save();
        canvas->translate(LayoutUnit(0), m_ascender  - (style()->font()->metrics().m_ascender) - borderTop() - paddingTop());
        setContentHeight(style()->font()->metrics().m_ascender - style()->font()->metrics().m_descender);
        setHeight(contentHeight() + paddingHeight() + borderHeight());
        FrameBox::paintBackgroundAndBorders(canvas);
        canvas->restore();

        m_frameRect = frameRectBack;
        m_padding = paddingBack;
        m_border = borderBack;
        m_margin = marginBack;
    }
}

void InlineNonReplacedBox::paint(PaintingContext& ctx)
{
    if (isEstablishesStackingContext()) {
        return;
    }
    // CHECK THIS at https://www.w3.org/TR/CSS2/zindex.html#stacking-defs
    if (isPositionedElement()) {
        if (ctx.m_paintingStage == PaintingPositionedElements) {
            paintBackgroundAndBorders(ctx.m_canvas);
            PaintingStage last = ctx.m_paintingStage;
            PaintingStage s = PaintingStage::PaintingNormalFlowBlock;
            while (s != PaintingStageEnd) {
                ctx.m_paintingStage = s;
                paintChildrenWith(ctx);
                s = (PaintingStage)(s + 1);
            }
            ctx.m_paintingStage = last;
        }
    } else if (ctx.m_paintingStage == PaintingNormalFlowInline && ctx.m_paintingInlineStage == PaintingInlineLevelElements) {
        paintBackgroundAndBorders(ctx.m_canvas);
        paintChildrenWith(ctx);
    } else {
        paintChildrenWith(ctx);
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
#ifdef STARFISH_ENABLE_TEST
void InlineNonReplacedBox::dump(int depth)
{
    InlineBox::dump(depth);

    printf(" origin %p", m_origin);

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
#endif
}
