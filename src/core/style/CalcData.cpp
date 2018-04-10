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
#include "core/style/CalcData.h"

namespace StarFish {
LayoutUnit CalcValue::specifiedValue(LayoutUnit parentContentLength,
                                     Node* n) const
{
    if (m_type.isLength()) {
        return m_data.m_lengthData.toLength().specifiedValue(
            parentContentLength, n);
    } else if (m_type.isPercentage()) {
        return Length(Length::Percent, m_data.m_numberData)
            .percentValue(parentContentLength);
    }

    return LayoutUnit();
}

LayoutUnit CalcValue::specifiedFontValue(Node* n) const
{
    if (m_type.isLength()) {
        return m_data.m_lengthData.toLength().specifiedFontValue(n);
    } else if (m_type.isPercentage()) {
        return Length(Length::Percent, m_data.m_numberData)
            .specifiedFontValue(n);
    }

    return LayoutUnit();
}

String* CalcValue::toString()
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
        builder.appendString(String::fromFloat(m_data.m_numberData * 100.f));
        builder.appendChar('%');
        return builder.finalize();
    }

    STARFISH_RELEASE_ASSERT_NOT_REACHED();
    return String::emptyString;
}

#define MUL true
#define DIV false

CalcValueType CalcTerm::type() const
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

float CalcTerm::numberValue() const
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

LayoutUnit CalcTerm::specifiedValue(LayoutUnit parentContentLength,
                                    Node* n) const
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

LayoutUnit CalcTerm::specifiedFontValue(Node* n) const
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

CSSAngle CalcTerm::angleValue() const
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

CSSTime CalcTerm::timeValue() const
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

String* CalcTerm::toString()
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
#undef MUL
#undef DIV

String* CalcData::toString()
{
    StringBuilder builder;

    builder.appendString(String::createASCIIString("calc("));
    auto it = m_terms.begin();
    builder.appendString((*it)->toString());
    it++;
    while (it != m_terms.end()) {
        builder.appendString(String::spaceString);
        String* r = (*it)->toString();

        if (r->charAt(0) == '-') {
            builder.appendString(String::createASCIIString("- "));
            builder.appendString(r->substring(1, r->length() - 1));
        } else if (r->charAt(0) == '+') {
            builder.appendString(String::createASCIIString("+ "));
            builder.appendString(r->substring(1, r->length() - 1));
        } else {
            builder.appendString(String::createASCIIString("+ "));
            builder.appendString(r);
        }

        it++;
    }
    builder.appendChar(')');
    return builder.finalize();
}
}
