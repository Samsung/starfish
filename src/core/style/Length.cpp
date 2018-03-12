/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/Node.h"
#include "core/layout/Frame.h"
#include "core/style/DefaultStyle.h"
#include "core/style/Length.h"
#include "core/style/CalcData.h"
#include "core/page/Window.h"

namespace StarFish {
void Length::changeToFixedIfNeeded(Length curFontSize, Length rootFontSize,
                                   Font* font, LayoutUnit viewportWidth,
                                   LayoutUnit viewportHeight, ComputedStyle* cs)
{
    if (isFontPercent()) {
        if (m_type == Rem && !rootFontSize.isFixed()) {
            return;
        } else if ((m_type == Em || m_type == Ex) && !curFontSize.isFixed()) {
            return;
        }
        Length unused = Length(Length::Fixed, 0);
        if (!curFontSize.isFixed()) {
            curFontSize = unused;
        }

        if (!rootFontSize.isFixed()) {
            rootFontSize = unused;
        }

        STARFISH_ASSERT(font);
        m_data =
            fontPercentValue(curFontSize.fixed(), rootFontSize.fixed(), font);
        m_type = Fixed;
    } else if (isViewportPercent()) {
        m_data = viewportPercentValue(viewportWidth, viewportHeight);
        m_type = Fixed;
        cs->m_seenViewPortUnitInStyle = true;
    } else if (isCalc()) {
        GCVector<CalcTerm*>& data = calcData()->terms();
        auto iter = data.begin();

        while (iter != data.end()) {
            GCVector<CalcValue>& data2 = (*iter)->values();
            auto iter2 = data2.begin();
            while (iter2 != data2.end()) {
                CalcValue& v = *iter2;
                if (v.type().isLength()) {
                    Length l = v.lengthValue().toLength();
                    if (!l.isComputed()) {
                        l.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                                viewportWidth, viewportHeight,
                                                cs);
                        v.setValue(CSSLength(CSSLength::PX, l.fixed()));
                    }
                }
                iter2++;
            }
            iter++;
        }
    }
}

bool Length::isCalcAndLengthOfType() const
{
    return isCalc() && calcData()->type().isLength();
}

bool Length::hasPercent() const
{
    if (isPercent()) {
        return true;
    }

    if (!isCalc()) {
        return false;
    }

    GCVector<CalcTerm*>& data = calcData()->terms();
    auto iter = data.begin();

    while (iter != data.end()) {
        GCVector<CalcValue>& data2 = (*iter)->values();
        auto iter2 = data2.begin();
        while (iter2 != data2.end()) {
            CalcValue& v = *iter2;
            if (v.type().isPercentage()) {
                return true;
            }
            iter2++;
        }
        iter++;
    }

    return false;
}

bool Length::hasViewportPercent() const
{
    if (isViewportPercent()) {
        return true;
    }

    if (!isCalc()) {
        return false;
    }

    GCVector<CalcTerm*>& data = calcData()->terms();
    auto iter = data.begin();

    while (iter != data.end()) {
        GCVector<CalcValue>& data2 = (*iter)->values();
        auto iter2 = data2.begin();
        while (iter2 != data2.end()) {
            CalcValue& v = *iter2;
            if (v.type().isLength()) {
                if (v.lengthValue().toLength().isViewportPercent()) {
                    return true;
                }
            }
            iter2++;
        }
        iter++;
    }

    return false;
}

float Length::specifiedValue(LayoutUnit parentLength, Frame* f) const
{
    return specifiedValue(parentLength, f->nearstNotAnonymousNode());
}

float Length::specifiedValue(LayoutUnit parentLength, Node* n) const
{
    STARFISH_ASSERT(n);
    STARFISH_ASSERT(isSpecified());
    if (isFixed()) {
        return fixed();
    } else if (isViewportPercent()) {
        Window* w = n->window();
        return viewportPercentValue(w->innerWidth(), w->innerHeight());
    } else if (isPercent()) {
        return percentValue(parentLength);
    } else if (isFontPercent()) {
        return fontPercentValue(n, false);
    } else {
        STARFISH_ASSERT(isCalc());
        return calcData()->specifiedValue(parentLength, n);
    }
}

float Length::specifiedFontValue(Node* n)
{
    Element* e;
    if (n->isElement()) {
        e = n->asElement();
    } else {
        e = n->parentElement();
    }

    return specifiedFontValue(e);
}

float Length::specifiedFontValue(Element* e)
{
    if (isFixed()) {
        return fixed();
    } else if (isPercent()) {
        Element* p = e->parentElement();
        float fixedParentFontSize;
        if (p) {
            fixedParentFontSize = p->style()->fixedFontSize();
        } else {
            fixedParentFontSize = e->document()->style()->fixedFontSize();
        }
        return percentValue(fixedParentFontSize);
    } else if (isViewportPercent()) {
        Window* w = e->window();
        return viewportPercentValue(w->innerWidth(), w->innerHeight());
    } else if (isFontPercent()) {
        return fontPercentValue(e, true);
    } else {
        STARFISH_ASSERT(isCalc());
        return calcData()->specifiedFontValue(e);
    }
}

float Length::fontPercentValue(LayoutUnit curFontSize, LayoutUnit rootFontSize,
                               Font* font) const
{
    if (m_type == Em) {
        return fontPercent() * curFontSize;
    } else if (m_type == Ex) {
        return fontPercent() * curFontSize * font->metrics().m_xheightRate;
    } else if (m_type == Ch) {
        LayoutUnit size =
            font->measureText(StringView(String::createUTF32String(U'\u0030')));
        return fontPercent() * size.toDouble();
    } else {
        STARFISH_ASSERT(m_type == Rem);
        return fontPercent() * rootFontSize;
    }
}

float Length::fontPercentValue(Node* n, bool isFontSize) const
{
    Element* e;
    if (n->isElement()) {
        e = n->asElement();
    } else {
        e = n->parentElement();
    }
    float fontSize;
    Font* font;
    if (isFontSize) {
        Element* p = e->parentElement();
        if (p) {
            fontSize = p->style()->fixedFontSize();
            font = p->style()->font();
        } else {
            fontSize = e->document()->style()->fixedFontSize();
            font = e->document()->style()->font();
        }
    } else {
        fontSize = e->style()->fixedFontSize();
        font = e->style()->font();
    }

    float rootElementFixedFontSize;

    if (n->document()->rootElement() && n->document()->rootElement()->style()) {
        rootElementFixedFontSize =
            n->document()->rootElement()->style()->fixedFontSize();
    } else {
        rootElementFixedFontSize = fontSize;
    }

    return fontPercentValue(fontSize, rootElementFixedFontSize, font);
}

bool Length::operator==(const Length& src) const
{
    if (m_type != src.m_type) {
        return false;
    }

    if (isCalc()) {
        return calcData()->toString()->equals(src.calcData()->toString());
    } else {
        return m_data.m_numberData == src.m_data.m_numberData;
    }
}

String* Length::dumpString()
{
    if (isCalc()) {
        return m_data.m_calcData->toString();
    }

    char temp[100];
    if (isFixed()) {
        snprintf(temp, sizeof(temp), "%.1f", fixed());
    } else if (isPercent()) {
        snprintf(temp, sizeof(temp), "%.1f%%", percent());
    } else if (isViewportPercent()) {
        if (m_type == Vw) {
            snprintf(temp, sizeof(temp), "%.1fvw", viewportPercent());
        } else if (m_type == Vh) {
            snprintf(temp, sizeof(temp), "%.1fvh", viewportPercent());
        } else if (m_type == Vmin) {
            snprintf(temp, sizeof(temp), "%.1fvmin", viewportPercent());
        } else if (m_type == Vmax) {
            snprintf(temp, sizeof(temp), "%.1fvmax", viewportPercent());
        }
    } else if (isFontPercent()) {
        if (m_type == Em) {
            snprintf(temp, sizeof(temp), "%.1fem", fontPercent());
        } else if (m_type == Ex) {
            snprintf(temp, sizeof(temp), "%.1fex", fontPercent());
        } else if (m_type == Rem) {
            snprintf(temp, sizeof(temp), "%.1frem", fontPercent());
        }
    } else if (isAuto()) {
        snprintf(temp, sizeof(temp), "auto");
    } else if (isInheritableNumber()) {
        snprintf(temp, sizeof(temp), "%.1f(num)", inheritableNumber());
    }
    return String::fromUTF8(temp);
}

Length operator*(const Length& a, const float b)
{
    if (a.isAuto()) {
        return Length();
    }

    if (a.isCalc()) {
        CalcData* data = new CalcData();
        GCVector<CalcTerm*>& data2 = a.calcData()->terms();
        auto iter = data2.begin();

        while (iter != data2.end()) {
            CalcTerm* term = new CalcTerm();
            GCVector<CalcValue>& term2 = (*iter)->values();
            auto iter2 = term2.begin();
            while (iter2 != term2.end()) {
                CalcValue& v = *iter2;
                if (term->hasValue()) {
                    term->appendValue(true, v);
                } else {
                    term->appendValue(v);
                }
                iter2++;
            }
            term->appendValue(true, CalcValue(b, false));
            iter++;
            data->appendTerm(term);
        }

        return Length(data);
    } else {
        return Length(a.type(), a.numberData() * b);
    }
}

Length operator*(const float a, const Length& b)
{
    return operator*(b, a);
}

Length operator/(const Length& a, const float b)
{
    return operator*(a, 1 / b);
}

Length operator/(const float a, const Length& b)
{
    return operator*(b, 1 / a);
}

void LengthSize::checkComputed(Length curFontSize, Length rootFontSize,
                               Font* font, LayoutSize windowSize,
                               ComputedStyle* cs)
{
    m_width.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                  windowSize.width(), windowSize.height(), cs);
    m_height.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                   windowSize.width(), windowSize.height(), cs);
}

void LengthPosition::checkComputed(Length curFontSize, Length rootFontSize,
                                   Font* font, LayoutSize windowSize,
                                   ComputedStyle* cs)
{
    m_x.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                              windowSize.width(), windowSize.height(), cs);
    m_y.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                              windowSize.width(), windowSize.height(), cs);
}

void LengthBox::checkComputed(Length curFontSize, Length rootFontSize,
                              Font* font, LayoutSize windowSize,
                              ComputedStyle* cs)
{
    m_left.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                 windowSize.width(), windowSize.height(), cs);
    m_right.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                  windowSize.width(), windowSize.height(), cs);
    m_top.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                windowSize.width(), windowSize.height(), cs);
    m_bottom.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                   windowSize.width(), windowSize.height(), cs);
}
}
