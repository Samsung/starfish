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

#ifndef __StarFishCalcData__
#define __StarFishCalcData__

#include "core/style/CSSAngle.h"
#include "core/style/CSSLength.h"
#include "core/style/CSSTime.h"

namespace StarFish {

class CalcValueType {
public:
    enum ValueType ENSURE_ENUM_UNSIGNED {
        None,
        Invalid, // Only for type checking
        Number,
        Length,
        Angle,
        Time,
        Percentage
    };

    STARFISH_MAKE_STACK_ALLOCATED();

    CalcValueType()
        : m_type(None)
    {
    }

    CalcValueType(ValueType type)
        : m_type(type)
    {
    }

    bool isNone() const
    {
        return m_type == None;
    }

    bool isInvalid() const
    {
        return m_type == Invalid;
    }

    bool isNumber() const
    {
        return m_type == Number;
    }

    bool isLength() const
    {
        return m_type == Length;
    }

    bool isAngle() const
    {
        return m_type == Angle;
    }

    bool isTime() const
    {
        return m_type == Time;
    }

    bool isPercentage() const
    {
        return m_type == Percentage;
    }

    bool operator==(CalcValueType& other) const
    {
        return m_type == other.m_type;
    }

    bool operator!=(CalcValueType& other) const
    {
        return !(operator==(other));
    }

    ValueType m_type : 3;
};

class CalcValue {
public:
    union CalcValueData {
        float m_numberData;
        CSSLength m_lengthData;
        CSSAngle m_angleData;
        CSSTime m_timeData;

        CalcValueData()
            : m_timeData(0)
        {
        }

        CalcValueData(float data)
        {
            m_timeData = 0; // initialize this union first
            m_numberData = data;
        }

        CalcValueData(CSSLength data)
        {
            m_timeData = 0; // initialize this union first
            m_lengthData = data;
        }

        CalcValueData(CSSAngle data)
        {
            m_timeData = 0; // initialize this union first
            m_angleData = data;
        }

        CalcValueData(CSSTime data)
            : m_timeData(data)
        {
        }
    };

    CalcValue()
        : m_type(CalcValueType::None)
        , m_data(0)
    {
    }

    CalcValue(CalcValueType type)
        : m_type(type)
        , m_data(0)
    {
    }

    CalcValue(float data, bool isPercentage)
        : m_type(isPercentage ? CalcValueType::Percentage
                              : CalcValueType::Number)
        , m_data(data)
    {
    }

    CalcValue(CSSLength data)
        : m_type(CalcValueType::Length)
        , m_data(data)
    {
    }

    CalcValue(CSSAngle data)
        : m_type(CalcValueType::Angle)
        , m_data(data)
    {
    }

    CalcValue(CSSTime data)
        : m_type(CalcValueType::Time)
        , m_data(data)
    {
    }

    void setType(CalcValueType type)
    {
        m_type = type;
    }

    CalcValueType type() const
    {
        return m_type;
    }

    void setValue(CalcValueData data)
    {
        m_data = data;
    }

    float numberValue() const
    {
        STARFISH_ASSERT(m_type.isNumber());
        return m_data.m_numberData;
    }

    float percentageValue() const
    {
        STARFISH_ASSERT(m_type.isPercentage());
        return m_data.m_numberData;
    }

    CSSLength lengthValue() const
    {
        STARFISH_ASSERT(m_type.isLength());
        return m_data.m_lengthData;
    }

    LayoutUnit specifiedValue(const LayoutUnit& parentContentLength,
                              Node* n) const;
    LayoutUnit specifiedFontValue(Node* n) const;

    CSSAngle angleValue() const
    {
        STARFISH_ASSERT(m_type.isAngle());
        return m_data.m_angleData;
    }

    CSSTime timeValue() const
    {
        STARFISH_ASSERT(m_type.isTime());
        return m_data.m_timeData;
    }

    String* toString();

private:
    CalcValueType m_type;
    CalcValueData m_data;
};

class CalcTerm : public gc {
public:
    CalcTerm()
    {
    }

    bool hasValue() const
    {
        return m_values.size() > 0;
    }

    void appendValue(CalcValue value)
    {
        STARFISH_ASSERT(m_values.size() == 0 && m_operators.size() == 0);
        m_values.push_back(value);
    }

    void appendValue(bool isMul, CalcValue value)
    {
        m_operators.push_back(isMul);
        m_values.push_back(value);
    }

    GCVector<CalcValue>& values()
    {
        return m_values;
    }

    CalcValueType type() const;

    float numberValue() const;

    LayoutUnit specifiedValue(const LayoutUnit& parentContentLength,
                              Node* n) const;

    LayoutUnit specifiedFontValue(Node* n) const;

    CSSAngle angleValue() const;

    CSSTime timeValue() const;

    String* toString();

private:
    GCVector<bool> m_operators;
    GCVector<CalcValue> m_values;
};

class CalcData : public gc {
public:
    CalcData()
    {
    }

    void appendTerm(CalcTerm* term)
    {
        m_terms.push_back(term);
    }

    GCVector<CalcTerm*>& terms()
    {
        return m_terms;
    }

    CalcValueType type() const
    {
        auto it = m_terms.begin();
        CalcValueType lType = (*it)->type();
        it++;
        while (it != m_terms.end()) {
            CalcValueType rType = (*it)->type();

            if (lType != rType) {
                // Percentage type overwrites length
                if (lType.isLength() && rType.isPercentage()) {
                    lType = rType;
                } else if (lType.isPercentage() && rType.isLength()) {
                } else {
                    return CalcValueType::Invalid;
                }
            }

            it++;
        }

        return lType;
    }

    float numberValue() const
    {
        auto it = m_terms.begin();
        float n = (*it)->numberValue();
        it++;
        while (it != m_terms.end()) {
            n += (*it)->numberValue();
            it++;
        }

        return n;
    }

    LayoutUnit specifiedValue(const LayoutUnit& parentContentLength,
                              Node* n) const
    {
        auto it = m_terms.begin();
        LayoutUnit l = (*it)->specifiedValue(parentContentLength, n);
        it++;
        while (it != m_terms.end()) {
            l += (*it)->specifiedValue(parentContentLength, n);
            it++;
        }

        return l;
    }

    LayoutUnit specifiedFontValue(Node* n) const
    {
        auto it = m_terms.begin();
        LayoutUnit l = (*it)->specifiedFontValue(n);
        it++;
        while (it != m_terms.end()) {
            l += (*it)->specifiedFontValue(n);
            it++;
        }

        return l;
    }

    CSSAngle angleValue() const
    {
        auto it = m_terms.begin();
        CSSAngle a = (*it)->angleValue();
        it++;
        while (it != m_terms.end()) {
            a += (*it)->angleValue();
            it++;
        }

        return a;
    }

    CSSTime timeValue() const
    {
        auto it = m_terms.begin();
        CSSTime t = (*it)->timeValue();
        it++;
        while (it != m_terms.end()) {
            t += (*it)->timeValue();
            it++;
        }

        return t;
    }

    String* toString();

private:
    GCVector<CalcTerm*> m_terms;
};
}

#endif
