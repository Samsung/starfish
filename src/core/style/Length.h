/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishLength__
#define __StarFishLength__

#include "core/layout/LayoutUtil.h"

namespace StarFish {

class String;
class Font;
class CalcData;
class Node;
class Frame;
class Element;
class ComputedStyle;

class Length {
public:
    enum Type {
        Auto,
        Percent,
        Fixed,

        // After finishing resolveStyle, ex/em values should be changed to Fixed
        Ex,
        Em,
        Rem,
        Vw,
        Vh,
        Vmin,
        Vmax,

        // This is for line-height
        // (font-related value but does not change to Fixed since inheritance
        // issue)
        InheritableNumber,
        Calc,
    };

    STARFISH_MAKE_STACK_ALLOCATED();

    Length(Type type = Auto, float data = 0.f)
        : m_data(data)
        , m_type(type)
    {
        STARFISH_ASSERT(!isCalc());
    }

    Length(CalcData* data)
        : m_data(data)
        , m_type(Calc)
    {
    }

    void changeToFixedIfNeeded(Length curFontSize, Length rootFontSize,
                               Font* font, LayoutUnit viewportWidth,
                               LayoutUnit viewportHeight, ComputedStyle* cs);

    void roundBorderWidth()
    {
        if (!isFixed()) {
            return;
        }
        // NOTE: Border Widths are rounded to the nearest integer number of
        // pixels, but values between zero and one pixels are always rounded
        // up to one device pixel.
        if (m_data.m_numberData > 0.0 && m_data.m_numberData < 1.0) {
            m_data.m_numberData = 1.0;
        } else {
            if (m_data.m_numberData < 0) {
                m_data.m_numberData -= 0.01;
            } else {
                m_data.m_numberData += 0.01;
            }
            m_data.m_numberData =
                ((m_data.m_numberData > std::numeric_limits<unsigned>::max()) ||
                 (m_data.m_numberData < std::numeric_limits<unsigned>::min()))
                    ? 0
                    : static_cast<unsigned>(m_data.m_numberData);
        }
    }

    bool isSpecified() const
    {
        return isFixed() || isPercent() || isViewportPercent() ||
               isFontPercent() || isCalc();
    }

    bool isDefinite(bool canApplyPercentage) const
    {
        if (isSpecified()) {
            if (isFixed() || isViewportPercent() || isFontPercent() ||
                isCalcAndLengthOfType()) {
                return true;
            } else {
                return canApplyPercentage;
            }
        }

        return false;
    }

    bool isAuto() const
    {
        return m_type == Auto;
    }

    bool isFixed() const
    {
        return m_type == Fixed;
    }

    bool isPercent() const
    {
        return m_type == Percent;
    }

    bool isViewportPercent() const
    {
        return Vw <= m_type && m_type <= Vmax;
    }

    bool isFontPercent() const
    {
        return Ex <= m_type && m_type <= Rem;
    }

    bool isInheritableNumber() const
    {
        return m_type == InheritableNumber;
    }

    bool isCalc() const
    {
        return m_type == Calc;
    }

    bool isCalcAndLengthOfType() const;

    bool isComputed() const
    {
        return isFixed() || isPercent() || isViewportPercent() || isAuto();
    }

    bool hasViewportPercent() const;

    Type type() const
    {
        return m_type;
    }

    float fontPercent() const
    {
        STARFISH_ASSERT(isFontPercent());
        return m_data.m_numberData;
    }

    float viewportPercent() const
    {
        STARFISH_ASSERT(isViewportPercent());
        return m_data.m_numberData;
    }

    float percent() const
    {
        STARFISH_ASSERT(isPercent());
        // 0~1
        return m_data.m_numberData;
    }

    float fixed() const
    {
        if (!isFixed()) {
            STARFISH_ASSERT_NOT_REACHED();
        }
        STARFISH_ASSERT(isFixed());
        return m_data.m_numberData;
    }

    float inheritableNumber() const
    {
        STARFISH_ASSERT(isInheritableNumber());
        return m_data.m_numberData;
    }

    float numberData() const
    {
        STARFISH_ASSERT(!isAuto() && !isCalc());
        return m_data.m_numberData;
    }

    CalcData* calcData() const
    {
        STARFISH_ASSERT(m_type == Calc);
        return m_data.m_calcData;
    }

    float specifiedValue(LayoutUnit parentLength, Frame* f) const;
    float specifiedValue(LayoutUnit parentLength, Node* n) const;
    float specifiedFontValue(Node* n);
    float specifiedFontValue(Element* e);

    float percentValue(LayoutUnit parentLength) const
    {
        STARFISH_ASSERT(isPercent());
        return parentLength * percent();
    }

    float viewportPercentValue(LayoutUnit viewportWidth,
                               LayoutUnit viewportHeight) const
    {
        if (m_type == Vw) {
            return viewportWidth * viewportPercent() / 100;
        } else if (m_type == Vh) {
            return viewportHeight * viewportPercent() / 100;
        } else if (m_type == Vmin) {
            return std::min(viewportWidth, viewportHeight) * viewportPercent() /
                   100;
        } else {
            STARFISH_ASSERT(m_type == Vmax);
            return std::max(viewportWidth, viewportHeight) * viewportPercent() /
                   100;
        }
    }

    float fontPercentValue(LayoutUnit curFontSize, LayoutUnit rootFontSize,
                           Font* font) const;
    float fontPercentValue(Node* n, bool isFontSize) const;

    bool isZero()
    {
        return isSpecified() && !m_data.m_numberData;
    }

    bool isPositiveOrZero()
    {
        STARFISH_ASSERT(isSpecified());
        return m_data.m_numberData >= 0;
    }

    bool operator==(const Length& src) const;
    bool operator!=(const Length& src) const
    {
        return !operator==(src);
    }

    String* dumpString();

protected:
    union ValueData {
        float m_numberData;
        CalcData* m_calcData;

        ValueData(float data)
            : m_numberData(data)
        {
        }

        ValueData(CalcData* data)
            : m_calcData(data)
        {
        }
    };
    ValueData m_data;
    Type m_type;
};

Length operator*(const Length& a, const float b);
Length operator*(const float a, const Length& b);
Length operator/(const Length& a, const float b);
Length operator/(const float a, const Length& b);

class LengthSize : public gc {
public:
    LengthSize()
    {
    }

    LengthSize(Length width)
        : m_width(width)
    {
    }

    LengthSize(Length width, Length height)
        : m_width(width)
        , m_height(height)
    {
    }

    Length width()
    {
        return m_width;
    }

    Length height()
    {
        return m_height;
    }

    bool operator==(const LengthSize& o) const
    {
        return this->m_width == o.m_width && this->m_height == o.m_height;
    }

    bool operator!=(const LengthSize& o) const
    {
        return !operator==(o);
    }

    void checkComputed(Length curFontSize, Length rootFontSize, Font* font,
                       LayoutSize windowSize, ComputedStyle* cs);

    Length m_width;
    Length m_height;
};

class LengthPosition : public gc {
public:
    LengthPosition()
    {
    }

    LengthPosition(Length x)
        : m_x(x)
    {
    }

    LengthPosition(Length x, Length y)
        : m_x(x)
        , m_y(y)
    {
    }

    Length x()
    {
        return m_x;
    }

    Length y()
    {
        return m_y;
    }

    bool operator==(const LengthPosition& o) const
    {
        return this->m_x == o.m_x && this->m_y == o.m_y;
    }

    bool operator!=(const LengthPosition& o) const
    {
        return !operator==(o);
    }

    void checkComputed(Length curFontSize, Length rootFontSize, Font* font,
                       LayoutSize windowSize, ComputedStyle* cs);

    Length m_x;
    Length m_y;
};

class LengthBox {
public:
    STARFISH_MAKE_STACK_ALLOCATED();

    LengthBox()
    {
    }

    LengthBox(float v)
        : m_left(Length(Length::Fixed, v))
        , m_right(Length(Length::Fixed, v))
        , m_top(Length(Length::Fixed, v))
        , m_bottom(Length(Length::Fixed, v))
    {
    }

    LengthBox(const Length& t, const Length& r, const Length& b,
              const Length& l)
        : m_left(l)
        , m_right(r)
        , m_top(t)
        , m_bottom(b)
    {
    }

    LengthBox(float t, float r, float b, float l)
        : m_left(Length(Length::Fixed, l))
        , m_right(Length(Length::Fixed, r))
        , m_top(Length(Length::Fixed, t))
        , m_bottom(Length(Length::Fixed, b))
    {
    }

    bool operator==(const LengthBox& o)
    {
        return this->m_left == o.m_left && this->m_right == o.m_right &&
               this->m_top == o.m_top && this->m_bottom == o.m_bottom;
    }

    bool operator!=(const LengthBox& o)
    {
        return !operator==(o);
    }

    void checkComputed(Length fontSize, Length rootFontSize, Font* font,
                       LayoutSize windowSize, ComputedStyle* cs);

    const Length& left() const
    {
        return m_left;
    }
    const Length& right() const
    {
        return m_right;
    }
    const Length& top() const
    {
        return m_top;
    }
    const Length& bottom() const
    {
        return m_bottom;
    }

    Length m_left;
    Length m_right;
    Length m_top;
    Length m_bottom;
};
}

#endif
