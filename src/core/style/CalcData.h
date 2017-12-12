/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishCalcData__
#define __StarFishCalcData__

#include "core/util/String.h"
#include "core/style/Length.h"
#include "core/style/Style.h"

namespace StarFish {

class CalcValueType {
public:
    enum ValueType {
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

        CalcValueData(float data)
            : m_numberData(data)
        {
        }

        CalcValueData(CSSLength data)
            : m_lengthData(data)
        {
        }

        CalcValueData(CSSAngle data)
            : m_angleData(data)
        {
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

    LayoutUnit specifiedValue(LayoutUnit parentContentLength, Node* n) const;
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

    String* toString()
    {
        if (m_type.isNumber()) {
            return String::fromFloat(m_data.m_numberData);
        } else if (m_type.isLength()) {
            return m_data.m_lengthData.toString();
        } else if (m_type.isAngle()) {
            return m_data.m_angleData.toString();
        } else if (m_type.isTime()) {
            return m_data.m_timeData.toString();
        } else if (m_type.isPercentage()) {
            StringBuilder builder;
            builder.appendString(String::fromFloat(m_data.m_numberData));
            builder.appendChar('%');
            return builder.finalize();
        }

        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return String::emptyString;
    }

private:
    CalcValueType m_type;
    CalcValueData m_data;
};

#define MUL true
#define DIV false

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

    CalcValueType type() const
    {
        STARFISH_ASSERT(m_values.size() - 1 == m_operators.size());
        auto it = m_values.begin();
        CalcValueType lType = (*it).type();
        it++;
        auto it2 = m_operators.begin();
        while (it != m_values.end()) {
            CalcValueType rType = (*it).type();
            bool operand = *it2;

            if (operand == MUL) {
                if (lType.isNumber()) {
                    lType = rType;
                } else if (!rType.isNumber()) {
                    return CalcValueType::Invalid;
                }
            } else {
                if (!(rType.isNumber() && (*it).numberValue() != 0)) {
                    return CalcValueType::Invalid;
                }
            }

            it++;
            it2++;
        }

        return lType;
    }

    float numberValue() const
    {
        STARFISH_ASSERT(m_values.size() - 1 == m_operators.size());
        auto it = m_values.begin();
        float n = (*it).numberValue();
        it++;
        auto it2 = m_operators.begin();
        while (it != m_values.end()) {
            bool operand = *it2;

            if (operand == MUL) {
                n *= (*it).numberValue();
            } else {
                n /= (*it).numberValue();
            }

            it++;
            it2++;
        }

        return n;
    }

    LayoutUnit specifiedValue(LayoutUnit parentContentLength, Node* n) const
    {
        STARFISH_ASSERT(m_values.size() - 1 == m_operators.size());
        auto it = m_values.begin();
        CalcValueType lType = (*it).type();
        float num = 1;
        LayoutUnit l;
        if (lType.isNumber()) {
            num = (*it).numberValue();
        } else {
            l = (*it).specifiedValue(parentContentLength, n);
        }
        it++;
        auto it2 = m_operators.begin();
        while (it != m_values.end()) {
            CalcValueType rType = (*it).type();
            bool operand = *it2;

            if (operand == MUL) {
                if (rType.isNumber()) {
                    num *= (*it).numberValue();
                } else {
                    l = (*it).specifiedValue(parentContentLength, n);
                    l *= num;
                    num = 1;
                }
            } else {
                if (rType.isNumber()) {
                    num /= (*it).numberValue();
                } else {
                    l = (*it).specifiedValue(parentContentLength, n);
                    l /= num;
                    num = 1;
                }
            }

            it++;
            it2++;
        }

        return num * l;
    }

    LayoutUnit specifiedFontValue(Node* n) const
    {
        STARFISH_ASSERT(m_values.size() - 1 == m_operators.size());
        auto it = m_values.begin();
        CalcValueType lType = (*it).type();
        float num = 1;
        LayoutUnit l;
        if (lType.isNumber()) {
            num = (*it).numberValue();
        } else {
            l = (*it).specifiedFontValue(n);
        }
        it++;
        auto it2 = m_operators.begin();
        while (it != m_values.end()) {
            CalcValueType rType = (*it).type();
            bool operand = *it2;

            if (operand == MUL) {
                if (rType.isNumber()) {
                    num *= (*it).numberValue();
                } else {
                    l = (*it).specifiedFontValue(n);
                    l *= num;
                    num = 1;
                }
            } else {
                if (rType.isNumber()) {
                    num /= (*it).numberValue();
                } else {
                    l = (*it).specifiedFontValue(n);
                    l /= num;
                    num = 1;
                }
            }

            it++;
            it2++;
        }

        return num * l;
    }

    CSSAngle angleValue() const
    {
        STARFISH_ASSERT(m_values.size() - 1 == m_operators.size());
        auto it = m_values.begin();
        CalcValueType lType = (*it).type();
        float num = 1;
        CSSAngle a;
        if (lType.isNumber()) {
            num = (*it).numberValue();
        } else {
            a = (*it).angleValue();
        }
        it++;
        auto it2 = m_operators.begin();
        while (it != m_values.end()) {
            CalcValueType rType = (*it).type();
            bool operand = *it2;

            if (operand == MUL) {
                if (lType.isNumber()) {
                    num *= (*it).numberValue();
                } else {
                    a = (*it).angleValue();
                    a *= num;
                    num = 1;
                }
            } else {
                if (lType.isNumber()) {
                    num /= (*it).numberValue();
                } else {
                    a = (*it).angleValue();
                    a /= num;
                    num = 1;
                }
            }

            it++;
            it2++;
        }

        return num * a;
    }

    CSSTime timeValue() const
    {
        STARFISH_ASSERT(m_values.size() - 1 == m_operators.size());
        auto it = m_values.begin();
        CalcValueType lType = (*it).type();
        float num = 1;
        CSSTime t;
        if (lType.isNumber()) {
            num = (*it).numberValue();
        } else {
            t = (*it).timeValue();
        }
        it++;
        auto it2 = m_operators.begin();
        while (it != m_values.end()) {
            CalcValueType rType = (*it).type();
            bool operand = *it2;

            if (operand == MUL) {
                if (lType.isNumber()) {
                    num *= (*it).numberValue();
                } else {
                    t = (*it).timeValue();
                    t *= num;
                    num = 1;
                }
            } else {
                if (lType.isNumber()) {
                    num /= (*it).numberValue();
                } else {
                    t = (*it).timeValue();
                    t /= num;
                    num = 1;
                }
            }

            it++;
            it2++;
        }

        return num * t;
    }

    String* toString()
    {
        STARFISH_ASSERT(m_values.size() - 1 == m_operators.size());
        StringBuilder builder;
        auto it = m_values.begin();
        builder.appendString((*it).toString());
        it++;
        auto it2 = m_operators.begin();
        while (it != m_values.end()) {
            builder.appendString(String::spaceString);
            bool operand = *it2;

            if (operand == MUL) {
                builder.appendChar('*');
            } else {
                builder.appendChar('/');
            }

            builder.appendString(String::spaceString);
            builder.appendString((*it).toString());

            it++;
            it2++;
        }

        return builder.finalize();
    }

private:
    GCVector<bool> m_operators;
    GCVector<CalcValue> m_values;
};

#undef MUL
#undef DIV

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

    LayoutUnit specifiedValue(LayoutUnit parentContentLength, Node* n) const
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

    String* toString()
    {
        StringBuilder builder;
        auto it = m_terms.begin();
        builder.appendString((*it)->toString());
        it++;
        while (it != m_terms.end()) {
            builder.appendString(String::spaceString);
            String* r = (*it)->toString();

            if (r->charAt(0) == '-') {
                builder.appendChar('-');
            } else {
                builder.appendChar('+');
            }

            builder.appendString(String::spaceString);
            builder.appendString(r->substring(1, r->length() - 1));

            it++;
        }

        return builder.finalize();
    }

private:
    GCVector<CalcTerm*> m_terms;
};
}

#endif
