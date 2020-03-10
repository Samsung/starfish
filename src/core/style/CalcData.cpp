/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#include "StarfishConfig.h"
#include "core/style/Style.h"
#include "core/style/CalcData.h"

namespace Starfish {
LayoutUnit CalcValue::specifiedValue(const LayoutUnit& parentContentLength,
                                     Node* n) const
{
    if (m_type.isLength()) {
        return m_data.m_lengthData.toLength().specifiedValue(
            parentContentLength, n);
    } else if (m_type.isPercentage()) {
        return Length(Length::Percent, m_data.m_numberData)
            .percentValue(parentContentLength);
    } else if (m_type.isCalcData()) {
        return m_data.m_calcData->specifiedValue(parentContentLength, n);
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
    } else if (m_type.isCalcData()) {
        return m_data.m_calcData->specifiedFontValue(n);
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
    } else if (m_type.isCalcData()) {
        String* s = m_data.m_calcData->toString();
        return s->substring(4, s->length() - 4);
    }

    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return String::emptyString;
}

bool CalcValue::equals(const CalcValue& with) const
{
    if (m_type != with.m_type) {
        return false;
    }

    if (m_type.isNumber()) {
        return m_data.m_numberData == with.m_data.m_numberData;
    } else if (m_type.isLength()) {
        return m_data.m_lengthData == with.m_data.m_lengthData;
    } else if (m_type.isAngle()) {
        return m_data.m_angleData == with.m_data.m_angleData;
    } else if (m_type.isTime()) {
        return m_data.m_timeData == with.m_data.m_timeData;
    } else if (m_type.isPercentage()) {
        return m_data.m_numberData == with.m_data.m_numberData;
    } else if (m_type.isCalcData()) {
        return m_data.m_calcData->equals(with.m_data.m_calcData);
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
}

#define MUL true
#define DIV false

CalcValueType CalcTerm::type() const
{
    STARFISH_ASSERT(m_values.size() - 1 == m_operators.size());
    auto it = m_values.begin();
    CalcValueType lType = (*it).type();
    if (lType.isCalcData()) {
        lType = it->calcDataValue()->type();
    }
    it++;
    auto it2 = m_operators.begin();
    while (it != m_values.end()) {
        CalcValueType rType = (*it).type();
        if (rType.isCalcData()) {
            rType = it->calcDataValue()->type();
        }
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

LayoutUnit CalcTerm::specifiedValue(const LayoutUnit& parentContentLength,
                                    Node* n) const
{
    STARFISH_ASSERT(m_values.size() - 1 == m_operators.size());
    auto it = m_values.begin();
    CalcValueType lType = (*it).type();
    float num = 1;
    LayoutUnit l = 1;
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

bool CalcTerm::equals(CalcTerm* with) const
{
    if (m_operators.size() != with->m_operators.size()) {
        return false;
    }
    if (m_values.size() != with->m_values.size()) {
        return false;
    }

    size_t len = m_operators.size();
    for (size_t i = 0; i < len; i++) {
        if (m_operators[i] != with->m_operators[i]) {
            return false;
        }
    }

    len = m_values.size();
    for (size_t i = 0; i < len; i++) {
        if (!m_values[i].equals(with->m_values[i])) {
            return false;
        }
    }

    return true;
}

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

bool CalcData::equals(CalcData* with) const
{
    if (m_terms.size() != with->m_terms.size()) {
        return false;
    }

    size_t len = m_terms.size();
    for (size_t i = 0; i < len; i++) {
        if (!m_terms[i]->equals(with->m_terms[i])) {
            return false;
        }
    }

    return true;
}
}
