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
#include <algorithm>

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

// A nested calc() operand is resolved by its own CalcData, like
// specifiedValue() does for lengths.
float CalcValue::numberValue() const
{
    if (m_type.isCalcData()) {
        return m_data.m_calcData->numberValue();
    }
    STARFISH_ASSERT(m_type.isNumber());
    return m_data.m_numberData;
}

CSSAngle CalcValue::angleValue() const
{
    if (m_type.isCalcData()) {
        return m_data.m_calcData->angleValue();
    }
    STARFISH_ASSERT(m_type.isAngle());
    return m_data.m_angleData;
}

CSSTime CalcValue::timeValue() const
{
    if (m_type.isCalcData()) {
        return m_data.m_calcData->timeValue();
    }
    STARFISH_ASSERT(m_type.isTime());
    return m_data.m_timeData;
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
        if (m_data.m_calcData->type() == CalcData::Type::kCalc) {
            // A nested calc() term serializes parenthesized: "(...)".
            return s->substring(4, s->length() - 4);
        }
        // min()/max()/clamp() keep their function name and parens.
        return s;
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
        lType = it->calcDataValue()->calcValueType();
    }
    it++;
    auto it2 = m_operators.begin();
    while (it != m_values.end()) {
        CalcValueType rType = (*it).type();
        if (rType.isCalcData()) {
            rType = it->calcDataValue()->calcValueType();
        }
        bool operand = *it2;

        if (operand == MUL) {
            if (lType.isNumber()) {
                lType = rType;
            } else if (!rType.isNumber()) {
                return CalcValueType::ValueKind::kInvalid;
            }
        } else {
            if (rType.isNumber()) {
                if (((*it).type().isCalcData() &&
                     (*it).calcDataValue()->numberValue() == 0)) {
                    return CalcValueType::ValueKind::kInvalid;
                } else if ((*it).type().isNumber() &&
                           (*it).numberValue() == 0) {
                    return CalcValueType::ValueKind::kInvalid;
                }
            } else {
                return CalcValueType::ValueKind::kInvalid;
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
    LayoutUnit result = 1;

    if (lType.isNumber()) {
        result = (*it).numberValue();
    } else {
        result = (*it).specifiedValue(parentContentLength, n);
    }
    it++;
    auto it2 = m_operators.begin();
    while (it != m_values.end()) {
        CalcValueType rType = (*it).type();
        bool operand = *it2;

        if (operand == MUL) {
            if (rType.isNumber()) {
                result *= (*it).numberValue();
            } else {
                result *=
                    (*it).specifiedValue(parentContentLength, n).toFloat();
            }
        } else {
            if (rType.isNumber()) {
                result /= (*it).numberValue();
            } else {
                result /=
                    (*it).specifiedValue(parentContentLength, n).toFloat();
            }
        }

        it++;
        it2++;
    }

    return result;
}

LayoutUnit CalcTerm::specifiedFontValue(Node* n) const
{
    STARFISH_ASSERT(m_values.size() - 1 == m_operators.size());
    auto it = m_values.begin();
    CalcValueType lType = (*it).type();
    LayoutUnit result = 1;

    if (lType.isNumber()) {
        result = (*it).numberValue();
    } else {
        result = (*it).specifiedFontValue(n);
    }
    it++;
    auto it2 = m_operators.begin();
    while (it != m_values.end()) {
        CalcValueType rType = (*it).type();
        bool operand = *it2;

        if (operand == MUL) {
            if (rType.isNumber()) {
                result *= (*it).numberValue();
            } else {
                result *= (*it).specifiedFontValue(n).toFloat();
            }
        } else {
            if (rType.isNumber()) {
                result /= (*it).numberValue();
            } else {
                result /= (*it).specifiedFontValue(n).toFloat();
            }
        }

        it++;
        it2++;
    }

    return result;
}

CSSAngle CalcTerm::angleValue() const
{
    STARFISH_ASSERT(m_values.size() - 1 == m_operators.size());
    auto it = m_values.begin();
    CalcValueType lType = (*it).type();
    CSSAngle result;
    if (lType.isNumber()) {
        result = (*it).numberValue();
    } else {
        result = (*it).angleValue().toDegreeValue();
    }
    it++;
    auto it2 = m_operators.begin();
    while (it != m_values.end()) {
        CalcValueType rType = (*it).type();
        bool operand = *it2;

        if (operand == MUL) {
            if (rType.isNumber()) {
                result *= (*it).numberValue();
            } else {
                result *= (*it).angleValue().toDegreeValue();
            }
        } else {
            if (rType.isNumber()) {
                result /= (*it).numberValue();
            } else {
                result /= (*it).angleValue().toDegreeValue();
            }
        }

        it++;
        it2++;
    }
    return result;
}

CSSTime CalcTerm::timeValue() const
{
    STARFISH_ASSERT(m_values.size() - 1 == m_operators.size());
    auto it = m_values.begin();
    CalcValueType lType = (*it).type();
    CSSTime result;
    if (lType.isNumber()) {
        result = (*it).numberValue();
    } else {
        result = (*it).timeValue().toTimeValue();
    }
    it++;
    auto it2 = m_operators.begin();
    while (it != m_values.end()) {
        CalcValueType rType = (*it).type();
        bool operand = *it2;

        if (operand == MUL) {
            if (rType.isNumber()) {
                result *= (*it).numberValue();
            } else {
                result *= (*it).timeValue().toTimeValue();
            }
        } else {
            if (rType.isNumber()) {
                result /= (*it).numberValue();
            } else {
                result /= (*it).timeValue().toTimeValue();
            }
        }

        it++;
        it2++;
    }

    return result;
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

CalcData::CalcData()
    : m_type(Type::kCalc)
{
}

CalcData::CalcData(const std::string& type)
{
    if (type == "max") {
        m_type = Type::kMax;
    } else if (type == "min") {
        m_type = Type::kMin;
    } else if (type == "clamp") {
        m_type = Type::kClamp;
    } else {
        m_type = Type::kCalc;
    }
}

CalcValueType CalcData::calcValueType() const
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
            } else if (lType.isNumber() && rType.isPercentage()) {
                // A number merges into a percentage summation
                // (css-values-4 type checking: <number> + <percentage>
                // resolves to <percentage>)
                lType = rType;
            } else if (lType.isPercentage() && rType.isNumber()) {
            } else {
                return CalcValueType::ValueKind::kInvalid;
            }
        }

        it++;
    }

    return lType;
}

float CalcData::numberValue() const
{
    auto it = m_terms.begin();
    float n = (*it)->numberValue();
    it++;
    while (it != m_terms.end()) {
        n += (*it)->numberValue();
        it++;
    }
    if (!m_isPositive) {
        n *= -1;
    }
    return n;
}

LayoutUnit CalcData::specifiedValue(const LayoutUnit& parentContentLength,
                                    Node* n) const
{
    if (m_type == Type::kMax) {
        STARFISH_ASSERT(m_argumentsStartPostion.size() == 2);
        LayoutUnit frist, second;
        for (size_t i = 0; i < m_argumentsStartPostion[1]; i++) {
            frist += m_terms[i]->specifiedValue(parentContentLength, n);
        }
        for (size_t i = m_argumentsStartPostion[1]; i < m_terms.size(); i++) {
            second += m_terms[i]->specifiedValue(parentContentLength, n);
        }
        return std::max(frist.toInt(), second.toInt());
    } else if (m_type == Type::kMin) {
        STARFISH_ASSERT(m_argumentsStartPostion.size() == 2);
        LayoutUnit frist, second;
        for (size_t i = 0; i < m_argumentsStartPostion[1]; i++) {
            frist += m_terms[i]->specifiedValue(parentContentLength, n);
        }
        for (size_t i = m_argumentsStartPostion[1]; i < m_terms.size(); i++) {
            second += m_terms[i]->specifiedValue(parentContentLength, n);
        }
        return std::min(frist.toInt(), second.toInt());
    } else if (m_type == Type::kClamp) {
        STARFISH_ASSERT(m_argumentsStartPostion.size() == 3);
        LayoutUnit frist, second, third;
        for (size_t i = 0; i < m_argumentsStartPostion[1]; i++) {
            frist += m_terms[i]->specifiedValue(parentContentLength, n);
        }
        for (size_t i = m_argumentsStartPostion[1];
             i < m_argumentsStartPostion[2]; i++) {
            second += m_terms[i]->specifiedValue(parentContentLength, n);
        }
        for (size_t i = m_argumentsStartPostion[2]; i < m_terms.size(); i++) {
            third += m_terms[i]->specifiedValue(parentContentLength, n);
        }

        return std::max(frist.toInt(), std::min(second.toInt(), third.toInt()));
    } else {
        STARFISH_ASSERT(m_type == Type::kCalc);
        auto it = m_terms.begin();
        LayoutUnit l = (*it)->specifiedValue(parentContentLength, n);
        it++;
        while (it != m_terms.end()) {
            l += (*it)->specifiedValue(parentContentLength, n);
            it++;
        }
        if (!m_isPositive) {
            l *= -1;
        }
        return l;
    }
}

LayoutUnit CalcData::specifiedFontValue(Node* n) const
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

CSSAngle CalcData::angleValue() const
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

CSSTime CalcData::timeValue() const
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

namespace {

    // Canonical serialization order of a calc() summation: numbers, then
    // percentages, then dimensions sorted by unit name (css-values-4,
    // "Serialization"). Each term keeps its own sign; only the display order
    // of the additive terms changes.
    int lengthUnitRank(const CSSLength& length)
    {
        switch (length.kind()) {
        case CSSLength::CH:
            return 0;
        case CSSLength::CM:
            return 1;
        case CSSLength::EM:
            return 2;
        case CSSLength::EX:
            return 3;
        case CSSLength::INCH:
            return 4;
        case CSSLength::MM:
            return 5;
        case CSSLength::PC:
            return 6;
        case CSSLength::PT:
            return 7;
        case CSSLength::PX:
            return 8;
        case CSSLength::REM:
            return 9;
        case CSSLength::VH:
            return 10;
        case CSSLength::VMAX:
            return 11;
        case CSSLength::VMIN:
            return 12;
        case CSSLength::VW:
            return 13;
        default:
            return 14;
        }
    }

    int termRank(CalcTerm* term)
    {
        // Nested min()/max()/clamp() terms keep their written position: they
        // are never merged or reordered.
        if (term->values().begin()->type().isCalcData()) {
            return 300;
        }
        CalcValueType type = term->type();
        if (type.isNumber()) {
            return 0;
        }
        if (type.isPercentage()) {
            return 100;
        }
        if (!type.isLength() && !type.isAngle() && !type.isTime()) {
            // Nested calc() or unresolved type: keep at the end, stable.
            return 300;
        }

        // The first non-number value carries the term's unit.
        auto it = term->values().begin();
        while (it != term->values().end() && (*it).type().isNumber()) {
            ++it;
        }
        if (it == term->values().end() || (*it).type().isCalcData()) {
            return 300;
        }

        if (type.isLength()) {
            return 200 + lengthUnitRank((*it).lengthValue());
        }
        if (type.isAngle()) {
            switch ((*it).angleValue().kind()) {
            case CSSAngle::DEG:
                return 200;
            case CSSAngle::GRAD:
                return 201;
            case CSSAngle::RAD:
                return 202;
            default:
                return 203; // TURN
            }
        }
        return 200 + ((*it).timeValue().kind() == CSSTime::S ? 1 : 0); // ms < s
    }

} // namespace

// Only bare values merge; product chains and nested calc() keep their place.
bool isSingleValueTerm(CalcTerm* term)
{
    return term->values().size() == 1;
}

bool canMergeTerms(CalcTerm* a, CalcTerm* b)
{
    if (!isSingleValueTerm(a) || !isSingleValueTerm(b)) {
        return false;
    }
    CalcValueType type = a->values().begin()->type();
    if (!(type == b->values().begin()->type()) || type.isCalcData() ||
        type.isNone() || type.isInvalid()) {
        return false;
    }
    if (type.isNumber() || type.isPercentage()) {
        return true;
    }
    if (type.isLength()) {
        return a->values().begin()->lengthValue().kind() ==
               b->values().begin()->lengthValue().kind();
    }
    // time merges into seconds, angles into degrees
    return type.isTime() || type.isAngle();
}

CalcTerm* mergeTermGroup(GCVector<CalcTerm*>& group)
{
    auto it = group.begin();
    CalcValue sum = *(*it)->values().begin();
    CalcValueType type = sum.type();
    ++it;
    while (it != group.end()) {
        CalcValue value = *(*it)->values().begin();
        if (type.isNumber()) {
            sum = CalcValue(sum.numberValue() + value.numberValue(), false);
        } else if (type.isPercentage()) {
            sum = CalcValue(sum.percentageValue() + value.percentageValue(),
                            true);
        } else if (type.isLength()) {
            sum = CalcValue(CSSLength(sum.lengthValue().kind(),
                                      sum.lengthValue().value() +
                                          value.lengthValue().value()));
        } else if (type.isTime()) {
            // the canonical time unit is seconds; toTimeValue() is ms
            sum = CalcValue(
                CSSTime(CSSTime::S, (sum.timeValue().toTimeValue() +
                                     value.timeValue().toTimeValue()) /
                                        1000.0));
        } else if (type.isAngle()) {
            sum = CalcValue(CSSAngle(CSSAngle::DEG,
                                     sum.angleValue().toDegreeValue() +
                                         value.angleValue().toDegreeValue()));
        }
        ++it;
    }
    CalcTerm* term = new CalcTerm();
    term->appendValue(sum);
    return term;
}

// Absolute length units canonicalize to px inside math functions
// (css-values-4: the canonical length unit is the px).
CalcTerm* canonicalizeTerm(CalcTerm* term)
{
    if (!isSingleValueTerm(term)) {
        return term;
    }
    CalcValue& value = *term->values().begin();
    if (!value.type().isLength()) {
        return term;
    }
    CSSLength length = value.lengthValue();
    float pixels;
    switch (length.kind()) {
    case CSSLength::INCH:
        pixels = length.value() * 96;
        break;
    case CSSLength::CM:
        pixels = length.value() * 96 / 2.54f;
        break;
    case CSSLength::MM:
        pixels = length.value() * 96 / 25.4f;
        break;
    case CSSLength::PT:
        pixels = length.value() * 96 / 72.f;
        break;
    case CSSLength::PC:
        pixels = length.value() * 16;
        break;
    default:
        return term;
    }
    CalcTerm* canonical = new CalcTerm();
    canonical->appendValue(CalcValue(CSSLength(CSSLength::PX, pixels)));
    return canonical;
}

void mergeConsecutiveSameUnit(GCVector<CalcTerm*>& terms,
                              GCVector<CalcTerm*>& merged)
{
    auto it = terms.begin();
    while (it != terms.end()) {
        CalcTerm* first = *it;
        auto run = it;
        size_t runSize = 1;
        ++run;
        while (run != terms.end() && canMergeTerms(first, *run)) {
            ++runSize;
            ++run;
        }
        if (runSize == 1) {
            merged.push_back(first);
        } else {
            GCVector<CalcTerm*> group;
            auto git = it;
            for (size_t k = 0; k < runSize; ++k) {
                group.push_back(*git);
                ++git;
            }
            merged.push_back(mergeTermGroup(group));
        }
        it = run;
    }
}

String* CalcData::toString()
{
    StringBuilder builder;

    if (m_type == Type::kMin) {
        builder.appendString(String::createASCIIString("min("));
    } else if (m_type == Type::kMax) {
        builder.appendString(String::createASCIIString("max("));
    } else if (m_type == Type::kClamp) {
        builder.appendString(String::createASCIIString("clamp("));
    } else {
        builder.appendString(String::createASCIIString("calc("));
    }

    // Canonicalize each term: absolute lengths resolve to px so that all
    // absolute units share the px group.
    GCVector<CalcTerm*> canonical;
    for (auto termIt = m_terms.begin(); termIt != m_terms.end(); ++termIt) {
        canonical.push_back(canonicalizeTerm(*termIt));
    }

    // Argument start indices in term order: a calc() summation is a single
    // argument, min()/max() take two and clamp() three.
    std::vector<size_t> starts;
    for (auto boundary = m_argumentsStartPostion.begin();
         boundary != m_argumentsStartPostion.end(); ++boundary) {
        starts.push_back(*boundary);
    }
    if (starts.empty()) {
        starts.push_back(0);
    }

    size_t termIndex = 0;
    for (size_t argIndex = 0; argIndex < starts.size(); ++argIndex) {
        size_t spanEnd = (argIndex + 1 < starts.size()) ? starts[argIndex + 1]
                                                        : canonical.size();
        if (spanEnd <= termIndex) {
            continue;
        }
        if (termIndex > 0) {
            builder.appendString(String::createASCIIString(", "));
        }

        // Each argument serializes as a canonical summation: sorted by
        // rank with same-unit bare terms summed (css-values-4,
        // "Serialization"). min()/max()/clamp() arguments keep their
        // written order.
        GCVector<CalcTerm*> argument;
        while (termIndex < spanEnd) {
            argument.push_back(canonical[termIndex]);
            ++termIndex;
        }
        std::stable_sort(
            argument.begin(), argument.end(),
            [](CalcTerm* a, CalcTerm* b) { return termRank(a) < termRank(b); });
        GCVector<CalcTerm*> mergedArgument;
        mergeConsecutiveSameUnit(argument, mergedArgument);

        auto termIt = mergedArgument.begin();
        builder.appendString((*termIt)->toString());
        ++termIt;
        while (termIt != mergedArgument.end()) {
            builder.appendString(String::spaceString);
            String* serialized = (*termIt)->toString();
            if (serialized->charAt(0) == '-') {
                builder.appendString(String::createASCIIString("- "));
                builder.appendString(
                    serialized->substring(1, serialized->length() - 1));
            } else if (serialized->charAt(0) == '+') {
                builder.appendString(String::createASCIIString("+ "));
                builder.appendString(
                    serialized->substring(1, serialized->length() - 1));
            } else {
                builder.appendString(String::createASCIIString("+ "));
                builder.appendString(serialized);
            }
            ++termIt;
        }
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

size_t CalcData::requiredArguemntsCount()
{
    size_t requiredArgc = 1;
    switch (m_type) {
    case Type::kMin:
    case Type::kMax:
        requiredArgc = 2;
        break;
    case Type::kClamp:
        requiredArgc = 3;
        break;
    default:
        requiredArgc = 1;
        break;
    }
    return requiredArgc;
}

} // namespace Starfish
