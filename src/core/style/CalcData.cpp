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
#include <cmath>

namespace Starfish {
// A nested calc() operand is resolved by its own CalcData, like
// angleValue() and timeValue() do.
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
            s = s->substring(4, s->length() - 4);
        }
        // min()/max()/clamp() keep their function name and parens; a
        // subtracted one keeps its sign.
        if (m_data.m_calcData->isPositive()) {
            return s;
        }
        StringBuilder builder;
        builder.appendChar('-');
        builder.appendString(s);
        return builder.finalize();
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

// A product of numbers and at most one other value (css-values-4
// #calc-type-checking), kept in float so that no operand is rounded alone.
template <typename Leaf>
float CalcTerm::evaluate(Leaf& leaf) const
{
    STARFISH_ASSERT(m_values.size() - 1 == m_operators.size());
    float result = 1;
    for (size_t i = 0; i < m_values.size(); i++) {
        const CalcValue& value = m_values[i];
        float factor;
        if (value.type().isNumber()) {
            factor = value.numberValue();
        } else if (value.type().isCalcData()) {
            factor = value.calcDataValue()->evaluate(leaf);
        } else {
            factor = leaf(value);
        }
        if (i && m_operators[i - 1] == DIV) {
            result /= factor;
        } else {
            result *= factor;
        }
    }
    return result;
}

template <typename Leaf>
CalcTerm* CalcTerm::resolvedCopy(Leaf& leaf) const
{
    CalcTerm* copy = new CalcTerm();
    copy->m_operators = m_operators;
    for (const CalcValue& value : m_values) {
        if (value.type().isLength()) {
            copy->m_values.push_back(
                CalcValue(CSSLength(CSSLength::PX, leaf(value))));
        } else if (value.type().isCalcData()) {
            copy->m_values.push_back(
                CalcValue(value.calcDataValue()->resolvedCopy(leaf)));
        } else {
            copy->m_values.push_back(value);
        }
    }
    return copy;
}

String* CalcTerm::toString()
{
    STARFISH_ASSERT(m_values.size() - 1 == m_operators.size());
    if (m_values.size() == 1) {
        return m_values[0].toString();
    }

    StringBuilder builder;
    // CSS Values 4 puts numeric factors first. The first multiplication
    // can commute, but moving later factors across a partial product
    // changes its evaluation order and can overflow or underflow.
    bool swapFirstPair = m_operators[0] == MUL &&
                         !m_values[0].type().isNumber() &&
                         m_values[1].type().isNumber();
    builder.appendString(m_values[swapFirstPair ? 1 : 0].toString());
    for (size_t i = 1; i < m_values.size(); ++i) {
        if (m_operators[i - 1] == DIV) {
            builder.appendString(" / ");
        } else {
            builder.appendString(" * ");
        }
        size_t index = i == 1 && swapFirstPair ? 0 : i;
        builder.appendString(m_values[index].toString());
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

namespace {

    // Canonical scalar kinds for min()/max() comparison: arguments are
    // comparable at parse time only when they share one kind
    // (css-values-4 "Simplification").
    enum class ScalarKind {
        kNumber,
        kPx,
        kDeg,
        kS,
    };

    bool canonicalScalar(const CalcValue& value, ScalarKind& kind, float& out)
    {
        if (value.type().isNumber()) {
            kind = ScalarKind::kNumber;
            out = value.numberValue();
            return true;
        }
        if (value.type().isPercentage()) {
            // Percentages resolve against the property's base, so they are
            // not comparable at parse time.
            return false;
        }
        if (value.type().isLength()) {
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
            case CSSLength::PX:
                pixels = length.value();
                break;
            default:
                return false; // relative units are not comparable yet
            }
            kind = ScalarKind::kPx;
            out = pixels;
            return true;
        }
        if (value.type().isAngle()) {
            kind = ScalarKind::kDeg;
            out = value.angleValue().toDegreeValue();
            return true;
        }
        if (value.type().isTime()) {
            kind = ScalarKind::kS;
            out = value.timeValue().toTimeValue() / 1000.0f;
            return true;
        }
        return false;
    }

} // namespace

bool CalcData::foldComparableArguments()
{
    if (m_type != Type::kMin && m_type != Type::kMax) {
        return false;
    }

    // A single-argument min()/max() is equivalent to calc() of that
    // argument whatever its type is (css-values-4 #comp-func).
    if (m_argumentsStartPostion.size() == 1 && m_terms.size() >= 1) {
        m_type = Type::kCalc;
        return true;
    }

    // One canonical scalar per argument span; every term must be a bare
    // value of the same kind across all arguments.
    std::vector<float> argumentValues;
    ScalarKind kind = ScalarKind::kNumber;
    bool foldable = !m_argumentsStartPostion.empty();
    size_t spanStart = 0;
    for (size_t boundary = 1;
         foldable && boundary < m_argumentsStartPostion.size(); ++boundary) {
        float sum = 0;
        for (size_t i = spanStart; i < m_argumentsStartPostion[boundary]; ++i) {
            CalcTerm* term = m_terms[i];
            ScalarKind termKind = ScalarKind::kNumber;
            float value = 0;
            if (term->values().size() != 1 ||
                !canonicalScalar(*term->values().begin(), termKind, value)) {
                foldable = false;
                break;
            }
            if (argumentValues.empty() && i == spanStart) {
                kind = termKind;
            } else if (termKind != kind) {
                foldable = false;
                break;
            }
            sum += value;
        }
        argumentValues.push_back(sum);
        spanStart = m_argumentsStartPostion[boundary];
    }
    if (foldable) {
        float sum = 0;
        for (size_t i = spanStart; i < m_terms.size(); ++i) {
            CalcTerm* term = m_terms[i];
            ScalarKind termKind = ScalarKind::kNumber;
            float value = 0;
            if (term->values().size() != 1 ||
                !canonicalScalar(*term->values().begin(), termKind, value)) {
                foldable = false;
                break;
            }
            if (argumentValues.empty() && i == spanStart) {
                kind = termKind;
            } else if (termKind != kind) {
                foldable = false;
                break;
            }
            sum += value;
        }
        if (foldable) {
            argumentValues.push_back(sum);
        }
    }
    if (!foldable) {
        return false;
    }

    float result = argumentValues[0];
    for (auto it = argumentValues.begin() + 1; it != argumentValues.end();
         ++it) {
        result = m_type == Type::kMin ? std::min(result, *it)
                                      : std::max(result, *it);
    }
    if (!m_isPositive) {
        result = -result;
        m_isPositive = true;
    }

    CalcTerm* term = new CalcTerm();
    switch (kind) {
    case ScalarKind::kPx:
        term->appendValue(CalcValue(CSSLength(CSSLength::PX, result)));
        break;
    case ScalarKind::kDeg:
        term->appendValue(CalcValue(CSSAngle(CSSAngle::DEG, result)));
        break;
    case ScalarKind::kS:
        term->appendValue(CalcValue(CSSTime(CSSTime::S, result)));
        break;
    default:
        term->appendValue(CalcValue(result, false));
        break;
    }
    m_terms.clear();
    m_terms.push_back(term);
    m_type = Type::kCalc;
    return true;
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
            } else {
                // <number> and <percentage> cannot be combined
                // (css-values-4 §5.6)
                return CalcValueType::ValueKind::kInvalid;
            }
        }

        it++;
    }

    return lType;
}

// min()/max()/clamp() compare their comma-separated argument sums
// (css-values-4 #comp-func); a subtracted function is negated.
template <typename Leaf>
float CalcData::evaluate(Leaf& leaf) const
{
    size_t arguments =
        m_type == Type::kCalc ? 1 : m_argumentsStartPostion.size();
    auto argument = [&](size_t k) -> float {
        size_t end =
            k + 1 < arguments ? m_argumentsStartPostion[k + 1] : m_terms.size();
        float sum = 0;
        for (size_t i = k ? m_argumentsStartPostion[k] : 0; i < end; i++) {
            sum += m_terms[i]->evaluate(leaf);
        }
        return sum;
    };
    float n = argument(0);
    if (m_type == Type::kClamp) {
        STARFISH_ASSERT(arguments == 3);
        n = std::max(n, std::min(argument(1), argument(2)));
    } else {
        for (size_t k = 1; k < arguments; k++) {
            n = m_type == Type::kMin ? std::min(n, argument(k))
                                     : std::max(n, argument(k));
        }
    }
    return m_isPositive ? n : -n;
}

template <typename Leaf>
CalcData* CalcData::resolvedCopy(Leaf& leaf) const
{
    CalcData* copy = new CalcData();
    copy->m_type = m_type;
    copy->m_isPositive = m_isPositive;
    copy->m_isNonNegative = m_isNonNegative;
    copy->m_argumentsStartPostion = m_argumentsStartPostion;
    for (CalcTerm* term : m_terms) {
        copy->m_terms.push_back(term->resolvedCopy(leaf));
    }
    return copy;
}

// A top-level calculation that would produce NaN produces 0 instead, and
// an infinite one the largest representable value of its sign
// (css-values-4 #top-level-calculation).
static float censor(float value,
                    float largest = std::numeric_limits<float>::max())
{
    return std::isnan(value) ? 0 : std::max(-largest, std::min(value, largest));
}

// Lengths are representable as far as layout's LayoutUnit reaches; the
// float rounds up to 2^25, which LayoutUnit clamps on conversion.
static float censorLength(float value)
{
    return censor(value, static_cast<float>(intMaxForLayoutUnit));
}

float CalcData::numberValue() const
{
    auto leaf = [](const CalcValue& value) { return value.numberValue(); };
    return clampToRange(censor(evaluate(leaf)));
}

static Length leafLength(const CalcValue& value)
{
    if (value.type().isPercentage()) {
        return Length(Length::Percent, value.percentageValue());
    }
    return value.lengthValue().toLength();
}

LayoutUnit CalcData::specifiedValue(const LayoutUnit& parentContentLength,
                                    Node* n) const
{
    auto leaf = [&](const CalcValue& value) {
        return leafLength(value).specifiedValue(parentContentLength, n);
    };
    return LayoutUnit(clampToRange(censorLength(evaluate(leaf))));
}

LayoutUnit CalcData::specifiedFontValue(Node* n) const
{
    auto leaf = [&](const CalcValue& value) {
        return leafLength(value).specifiedFontValue(n);
    };
    return LayoutUnit(clampToRange(censorLength(evaluate(leaf))));
}

CSSAngle CalcData::angleValue() const
{
    auto leaf = [](const CalcValue& value) {
        return value.angleValue().toDegreeValue();
    };
    return CSSAngle(CSSAngle::DEG, clampToRange(censor(evaluate(leaf))));
}

CSSTime CalcData::timeValue() const
{
    auto leaf = [](const CalcValue& value) {
        return value.timeValue().toTimeValue();
    };
    return CSSTime(CSSTime::MS, clampToRange(censor(evaluate(leaf))));
}

namespace {

    // Resolves a leaf at computed-value time: font-relative and viewport
    // lengths become px, while percentages wait for layout unless their
    // basis is known.
    struct ComputedLeaf {
        ComputedLeaf(const Length& curFontSize, const Length& rootFontSize,
                     Optional<Font*> font, LayoutUnit viewportWidth,
                     LayoutUnit viewportHeight, Optional<ComputedStyle*> cs,
                     Optional<float> percentBasis)
            : curFontSize(curFontSize)
            , rootFontSize(rootFontSize)
            , font(font)
            , viewportWidth(viewportWidth)
            , viewportHeight(viewportHeight)
            , cs(cs)
            , percentBasis(percentBasis)
        {
        }

        const Length& curFontSize;
        const Length& rootFontSize;
        Optional<Font*> font;
        LayoutUnit viewportWidth;
        LayoutUnit viewportHeight;
        // Receives the viewport-unit flag that has a resize recompute it.
        Optional<ComputedStyle*> cs;
        Optional<float> percentBasis;
        bool hasPx = false;
        bool hasPercent = false;
        bool hasRelative = false;
        bool unresolved = false;

        float operator()(const CalcValue& value)
        {
            if (value.type().isPercentage()) {
                if (percentBasis) {
                    hasPx = true;
                    return value.percentageValue() * percentBasis.value();
                }
                hasPercent = true;
                return value.percentageValue();
            }
            Length length = value.lengthValue().toLength();
            if (length.isFontPercent() || length.isViewportPercent()) {
                hasRelative = true;
                length.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                             viewportWidth, viewportHeight, cs);
            }
            if (!length.isFixed()) {
                unresolved = true;
                return 0;
            }
            hasPx = true;
            return length.fixed();
        }
    };

} // namespace

Length CalcData::computedLength(const Length& curFontSize,
                                const Length& rootFontSize,
                                Optional<Font*> font, LayoutUnit viewportWidth,
                                LayoutUnit viewportHeight,
                                Optional<ComputedStyle*> cs, bool inherited,
                                Optional<float> percentBasis)
{
    ComputedLeaf leaf(curFontSize, rootFontSize, font, viewportWidth,
                      viewportHeight, cs, percentBasis);
    float value = clampToRange(censorLength(evaluate(leaf)));
    if (!leaf.unresolved && !leaf.hasPercent) {
        return Length(Length::Fixed, value);
    }
    // Percentages are not compared before layout, as their basis may be
    // negative (css-values-4 #calc-simplification): min()/max()/clamp() of
    // them stay.
    bool hasFunction = m_type != Type::kCalc;
    for (CalcTerm* term : m_terms) {
        for (const CalcValue& termValue : term->values()) {
            hasFunction |= termValue.type().isCalcData();
        }
    }
    if (!leaf.unresolved && !leaf.hasPx && !hasFunction) {
        return Length(Length::Percent, value);
    }
    if (inherited && leaf.hasRelative && !leaf.unresolved) {
        return Length(resolvedCopy(leaf));
    }
    // shortcut: for a property that is not inherited, a calc() that keeps
    // percentages also keeps its font-relative and viewport lengths, as they
    // resolve at used time against the same element; a child taking the
    // value with `inherit` resolves them against its own font instead, and
    // getComputedStyle() of an element without boxes shows them unresolved.
    // Copy as for inherited properties if either matters, at a CalcData per
    // element.
    return Length(this);
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
        // Products and nested functions are operator nodes. CSS Values 4
        // keeps them after plain values, in their original order.
        if (term->values().size() != 1 ||
            term->values()[0].type().isCalcData()) {
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

// CSS Values 4 simplifies sums of sums before combining same-unit terms.
static void appendCanonicalTerms(CalcTerm* term, GCVector<CalcTerm*>& terms)
{
    if (isSingleValueTerm(term) && term->values()[0].type().isCalcData()) {
        CalcData* nested = term->values()[0].calcDataValue();
        if (nested->type() == CalcData::Type::kCalc && nested->isPositive()) {
            GCVector<CalcTerm*> children;
            for (CalcTerm* child : nested->terms()) {
                appendCanonicalTerms(child, children);
            }
            // Reduce the inner sum before combining it with outer terms.
            // Flattening first can lose a small outer addend to rounding.
            mergeConsecutiveSameUnit(children, terms);
            return;
        }
    }
    terms.push_back(canonicalizeTerm(term));
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

    // A calc() summation is a single argument; min() and max() take a
    // variable number of arguments, and clamp() takes three.
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
                                                        : m_terms.size();
        // A folded min()/max() keeps its argument boundaries while its
        // terms reduce to one; spans cannot exceed the remaining terms.
        if (spanEnd > m_terms.size()) {
            spanEnd = m_terms.size();
        }
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
            appendCanonicalTerms(m_terms[termIndex], argument);
            ++termIndex;
        }
        std::stable_sort(
            argument.begin(), argument.end(),
            [](CalcTerm* a, CalcTerm* b) { return termRank(a) < termRank(b); });
        GCVector<CalcTerm*> mergedArgument;
        mergeConsecutiveSameUnit(argument, mergedArgument);

        auto serializeTerm = [&](CalcTerm* term) {
            String* serialized = term->toString();
            if (mergedArgument.size() == 1 || isSingleValueTerm(term)) {
                return serialized;
            }
            StringBuilder product;
            product.appendChar('(');
            product.appendString(serialized);
            product.appendChar(')');
            return product.finalize();
        };

        auto termIt = mergedArgument.begin();
        builder.appendString(serializeTerm(*termIt));
        ++termIt;
        while (termIt != mergedArgument.end()) {
            builder.appendString(String::spaceString);
            String* serialized = serializeTerm(*termIt);
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
    // min()/max()/clamp() of the same terms, or a subtracted one, is a
    // different value.
    if (m_type != with->m_type || m_isPositive != with->m_isPositive ||
        m_terms.size() != with->m_terms.size()) {
        return false;
    }
    if (m_type != Type::kCalc) {
        if (m_argumentsStartPostion.size() !=
            with->m_argumentsStartPostion.size()) {
            return false;
        }
        for (size_t i = 0; i < m_argumentsStartPostion.size(); i++) {
            if (m_argumentsStartPostion[i] !=
                with->m_argumentsStartPostion[i]) {
                return false;
            }
        }
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
