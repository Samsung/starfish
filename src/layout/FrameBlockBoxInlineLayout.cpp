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
#include "FrameBlockBoxInlineLayout.h"

#include "FrameBlockBox.h"
#include "FrameDocument.h"
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

FontHeights LineFormattingContext::computeVerticalProperties(FrameBox* parentBox, ComputedStyle* parentStyle, bool dueToBr)
{
    LayoutUnit ascender = parentStyle->font()->metrics().m_ascender;
    LayoutUnit descender = parentStyle->font()->metrics().m_descender;
    bool hasBoxOtherThanText = false;
    bool hasBoxOtherThanCollapsedInlineNonReplacedBox = false;
    bool hasNormalFlowChild = false;

    std::vector<FrameBox*, gc_allocator_ignore_off_page<FrameBox*> >* boxes;
    if (parentBox->isLineBox()) {
        boxes = &parentBox->asLineBox()->boxes();
    } else {
        boxes = &parentBox->asInlineBox()->asInlineNonReplacedBox()->boxes();
    }

    // 1. set relative y-pos from baseline (only if it needs)
    // 2. find max ascender and descender
    LayoutUnit maxAscenderSoFar = 0;
    LayoutUnit maxDescenderSoFar = intMaxForLayoutUnit;
    for (size_t k = 0; k < boxes->size(); k ++) {
        FrameBox* box = (*boxes)[k];
        if (!box->isNormalFlow()) {
            continue;
        } else {
            hasNormalFlowChild = true;
        }
        VerticalAlignValue va = box->style()->verticalAlign();
        if (box->isInlineBox()) {
            if (box->asInlineBox()->isInlineTextBox()) {
                hasBoxOtherThanCollapsedInlineNonReplacedBox = true;
            } else {
                hasBoxOtherThanText = true;
                InlineNonReplacedBox* rb = box->asInlineBox()->asInlineNonReplacedBox();
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
                    LayoutUnit halfXHeight = (parentStyle->font()->metrics().m_xheightRate * parentStyle->font()->size()) / 2;
                    rb->setY(halfHeight + halfXHeight);
                    maxAscenderSoFar = std::max(halfHeight + halfXHeight, maxAscenderSoFar);
                    maxDescenderSoFar = std::min(-1 * (halfHeight - halfXHeight), maxDescenderSoFar);
                } else if (va == VerticalAlignValue::SubVAlignValue) {
                    rb->setY(descender + rb->ascender());
                    maxAscenderSoFar = std::max(descender + rb->ascender(), maxAscenderSoFar);
                    maxDescenderSoFar = std::min(descender + rb->decender(), maxDescenderSoFar);
                } else if (va == VerticalAlignValue::SuperVAlignValue) {
                    // Placing a superscript is font and browser dependent.
                    // We place superscript above the baseline by 1/2 of ascender (following blink)
                    // (i.e, the baseline of superscript is aligned with 1/2 of the ascender)
                    rb->setY(ascender / 2 + rb->ascender());
                    maxAscenderSoFar = std::max(ascender / 2 + rb->ascender(), maxAscenderSoFar);
                    maxDescenderSoFar = std::min(ascender / 2 + rb->decender(), maxDescenderSoFar);
                } else if (va == VerticalAlignValue::TextTopVAlignValue) {
                    maxDescenderSoFar = std::min(ascender - rb->height(), maxDescenderSoFar);
                } else if (va == VerticalAlignValue::TextBottomVAlignValue) {
                    maxAscenderSoFar = std::max(descender + rb->height(), maxAscenderSoFar);
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
                LayoutUnit halfXHeight = (parentStyle->font()->metrics().m_xheightRate * parentStyle->font()->size()) / 2;
                box->setY(halfHeight + halfXHeight);
                maxAscenderSoFar = std::max(halfHeight + halfXHeight, maxAscenderSoFar);
                maxDescenderSoFar = std::min(-1 * (halfHeight - halfXHeight), maxDescenderSoFar);
            } else if (va == VerticalAlignValue::SubVAlignValue) {
                box->setY(descender + boxHeight);
                maxAscenderSoFar = std::max(descender + boxHeight, maxAscenderSoFar);
            } else if (va == VerticalAlignValue::SuperVAlignValue) {
                box->setY(ascender / 2 + boxHeight);
                maxAscenderSoFar = std::max(ascender / 2 + boxHeight, maxAscenderSoFar);
            } else if (va == VerticalAlignValue::TextTopVAlignValue) {
                maxDescenderSoFar = std::min(ascender - boxHeight, maxDescenderSoFar);
            } else if (va == VerticalAlignValue::TextBottomVAlignValue) {
                maxAscenderSoFar = std::max(descender + boxHeight, maxAscenderSoFar);
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
        } else if (box->isFrameBlockBox() && ((box->style()->display() == InlineBlockDisplayValue) || (box->style()->display() == InlineTableDisplayValue))) {
            hasBoxOtherThanText = true;
            hasBoxOtherThanCollapsedInlineNonReplacedBox = true;
            LayoutUnit boxHeight = box->boxHeight();
            if (va == VerticalAlignValue::BaselineVAlignValue) {
                LayoutUnit ascender = inlineBlockAscender(box->asFrameBlockBox());
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
                maxDescenderSoFar = std::min(ascender - boxHeight, maxDescenderSoFar);
            } else if (va == VerticalAlignValue::TextBottomVAlignValue) {
                maxAscenderSoFar = std::max(descender + boxHeight, maxAscenderSoFar);
            } else if (va == VerticalAlignValue::NumericVAlignValue) {
                LayoutUnit ascender = inlineBlockAscender(box->asFrameBlockBox());
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

    if (parentBox->isLineBox()) {
        if (!hasNormalFlowChild) {
            if (dueToBr) {
                return FontHeights {parentStyle->font()->metrics().m_ascender, parentStyle->font()->metrics().m_descender};
            }

            return FontHeights {0, 0};
        }

        if (!hasBoxOtherThanCollapsedInlineNonReplacedBox) {
            return FontHeights {0, 0};
        }
    } else {
        if (!hasNormalFlowChild) {
            if (parentBox->width() == 0 && parentBox->marginLeft() == 0 && parentBox->marginRight() == 0
                && parentBox->style()->hasNormalLineHeight()) {
                parentBox->asInlineBox()->asInlineNonReplacedBox()->markCollapsed();
                return FontHeights {ascender, descender};
            }
        }
    }

    // Consider parent's font ascender/descender
    LayoutUnit maxAscender = std::max(maxAscenderSoFar, ascender);
    LayoutUnit maxDescender = std::min(maxDescenderSoFar, descender);

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
        FrameBox* box = (*boxes)[k];
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

    LayoutUnit height = maxAscender - maxDescender;
    for (size_t k = 0; k < boxes->size(); k ++) {
        FrameBox* f = (*boxes)[k];

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
                        STARFISH_ASSERT(f->isFrameBlockBox() && ((f->style()->display() == InlineBlockDisplayValue) || (f->style()->display() == InlineTableDisplayValue)));
                        LayoutUnit ascender = inlineBlockAscender(f->asFrameBlockBox());
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
                    f->setY(maxAscender - ascender + marginTop);
                } else if (va == VerticalAlignValue::TextBottomVAlignValue) {
                    f->setY(maxAscender - descender - f->height() - marginBottom);
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

    return FontHeights {maxAscender, maxDescender};
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

static UBiDiDirection getTextDir(const StringView& sv, size_t start, size_t end)
{
    UTF16NonGCString str = sv.originalString()->toUTF16NonGCString(start, end);
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
    if (box->style()->unicodeBidi() == UnicodeBidiValue::IsolateUnicodeBidiValue) {
        return true;
    } else if (box->style()->unicodeBidi() == UnicodeBidiValue::EmbedUnicodeBidiValue) {
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
                return fetchContentForResolveBidi(b->boxes()[0]);
            }
            return b;
        } else {
            return box;
        }
    } else {
        return box;
    }
}

void LineFormattingContext::splitInlineBoxesAndMarkDirectionForResolveBidi(DirectionValue parentDir, std::vector<FrameBox*, gc_allocator_ignore_off_page<FrameBox*>>& boxes)
{
    for (size_t i = 0; i < boxes.size(); i ++) {
        FrameBox* box = boxes[i];
        if (box->isInlineBox()) {
            InlineBox* ib = box->asInlineBox();
            if (ib->isInlineTextBox()) {
            } else if (ib->isInlineNonReplacedBox()) {
                if (!hasIsolateBidiContent(ib->asInlineNonReplacedBox())) {
                    splitInlineBoxesAndMarkDirectionForResolveBidi(parentDir, ib->asInlineNonReplacedBox()->boxes());
                    // split every content for resolve bidi after
                    InlineNonReplacedBox* inrb = ib->asInlineNonReplacedBox();
                    STARFISH_ASSERT(m_dataForRestoreLeftRightOfMBPAfterResolveBidiLinePerLine.find(inrb->origin()) == m_dataForRestoreLeftRightOfMBPAfterResolveBidiLinePerLine.end());

                    DataForRestoreLeftRightOfMBPAfterResolveBidiLinePerLine mbpData(inrb);

                    m_dataForRestoreLeftRightOfMBPAfterResolveBidiLinePerLine.insert({inrb->origin(), mbpData});

                    std::vector<FrameBox*, gc_allocator_ignore_off_page<FrameBox*>>& boxesNeedsCopy = inrb->boxes();
                    if (boxesNeedsCopy.size()) {
                        FrameBox* parent = ib->layoutParent()->asFrameBox();
                        LayoutUnit x = box->x();

                        STARFISH_ASSERT(boxesNeedsCopy.size());
                        boxes.erase(boxes.begin() + i);

                        size_t insertPos = i;

                        for (size_t j = 0; j < boxesNeedsCopy.size(); j++) {
                            InlineNonReplacedBox* newBox = new InlineNonReplacedBox(inrb);
                            newBox->setX(boxesNeedsCopy[j]->x() + ib->x());
                            newBox->setY(ib->y());
                            boxesNeedsCopy[j]->setX(0);
                            newBox->insertInlineBox(boxesNeedsCopy[j]);
                            newBox->setLayoutParent(parent);
                            if (boxesNeedsCopy[j]->isNormalFlow()) {
                                newBox->setWidth(boxesNeedsCopy[j]->width());
                            }
                            newBox->setHeight(ib->height());
                            newBox->m_ascender = inrb->ascender();
                            newBox->m_descender = inrb->decender();
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

CharDirection LineFormattingContext::contentDir(FrameBox* box)
{
    if (box->isInlineBox()) {
        InlineBox* ib = box->asInlineBox();
        if (ib->isInlineTextBox()) {
            return ib->asInlineTextBox()->charDirection();
        } else if (ib->isInlineNonReplacedBox()) {
            InlineNonReplacedBox* b = ib->asInlineNonReplacedBox();
            if (hasIsolateBidiContent(b)) {
                // when unicode-bidi property is isolate, we should return neutral direction
                return CharDirection::Neutral;
            } else {
                const std::vector<FrameBox*, gc_allocator_ignore_off_page<FrameBox*>>& boxes = b->boxes();
                for (size_t i = 0; i < boxes.size(); i ++) {
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
        } else {
            return CharDirection::Neutral;
        }
    } else if (box->isFrameBox()) {
        auto iter = m_computedDirectionValuePerFrame.find(box);
        if (iter != m_computedDirectionValuePerFrame.end()) {
            return (iter->second == DirectionValue::LtrDirectionValue) ? CharDirection::Ltr : CharDirection::Rtl;
        } else {
            return CharDirection::Neutral;
        }
    } else {
        return CharDirection::Neutral;
    }
}

void LineFormattingContext::reassignLeftRightMBPOfInlineNonReplacedBoxPreProcess(std::vector<FrameBox*, gc_allocator_ignore_off_page<FrameBox*>>& boxes)
{
    for (size_t i = 0; i < boxes.size(); i ++) {
        FrameBox* box = boxes[i];
        if (box->isInlineBox()) {
            InlineBox* ib = box->asInlineBox();
            if (ib->isInlineTextBox()) {
            } else if (ib->isInlineNonReplacedBox()) {
                if (!hasIsolateBidiContent(ib->asInlineNonReplacedBox())) {
                    reassignLeftRightMBPOfInlineNonReplacedBoxPreProcess(ib->asInlineNonReplacedBox()->boxes());

                    // find last box
                    FrameInline* origin = ib->asInlineNonReplacedBox()->origin();
                    m_checkLastInlineNonReplacedPerLine[origin] = ib->asInlineNonReplacedBox();
                }
            }
        }
    }
}

void LineFormattingContext::reassignLeftRightMBPOfInlineNonReplacedBox(std::vector<FrameBox*, gc_allocator_ignore_off_page<FrameBox*>>& boxes)
{
    for (size_t i = 0; i < boxes.size(); i ++) {
        FrameBox* box = boxes[i];
        if (box->isInlineBox()) {
            InlineBox* ib = box->asInlineBox();
            if (ib->isInlineTextBox()) {
            } else if (ib->isInlineNonReplacedBox()) {
                if (!hasIsolateBidiContent(ib->asInlineNonReplacedBox())) {
                    reassignLeftRightMBPOfInlineNonReplacedBox(ib->asInlineNonReplacedBox()->boxes());

                    // restore left
                    FrameInline* origin = ib->asInlineNonReplacedBox()->origin();
                    auto iter = m_dataForRestoreLeftRightOfMBPAfterResolveBidiLinePerLine.find(origin);

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
                    if (m_checkLastInlineNonReplacedPerLine[origin] == ib) {
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

void LineFormattingContext::resolveBidi(DirectionValue parentDir, std::vector<FrameBox*, gc_allocator_ignore_off_page<FrameBox*>>& boxes)
{
    splitInlineBoxesAndMarkDirectionForResolveBidi(parentDir, boxes);

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
                    const StringView& sv = ib->asInlineTextBox()->textRun().m_stringView;
                    if (isNumber(sv.originalString(), sv.start(), sv.end())) {
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
                    const StringView& sv = ib->asInlineTextBox()->textRun().m_stringView;
                    if (isNumber(sv.originalString(), sv.start(), sv.end())) {
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
        std::vector<FrameBox*, gc_allocator_ignore_off_page<FrameBox*>> oldBoxes = std::move(boxes);
        boxes.reserve(oldBoxes.size());
        std::vector<FrameBox*> rtlStorage; // use normal allocator because oldBoxes has strong reference

        CharDirection currentDirection = CharDirection::Ltr;
        for (size_t i = 0; i < oldBoxes.size(); i ++) {
            CharDirection currentDirectionBefore = currentDirection;
            FrameBox* box = fetchContentForResolveBidi(oldBoxes[i]);
            CharDirection dir = contentDir(box);
            if (dir == CharDirection::Neutral) {
                CharDirection nextDir = CharDirection::Ltr;
                for (size_t j = i + 1; j < oldBoxes.size(); j ++) {
                    nextDir = contentDir(fetchContentForResolveBidi(oldBoxes[j]));
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
                        const StringView& sv = ib->asInlineTextBox()->textRun().m_stringView;
                        if (sv.originalString()->containsOnlyWhitespace(sv.start(), sv.end())) {
                        } else {
                            if (isNumber(sv.originalString(), sv.start(), sv.end())) {
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
        std::vector<FrameBox*, gc_allocator_ignore_off_page<FrameBox*>> oldBoxes = std::move(boxes);
        boxes.reserve(oldBoxes.size());
        std::vector<FrameBox*> ltrStorage; // use normal allocator because oldBoxes has strong reference

        CharDirection currentDirection = CharDirection::Rtl;
        for (size_t i = 0; i < oldBoxes.size(); i ++) {
            CharDirection currentDirectionBefore = currentDirection;
            FrameBox* box = fetchContentForResolveBidi(oldBoxes[i]);
            CharDirection dir = contentDir(box);
            if (dir == CharDirection::Neutral) {
                CharDirection nextDir = CharDirection::Rtl;
                for (size_t j = i + 1; j < oldBoxes.size(); j ++) {
                    nextDir = contentDir(fetchContentForResolveBidi(oldBoxes[j]));
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
                        const StringView& sv = ib->asInlineTextBox()->textRun().m_stringView;
                        if (sv.originalString()->containsOnlyWhitespace(sv.start(), sv.end())) {
                        } else {
                            if (isNumber(sv.originalString(), sv.start(), sv.end())) {
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
        0, 0, 0, 125, 0, 123, 0, 0
    };

    for (size_t i = 0; i < boxes.size(); i ++) {
        FrameBox* box = fetchContentForResolveBidi(boxes[i]);
        if (box->isInlineBox()) {
            if (box->asInlineBox()->isInlineTextBox() && box->asInlineBox()->asInlineTextBox()->charDirection() == CharDirection::Rtl) {
                InlineTextBox* b = box->asInlineBox()->asInlineTextBox();
                const StringView& sv = b->textRun().m_stringView;
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
                    if (sv.originalString()->isASCIIString()) {
                        ASCIIString str;
                        for (size_t j = sv.start(); j < sv.end(); j++) {
                            char32_t ch = sv.originalString()->charAt(j);
                            if (ch < 128) {
                                if (parenthesisMap[ch]) {
                                    ch = (char32_t) parenthesisMap[ch];
                                }
                            }
                            str += (char)ch;
                        }
                        b->setText(new StringDataASCII(std::move(str)));
                    } else {
                        UTF32String str;
                        for (size_t j = sv.start(); j < sv.end(); j++) {
                            char32_t ch = sv.originalString()->charAt(j);
                            if (ch < 128) {
                                if (parenthesisMap[ch]) {
                                    ch = (char32_t) parenthesisMap[ch];
                                }
                            }
                            str += ch;
                        }
                        b->setText(new StringDataUTF32(std::move(str)));
                    }
                }
            }
        }
    }


    // reassign left, right mbp for inlineNonReplacedBox
    reassignLeftRightMBPOfInlineNonReplacedBoxPreProcess(boxes);
    reassignLeftRightMBPOfInlineNonReplacedBox(boxes);

    m_dataForRestoreLeftRightOfMBPAfterResolveBidiLinePerLine.clear();
    m_checkLastInlineNonReplacedPerLine.clear();
}

static void removeBoxFromLine(FrameBox* box)
{
    if (box->layoutParent()->asFrameBox()->isLineBox()) {
        auto& boxes = box->layoutParent()->asFrameBox()->asLineBox()->boxes();
        boxes.erase(std::find(boxes.begin(), boxes.end(), box));
    } else {
        auto& boxes = box->layoutParent()->asFrameBox()->asInlineBox()->asInlineNonReplacedBox()->boxes();
        auto self = box->layoutParent()->asFrameBox()->asInlineBox()->asInlineNonReplacedBox();
        LayoutUnit w = box->asInlineBox()->asInlineTextBox()->width();
        while (true) {
            self->setWidth(self->width() - w);
            if (self->layoutParent()->asFrameBox()->isLineBox()) {
                break;
            }
            self = self->layoutParent()->asFrameBox()->asInlineBox()->asInlineNonReplacedBox();
        }
        boxes.erase(std::find(boxes.begin(), boxes.end(), box));
    }
}

LineFormattingContext::LineFormattingContext(FrameBlockBox& block, LayoutContext& ctx, const LayoutUnit& lineBoxX, const LayoutUnit& lineBoxY, const LayoutUnit& lineBoxWidth)
    : m_lineBoxY(lineBoxY)
    , m_unprocessedStartingMBPWidth(0)
    , m_block(block)
    , m_layoutContext(ctx)
    , m_inlineBoxIndex(0)
{
    m_absPosition = block.absolutePoint(m_layoutContext.frameDocument()->asFrameBox());
    m_leftBoundary = m_absPosition.x() + block.paddingLeft() + block.borderLeft();
    m_rightBoundary = m_leftBoundary + block.contentWidth();
    m_block.m_lineBoxes.clear();
    // m_block.m_lineBoxes.shrink_to_fit();
    resetLineBox();
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
        if (hasNormalFlowContent) {
            m_layoutContext.registerYPositionForVerticalAlignInlineBlock(lb);
        }
    }
}

template <typename Box>
FrameBox* InlineBoxLayoutParentBox<Box>::findLastInlineBox()
{
    for (size_t i = m_boxes.size() - 1; i != SIZE_MAX; i--) {
        if (m_boxes[i]->isInlineBox()) {
            InlineBox* b = m_boxes[i]->asInlineBox();
            if (b->isInlineNonReplacedBox()) {
                auto r = b->asInlineNonReplacedBox()->findLastInlineBox();
                if (r) {
                    return r;
                }
            } else {
                return b;
            }
        } else if (m_boxes[i]->isNormalFlow()) {
            return m_boxes[i];
        }
    }

    return nullptr;
}

template <typename Box>
LayoutUnit InlineBoxLayoutParentBox<Box>::layoutInlineBoxes(LayoutUnit start)
{
    LayoutUnit x = start;
    for (size_t k = 0; k < m_boxes.size(); k++) {
        FrameBox* childBox = m_boxes[k];
        if (childBox->isNormalFlow()) {
            childBox->setX(x + childBox->marginLeft());
            x += childBox->boxWidth();
        } else if (childBox->style()->position() == AbsolutePositionValue) {
            childBox->setX(x);
        }
    }

    return x;
}

template <typename Box>
void InlineBoxLayoutParentBox<Box>::registerRelativePositionInlineBoxes(LayoutContext& ctx)
{
    for (size_t k = 0; k < m_boxes.size(); k++) {
        FrameBox* childBox = m_boxes[k];
        STARFISH_ASSERT(childBox != nullptr);

        if (!childBox->isFrameBlockBox()) {
            if (childBox->style()->position() == PositionValue::RelativePositionValue) {
                ctx.registerRelativePositionedFrames(childBox, true);
            }

            if (childBox->isInlineBox() && childBox->asInlineBox()->isInlineNonReplacedBox()) {
                childBox->asInlineBox()->asInlineNonReplacedBox()->registerRelativePositionInlineBoxes(ctx);
            }
        }
    }
}

void LineFormattingContext::markInlineBoxIndex(FrameBox* box)
{
    box->setInlineBoxIndex(m_inlineBoxIndex);
    m_inlineBoxIndex++;
}

static bool dontBreakLine(LineFormattingContext* ctx, Frame* f, LayoutUnit width);
static bool clearAffected(int hasFloat, Frame* f)
{
    return (f->style()->clear() == LeftClearValue && (hasFloat & LineFormattingContext::HasLeft) == 0)
        || (f->style()->clear() == RightClearValue && (hasFloat & LineFormattingContext::HasRight) == 0)
        || (f->style()->clear() == BothClearValue && hasFloat == LineFormattingContext::HasNone);
}
static bool canInsertFloatingBox(LineFormattingContext* ctx, FrameBox* f)
{
    FloatingBoxLayoutContext* fbCtx = &ctx->m_floatingBoxLayoutContexts[ctx->m_floatingBoxLayoutContexts.size() - 1];

    if (f->style()->clear() == NoneClearValue || clearAffected(fbCtx->m_hasFloat, f)) {
    } else {
        ctx->makeFloatingBoxLayoutContextDueToClearIfNeeds(f);
    }

    LineBox* lineBox = ctx->currentLine();
    fbCtx = &ctx->m_floatingBoxLayoutContexts[ctx->m_floatingBoxLayoutContexts.size() - 1];

    return ctx->m_absPosition.y() + lineBox->y() + fbCtx->m_y >= ctx->m_layoutContext.lastTopLoc();
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

void LineFormattingContext::makeFloatingBoxLayoutContextDueToClearIfNeeds(FrameBox* box)
{
    LayoutUnit clearedDistanceToFloatBottom = m_layoutContext.clearedDistanceToFloatBottom(m_absPosition.y() + m_lineBoxY, box->style()->clear());

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
        if ((m_pendingFloatingBoxNumsBeforeCurrentLine > 0 || !onlyAllowBeforeCurrentLine)
            && canInsertFloatingBox(this, box)
            && dontBreakLine(this, box, box->boxWidth())) {
            generateFloatingBoxAndReLayoutLineBoxIfNeeds(box);

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

void LineFormattingContext::unregisterAbsolutePositionedBoxes()
{
    LineBox* lineBox = currentLine();
    DirectionValue dir = m_block.style()->direction();
    for (size_t i = 0; i < m_absolutePositionedBoxes.size(); i ++) {
        FrameBox* box = m_absolutePositionedBoxes[i].first;
        if (box->style()->originalDisplay() == BlockDisplayValue) {
            if (dir == DirectionValue::LtrDirectionValue) {
                box->setX(m_leftBoundary - m_absPosition.x() - m_lineBoxX);
            } else {
                box->setX(m_rightBoundary - m_absPosition.x() - m_lineBoxX);
            }

            if (m_absolutePositionedBoxes[i].second) {
                box->setY(lineBox->height());
            } else {
                box->setY(0);
            }
        } else {
            box->setY(0);
            FrameBox* parent = box->layoutParent()->asFrameBox();
            if (parent->isLineBox()) {
                STARFISH_ASSERT(parent == lineBox);
                auto& boxes = parent->asLineBox()->boxes();
                boxes.erase(std::find(boxes.begin(), boxes.end(), box));
            } else {
                auto& boxes = parent->asInlineBox()->asInlineNonReplacedBox()->boxes();
                auto pos = box->absolutePoint(lineBox);
                boxes.erase(std::find(boxes.begin(), boxes.end(), box));
                box->setX(pos.x());
            }
        }
        lineBox->insertInlineBox(box);
    }

    m_absolutePositionedBoxes.clear();
}

void LineFormattingContext::layoutLineBox(LayoutUnit yDiff, LayoutUnit height)
{
    LineBox* lineBox = currentLine();
    int hasFloat = HasNone;
    std::pair<LayoutUnit, LayoutUnit> boundaries =
        m_layoutContext.horizontalBoundaryBetweenFloatingBoxes(m_absPosition.y() + m_lineBoxY + yDiff, height, m_leftBoundary, m_rightBoundary);
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
        FloatingBoxLayoutContext fbCtx = FloatingBoxLayoutContext(hasFloat, yDiff, m_lineBoxX, m_lineBoxWidth);
        m_floatingBoxLayoutContexts.push_back(fbCtx);
    } else {
        if (m_floatingBoxLayoutContexts[m_floatingBoxLayoutContexts.size() - 1].m_y < yDiff) {
            FloatingBoxLayoutContext fbCtx = FloatingBoxLayoutContext(hasFloat, yDiff, m_lineBoxX, m_lineBoxWidth);
            m_floatingBoxLayoutContexts.push_back(fbCtx);
        }
    }
}

void LineFormattingContext::insertInlineNonReplacedBox(InlineNonReplacedBox* self, InlineNonReplacedBox* layoutParent)
{
    if (layoutParent) {
        layoutParent->insertInlineBox(self);
    } else {
        currentLine()->insertInlineBox(self);
        markInlineBoxIndex(self);
    }
}

void LineFormattingContext::generateFloatingBoxAndReLayoutLineBoxIfNeeds(FrameBox* box)
{
    if (box->height() == 0 && box->marginHeight() == 0) {
        return;
    }

    FloatingBoxLayoutContext& fbCtx = *m_floatingBoxLayoutContexts.rbegin();
    if (box->style()->floating() == LeftFloatValue) {
        fbCtx.m_hasFloat |= HasLeft;
        LayoutUnit lastAccumulatedLeftFloatBoxWidth = fbCtx.m_accumulatedLeftFloatBoxWidth;
        LayoutUnit oldLineBoxX = m_lineBoxX;
        fbCtx.m_accumulatedLeftFloatBoxWidth += box->boxWidth();
        if (fbCtx.m_y == 0 && box->boxWidth() > 0) {
            m_lineBoxX = fbCtx.m_originalLineBoxX + fbCtx.m_accumulatedLeftFloatBoxWidth;
            m_lineBoxWidth = fbCtx.m_originalLineBoxWidth - fbCtx.m_accumulatedLeftFloatBoxWidth - fbCtx.m_accumulatedRightFloatBoxWidth;
        }

        box->setX(fbCtx.m_originalLineBoxX + lastAccumulatedLeftFloatBoxWidth + box->marginLeft() - m_lineBoxX);

        if (oldLineBoxX != m_lineBoxX) {
            reCacheFloatingBoxes(oldLineBoxX - m_lineBoxX);
        }
    } else {
        fbCtx.m_hasFloat |= HasRight;
        fbCtx.m_accumulatedRightFloatBoxWidth += box->boxWidth();
        if (fbCtx.m_y == 0 && box->boxWidth() > 0) {
            m_lineBoxWidth = fbCtx.m_originalLineBoxWidth - fbCtx.m_accumulatedLeftFloatBoxWidth - fbCtx.m_accumulatedRightFloatBoxWidth;
        }

        LayoutUnit rightFloatX = fbCtx.m_originalLineBoxX + fbCtx.m_originalLineBoxWidth - fbCtx.m_accumulatedRightFloatBoxWidth;
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

static bool containOnlyWhiteSpace(FrameBox* box)
{
    if (!box) {
        return true;
    }

    if (box->isInlineBox() && box->asInlineBox()->isInlineTextBox()) {
        const StringView& sv = box->asInlineBox()->asInlineTextBox()->textRun().m_stringView;
        if (sv.length() == 1 && sv.originalString()->charAt(sv.start()) == ' ') {
            return true;
        }
    }

    return false;
}

void LineFormattingContext::removeDanglingSpaceFromLine()
{
    LineBox* lineBox = currentLine();

    FrameBox* last = lineBox->findLastInlineBox();
    while (last) {
        if (containOnlyWhiteSpace(last)) {
            removeBoxFromLine(last);
            m_currentLineWidth -= last->boxWidth();
        } else {
            break;
        }
        last = lineBox->findLastInlineBox();
    }
}

void LineFormattingContext::computeHorizontalProperties()
{
    LineBox* back = m_block.m_lineBoxes.back();

    resolveBidi(m_block.style()->direction(), back->boxes());

    back->setX(m_lineBoxX);
    back->setWidth(m_lineBoxWidth);
    LayoutUnit inlineBoxesWidth = back->layoutInlineBoxes(0);

    // text align
    if (m_block.style()->textAlign() == SideValue::LeftSideValue) {
    } else if (m_block.style()->textAlign() == SideValue::RightSideValue) {
        LayoutUnit diff = (m_lineBoxWidth - inlineBoxesWidth);
        for (size_t k = 0; k < back->m_boxes.size(); k++) {
            FrameBox* childBox = back->m_boxes[k];
            if (childBox->style()->floating() == NoneFloatValue) {
                childBox->moveX(diff);
            }
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
        LayoutUnit diff = (m_lineBoxWidth - inlineBoxesWidth) / 2;
        if (diff > 0) {
            for (size_t k = 0; k < back->m_boxes.size(); k++) {
                FrameBox* childBox = back->m_boxes[k];
                if (childBox->style()->floating() == NoneFloatValue) {
                    childBox->moveX(diff);
                }
            }
        }
    }
}

LayoutUnit LineFormattingContext::distanceToNextLineBox(FrameLineBreak* br, bool hasMoreInlineBoxes)
{
    LineBox* lineBox = currentLine();
    STARFISH_ASSERT(lineBox != nullptr);
    LayoutUnit distance = lineBox->height();
    FloatingBoxLayoutContext& fbCtx = (*m_floatingBoxLayoutContexts.rbegin());

    if (fbCtx.m_hasFloat != HasNone) {
        if (br != nullptr) {
            distance = std::max(lineBox->height(),
                m_layoutContext.clearedDistanceToFloatBottom(m_absPosition.y() + m_lineBoxY, br->style()->clear()));
            lineBox->setHeight(distance);
        } else {
            if (m_currentLineWidth == 0 || lineBox->height() == 0) {
                distance = m_layoutContext.nextDistanceToFloatBottom(m_absPosition.y() + m_lineBoxY, lineBox->height());
            }
        }
    }

    if (!hasMoreInlineBoxes && distance == 0 && m_pendingFloatingBoxes.size() > 0) {
        // if there are only pending float box and the height which y diff for the next linebox is zero,
        // then it goes infinite loop. It happens because 2 things. First, the rule that float box can't be inserted
        // into the line box when the last top position of float box whose direction is the same with the
        // direction of float box we are trying to insert. Second, this kind of float box can't be
        // detected by using current y position of line box and its height.
        LayoutUnit curYPos = m_absPosition.y() + m_lineBoxY;
        LayoutUnit yToPos = m_layoutContext.lastTopLoc();
        distance = yToPos - curYPos;
    }

    return distance;
}

template <typename Iter>
void LineFormattingContext::sortInlineBoxes(Iter& iter)
{
    std::sort(iter.begin(), iter.end(), [](FrameBox* a, FrameBox* b) {
        return a->inlineBoxIndex() > b->inlineBoxIndex();
    });
}

void LineFormattingContext::removeAllInlineBoxes()
{
    LineBox* lineBox = currentLine();
    auto iter = lineBox->boxes().begin();

    bool hasAlreadyPendingInlineBoxes = (m_pendingInlineBoxes.size() > 0);
    size_t index = 0;

    while (iter != lineBox->boxes().end()) {
        if ((*iter)->style()->floating() != NoneFloatValue && (*iter)->style()->position() != AbsolutePositionValue) {
            iter++;
            continue;
        }

        // already its x, y positions are somehow determined, so we have to reset the values.
        (*iter)->setX(0);
        (*iter)->setY(0);
        if (!hasAlreadyPendingInlineBoxes) {
            m_pendingInlineBoxes.push_back(*iter);
        } else {
            m_pendingInlineBoxes.insert(m_pendingInlineBoxes.begin() + index, *iter);
            index++;
        }

        iter = lineBox->boxes().erase(iter);
    }

    if (m_block.style()->direction() == RtlDirectionValue) {
        sortInlineBoxes(m_pendingInlineBoxes);
        sortInlineBoxes(lineBox->boxes());
    }

#ifndef NDEBUG
    iter = lineBox->boxes().begin();
    // LineBox should only contain floating boxes.
    while (iter != lineBox->boxes().end()) {
        STARFISH_ASSERT((*iter)->style()->floating() != NoneFloatValue);
        iter++;
    }

    // Pending inline boxes should be put by the order as they were initially inserted into the line box.
    size_t lastInlineboxIndex = SIZE_MAX;
    auto iter2 = m_pendingInlineBoxes.begin();

    while (iter2 != m_pendingInlineBoxes.end()) {
        if (lastInlineboxIndex != SIZE_MAX) {
            STARFISH_ASSERT((*iter2)->inlineBoxIndex() > lastInlineboxIndex);
        }

        lastInlineboxIndex = (*iter2)->inlineBoxIndex();
        iter2++;
    }
#endif
}

void LineFormattingContext::insertPendingInlineBoxes()
{
    bool firstWhite = true;
    auto iter = m_pendingInlineBoxes.begin();

    while (iter != m_pendingInlineBoxes.end()) {
        FrameBox* box = *iter;
        STARFISH_ASSERT(box->isNormalFlow() || box->style()->position() == AbsolutePositionValue);

        if (firstWhite) {
            if (containOnlyWhiteSpace(box)) {
                iter = m_pendingInlineBoxes.erase(iter);
                continue;
            }
        }

        firstWhite = false;
        if (dontBreakLine(this, box, box->boxWidth())) {
            m_currentLineWidth += box->boxWidth();

            currentLine()->insertInlineBox(box);

            iter = m_pendingInlineBoxes.erase(iter);
            continue;
        }
        break;
    }
}

void LineFormattingContext::resetLineBox()
{
    LineBox* lineBox = new LineBox(&m_block);
    m_block.m_lineBoxes.push_back(lineBox);
    m_floatingBoxLayoutContexts.clear();
    layoutLineBox(0, 0);
    m_currentLineWidth = 0;
    if (m_pendingInlineBoxes.size() == 0) {
        m_pendingFloatingBoxNumsBeforeCurrentLine = m_pendingFloatingBoxes.size();
    }
    m_shouldLineBreakForAbsolutePositionedBox = false;
    m_shouldLineBreakForBr = true;
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
        } else if (childBox->style()->position() == AbsolutePositionValue) {
            continue;
        } else {
            STARFISH_ASSERT(childBox->style()->floating() != NoneFloatValue);
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
        if ((*iter)->style()->floating() != NoneFloatValue && (*iter)->style()->position() != AbsolutePositionValue) {
            iter++;
            continue;
        }

        FrameBox* box = *iter;

        // Check boundary as if inline boxes are not vertical aligned yet, so we don't consider box->y() here.
        if (m_layoutContext.isCollidedWithFloatingBoxes(LayoutLocation(m_absPosition.x() + m_lineBoxX + box->x(), m_absPosition.y() + m_lineBoxY),
            box, m_leftBoundary, m_rightBoundary)) {
            return true;
        }

        iter++;
    }

    return false;
}

void LineFormattingContext::finishLineForLineBox(FrameLineBreak* br, bool isLastLine)
{
    LineBox* back = currentLine();

    reComputeVerticalProperties:
    FontHeights fontHeights = computeVerticalProperties(back, m_block.style(), br != nullptr);
    LayoutUnit height = fontHeights.m_ascender - fontHeights.m_descender;

    FloatingBoxLayoutContext& fbCtx = *m_floatingBoxLayoutContexts.begin();
    if (fbCtx.m_hasFloat != HasNone && m_pendingFloatingBoxes.size() > 0 && height > 0) {
        LayoutUnit nextDistanceToFloatBottom = m_layoutContext.nextDistanceToFloatBottom(m_absPosition.y() + m_lineBoxY, 0);
        FloatingBoxLayoutContext* lastFbCtx = &(*m_floatingBoxLayoutContexts.rbegin());
        if (nextDistanceToFloatBottom != 0 && height > nextDistanceToFloatBottom && nextDistanceToFloatBottom > lastFbCtx->m_y) {
            makeFloatingBoxLayoutContext(nextDistanceToFloatBottom);
            insertPendingFloatingBoxes();
            removeAllInlineBoxes();
            m_currentLineWidth = 0;
            insertPendingInlineBoxes();

            goto reComputeVerticalProperties;
        }
    }

    back->m_ascender = fontHeights.m_ascender;
    back->m_descender = fontHeights.m_descender;
    back->setHeight(height);
    removeDanglingSpaceFromLine();
    // Should check if there has enough space for pending block box due to removing
    // white space from above function `removeDanglingSpaceFromLine`
    insertPendingFloatingBoxes();
    computeHorizontalProperties();
    registerInlineContent();

    if (m_layoutContext.isCollidedWithFloatingBoxes(LayoutLocation(m_absPosition.x() + m_lineBoxX, m_absPosition.y() + m_lineBoxY),
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

            goto reComputeVerticalProperties;
        }
    }

    LayoutUnit yDiff = distanceToNextLineBox(br, !isLastLine || m_pendingInlineBoxes.size() > 0);

    unregisterAbsolutePositionedBoxes();
    m_lineBoxY += yDiff;

    if (isLastLine) {
        if (m_pendingInlineBoxes.size() > 0 || m_pendingFloatingBoxes.size() > 0) {
            breakLineForLineBox(nullptr, isLastLine, true);
        }
    }
}

void LineFormattingContext::breakLineForLineBox(FrameLineBreak* br, bool isLastLine, bool skipFinishLine)
{
    /* if (dueToBr == false)
        m_breakedLinesSet.insert(m_block.m_lineBoxes.size() - 1); */

    if (!skipFinishLine) {
        finishLineForLineBox(br, isLastLine);
    }

    resetLineBox();

    insertPendingFloatingBoxes();
    insertPendingInlineBoxes();

    if (m_pendingInlineBoxes.size() > 0) {
        breakLineForLineBox(nullptr, isLastLine, false);
    } else if (isLastLine) {
        finishLineForLineBox(nullptr, isLastLine);
    }
}

static bool hasBreakableWhiteSpaceProperty(Frame* f)
{
    return f->style()->whiteSpace() == WhiteSpaceValue::NormalWhiteSpaceValue;
}

static bool canInsertInlineBox(LineFormattingContext* ctx, Frame* f, LayoutUnit width)
{
    if (f->style()->floating() == NoneFloatValue) {
        return width <= (ctx->m_lineBoxWidth - ctx->m_currentLineWidth - ctx->m_unprocessedStartingMBPWidth);
    } else {
        FloatingBoxLayoutContext& fbCtx = *ctx->m_floatingBoxLayoutContexts.rbegin();
        LayoutUnit remainedWidth = fbCtx.m_originalLineBoxWidth - fbCtx.m_accumulatedLeftFloatBoxWidth - fbCtx.m_accumulatedRightFloatBoxWidth;
        if (fbCtx.m_y == 0) {
            return width <= (remainedWidth - ctx->m_currentLineWidth - ctx->m_unprocessedStartingMBPWidth);
        } else {
            return width <= remainedWidth;
        }
    }
}

static bool dontBreakLine(LineFormattingContext* ctx, Frame* f, LayoutUnit width)
{
    return (!ctx->hasFloatingBoxAlreadyInLineBox(f) && ctx->m_currentLineWidth == 0)
        || canInsertInlineBox(ctx, f, width)
        || !hasBreakableWhiteSpaceProperty(f);
}

void LineFormattingContext::generateInlineBox(FrameBox* box)
{
    if (m_currentLayoutParent->isInlineBox() && m_currentLayoutParent->asInlineBox()->isInlineNonReplacedBox()) {
        InlineNonReplacedBox* self = m_currentLayoutParent->asInlineBox()->asInlineNonReplacedBox();
        self->insertInlineBox(box);
        self->setWidth(self->width() + box->boxWidth());
        self->setStartingMBP(this);
    } else {
        currentLine()->insertInlineBox(box);
        markInlineBoxIndex(box);
    }
    m_shouldLineBreakForAbsolutePositionedBox = true;
}

void LineFormattingContext::registerAbsolutePositionedBox(FrameBox* box)
{
    m_layoutContext.registerAbsolutePositionedFrames(box);
    m_absolutePositionedBoxes.push_back(std::make_pair(box, m_shouldLineBreakForAbsolutePositionedBox));
    markInlineBoxIndex(box);

    if (box->style()->originalDisplay() != BlockDisplayValue) {
        if (m_currentLayoutParent->isInlineBox() && m_currentLayoutParent->asInlineBox()->isInlineNonReplacedBox()) {
            m_currentLayoutParent->asInlineBox()->asInlineNonReplacedBox()->insertInlineBox(box);
        } else {
            currentLine()->insertInlineBox(box);
        }
    }
}

void LineFormattingContext::breakLine(FrameLineBreak* br)
{
    if (m_currentLayoutParent->isInlineBox() && m_currentLayoutParent->asInlineBox()->isInlineNonReplacedBox()) {
        breakLineForInlineNonReplacedBox(br);
    } else {
        breakLineForLineBox(br, false, false);
    }
}

void LineFormattingContext::generateInlineTextBox(FrameText* f, LayoutUnit textWidth, String* srcTxt, size_t offset, size_t nextOffset, bool isWhiteSpace)
{
    const std::vector<TextRun>& runs = m_textRunsPerFrameText[f->asFrameText()];
    if (isWhiteSpace) {
        CharDirection dir = CharDirection::Ltr;
        for (size_t i = 0; i < runs.size(); i ++) {
            if (offset <= runs[i].m_stringView.start() && runs[i].m_stringView.end() <= nextOffset) {
                dir = runs[i].m_direction;
                break;
            }
        }
        InlineBox* ib = new InlineTextBox(f->asFrameText(), TextRun(f->asFrameText(), String::spaceString, 0, 1, dir));
        ib->setWidth(textWidth);
        ib->setHeight(f->style()->font()->metrics().m_fontHeight);
        generateInlineBox(ib);
    } else {
        size_t startPos = offset;
        size_t endPos = nextOffset;

        std::set<size_t> splitPosition;

        for (size_t i = 0; i < runs.size(); i ++) {
            TextRun r = runs[i];
            if (offset < r.m_stringView.start() && r.m_stringView.start() < nextOffset) {
                splitPosition.insert(r.m_stringView.start());
            }
            if (offset < r.m_stringView.end() && r.m_stringView.end() < nextOffset) {
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
                for (size_t i = 0; i < runs.size(); i ++) {
                    if (runs[i].m_stringView.start() <= start && end <= runs[i].m_stringView.end()) {
                        dir = runs[i].m_direction;
                        break;
                    }
                }

                InlineBox* ib = new InlineTextBox(f->asFrameText(), TextRun(f->asFrameText(), srcTxt, start, end, dir));
                ib->setWidth(f->style()->font()->measureText(ib->asInlineTextBox()->textRun().m_stringView));
                ib->setHeight(f->style()->font()->metrics().m_fontHeight);
                generateInlineBox(ib);
                start = end;
                iter++;
            }
            STARFISH_ASSERT(end != SIZE_MAX);
            if (end != nextOffset) {
                CharDirection dir = CharDirection::Ltr;
                for (size_t i = 0; i < runs.size(); i ++) {
                    if (runs[i].m_stringView.start() <= end && nextOffset <= runs[i].m_stringView.end()) {
                        dir = runs[i].m_direction;
                        break;
                    }
                }
                InlineBox* ib = new InlineTextBox(f->asFrameText(), TextRun(f->asFrameText(), srcTxt, end, nextOffset, dir));
                ib->setWidth(f->style()->font()->measureText(ib->asInlineTextBox()->textRun().m_stringView));
                ib->setHeight(f->style()->font()->metrics().m_fontHeight);
                generateInlineBox(ib);
            }
        } else {
            CharDirection dir = CharDirection::Ltr;
            for (size_t i = 0; i < runs.size(); i ++) {
                if (runs[i].m_stringView.start() <= offset && nextOffset <= runs[i].m_stringView.end()) {
                    dir = runs[i].m_direction;
                    break;
                }
            }
            InlineBox* ib = new InlineTextBox(f->asFrameText(), TextRun(f->asFrameText(), srcTxt, offset, nextOffset, dir));
            ib->setWidth(textWidth);
            ib->setHeight(f->style()->font()->metrics().m_fontHeight);
            generateInlineBox(ib);
        }
    }
    m_currentLineWidth += textWidth;
}

void LineFormattingContext::generateInlineNonReplacedBox(FrameInline* f)
{
    if (m_currentLayoutParent->isInlineBox() && m_currentLayoutParent->asInlineBox()->isInlineNonReplacedBox()) {
        InlineNonReplacedBox* inlineBox = new InlineNonReplacedBox(f);
        insertInlineNonReplacedBox(inlineBox, m_currentLayoutParent->asInlineBox()->asInlineNonReplacedBox());
        inlineBox->layoutInline(this);
        FrameBox* newLayoutParent = m_currentLayoutParent->layoutParent()->asFrameBox();
        newLayoutParent->setWidth(newLayoutParent->width() + m_currentLayoutParent->boxWidth());
        m_currentLayoutParent = newLayoutParent;
    } else {
        FrameBox* oldLayoutParent = m_currentLayoutParent;
        InlineNonReplacedBox* inlineBox = new InlineNonReplacedBox(f);
        insertInlineNonReplacedBox(inlineBox, nullptr);
        inlineBox->layoutInline(this);
        STARFISH_ASSERT(m_unprocessedStartingMBPWidth == 0);

        if (currentLine()->boxes().size() == 1) {
            FrameBox* box = currentLine()->boxes().back();
            if (box->isInlineBox() && box->asInlineBox()->isInlineNonReplacedBox()) {
                // TODO: We should remove line box whose boxes' size is 0, and in below condition,
                // add more condition which there was absolute position set its layout parent to this line box
                // to be removed, or the lastInlineBox's line-height or vertical-align which set to
                // different value.
                InlineNonReplacedBox* lastInlineBox = box->asInlineBox()->asInlineNonReplacedBox();
                if (lastInlineBox->boxes().size() == 1) {
                    if (containOnlyWhiteSpace(lastInlineBox->boxes().back())) {
                        m_currentLineWidth = 0;
                        currentLine()->boxes().erase(currentLine()->boxes().end() - 1);
                    }
                }
            }
        }
        m_currentLayoutParent = oldLayoutParent;
    }
}

void LineFormattingContext::generateInlineBoxes(Frame *origin)
{
    Frame* f = origin->firstChild();
    while (f) {
        // Don't put any inline box leaving pending inline boxes ahead.
        STARFISH_ASSERT(m_pendingInlineBoxes.size() == 0);

        if (f->style() && f->style()->position() == PositionValue::AbsolutePositionValue) {
            registerAbsolutePositionedBox(f->asFrameBox());
            f = f->next();
            continue;
        }

        if (f->isFrameText()) {
            String* txt = f->asFrameText()->text();
            // split the text into tokens using the ICU divider, and for each token, execute the following function
            textDividerForLayout(m_layoutContext.starFish(), txt, [&, f, origin](String* srcTxt, size_t offset, size_t nextOffset, bool isWhiteSpace, bool canBreak) {
                textAppendRetry:
                if (isWhiteSpace) {
                    if (offset == 0) {
                        FrameBox* last = currentLine()->findLastInlineBox();

                        if (containOnlyWhiteSpace(last)) {
                            return;
                        }
                    } else if (nextOffset == srcTxt->length() && f == origin->lastChild()) {
                        if (!origin->isFrameInline()) {
                            return;
                        }
                    } else if (m_currentLineWidth == 0) {
                        return;
                    }
                }

                LayoutUnit textWidth;
                if (isWhiteSpace) {
                    textWidth = f->style()->font()->spaceWidth();
                } else {
                    textWidth = f->style()->font()->measureText(StringView(srcTxt, offset, nextOffset));
                }

                if (dontBreakLine(this, f, textWidth)) {
                } else {
                    // try this at nextline
                    breakLine(nullptr);
                    m_shouldLineBreakForBr = false;
                    goto textAppendRetry;
                }
                m_shouldLineBreakForBr = true;

                generateInlineTextBox(f->asFrameText(), textWidth, srcTxt, offset, nextOffset, isWhiteSpace);
            });

        } else if (f->isFrameReplaced()) {
            m_shouldLineBreakForAbsolutePositionedBox = true;
            FrameReplaced* r = f->asFrameReplaced();

            r->layout(m_layoutContext, Frame::LayoutWantToResolve::ResolveAll);

            if (r->style()->floating() != NoneFloatValue) {
                markInlineBoxIndex(r);
                if (m_pendingFloatingBoxes.size() == 0
                    && canInsertFloatingBox(this, r)
                    && dontBreakLine(this, r, r->boxWidth())) {
                    generateFloatingBoxAndReLayoutLineBoxIfNeeds(r);
                } else {
                    m_pendingFloatingBoxes.push_back(r);
                }
            } else {
                insertReplacedBox:
                if (dontBreakLine(this, r, r->boxWidth())) {
                    m_currentLineWidth += (r->boxWidth());
                    generateInlineBox(r);
                } else {
                    breakLine(nullptr);
                    goto insertReplacedBox;
                }
            }
        } else if (f->isFrameBlockBox() || f->isFrameTableBox()) {
            FrameBlockBox* r = f->asFrameBlockBox();

            if ((f->style()->display() == DisplayValue::InlineBlockDisplayValue) || (f->style()->display() == DisplayValue::InlineTableDisplayValue)) {
                m_shouldLineBreakForAbsolutePositionedBox = true;
                // inline-block, inline-table
                m_layoutContext.pushInlineBlockBox(r);
                f->setLayoutParent(m_currentLayoutParent);
                f->layout(m_layoutContext, Frame::LayoutWantToResolve::ResolveAll);
                LayoutUnit ascender;

                std::pair<bool, LayoutUnit> p = m_layoutContext.readRegisteredLastLineBoxYPos(r);
                if (p.first && r->style()->overflow() == OverflowValue::VisibleOverflow) {
                    ascender = p.second;
                } else {
                    ascender = f->asFrameBox()->height();
                }
                m_layoutContext.popInlineBlockBox();
                registerInlineBlockAscender(ascender, r);

                insertInlineBlockBox:
                if (dontBreakLine(this, r, r->boxWidth())) {
                    m_currentLineWidth += (r->boxWidth());
                    generateInlineBox(r);
                } else {
                    breakLine(nullptr);
                    goto insertInlineBlockBox;
                }
            } else {
                STARFISH_ASSERT(f->style()->floating() != FloatValue::NoneFloatValue);

                f->setLayoutParent(m_currentLayoutParent);
                f->layout(m_layoutContext, Frame::LayoutWantToResolve::ResolveAll);

                markInlineBoxIndex(r);
                if (m_pendingFloatingBoxes.size() == 0
                    && canInsertFloatingBox(this, r)
                    && dontBreakLine(this, r, r->boxWidth())) {
                    generateFloatingBoxAndReLayoutLineBoxIfNeeds(r);
                } else {
                    m_pendingFloatingBoxes.push_back(r);
                }
            }
        } else if (f->isFrameLineBreak()) {
            if (!m_shouldLineBreakForBr) {
                FloatingBoxLayoutContext& fbCtx = *m_floatingBoxLayoutContexts.begin();
                if (clearAffected(fbCtx.m_hasFloat, f)) {
                    breakLine(f->asFrameLineBreak());
                }
                m_shouldLineBreakForBr = true;
            } else {
                breakLine(f->asFrameLineBreak());
            }
        } else if (f->isFrameInline()) {
            generateInlineNonReplacedBox(f->asFrameInline());
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        f = f->next();
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
            if (f->style()->unicodeBidi() == UnicodeBidiValue::EmbedUnicodeBidiValue
                || f->style()->unicodeBidi() == UnicodeBidiValue::IsolateUnicodeBidiValue) {
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

    size_t start = target.m_stringView.start();
    size_t end = breakTextIndex + 1;
    runs.insert(runs.begin() + breakRunIndex++, TextRun(target.m_frameText, target.m_stringView.originalString(), start, end, charDirFromICUDir(getTextDir(target.m_stringView, start, end))));

    start = breakTextIndex + 1;
    end = target.m_stringView.end();
    runs.insert(runs.begin() + breakRunIndex, TextRun(target.m_frameText, target.m_stringView.originalString(), start, end, charDirFromICUDir(getTextDir(target.m_stringView, start, end))));
}

static std::vector<TextRun> textBidiResolver(FrameText* frameText, DirectionValue directionValue)
{
    std::vector<TextRun> result;
    UBiDi* bidi = ubidi_open();
    UTF16NonGCString str = frameText->text()->toUTF16NonGCString();
    UErrorCode err = (UErrorCode)0;
    ubidi_setPara(bidi, (const UChar*)str.data(), str.length(), directionValue == DirectionValue::LtrDirectionValue ? UBIDI_DEFAULT_LTR : UBIDI_DEFAULT_RTL, NULL, &err);
    STARFISH_ASSERT(U_SUCCESS(err));
    size_t total = ubidi_countRuns(bidi, &err);
    STARFISH_ASSERT(U_SUCCESS(err));
    if (total == 1) {
        UBiDiDirection dir = getTextDir(StringView(frameText->text(), 0, frameText->text()->length()), 0, frameText->text()->length());
        result.push_back(TextRun(frameText, frameText->text(), 0, frameText->text()->length(), charDirFromICUDir(dir)));
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

            result.push_back(TextRun(frameText, frameText->text(), utf32Pos, utf32Pos + utf32Len, charDirFromICUDir(dir)));
            utf32Pos += utf32Len;

            start = end;
        }
    }
    ubidi_close(bidi);

    for (size_t i = 0; i < result.size(); i ++) {
        if (isNumber(result[i].m_stringView.originalString(), result[i].m_stringView.start(), result[i].m_stringView.end())) {
            result[i].m_direction = CharDirection::Neutral;
            continue;
        }

        // check it has leading neutral chars
        char32_t first = result[i].m_stringView.originalString()->charAt(result[i].m_stringView.start());
        if ((result[i].m_stringView.end() - result[i].m_stringView.start()) > 1 && charDirection(first) == 2 && !isNumberChar(first)) {
            breakTextRun(result, i, result[i].m_stringView.start());
            i--;
            continue;
        }

        // check it has trailing neutral chars
        if (result[i].m_stringView.end() - result[i].m_stringView.start() > 1) {
            size_t lastPos = result[i].m_stringView.end() - 1;
            char32_t last = result[i].m_stringView.originalString()->charAt(lastPos);
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
            ctx.m_computedDirectionValuePerFrame[putOffNeutralFrames[i]] = result;
        }

        CharDirection ch = (result == DirectionValue::LtrDirectionValue ? CharDirection::Ltr : CharDirection::Rtl);
        for (size_t i = 0; i < putOffTextRuns.size(); i ++) {
            TextRun run = putOffTextRuns[i];
            ctx.m_textRunsPerFrameText[run.m_frameText].push_back(TextRun(run.m_frameText, run.m_stringView.originalString(), run.m_stringView.start(), run.m_stringView.end(), ch));
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
                    if (isNumber(run.m_stringView.originalString(), run.m_stringView.start(), run.m_stringView.end())) {
                        continue;
                    }
                    if (putOffTextRuns.size() == 0) {
                        directionValueWhenNeturalMeets = currentDirection;
                    }
                    putOffTextRuns.push_back(run);
                } else {
                    STARFISH_ASSERT(run.m_direction == CharDirection::Ltr || run.m_direction == CharDirection::Rtl);
                    everMeetNonNeutralThing = true;
                    ctx.m_textRunsPerFrameText[f->asFrameText()].push_back(TextRun(f->asFrameText(), run.m_stringView.originalString(), run.m_stringView.start(), run.m_stringView.end(), run.m_direction));
                    flushNeurtal(run.m_direction == CharDirection::Ltr ? DirectionValue::LtrDirectionValue : DirectionValue::RtlDirectionValue);
                }
            }

        } else if (f->isFrameBox()) {
            if (everMeetNonNeutralThing) {
                putOffNeutral(f);
            } else {
                ctx.m_computedDirectionValuePerFrame[f] = directionValue;
            }
        } else if (f->isFrameInline()) {
            if (f->style()->unicodeBidi() == UnicodeBidiValue::EmbedUnicodeBidiValue) {
                everMeetNonNeutralThing = true;
                flushNeurtal(f->style()->direction());
            } else if (f->style()->unicodeBidi() == UnicodeBidiValue::IsolateUnicodeBidiValue) {
                if (everMeetNonNeutralThing) {
                    putOffNeutral(f);
                } else {
                    ctx.m_computedDirectionValuePerFrame[f] = directionValue;
                }
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        } else if (f->isFrameLineBreak()) {
            everMeetNonNeutralThing = true;
            flushNeurtal(directionValue);
        }
    }

    flushNeurtal(directionValue);
}

void InlineNonReplacedBox::setStartingMBP(LineFormattingContext* lineFormattingContext)
{
    InlineNonReplacedBox* current = this;
    while (current) {
        if (!current->isProcessedStartingMBP()) {
            if (current->style()->direction() == LtrDirectionValue) {
                current->setWidth(current->width() + current->paddingLeft() + current->borderLeft());
            } else {
                current->setWidth(current->width() + current->paddingRight() + current->borderRight());
            }
            LayoutUnit unprocessedStartingMBPWidth = current->startingMBPWidth();
            lineFormattingContext->m_currentLineWidth += unprocessedStartingMBPWidth;
            lineFormattingContext->m_unprocessedStartingMBPWidth -= unprocessedStartingMBPWidth;
            current->markProcessedStartingMBP();
        } else {
            break;
        }

        if (current->layoutParent()->asFrameBox()->isLineBox()) {
            break;
        }

        current = current->layoutParent()->asFrameBox()->asInlineBox()->asInlineNonReplacedBox();
    }
}

void InlineNonReplacedBox::setEndingMBP(LineFormattingContext* lineFormattingContext)
{
    LayoutUnit unprocessedEndingMBP = endingMBPWidth();
    lineFormattingContext->m_currentLineWidth += unprocessedEndingMBP;
}

void InlineNonReplacedBox::unsetTopBottomMBP()
{
    m_margin.setTop(0);
    m_margin.setBottom(0);
    m_border.setTop(0);
    m_border.setBottom(0);
    m_padding.setTop(0);
    m_padding.setBottom(0);
}

void InlineNonReplacedBox::unsetLeftMBP()
{
    m_margin.setLeft(0);
    m_border.setLeft(0);
    m_padding.setLeft(0);

    m_orgMargin.setLeft(0);
    m_orgBorder.setLeft(0);
    m_orgPadding.setLeft(0);
}

void InlineNonReplacedBox::unsetRightMBP()
{
    m_margin.setRight(0);
    m_border.setRight(0);
    m_padding.setRight(0);

    m_orgMargin.setRight(0);
    m_orgBorder.setRight(0);
    m_orgPadding.setRight(0);
}

void InlineNonReplacedBox::unsetStartingMBP(DirectionValue direction)
{
    if (direction == LtrDirectionValue) {
        unsetLeftMBP();
    } else {
        unsetRightMBP();
    }
}

void InlineNonReplacedBox::unsetEndingMBP(DirectionValue direction)
{
    if (direction == LtrDirectionValue) {
        unsetRightMBP();
    } else {
        unsetLeftMBP();
    }
}

void InlineNonReplacedBox::resetOrgMBP(InlineNonReplacedBoxMBPStore* store)
{
    if (store) {
        m_orgMargin = store->m_orgMargin;
        m_orgBorder = store->m_orgBorder;
        m_orgPadding = store->m_orgPadding;
    } else {
        m_orgBorder = m_border;
        m_orgPadding = m_padding;
        m_orgMargin = m_margin;
    }
}

void InlineNonReplacedBox::resetLeftRightMBP(InlineNonReplacedBoxMBPStore* store)
{
    m_margin.setLeft(store->m_orgMargin.left());
    m_margin.setRight(store->m_orgMargin.right());
    m_border.setLeft(store->m_orgBorder.left());
    m_border.setRight(store->m_orgBorder.right());
    m_padding.setLeft(store->m_orgPadding.left());
    m_padding.setRight(store->m_orgPadding.right());
}

void LineFormattingContext::finishLineForInlineNonReplacedBox(FrameLineBreak* br, bool isLastNode)
{
    InlineNonReplacedBox* self = m_currentLayoutParent->asInlineBox()->asInlineNonReplacedBox();

    if (isLastNode) {
        self->setStartingMBP(this);
        self->setEndingMBP(this);
    }

    if (hasIsolateBidiContent(self)) {
        resolveBidi(self->style()->direction(), self->boxes());
    }
    self->layoutInlineBoxes(self->paddingLeft() + self->borderLeft());

    InlineNonReplacedBox* current = self;
    InlineNonReplacedBox* last = nullptr;
    while (current) {
        current->setWidth(std::max(LayoutUnit(0), current->width() + current->paddingWidth() + current->borderWidth()));
        if (last) {
            current->setWidth(current->width() + last->boxWidth());
        }
        last = current;

        FontHeights fontHeights = computeVerticalProperties(current, current->style(), br);
        current->m_ascender = fontHeights.m_ascender;
        current->m_descender = fontHeights.m_descender;

        current->setContentHeight(fontHeights.m_ascender - fontHeights.m_descender);

        if (current->layoutParent()->asFrameBox()->isLineBox()) {
            break;
        }

        if (isLastNode) {
            break;
        }

        current = current->layoutParent()->asFrameBox()->asInlineBox()->asInlineNonReplacedBox();
    }
}

void LineFormattingContext::breakLineForInlineNonReplacedBox(FrameLineBreak* br)
{
    InlineNonReplacedBox* self = m_currentLayoutParent->asInlineBox()->asInlineNonReplacedBox();

    if (self->boxes().size() == 0 && self->layoutParent()->isLineBox()) {
        auto& boxes = currentLine()->boxes();
        boxes.erase(std::find(boxes.begin(), boxes.end(), self));
        breakLineForLineBox(br, false, false);
        currentLine()->insertInlineBox(self);
        return;
    }

    finishLineForInlineNonReplacedBox(br, false);

    std::vector<std::pair<InlineNonReplacedBox*, InlineNonReplacedBoxMBPStore>, gc_allocator_ignore_off_page<std::pair<InlineNonReplacedBox*, InlineNonReplacedBoxMBPStore>>> stack;

    InlineNonReplacedBox* current = self;
    while (current) {
        if (current->isProcessedStartingMBP()) {
            InlineNonReplacedBoxMBPStore store(current);
            store.unsetStartingMBP(current->style()->direction());
            stack.push_back(std::make_pair(current, store));
        } else {
            stack.push_back(std::make_pair(current, InlineNonReplacedBoxMBPStore(current)));
            current->unsetStartingMBP(current->style()->direction());
        }

        current->unsetEndingMBP(current->style()->direction());

        if (current->layoutParent()->asFrameBox()->isLineBox()) {
            break;
        }

        current = current->layoutParent()->asFrameBox()->asInlineBox()->asInlineNonReplacedBox();
    }

    breakLineForLineBox(br, false, false);

    auto riter = stack.rbegin();
    InlineNonReplacedBox* layoutParent = nullptr;
    while (stack.rend() != riter) {
        InlineNonReplacedBox* origin = riter->first;
        InlineNonReplacedBoxMBPStore store = riter->second;
        InlineNonReplacedBox* newInrpBox = new InlineNonReplacedBox(origin);

        insertInlineNonReplacedBox(newInrpBox, layoutParent);
        layoutParent = newInrpBox;

        newInrpBox->resetOrgMBP(&store);
        newInrpBox->resetLeftRightMBP(&store);
        m_currentLayoutParent = newInrpBox;
        riter++;
    }
}

LayoutUnit FrameBlockBox::layoutInline(LayoutContext& ctx)
{
    if (!isNecessaryBlockBox()) {
        return 0;
    }

    LayoutUnit inlineContentWidth = contentWidth();
    LayoutUnit top = paddingTop() + borderTop();
    LayoutUnit bottom = paddingBottom() + borderBottom();
    LayoutUnit left = paddingLeft() + borderLeft();
    MarginInfo marginInfo(top, bottom, isEstablishesBlockFormattingContext() || isFrameDocument(), style()->height());
    setMarginInfo(&marginInfo);
    LineFormattingContext lineFormattingContext(*this, ctx, left, top, inlineContentWidth);

    // compute directions
    computeDirection(lineFormattingContext, this, style()->direction());

    lineFormattingContext.m_currentLayoutParent = this;
    lineFormattingContext.generateInlineBoxes(this);

    lineFormattingContext.finishLineForLineBox(nullptr, true);
    STARFISH_ASSERT(lineFormattingContext.m_pendingFloatingBoxes.size() == 0 && lineFormattingContext.m_pendingInlineBoxes.size() == 0);

    auto iter = m_lineBoxes.begin();

    while (iter != m_lineBoxes.end()) {
        LineBox* lineBox = *iter;
        STARFISH_ASSERT(lineBox != nullptr);

        if (lineBox->boxes().size() == 0) {
            if (lineBox->height() == 0) {
                iter = m_lineBoxes.erase(iter);
                continue;
            }
        } else {
            FrameBox* box = *lineBox->boxes().rbegin();
            if (box->isInlineBox() && box->asInlineBox()->isInlineNonReplacedBox()
                && box->width() == 0 && box->marginLeft() == 0 && box->marginRight() == 0) {
                if (lineBox->boxes().size() == 1) {
                    iter = m_lineBoxes.erase(iter);
                    continue;
                } else {
                    lineBox->boxes().erase(lineBox->boxes().end() - 1);
                }
            }
        }

        lineBox->registerRelativePositionInlineBoxes(ctx);
        iter++;
    }

    auto riter = m_lineBoxes.rbegin();
    LayoutUnit lineBoxBottom;
    while (riter != m_lineBoxes.rend()) {
        LineBox* lineBox = *riter;
        STARFISH_ASSERT(lineBox != nullptr);

        if (lineBox->height() != 0) {
            lineBoxBottom = lineBox->y() + lineBox->height();
            break;
        }

        riter++;
    }

    if (isEstablishesBlockFormattingContext()) {
        lineBoxBottom = std::max(lineBoxBottom, ctx.clearedDistanceToFloatBottom(lineFormattingContext.m_absPosition.y(), BothClearValue));
    }

    if (lineBoxBottom > 0) {
        return lineBoxBottom - top;
    }

    return LayoutUnit(0);
}


void InlineNonReplacedBox::layoutInline(LineFormattingContext* lineFormattingContext)
{
    LayoutUnit inlineContentWidth = lineFormattingContext->m_block.contentWidth();

    computeBorderMarginPadding(inlineContentWidth);
    resetOrgMBP();
    unsetTopBottomMBP();
    lineFormattingContext->m_unprocessedStartingMBPWidth += startingMBPWidth();

    lineFormattingContext->m_currentLayoutParent = this;
    lineFormattingContext->generateInlineBoxes(origin());

    lineFormattingContext->finishLineForInlineNonReplacedBox(nullptr, true);
}

void FrameBlockBox::computePreferredWidth(PreferredWidthContext& ctx)
{
    LayoutUnit remainWidth = ctx.lastKnownWidth();
    LayoutUnit minWidth;

    if (!isNecessaryBlockBox()) {
        return;
    }

    LayoutUnit mbp = PreferredWidthContext::computeMinimumWidthDueToMBP(style());
    Length width = style()->width();
    LayoutUnit w;

    if (width.isSpecified()) {
        if (width.isFixed()) {
            w = width.fixed();
        } else {
            LayoutUnit parentContentWidth = ctx.layoutContext().parentContentWidth(this);
            w = parentContentWidth * width.percent() - mbp;
        }
        ctx.updatePreferredMinWidth(w);
        ctx.updatePreferredWidth(w);
        return;
    }

    if (hasBlockFlow()) {
        Frame* child = firstChild();
        while (child) {
            STARFISH_ASSERT(child->isNormalFlow());
            LayoutUnit mbp = PreferredWidthContext::computeMinimumWidthDueToMBP(child->style());
            PreferredWidthContext newCtx(ctx.layoutContext(), ctx.lastKnownWidth() - mbp, 0);
            child->computePreferredWidth(newCtx);
            ctx.updatePreferredWidth(newCtx.preferredWidth() + mbp);
            child = child->next();
        }
    } else {
        LayoutUnit currentLineWidth = 0;
        int hasFloat = LineFormattingContext::HasNone;
        std::function<void(Frame*)> computeInlineLayout = [&](Frame* f)
        {
            // current
            if (f->style()->position() == AbsolutePositionValue) {
                return;
            }

            bool whiteSpaceCanBreak = hasBreakableWhiteSpaceProperty(f);

            if (f->isFrameText()) {
                String* s = f->asFrameText()->text();
                textDividerForLayout(ctx.layoutContext().starFish(), s, [&](String* srcTxt, size_t offset, size_t nextOffset, bool isWhiteSpace, bool canBreak) {
                    if (isWhiteSpace) {
                        if (offset == 0) {
                            if (currentLineWidth == 0) {
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
                        w = f->style()->font()->measureText(StringView(srcTxt, offset, nextOffset));
                    }

                    ctx.updatePreferredMinWidth(w);

                    if (whiteSpaceCanBreak) {
                        if (currentLineWidth + w < remainWidth) {
                            currentLineWidth += w;
                        } else {
                            ctx.updatePreferredWidth(remainWidth);
                            if (isWhiteSpace) {
                                currentLineWidth = 0;
                            } else {
                                currentLineWidth = w;
                            }
                        }
                    } else {
                        currentLineWidth += w;
                    }

                    ctx.setIsWhiteSpaceAtLast(isWhiteSpace);
                });
            } else if (f->isFrameBlockBox()) {
                LayoutUnit mbp = PreferredWidthContext::computeMinimumWidthDueToMBP(f->style());
                PreferredWidthContext newCtx(ctx.layoutContext(), ctx.lastKnownWidth() - mbp, 0);
                f->computePreferredWidth(newCtx);
                LayoutUnit w = newCtx.preferredWidth() + mbp;
                ctx.updatePreferredWidth(w);

                if (whiteSpaceCanBreak) {
                    if (f->style()->floating() == NoneFloatValue || f->style()->clear() == NoneClearValue
                        || clearAffected(hasFloat, f)) {
                        if (currentLineWidth + w < remainWidth) {
                            currentLineWidth += w;
                        } else {
                            ctx.updatePreferredWidth(remainWidth);
                            currentLineWidth = w;
                        }
                    } else {
                        ctx.updatePreferredWidth(currentLineWidth);
                        hasFloat = LineFormattingContext::HasNone;
                        currentLineWidth = w;
                    }
                } else {
                    currentLineWidth += w;
                }

                if (f->style()->floating() == LeftFloatValue) {
                    hasFloat |= LineFormattingContext::HasLeft;
                } else if (f->style()->floating() == RightFloatValue) {
                    hasFloat |= LineFormattingContext::HasRight;
                }

                ctx.setIsWhiteSpaceAtLast(true);
            } else if (f->isFrameLineBreak()) {
                // linebreaks
                ctx.updatePreferredWidth(currentLineWidth);
                currentLineWidth = 0;

                ctx.setIsWhiteSpaceAtLast(false);
            } else if (f->isFrameInline()) {
                auto checkMBP = [&](Length l)
                {
                    if (l.isFixed()) {
                        if (whiteSpaceCanBreak) {
                            if (currentLineWidth + l.fixed() < remainWidth) {
                                currentLineWidth += l.fixed();
                            } else {
                                ctx.updatePreferredWidth(remainWidth);
                                currentLineWidth = l.fixed();
                            }
                        } else {
                            currentLineWidth += l.fixed();
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

                LayoutUnit mbp = PreferredWidthContext::computeMinimumWidthDueToMBP(f->style());
                PreferredWidthContext newCtx(ctx.layoutContext(), remainWidth - mbp, 0);
                f->computePreferredWidth(newCtx);
                LayoutUnit w = newCtx.preferredWidth() + mbp;
                ctx.updatePreferredWidth(w);

                if (whiteSpaceCanBreak) {
                    if (currentLineWidth + w < remainWidth) {
                        currentLineWidth += w;
                    } else {
                        ctx.updatePreferredWidth(remainWidth);
                        currentLineWidth = w;
                    }
                } else {
                    currentLineWidth += w;
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
        ctx.updatePreferredWidth(currentLineWidth);
    }
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

void InlineTextBox::paint(PaintingContext& ctx)
{
    if (ctx.m_paintingInlineStage == PaintingInlineLevelElements) {
        if (ctx.m_paintingStage == PaintingNormalFlowInline) {
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
            ctx.m_canvas->drawText(0, 0, contentWidth(), m_textRun.m_stringView);
        }
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
            PaintingStage s = PaintingStage::PaintingNormalFlowBlock;
            while (s != PaintingStageEnd) {
                ctx.m_paintingStage = s;
                paintChildrenWith(ctx);
                s = (PaintingStage)(s + 1);
            }
            ctx.m_paintingStage = PaintingPositionedElements;
        }
    } else if (style()->floating() != NoneFloatValue) {
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
        f->dump(depth + 2);
        if (iter + 1 != boxes().end())
            puts("");
        iter++;
    }
}
#endif
}
