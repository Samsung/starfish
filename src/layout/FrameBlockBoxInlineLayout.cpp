#include "StarFishConfig.h"
#include "FrameBlockBox.h"
#include "FrameText.h"
#include "FrameInline.h"

namespace StarFish {

static LayoutUnit computeVerticalProperties(FrameBox* parentBox, ComputedStyle* parentStyle, LayoutUnit& ascenderInOut, LayoutUnit& descenderInOut, LineFormattingContext& ctx)
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
                    maxDescenderSoFar = std::min(box->y() - box->height(), maxDescenderSoFar);
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
        } else {
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

static void resolveBidi(DirectionValue parentDir, std::vector<FrameBox*, gc_allocator<FrameBox*>>& boxes)
{
    InlineTextBox::CharDirection defaultCharDirection = parentDir == DirectionValue::LtrDirectionValue ? InlineTextBox::CharDirection::Ltr : InlineTextBox::CharDirection::Rtl;
    bool everSeenUTF32String = parentDir == DirectionValue::RtlDirectionValue;
    InlineTextBox::CharDirection charDirectionBefore = defaultCharDirection;
    for (size_t i = 0; i < boxes.size(); i ++) {
        FrameBox* box = boxes[i];
        if (box->isInlineBox()) {
            InlineBox* ib = box->asInlineBox();
            if (ib->isInlineTextBox()) {
                InlineTextBox* tb = ib->asInlineTextBox();
                if (tb->text()->isASCIIString()) {
                    if (!everSeenUTF32String)
                        tb->setCharDirection(InlineTextBox::CharDirection::Ltr);
                    else {
                        if (tb->text()->containsOnlyWhitespace()) {
                            tb->setCharDirection(defaultCharDirection);
                        } else {
                            bool isNeturalRun = true;
                            String* txt = tb->text();
                            for (size_t i = 0; i < txt->length(); i ++) {
                                char ret = charDirection(txt->charAt(i));
                                if (ret != 2) {
                                    isNeturalRun = false;
                                    break;
                                }
                            }
                            if (isNeturalRun) {
                                tb->setCharDirection(defaultCharDirection);
                            } else {
                                tb->setCharDirection(InlineTextBox::CharDirection::Ltr);
                            }
                        }
                    }
                } else {
                    if (!everSeenUTF32String) {
                        everSeenUTF32String = true;
                        i = SIZE_MAX;
                        continue;
                    }
                    UBiDi* bidi = ubidi_open();
                    UTF16String str = tb->text()->toUTF16String();
                    UErrorCode err = (UErrorCode)0;
                    ubidi_setPara(bidi, (const UChar*)str.data(), str.length(), parentDir == DirectionValue::LtrDirectionValue ? UBIDI_DEFAULT_LTR : UBIDI_DEFAULT_RTL, NULL, &err);
                    STARFISH_ASSERT(U_SUCCESS(err));
                    size_t total = ubidi_countRuns(bidi, &err);
                    STARFISH_ASSERT(U_SUCCESS(err));
                    InlineTextBox* lastTextBox = tb;
                    if (total == 1) {
                        int32_t start, length;
                        UBiDiDirection dir = ubidi_getVisualRun(bidi, 0, &start, &length);
                        tb->setCharDirection(dir == UBIDI_RTL ? InlineTextBox::CharDirection::Rtl : InlineTextBox::CharDirection::Ltr);
                        charDirectionBefore = dir == UBIDI_RTL ? InlineTextBox::CharDirection::Rtl : InlineTextBox::CharDirection::Ltr;
                    } else {
                        LayoutUnit x = tb->x();
                        LayoutUnit y = tb->y();
                        size_t insertPos = i;
                        boxes.erase(boxes.begin() + i);
                        int32_t start = 0;
                        int32_t end;
                        for (size_t i = 0; i < total; i ++) {
                            ubidi_getLogicalRun(bidi, start, &end, NULL);
                            String* t = tb->text()->substring(start, end);
                            // puts(t->utf8Data());
                            start = end;
                            UBiDiDirection dir = ubidi_getBaseDirection((const UChar*)t->toUTF16String().data(), t->length());

                            InlineTextBox* box = new InlineTextBox(tb->node(), tb->style(), nullptr, t, tb->origin(), dir == UBIDI_RTL ? InlineTextBox::CharDirection::Rtl : InlineTextBox::CharDirection::Ltr);
                            box->setLayoutParent(tb->layoutParent());
                            box->setX(x);
                            box->setY(y);
                            LayoutUnit width = tb->style()->font()->measureText(t);
                            box->setWidth(width);
                            box->setHeight(tb->height());
                            x += width;
                            boxes.insert(boxes.begin() + insertPos++, box);
                            charDirectionBefore = dir == UBIDI_RTL ? InlineTextBox::CharDirection::Rtl : InlineTextBox::CharDirection::Ltr;
                            lastTextBox = box;
                        }
                    }
                    ubidi_close(bidi);

                    auto splitBoxAt = [&boxes, &defaultCharDirection](InlineTextBox* where, size_t idx, bool neturalAtLast)
                    {
                        auto iter = std::find(boxes.begin(), boxes.end(), where);
                        size_t boxIdx = iter - boxes.begin();
                        boxes.erase(iter);
                        String* newText = where->text()->substring(0, idx + 1);

                        InlineTextBox* box = new InlineTextBox(where->node(), where->style(), nullptr, newText, where->origin(), neturalAtLast ? where->charDirection() : defaultCharDirection);
                        box->setLayoutParent(where->layoutParent());
                        LayoutUnit x = where->x();
                        LayoutUnit y = where->y();
                        box->setX(x);
                        box->setY(y);
                        LayoutUnit width = where->style()->font()->measureText(newText);
                        box->setWidth(width);
                        box->setHeight(where->height());
                        x += width;
                        boxes.insert(boxes.begin() + boxIdx++, box);

                        String* newText2 = where->text()->substring(idx + 1, where->text()->length() - idx);
                        box = new InlineTextBox(where->node(), where->style(), nullptr, newText2, where->origin(), neturalAtLast ? defaultCharDirection : where->charDirection());
                        box->setLayoutParent(where->layoutParent());
                        box->setX(x);
                        box->setY(y);
                        width = where->style()->font()->measureText(newText2);
                        box->setWidth(width);
                        box->setHeight(where->height());
                        boxes.insert(boxes.begin() + boxIdx, box);

                        return box;
                    };

                    // check it has leading netural chars
                    char32_t first = lastTextBox->text()->charAt(0);
                    if (charDirection(first) == 2) {
                        size_t idx = 1;
                        while (idx < lastTextBox->text()->length()) {
                            if (charDirection(lastTextBox->text()->charAt(idx)) != 2) {
                                break;
                            }
                            idx++;
                        }

                        if (idx != lastTextBox->text()->length()) {
                            lastTextBox = splitBoxAt(lastTextBox, idx - 1, false);
                        }
                    }

                    // check it has trailing neutral chars
                    size_t lastPos = lastTextBox->text()->length() - 1;
                    char32_t lastChar = lastTextBox->text()->charAt(lastPos);
                    size_t i = lastPos;
                    if (charDirection(lastChar) == 2) {
                        while (true) {
                            if (charDirection(lastTextBox->text()->charAt(i)) != 2) {
                                break;
                            }
                            if (i == 0) {
                                break;
                            } else {
                                i--;
                            }
                        }
                    }

                    // FIXME: Is there a isNumber() for unicode?
                    auto isNumber = [](String* text) {
                        for (unsigned i=0; i < text->length(); i++) {
                            unsigned d = text->charAt(i);
                            if (('0' <= d && d <= '9') ||
                                 (0x0660 <= d && d <= 0x0669) || // 0 to 9 in Arabic
                                 (0x06F0 <= d && d <= 0x06F9)) { // 0 to 9 in Extended Arabic
                            } else {
                                return false;
                            }
                        }
                        return true;
                    };

                    if (i < lastPos) {
                        String* text = lastTextBox->text();
                        if (isNumber(text)) {
                            // if text is a number, do not split. The direction of number should be determined by surrounding text
                            tb->setCharDirection(InlineTextBox::CharDirection::Netural);
                        } else {
                            splitBoxAt(lastTextBox, i, true);
                        }
                    }
                }
            }
        }
    }

    if (parentDir == DirectionValue::LtrDirectionValue) {
        bool lastLTRState = true;
        size_t RTLStartPos = SIZE_MAX;
        for (size_t i = 0; i < boxes.size(); i ++) {
            FrameBox* box = boxes[i];
            if (box->isInlineBox()) {
                InlineBox* ib = box->asInlineBox();
                if (ib->isInlineTextBox()) {
                    if (lastLTRState) {
                        if (ib->asInlineTextBox()->charDirection() == InlineTextBox::CharDirection::Rtl) {
                            // l->r
                            RTLStartPos = i;
                            lastLTRState = false;
                        } else {
                            // l->l
                        }
                    } else {
                        if (ib->asInlineTextBox()->charDirection() == InlineTextBox::CharDirection::Rtl ||
                            ib->asInlineTextBox()->charDirection() == InlineTextBox::CharDirection::Netural) {
                            // r->r
                        } else {
                            // r->l
                            if (ib->asInlineTextBox()->text()->containsOnlyWhitespace() && (i + 1)< boxes.size()) {
                                if (boxes[i + 1]->isInlineBox() && boxes[i + 1]->asInlineBox()->isInlineTextBox()) {
                                    if (boxes[i + 1]->asInlineBox()->asInlineTextBox()->charDirection() == InlineTextBox::CharDirection::Rtl ||
                                        boxes[i + 1]->asInlineBox()->asInlineTextBox()->charDirection() == InlineTextBox::CharDirection::Netural) {
                                        continue;
                                    }
                                }
                            }
                            std::reverse(boxes.begin() + RTLStartPos, boxes.begin() + i);
                            lastLTRState = true;
                            RTLStartPos = SIZE_MAX;
                        }
                    }
                }
            }
        }
        if (!lastLTRState) {
            std::reverse(boxes.begin() + RTLStartPos, boxes.end());
        }
    } else {
        // reverse every boxes
        std::reverse(boxes.begin(), boxes.end());

        // reverse among ltr blockes
        bool lastLTRState = false;
        size_t LTRStartPos = 0;
        for (size_t i = 0; i < boxes.size(); i ++) {
            FrameBox* box = boxes[i];
            if (box->isInlineBox()) {
                InlineBox* ib = box->asInlineBox();
                if (ib->isInlineTextBox()) {
                    if (lastLTRState) {
                        if (ib->asInlineTextBox()->charDirection() == InlineTextBox::CharDirection::Rtl) {
                            // l->r
                            if (ib->asInlineTextBox()->text()->containsOnlyWhitespace() && (i + 1)< boxes.size()) {
                                if (boxes[i + 1]->isInlineBox() && boxes[i + 1]->asInlineBox()->isInlineTextBox()) {
                                    if (boxes[i + 1]->asInlineBox()->asInlineTextBox()->charDirection() == InlineTextBox::CharDirection::Ltr) {
                                        continue;
                                    }
                                }
                            }
                            std::reverse(boxes.begin() + LTRStartPos, boxes.begin() + i);
                            lastLTRState = false;
                            LTRStartPos = SIZE_MAX;
                        } else {
                            // l->l
                        }
                    } else {
                        if (ib->asInlineTextBox()->charDirection() == InlineTextBox::CharDirection::Rtl) {
                            // r->r
                        } else {
                            // r->l
                            LTRStartPos = i;
                            lastLTRState = true;
                        }
                    }
                }
            }
        }
        if (lastLTRState) {
            std::reverse(boxes.begin() + LTRStartPos, boxes.end());
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
            0, 0, 125, 0, 123, 0, 0, 0
        };
        for (size_t i = 0; i < boxes.size(); i ++) {
            FrameBox* box = boxes[i];
            if (box->isInlineBox()) {
                if (box->asInlineBox()->isInlineTextBox()) {
                    InlineTextBox* b = box->asInlineBox()->asInlineTextBox();
                    String* t = b->text();
                    if (t->length() == 1) {
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
                                            ch = parenthesisMap[ch];
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
                                            ch = parenthesisMap[ch];
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
        }
    }
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

void LineFormattingContext::completeLastLine()
{
    LineBox* back = m_block.m_lineBoxes.back();
    FrameBox* last = findLastInlineBox(back);
    if (last && last->isInlineBox() && last->asInlineBox()->isInlineTextBox()) {
        if (last->asInlineBox()->asInlineTextBox()->text()->equals(String::spaceString)) {
            removeBoxFromLine(last);
        }
    }
    resolveBidi(m_block.style()->direction(), back->boxes());
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

void inlineBoxGenerator(Frame* origin, LayoutContext& ctx, LineFormattingContext& lineFormattingContext, LayoutUnit inlineContentWidth, LayoutUnit& extraWidthDueToFrameInlineFirstChild,
    std::function<void(FrameBox*)> gotInlineBoxCallback, std::function<void(bool)> lineBreakCallback, std::function<void(FrameInline*)> frameInlineCallback)
{
    Frame* f = origin->firstChild();
    while (f) {
        if (!f->isNormalFlow()) {
            LayoutUnit preferredWidth;
            if (f->asFrameBox()->shouldComputePreferredWidth()) {
                LayoutUnit mbp = ComputePreferredWidthContext::computeMinimumWidthDueToMBP(f->style());
                ComputePreferredWidthContext p(ctx, inlineContentWidth - mbp);
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
            textDividerForLayout(ctx.starFish(), txt, [&](String* srcTxt, size_t offset, size_t nextOffset, bool isWhiteSpace, bool canBreak) {
                textAppendRetry:
                if (isWhiteSpace) {
                    if (offset == 0) {
                        FrameBox* last = findLastInlineBox(lineFormattingContext.currentLine());
                        if (last && last->isInlineBox() && last->asInlineBox()->isInlineTextBox()) {
                            String* str = last->asInlineBox()->asInlineTextBox()->text();
                            if (str->containsOnlyWhitespace()) {
                                return;
                            }
                        }
                        if (lineFormattingContext.m_currentLineWidth == 0)
                            return;
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

                if (/*couldNotBreak || */(lineFormattingContext.m_currentLineWidth == 0) || textWidth <= (inlineContentWidth - lineFormattingContext.m_currentLineWidth - extraWidthDueToFrameInlineFirstChild)) {

                } else {
                    // try this at nextline
                    lineBreakCallback(false);
                    goto textAppendRetry;
                }
                InlineBox* ib = new InlineTextBox(f->node(), f->style(), nullptr, resultString, f->asFrameText(), InlineTextBox::CharDirection::Mixed);
                ib->setWidth(textWidth);
                ib->setHeight(f->style()->font()->metrics().m_fontHeight);
                lineFormattingContext.m_currentLineWidth += textWidth;
                gotInlineBoxCallback(ib);
            });

        } else if (f->isFrameReplaced()) {
            FrameReplaced* r = f->asFrameReplaced();
            r->layout(ctx, Frame::LayoutWantToResolve::ResolveAll);

        insertReplacedBox:
            if ((r->width() + r->marginWidth()) <= (inlineContentWidth - lineFormattingContext.m_currentLineWidth - extraWidthDueToFrameInlineFirstChild) || lineFormattingContext.m_currentLineWidth == 0) {
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
            f->layout(ctx, Frame::LayoutWantToResolve::ResolveAll);

            LayoutUnit ascender = 0;
            if (ctx.lastLineBox() && r->isAncestorOf(ctx.lastLineBox()) && r->style()->overflow() == OverflowValue::VisibleOverflow) {
                LayoutUnit topToLineBox = ctx.lastLineBox()->absolutePointWithoutRelativePosition(r).y();
                ascender = topToLineBox + ctx.lastLineBox()->ascender();
            } else {
                ascender = f->asFrameBox()->height();
            }
            lineFormattingContext.registerInlineBlockAscender(ascender, r);

        insertBlockBox:
            if ((r->width() + r->marginWidth()) <= (inlineContentWidth - lineFormattingContext.m_currentLineWidth - extraWidthDueToFrameInlineFirstChild) || lineFormattingContext.m_currentLineWidth == 0) {
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
    if (!isNecessaryBlockBox()) {
        return std::make_pair(0, LayoutRect(0, 0, 0, 0));
    }

    LayoutRect visibleRect(0, 0, 0, 0);
    LayoutUnit lineBoxX = paddingLeft() + borderLeft();
    LayoutUnit lineBoxY = paddingTop() + borderTop();
    LayoutUnit inlineContentWidth = contentWidth();
    LineFormattingContext lineFormattingContext(*this, ctx, inlineContentWidth);
    LayoutUnit unused;

    inlineBoxGenerator(this, ctx, lineFormattingContext, inlineContentWidth, unused, [&](FrameBox* ib) {
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
    computeVerticalProperties(back, style(), ascender, descender, lineFormattingContext);
    back->m_ascender = ascender;
    back->m_descender = descender;
    back->m_frameRect.setHeight(ascender - descender);

    lineFormattingContext.completeLastLine();

    if (m_lineBoxes.size() && m_lineBoxes.back()->boxes().size() == 0) {
        m_lineBoxes.erase(m_lineBoxes.end() - 1);
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

        lineFormattingContext.registerInlineContent();
    }

    if (layoutParentBox) {
        LayoutUnit w = self->marginLeft() + self->borderLeft() + self->paddingLeft();
        lineFormattingContext.m_currentLineWidth += w;
        self->setWidth(self->paddingLeft() + self->borderLeft());
    }

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

            if (origin->style()->direction() == DirectionValue::LtrDirectionValue) {
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
            } else {
                newBox->m_margin.setLeft(origin->m_orgMargin.left());
                newBox->m_border.setLeft(origin->m_orgBorder.left());
                newBox->m_padding.setLeft(origin->m_orgPadding.left());

                newBox->m_orgMargin.setLeft(origin->m_orgMargin.left());
                newBox->m_orgBorder.setLeft(origin->m_orgBorder.left());
                newBox->m_orgPadding.setLeft(origin->m_orgPadding.left());

                origin->m_margin.setLeft(0);
                origin->m_border.setLeft(0);
                origin->m_padding.setLeft(0);

                origin->m_orgMargin.setLeft(0);
                origin->m_orgBorder.setLeft(0);
                origin->m_orgPadding.setLeft(0);
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

    auto finishLayout = [&](bool lastNode = false)
    {
        resolveBidi(self->style()->direction(), self->boxes());
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

    LayoutUnit extraWidthDueToFrameInlineFirstChild;
    LayoutUnit unprocessedLeftWidths;
    LayoutUnit unprocessedRightWidths;
    if (!layoutParentBox) {
        unprocessedLeftWidths = self->marginLeft() + self->borderLeft() + self->paddingLeft();
        unprocessedRightWidths = self->marginRight() + self->borderRight() + self->paddingRight();
        extraWidthDueToFrameInlineFirstChild = unprocessedLeftWidths + unprocessedRightWidths;
    }
    inlineBoxGenerator(self->m_origin, ctx, lineFormattingContext, inlineContentWidth, extraWidthDueToFrameInlineFirstChild, [&](FrameBox* ib)
    {
        self->boxes().push_back(ib);
        if (self->style()->direction() == LtrDirectionValue) {
            if (unprocessedLeftWidths) {
                self->setWidth(self->width() + self->borderLeft() + self->paddingLeft());
                lineFormattingContext.m_currentLineWidth += unprocessedLeftWidths;
                unprocessedLeftWidths = 0;
                extraWidthDueToFrameInlineFirstChild = unprocessedRightWidths;
            }
        } else {
            if (unprocessedRightWidths) {
                self->setWidth(self->width() + self->borderRight() + self->paddingRight());
                lineFormattingContext.m_currentLineWidth += unprocessedRightWidths;
                unprocessedRightWidths = 0;
                extraWidthDueToFrameInlineFirstChild = unprocessedLeftWidths;
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
        // FIXME consider dueToBr
        if (!self->boxes().size() && !layoutParentBox) {
            lineFormattingContext.currentLine()->boxes().erase(std::find(lineFormattingContext.currentLine()->boxes().begin(), lineFormattingContext.currentLine()->boxes().end(), self));
            lineFormattingContext.breakLine(false);
            lineFormattingContext.currentLine()->boxes().push_back(self);
            self->setLayoutParent(lineFormattingContext.currentLine());
        } else {
            finishLayout();
            breakLine();
        }
    }, [&](FrameInline* f)
    {
        LayoutUnit extra;
        if (self->style()->direction() == LtrDirectionValue) {
            if (unprocessedLeftWidths) {
                lineFormattingContext.m_currentLineWidth += unprocessedLeftWidths;
                extra = unprocessedLeftWidths;
                unprocessedLeftWidths = 0;
                extraWidthDueToFrameInlineFirstChild = unprocessedRightWidths;
            }
        } else {
            if (unprocessedRightWidths) {
                lineFormattingContext.m_currentLineWidth += unprocessedRightWidths;
                extra = unprocessedRightWidths;
                unprocessedRightWidths = 0;
                extraWidthDueToFrameInlineFirstChild = unprocessedLeftWidths;
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
    });

    finishLayout(true);

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
            ctx.setResult(minWidth);
        } else if (ctx.lastKnownWidth() - minWidth > 0) {
            Frame* child = firstChild();
            while (child) {
                if (child->isNormalFlow()) {
                    LayoutUnit mbp = ComputePreferredWidthContext::computeMinimumWidthDueToMBP(child->style());
                    ComputePreferredWidthContext newCtx(ctx.layoutContext(), ctx.lastKnownWidth() - mbp);
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
                        if (offset == 0 && f == f->parent()->firstChild()) {
                            return;
                        } else if (nextOffset == srcTxt->length() && f == f->parent()->lastChild()) {
                            return;
                        }
                    }

                    LayoutUnit w = 0;
                    if (isWhiteSpace) {
                        w = f->style()->font()->spaceWidth();
                    } else {
                        w = f->style()->font()->measureText(srcTxt->substring(offset, nextOffset - offset));
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
                        ctx.setResult(remainWidth);
                        currentLineWidth = 0;
                    }
                });
            } else if (f->isFrameBlockBox()) {
                LayoutUnit mbp = ComputePreferredWidthContext::computeMinimumWidthDueToMBP(f->style());
                ComputePreferredWidthContext newCtx(ctx.layoutContext(), remainWidth - mbp);
                f->computePreferredWidth(newCtx);
                ctx.setResult(newCtx.result() + mbp);
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
                    ctx.setResult(remainWidth);
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
