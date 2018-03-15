/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "core/modules/threading/Thread.h"
#include "core/style/CounterStyle.h"

namespace StarFish {

bool CounterStyle::exceedBound(int32_t pos) const
{
    if (!m_ranges.size()) {
        return getDefaultRange().exceed(pos);
    }
    size_t size = m_ranges.size();
    for (size_t i = 0; i < size; i++) {
        if (!m_ranges[i].exceed(pos)) {
            return false;
        }
    }
    return true;
}

String* CounterStyle::getFallbackSymbol(
    int32_t pos, GCVector<const CounterStyle*>& failedCounters) const
{
    STARFISH_ASSERT(fallback());
    // NOTE If a loop in the specified fallbacks is detected,
    // the decimal style must be used instead.
    size_t size = failedCounters.size();
    for (size_t i = 0; i < size; i++) {
        if (failedCounters[i] == fallback()) {
            return CounterStyle::getDecimalCounter()->getSymbolAt(pos);
        }
    }
    failedCounters.push_back(this);
    return fallback()->getSymbolAt(pos, failedCounters);
}

Nullable<String*> CounterStyle::getCyclicSymbol(int32_t pos) const
{
    if (exceedBound(pos)) {
        return Nullable<String*>();
    }
    size_t size = m_symbols.size();
    if (size == 1) {
        return m_symbols[0];
    }
    int32_t index = (pos - 1) % size;
    if (index < 0) {
        return m_symbols[size + index];
    }
    return m_symbols[index];
}

Nullable<String*> CounterStyle::getFixedSymbol(int32_t pos) const
{
    int32_t start = firstSymbolValue();
    // NOTE This is safe becase symbols.size() can not exceed kMaxRangeValue
    int32_t size = (int32_t)m_symbols.size();
    if (pos < start || pos >= start + size || exceedBound(pos)) {
        return Nullable<String*>();
    }
    return m_symbols[pos - start];
}

Nullable<String*> CounterStyle::getSymbolicSymbol(int32_t pos) const
{
    if (exceedBound(pos) || pos == 0) {
        // NOTE In symbolic system, index 0 does not match any symbol
        return Nullable<String*>();
    }
    size_t size = m_symbols.size();
    STARFISH_ASSERT(pos != 0);
    size_t positivePos = pos < 0 ? -pos - 1 : pos - 1;
    size_t repeat = positivePos / size;
    String* symbol = m_symbols[positivePos % size];
    while (repeat--) {
        symbol = symbol->concat(symbol);
    }
    return symbol;
}

Nullable<String*> CounterStyle::getAlphabeticSymbol(int32_t pos) const
{
    if (exceedBound(pos) || pos == 0) {
        // NOTE In alphabetic system, index 0 does not match any symbol
        return Nullable<String*>();
    }
    size_t size = m_symbols.size();
    STARFISH_ASSERT(pos != 0);
    size_t positivePos = pos < 0 ? -pos - 1 : pos - 1;
    String* symbol = m_symbols[positivePos % size];
    positivePos /= size;
    while (positivePos > 0) {
        positivePos -= 1;
        symbol = m_symbols[positivePos % size]->concat(symbol);
        positivePos /= size;
    }
    return symbol;
}

Nullable<String*> CounterStyle::getNumericSymbol(int32_t pos) const
{
    if (exceedBound(pos)) {
        return Nullable<String*>();
    }
    size_t size = m_symbols.size();
    size_t positivePos = pos < 0 ? -pos : pos;
    String* symbol = m_symbols[positivePos % size];
    positivePos /= size;
    while (positivePos > 0) {
        symbol = m_symbols[positivePos % size]->concat(symbol);
        positivePos /= size;
    }
    return symbol;
}

Nullable<String*> CounterStyle::getAdditiveSymbol(int32_t pos) const
{
    if (exceedBound(pos)) {
        return Nullable<String*>();
    }
    size_t size = m_additiveSymbols.size();
    size_t positivePos = pos < 0 ? -pos : pos;
    if (positivePos == 0) {
        if (m_additiveSymbols.back().m_weight == 0) {
            return m_additiveSymbols.back().m_symbol;
        }
        return Nullable<String*>();
    }
    String* result = String::emptyString;
    for (size_t i = 0; i < size && positivePos != 0; i++) {
        const CounterStyle::AdditiveTuple& tuple = m_additiveSymbols[i];
        if (tuple.m_weight != 0 && positivePos >= tuple.m_weight) {
            size_t repeat = positivePos / tuple.m_weight;
            positivePos -= (repeat * tuple.m_weight);
            String* peice = String::emptyString;
            while (repeat--) {
                peice = peice->concat(tuple.m_symbol);
            }
            result = result->concat(peice);
        }
    }
    if (positivePos > 0) {
        return Nullable<String*>();
    }
    return result;
}

String* CounterStyle::getSymbolAt(int32_t pos) const
{
    GCVector<const CounterStyle*> failedCounters;
    return getSymbolAt(pos, failedCounters);
}

String* CounterStyle::getSymbolAt(
    int32_t pos, GCVector<const CounterStyle*>& failedCounters) const
{
    STARFISH_ASSERT(m_symbols.size() || m_additiveSymbols.size());
    Nullable<String*> result;
    switch (m_system) {
    case CounterStyle::NoneSystem:
        return String::emptyString;
    case CounterStyle::CyclicSystem:
        result = getCyclicSymbol(pos);
        break;
    case CounterStyle::NumericSystem:
        result = getNumericSymbol(pos);
        break;
    case CounterStyle::FixedSystem:
        result = getFixedSymbol(pos);
        break;
    case CounterStyle::AlphabeticSystem:
        result = getAlphabeticSymbol(pos);
        break;
    case CounterStyle::SymbolicSystem:
        result = getSymbolicSymbol(pos);
        break;
    case CounterStyle::AdditiveSystem:
        result = getAdditiveSymbol(pos);
        break;
    case CounterStyle::ExtendsSystem:
        break;
    }
    if (!result.hasValue()) {
        return getFallbackSymbol(pos, failedCounters);
    }
    return result.getValue();
}

String* CounterStyle::generateLabel(int32_t pos) const
{
    String* symbol = getSymbolAt(pos);

    // label = prefix + negativePrefix + padded symbol + negativeSuffix + suffix
    StringBuilder stringBuilder;
    stringBuilder.appendString(prefix());
    bool needNegativeSign = pos < 0 && !ignoreNegativeSign();
    if (needNegativeSign) {
        stringBuilder.appendString(negativePrefix());
    }
    if (padSymbol()->length()) {
        int diff = (int)padWidth() - (int)symbol->length();
        for (; diff > 0; diff--) {
            stringBuilder.appendString(padSymbol());
        }
    }
    stringBuilder.appendString(symbol);
    if (needNegativeSign) {
        stringBuilder.appendString(negativeSuffix());
    }
    stringBuilder.appendString(suffix());
    return stringBuilder.finalize();
}

String* CounterStyle::getDefaultNegative()
{
    STARFISH_ASSERT(isMainThread());
    static String* negative = String::emptyString;
    if (!negative->length()) {
        negative = String::createASCIIStringWithNoGC("-");
    }
    return negative;
}

String* CounterStyle::getDefaultSuffix()
{
    STARFISH_ASSERT(isMainThread());
    static String* suffix = String::emptyString;
    if (!suffix->length()) {
        suffix = String::createASCIIStringWithNoGC(". ");
    }
    return suffix;
}

const CounterStyle* CounterStyle::getKnownCounter(String* name)
{
    if (name->equalsIgnoreCase("disc")) {
        return getDiscCounter();
    }
    if (name->equalsIgnoreCase("decimal")) {
        return getDecimalCounter();
    }
    if (name->equalsIgnoreCase("circle")) {
        return getCircleCounter();
    }
    if (name->equalsIgnoreCase("square")) {
        return getSquareCounter();
    }
    if (name->equalsIgnoreCase("decimal-leading-zero")) {
        return getDecimalLeadingZeroCounter();
    }
    if (name->equalsIgnoreCase("lower-roman")) {
        return getLowerRomanCounter();
    }
    if (name->equalsIgnoreCase("upper-roman")) {
        return getUpperRomanCounter();
    }
    if (name->equalsIgnoreCase("lower-greek")) {
        return getLowerGreekCounter();
    }
    if (name->equalsIgnoreCase("lower-latin")) {
        return getLowerLatinCounter();
    }
    if (name->equalsIgnoreCase("upper-latin")) {
        return getUpperLatinCounter();
    }
    if (name->equalsIgnoreCase("armenian")) {
        return getArmenianCounter();
    }
    if (name->equalsIgnoreCase("georgian")) {
        return getGeorgianCounter();
    }
    if (name->equalsIgnoreCase("lower-alpha")) {
        return getLowerAlphaCounter();
    }
    if (name->equalsIgnoreCase("upper-alpha")) {
        return getUpperAlphaCounter();
    }
    return nullptr;
}

const CounterStyle* CounterStyle::getNoneCounter()
{
    STARFISH_ASSERT(isMainThread());
    static CounterStyle* counter;
    if (counter == nullptr) {
        counter = new (NoGC)
            CounterStyle(String::emptyString, CounterStyle::NoneSystem);
    }
    return counter;
}

const CounterStyle* CounterStyle::getDiscCounter()
{
    STARFISH_ASSERT(isMainThread());
    static CounterStyle* counter;
    if (counter == nullptr) {
        counter = new (NoGC)
            CounterStyle(String::createASCIIString("disc"), CyclicSystem);
        counter->setSuffix(String::spaceString);
        counter->addSymbol(String::createUTF32String(0x2022));
    }
    return counter;
}

const CounterStyle* CounterStyle::getDecimalCounter()
{
    STARFISH_ASSERT(isMainThread());
    static CounterStyle* counter;
    static const int kDecimalN = 10;
    static const char kDecimalS[kDecimalN] = { '0', '1', '2', '3', '4',
                                               '5', '6', '7', '8', '9' };
    if (counter == nullptr) {
        counter = new (NoGC)
            CounterStyle(String::createASCIIString("decimal"), NumericSystem);
        GCVector<String*>& symbols = counter->symbols();
        for (size_t i = 0; i < kDecimalN; i++) {
            symbols.emplace_back(String::createASCIIString(kDecimalS[i]));
        }
    }
    return counter;
}

const CounterStyle* CounterStyle::getCircleCounter()
{
    STARFISH_ASSERT(isMainThread());
    static CounterStyle* counter;
    if (counter == nullptr) {
        counter = new (NoGC)
            CounterStyle(String::createASCIIString("circle"), CyclicSystem);
        counter->setSuffix(String::spaceString);
        counter->symbols().emplace_back(String::createUTF32String(0x25E6));
    }
    return counter;
}

const CounterStyle* CounterStyle::getSquareCounter()
{
    STARFISH_ASSERT(isMainThread());
    static CounterStyle* counter;
    if (counter == nullptr) {
        counter = new (NoGC)
            CounterStyle(String::createASCIIString("square"), CyclicSystem);
        counter->setSuffix(String::spaceString);
        counter->symbols().emplace_back(String::createUTF32String(0x25FE));
    }
    return counter;
}

const CounterStyle* CounterStyle::getDecimalLeadingZeroCounter()
{
    STARFISH_ASSERT(isMainThread());
    static CounterStyle* counter;
    if (counter == nullptr) {
        counter = new (NoGC)
            CounterStyle(String::createASCIIString("decimal-leading-zero"),
                         getDecimalCounter());
        counter->setPadWidth(2);
        counter->setPadSymbol(String::createASCIIString('0'));
    }
    return counter;
}

const CounterStyle* CounterStyle::getLowerRomanCounter()
{
    STARFISH_ASSERT(isMainThread());
    static CounterStyle* counter;
    if (counter == nullptr) {
        counter = new (NoGC) CounterStyle(
            String::createASCIIString("lower-roman"), AdditiveSystem);
        GCVector<AdditiveTuple>& symbols = counter->additiveSymbols();
        symbols.emplace_back(1000, String::createASCIIString('m'));
        symbols.emplace_back(900, String::createASCIIString("cm"));
        symbols.emplace_back(500, String::createASCIIString('d'));
        symbols.emplace_back(400, String::createASCIIString("cd"));
        symbols.emplace_back(100, String::createASCIIString('c'));
        symbols.emplace_back(90, String::createASCIIString("xc"));
        symbols.emplace_back(50, String::createASCIIString('l'));
        symbols.emplace_back(40, String::createASCIIString("xl"));
        symbols.emplace_back(10, String::createASCIIString('x'));
        symbols.emplace_back(9, String::createASCIIString("ix"));
        symbols.emplace_back(5, String::createASCIIString('v'));
        symbols.emplace_back(4, String::createASCIIString("iv"));
        symbols.emplace_back(1, String::createASCIIString('i'));
        counter->addRange(1, 3999);
    }
    return counter;
}

const CounterStyle* CounterStyle::getUpperRomanCounter()
{
    STARFISH_ASSERT(isMainThread());
    static CounterStyle* counter;
    if (counter == nullptr) {
        counter = new (NoGC) CounterStyle(
            String::createASCIIString("upper-roman"), AdditiveSystem);
        GCVector<AdditiveTuple>& symbols = counter->additiveSymbols();
        symbols.emplace_back(1000, String::createASCIIString('M'));
        symbols.emplace_back(900, String::createASCIIString("CM"));
        symbols.emplace_back(500, String::createASCIIString('D'));
        symbols.emplace_back(400, String::createASCIIString("CD"));
        symbols.emplace_back(100, String::createASCIIString('C'));
        symbols.emplace_back(90, String::createASCIIString("XC"));
        symbols.emplace_back(50, String::createASCIIString('L'));
        symbols.emplace_back(40, String::createASCIIString("XL"));
        symbols.emplace_back(10, String::createASCIIString('X'));
        symbols.emplace_back(9, String::createASCIIString("IX"));
        symbols.emplace_back(5, String::createASCIIString('V'));
        symbols.emplace_back(4, String::createASCIIString("IV"));
        symbols.emplace_back(1, String::createASCIIString('I'));
        counter->addRange(1, 3999);
    }
    return counter;
}

const CounterStyle* CounterStyle::getLowerGreekCounter()
{
    STARFISH_ASSERT(isMainThread());
    static CounterStyle* counter;
    if (counter == nullptr) {
        counter = new (NoGC) CounterStyle(
            String::createASCIIString("lower-greek"), AlphabeticSystem);
        GCVector<String*>& symbols = counter->symbols();
        symbols.push_back(String::createUTF32String(0x3B1));
        symbols.push_back(String::createUTF32String(0x3B2));
        symbols.push_back(String::createUTF32String(0x3B3));
        symbols.push_back(String::createUTF32String(0x3B4));
        symbols.push_back(String::createUTF32String(0x3B5));
        symbols.push_back(String::createUTF32String(0x3B6));
        symbols.push_back(String::createUTF32String(0x3B7));
        symbols.push_back(String::createUTF32String(0x3B8));
        symbols.push_back(String::createUTF32String(0x3B9));
        symbols.push_back(String::createUTF32String(0x3BA));
        symbols.push_back(String::createUTF32String(0x3BB));
        symbols.push_back(String::createUTF32String(0x3BC));
        symbols.push_back(String::createUTF32String(0x3BD));
        symbols.push_back(String::createUTF32String(0x3BE));
        symbols.push_back(String::createUTF32String(0x3BF));
        symbols.push_back(String::createUTF32String(0x3C0));
        symbols.push_back(String::createUTF32String(0x3C1));
        symbols.push_back(String::createUTF32String(0x3C3));
        symbols.push_back(String::createUTF32String(0x3C4));
        symbols.push_back(String::createUTF32String(0x3C5));
        symbols.push_back(String::createUTF32String(0x3C6));
        symbols.push_back(String::createUTF32String(0x3C7));
        symbols.push_back(String::createUTF32String(0x3C8));
        symbols.push_back(String::createUTF32String(0x3C9));
    }
    return counter;
}

const CounterStyle* CounterStyle::getLowerLatinCounter()
{
    STARFISH_ASSERT(isMainThread());
    static CounterStyle* counter;
    if (counter == nullptr) {
        counter = new (NoGC) CounterStyle(
            String::createASCIIString("lower-latin"), getLowerAlphaCounter());
    }
    return counter;
}

const CounterStyle* CounterStyle::getUpperLatinCounter()
{
    STARFISH_ASSERT(isMainThread());
    static CounterStyle* counter;
    if (counter == nullptr) {
        counter = new (NoGC) CounterStyle(
            String::createASCIIString("upper-latin"), getUpperAlphaCounter());
    }
    return counter;
}

const CounterStyle* CounterStyle::getArmenianCounter()
{
    STARFISH_ASSERT(isMainThread());
    static CounterStyle* counter;
    if (counter == nullptr) {
        counter = new (NoGC)
            CounterStyle(String::createASCIIString("armenian"), AdditiveSystem);
        GCVector<AdditiveTuple>& symbols = counter->additiveSymbols();
        symbols.emplace_back(9000, String::createUTF32String(0x554));
        symbols.emplace_back(8000, String::createUTF32String(0x553));
        symbols.emplace_back(7000, String::createUTF32String(0x552));
        symbols.emplace_back(6000, String::createUTF32String(0x551));
        symbols.emplace_back(5000, String::createUTF32String(0x550));
        symbols.emplace_back(4000, String::createUTF32String(0x54F));
        symbols.emplace_back(3000, String::createUTF32String(0x54E));
        symbols.emplace_back(2000, String::createUTF32String(0x54D));
        symbols.emplace_back(1000, String::createUTF32String(0x54C));
        symbols.emplace_back(900, String::createUTF32String(0x54B));
        symbols.emplace_back(800, String::createUTF32String(0x54A));
        symbols.emplace_back(700, String::createUTF32String(0x549));
        symbols.emplace_back(600, String::createUTF32String(0x548));
        symbols.emplace_back(500, String::createUTF32String(0x547));
        symbols.emplace_back(400, String::createUTF32String(0x546));
        symbols.emplace_back(300, String::createUTF32String(0x545));
        symbols.emplace_back(200, String::createUTF32String(0x544));
        symbols.emplace_back(100, String::createUTF32String(0x543));
        symbols.emplace_back(90, String::createUTF32String(0x542));
        symbols.emplace_back(80, String::createUTF32String(0x541));
        symbols.emplace_back(70, String::createUTF32String(0x540));
        symbols.emplace_back(60, String::createUTF32String(0x53F));
        symbols.emplace_back(50, String::createUTF32String(0x53E));
        symbols.emplace_back(40, String::createUTF32String(0x53D));
        symbols.emplace_back(30, String::createUTF32String(0x53C));
        symbols.emplace_back(20, String::createUTF32String(0x53B));
        symbols.emplace_back(10, String::createUTF32String(0x53A));
        symbols.emplace_back(9, String::createUTF32String(0x539));
        symbols.emplace_back(8, String::createUTF32String(0x538));
        symbols.emplace_back(7, String::createUTF32String(0x537));
        symbols.emplace_back(6, String::createUTF32String(0x536));
        symbols.emplace_back(5, String::createUTF32String(0x535));
        symbols.emplace_back(4, String::createUTF32String(0x534));
        symbols.emplace_back(3, String::createUTF32String(0x533));
        symbols.emplace_back(2, String::createUTF32String(0x532));
        symbols.emplace_back(1, String::createUTF32String(0x531));
        counter->addRange(1, 9999);
    }
    return counter;
}

const CounterStyle* CounterStyle::getGeorgianCounter()
{
    STARFISH_ASSERT(isMainThread());
    static CounterStyle* counter;
    if (counter == nullptr) {
        counter = new (NoGC)
            CounterStyle(String::createASCIIString("georgian"), AdditiveSystem);
        GCVector<AdditiveTuple>& symbols = counter->additiveSymbols();
        symbols.emplace_back(10000, String::createUTF32String(0x10F5));
        symbols.emplace_back(9000, String::createUTF32String(0x10F0));
        symbols.emplace_back(8000, String::createUTF32String(0x10EF));
        symbols.emplace_back(7000, String::createUTF32String(0x10F4));
        symbols.emplace_back(6000, String::createUTF32String(0x10EE));
        symbols.emplace_back(5000, String::createUTF32String(0x10ED));
        symbols.emplace_back(4000, String::createUTF32String(0x10EC));
        symbols.emplace_back(3000, String::createUTF32String(0x10EB));
        symbols.emplace_back(2000, String::createUTF32String(0x10EA));
        symbols.emplace_back(1000, String::createUTF32String(0x10E9));
        symbols.emplace_back(900, String::createUTF32String(0x10E8));
        symbols.emplace_back(800, String::createUTF32String(0x10E7));
        symbols.emplace_back(700, String::createUTF32String(0x10E6));
        symbols.emplace_back(600, String::createUTF32String(0x10E5));
        symbols.emplace_back(500, String::createUTF32String(0x10E4));
        symbols.emplace_back(400, String::createUTF32String(0x10F3));
        symbols.emplace_back(300, String::createUTF32String(0x10E2));
        symbols.emplace_back(200, String::createUTF32String(0x10E1));
        symbols.emplace_back(100, String::createUTF32String(0x10E0));
        symbols.emplace_back(90, String::createUTF32String(0x10DF));
        symbols.emplace_back(80, String::createUTF32String(0x10DE));
        symbols.emplace_back(70, String::createUTF32String(0x10DD));
        symbols.emplace_back(60, String::createUTF32String(0x10F2));
        symbols.emplace_back(50, String::createUTF32String(0x10DC));
        symbols.emplace_back(40, String::createUTF32String(0x10DB));
        symbols.emplace_back(30, String::createUTF32String(0x10DA));
        symbols.emplace_back(20, String::createUTF32String(0x10D9));
        symbols.emplace_back(10, String::createUTF32String(0x10D8));
        symbols.emplace_back(9, String::createUTF32String(0x10D7));
        symbols.emplace_back(8, String::createUTF32String(0x10F1));
        symbols.emplace_back(7, String::createUTF32String(0x10D6));
        symbols.emplace_back(6, String::createUTF32String(0x10D5));
        symbols.emplace_back(5, String::createUTF32String(0x10D4));
        symbols.emplace_back(4, String::createUTF32String(0x10D3));
        symbols.emplace_back(3, String::createUTF32String(0x10D2));
        symbols.emplace_back(2, String::createUTF32String(0x10D1));
        symbols.emplace_back(1, String::createUTF32String(0x10D0));
        counter->addRange(1, 19999);
    }
    return counter;
}

const CounterStyle* CounterStyle::getLowerAlphaCounter()
{
    STARFISH_ASSERT(isMainThread());
    static CounterStyle* counter;
    static const int kAlphaN = 26;
    static const char kAlphaS[kAlphaN] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g',
                                           'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                           'o', 'p', 'q', 'r', 's', 't', 'u',
                                           'v', 'w', 'x', 'y', 'z' };
    if (counter == nullptr) {
        counter = new (NoGC) CounterStyle(
            String::createASCIIString("lower-alpha"), AlphabeticSystem);
        GCVector<String*>& symbols = counter->symbols();
        for (size_t i = 0; i < kAlphaN; i++) {
            symbols.emplace_back(String::createASCIIString(kAlphaS[i]));
        }
    }
    return counter;
}

const CounterStyle* CounterStyle::getUpperAlphaCounter()
{
    STARFISH_ASSERT(isMainThread());
    static CounterStyle* counter;
    static const int kAlphaN = 26;
    static const char kAlphaS[kAlphaN] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G',
                                           'H', 'I', 'J', 'K', 'L', 'M', 'N',
                                           'O', 'P', 'Q', 'R', 'S', 'T', 'U',
                                           'V', 'W', 'X', 'Y', 'Z' };
    if (counter == nullptr) {
        counter = new (NoGC) CounterStyle(
            String::createASCIIString("upper-alpha"), AlphabeticSystem);
        GCVector<String*>& symbols = counter->symbols();
        for (size_t i = 0; i < kAlphaN; i++) {
            symbols.emplace_back(String::createASCIIString(kAlphaS[i]));
        }
    }
    return counter;
}

bool CounterStyle::valid() const
{
    if (m_system != NoneSystem && !m_symbols.size() &&
        !m_additiveSymbols.size()) {
        return false;
    }
    // If the lower bound of any range is higher than the upper bound,
    // the entire descriptor is invalid and must be ignored.
    size_t size = m_ranges.size();
    for (size_t i = 0; i < size; i++) {
        if (m_ranges[i].m_min > m_ranges[i].m_max) {
            return false;
        }
    }
    switch (m_system) {
    case ExtendsSystem:
        if (!m_systemExtra.m_extends) {
            return false;
        }
        break;
    case AdditiveSystem: {
        uint32_t lastValue = m_additiveSymbols[0].m_weight;
        size_t size = m_additiveSymbols.size();
        for (size_t i = 1; i < size; i++) {
            if (m_additiveSymbols[i].m_weight >= lastValue) {
                return false;
            }
            lastValue = m_additiveSymbols[i].m_weight;
        }
        break;
    }
    default:
        break;
    }
    return true;
}

bool CounterStyle::equals(const CounterStyle* b) const
{
    if (this == b) {
        return true;
    }
    if (!b) {
        return false;
    }
    if (!m_name->equals(b->m_name)) {
        return false;
    }
    if (m_system != b->m_system) {
        return false;
    }
    if (m_symbols.size() != b->m_symbols.size()) {
        return false;
    }
    if (m_additiveSymbols.size() != b->m_additiveSymbols.size()) {
        return false;
    }
    size_t size = m_symbols.size();
    for (size_t i = 0; i < size; i++) {
        if (!m_symbols[i]->equals(b->m_symbols[i])) {
            return false;
        }
    }
    size = m_additiveSymbols.size();
    for (size_t i = 0; i < size; i++) {
        if (m_additiveSymbols[i] != b->m_additiveSymbols[i]) {
            return false;
        }
    }
    if (!m_prefix->equals(b->m_prefix)) {
        return false;
    }
    if (!m_suffix->equals(b->m_suffix)) {
        return false;
    }
    if (!m_negativePrefix->equals(b->m_negativePrefix)) {
        return false;
    }
    if (!m_negativeSuffix->equals(b->m_negativeSuffix)) {
        return false;
    }
    if (m_ranges.size() != b->m_ranges.size()) {
        return false;
    }
    // Sorted
    size = m_ranges.size();
    for (size_t i = 0; i < size; i++) {
        if (m_ranges[i].m_min != b->m_ranges[i].m_min ||
            m_ranges[i].m_max != b->m_ranges[i].m_max) {
            return false;
        }
    }
    if (m_padWidth != b->m_padWidth) {
        return false;
    }
    if (!m_padSymbol->equals(b->m_padSymbol)) {
        return false;
    }
    if (m_system == ExtendsSystem) {
        STARFISH_ASSERT(m_systemExtra.m_extends);
        STARFISH_ASSERT(b->m_systemExtra.m_extends);
        if (!m_systemExtra.m_extends->equals(b->m_systemExtra.m_extends)) {
            return false;
        }
    }
    if (m_system == FixedSystem &&
        m_systemExtra.m_firstSymbolValue !=
            b->m_systemExtra.m_firstSymbolValue) {
        return false;
    }
    if (m_fallback) {
        return m_fallback->equals(b->m_fallback);
    }
    return !b->m_fallback;
}
} /* namespace StarFish */
