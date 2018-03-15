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

#ifndef __StarFishCounterStyle__
#define __StarFishCounterStyle__

namespace StarFish {

class Document;
class CounterStyle;

// class CounterLabelBuilder : public gc {
//     STARFISH_MAKE_STACK_ALLOCATED()

// public:
//     static CounterLabelBuilder create(const CounterStyle* counter, int32_t
//     pos = 0);
//     String* proceed();

// protected:
//     CounterLabelBuilder(const CounterStyle* counter, int32_t pos);
//     CounterLabelBuilder(const CounterStyle* counter)
//         : CounterLabelBuilder(counter, 0)
//     {
//     }

//     bool exceedBound();

//     String* getSymbol();
//     String* getSymbol(GCVector<const CounterStyle*>& failedCounters);
//     String* getFallbackSymbol(GCVector<const CounterStyle*>& failedCounters);

//     Nullable<String*> getCyclicSymbol();
//     Nullable<String*> getFixedSymbol();
//     Nullable<String*> getSymbolicSymbol();
//     Nullable<String*> getAlphabeticSymbol();
//     Nullable<String*> getNumericSymbol();
//     Nullable<String*> getAdditiveSymbol();

// protected:
//     int32_t m_pos;
//     const CounterStyle* m_counterStyle;
// };

class CounterStyle : public gc {
public:
    enum System {
        NoneSystem,
        CyclicSystem,
        FixedSystem,
        SymbolicSystem,
        AlphabeticSystem,
        NumericSystem,
        AdditiveSystem,
        ExtendsSystem
    };

    static const int32_t kMaxRangeValue = (1 << 15) - 1;
    static const int32_t kMinRangeValue = -1 * kMaxRangeValue - 1;
    static const int32_t kPositiveInfiniteRangeValue = kMaxRangeValue + 1;
    static const int32_t kNegativeInfiniteRangeValue = kMinRangeValue - 1;

    struct AdditiveTuple {
        AdditiveTuple(uint32_t weight, String* symbol)
            : m_weight(weight)
            , m_symbol(symbol)
        {
        }
        bool operator==(const AdditiveTuple& other) const
        {
            return m_weight == other.m_weight &&
                   m_symbol->equals(other.m_symbol);
        }

        bool operator!=(const AdditiveTuple& other) const
        {
            return !operator==(other);
        }

        uint32_t m_weight;
        String* m_symbol;
    };

protected:
    union SystemExtra {
        int32_t m_firstSymbolValue;
        const CounterStyle* m_extends;

        SystemExtra(int32_t v)
            : m_firstSymbolValue(v)
        {
        }
        SystemExtra(const CounterStyle* v)
            : m_extends(v)
        {
        }
    };

    struct Range {
        Range(int32_t min, int32_t max)
            : m_min(min)
            , m_max(max)
        {
        }

        bool exceed(int32_t v) const
        {
            if (m_min != kNegativeInfiniteRangeValue && v < m_min) {
                return true;
            }
            if (m_max != kPositiveInfiniteRangeValue && v > m_max) {
                return true;
            }
            return false;
        }

        int32_t m_min;
        int32_t m_max;
    };

public:
    CounterStyle(String* v, System s)
        : m_name(v)
        , m_system(s)
        , m_systemExtra(1)
        , m_negativePrefix(getDefaultNegative())
        , m_negativeSuffix(String::emptyString)
        , m_prefix(String::emptyString)
        , m_suffix(getDefaultSuffix())
        , m_padWidth(0)
        , m_padSymbol(String::emptyString)
        , m_fallback()
    {
        STARFISH_ASSERT(s != ExtendsSystem);
    }

    CounterStyle(String* v, const CounterStyle* parent)
        : m_name(v)
        , m_system(ExtendsSystem)
        , m_systemExtra(parent)
        , m_negativePrefix(parent->negativePrefix())
        , m_negativeSuffix(parent->negativeSuffix())
        , m_prefix(parent->prefix())
        , m_suffix(parent->suffix())
        , m_ranges(parent->m_ranges)
        , m_padWidth(parent->padWidth())
        , m_padSymbol(parent->padSymbol())
        , m_fallback(parent->fallback())
        , m_symbols(parent->m_symbols)
    {
    }

    static const CounterStyle* getKnownCounter(String* name);
    static const CounterStyle* getNoneCounter();
    static const CounterStyle* getDiscCounter();
    static const CounterStyle* getDecimalCounter();
    static const CounterStyle* getCircleCounter();
    static const CounterStyle* getSquareCounter();
    static const CounterStyle* getDecimalLeadingZeroCounter();
    static const CounterStyle* getLowerRomanCounter();
    static const CounterStyle* getUpperRomanCounter();
    static const CounterStyle* getLowerGreekCounter();
    static const CounterStyle* getLowerLatinCounter();
    static const CounterStyle* getUpperLatinCounter();
    static const CounterStyle* getArmenianCounter();
    static const CounterStyle* getGeorgianCounter();
    static const CounterStyle* getLowerAlphaCounter();
    static const CounterStyle* getUpperAlphaCounter();
    static String* getDefaultNegative();
    static String* getDefaultSuffix();

    bool valid() const;

    String* name() const
    {
        if (m_system == NoneSystem) {
            return String::createASCIIString("none");
        }
        return m_name;
    }

    System system() const
    {
        return m_system;
    }

    int32_t firstSymbolValue() const
    {
        STARFISH_ASSERT(m_system == FixedSystem);
        return m_systemExtra.m_firstSymbolValue;
    }

    void setFirstSymbolValue(int32_t v)
    {
        STARFISH_ASSERT(m_system == FixedSystem);
        m_systemExtra.m_firstSymbolValue = v;
    }

    const CounterStyle* extends() const
    {
        STARFISH_ASSERT(m_system == ExtendsSystem);
        STARFISH_ASSERT(m_systemExtra.m_extends);
        return m_systemExtra.m_extends;
    }
    // NOTE Do not provide public setExtends() method to prevent cycle

    String* negativePrefix() const
    {
        return m_negativePrefix;
    }

    void setNegativePrefix(String* v)
    {
        m_negativePrefix = v;
    }

    String* negativeSuffix() const
    {
        return m_negativeSuffix;
    }

    void setNegativeSuffix(String* v)
    {
        m_negativeSuffix = v;
    }

    String* prefix() const
    {
        return m_prefix;
    }

    void setPrefix(String* v)
    {
        m_prefix = v;
    }

    String* suffix() const
    {
        return m_suffix;
    }

    void setSuffix(String* v)
    {
        m_suffix = v;
    }

    void addRange(int32_t min, int32_t max)
    {
        min = min < kMinRangeValue ? kMinRangeValue : min;
        max = max > kMaxRangeValue ? kMaxRangeValue : max;
        m_ranges.emplace_back(min, max);
    }

    const CounterStyle* fallback() const
    {
        if (!m_fallback) {
            return getDecimalCounter();
        }
        return m_fallback;
    }

    void setFallback(CounterStyle* v)
    {
        STARFISH_ASSERT(v);
        if (v != this) {
            m_fallback = v;
        }
    }

    void setSymbols(const GCVector<String*>& ref)
    {
        STARFISH_ASSERT(m_system != AdditiveSystem);
        m_symbols = ref;
    }

    void setSortedAddictiveSymbols(const GCVector<AdditiveTuple>& ref)
    {
        STARFISH_ASSERT(m_system == AdditiveSystem);
        m_additiveSymbols = ref;
    }

    uint32_t padWidth() const
    {
        return m_padWidth;
    }

    void setPadWidth(uint32_t v)
    {
        m_padWidth = v;
    }

    String* padSymbol() const
    {
        return m_padSymbol;
    }

    void setPadSymbol(String* v)
    {
        m_padSymbol = v;
    }

    bool equals(const CounterStyle* other) const;

    String* generateLabel(int32_t index) const;

protected:
    Range getDefaultRange() const
    {
        int32_t min = kNegativeInfiniteRangeValue;
        int32_t max = kPositiveInfiniteRangeValue;
        switch (m_system) {
        case AlphabeticSystem:
        case SymbolicSystem:
            min = 1;
            break;
        case AdditiveSystem:
            max = 0;
            break;
        default:
            break;
        }
        return Range(min, max);
    }

    bool ignoreNegativeSign() const
    {
        switch (m_system) {
        case ExtendsSystem:
            return extends()->ignoreNegativeSign();
        case NoneSystem:
        case CyclicSystem:
        case FixedSystem:
            return true;
        default:
            break;
        }
        return false;
    }

    GCVector<String*>& symbols()
    {
        STARFISH_ASSERT(m_system != AdditiveSystem);
        return m_symbols;
    }

    void addSymbol(String* s)
    {
        STARFISH_ASSERT(m_system != AdditiveSystem);
        STARFISH_ASSERT(m_system != ExtendsSystem);
        if (m_symbols.size() < kMaxRangeValue) {
            m_symbols.push_back(s);
        }
    }

    GCVector<AdditiveTuple>& additiveSymbols()
    {
        STARFISH_ASSERT(m_system == AdditiveSystem);
        return m_additiveSymbols;
    }

    void addAdditiveSymbol(uint32_t weight, String* s)
    {
        STARFISH_ASSERT(m_system == AdditiveSystem);
        if (m_additiveSymbols.size() < kMaxRangeValue) {
            m_additiveSymbols.emplace_back(weight, s);
        }
    }

    bool exceedBound(int32_t index) const;
    String* getSymbolAt(int32_t index) const;
    String* getSymbolAt(int32_t index,
                        GCVector<const CounterStyle*>& failedCounters) const;
    String* getFallbackSymbol(
        int32_t index, GCVector<const CounterStyle*>& failedCounters) const;
    Nullable<String*> getCyclicSymbol(int32_t index) const;
    Nullable<String*> getFixedSymbol(int32_t index) const;
    Nullable<String*> getSymbolicSymbol(int32_t index) const;
    Nullable<String*> getAlphabeticSymbol(int32_t index) const;
    Nullable<String*> getNumericSymbol(int32_t index) const;
    Nullable<String*> getAdditiveSymbol(int32_t index) const;

protected:
    String* m_name;
    System m_system;
    SystemExtra m_systemExtra;
    String* m_negativePrefix;
    String* m_negativeSuffix;
    String* m_prefix;
    String* m_suffix;
    GCAtomicVector<Range> m_ranges;
    uint32_t m_padWidth;
    String* m_padSymbol;
    const CounterStyle* m_fallback;
    GCVector<String*> m_symbols;
    GCVector<AdditiveTuple> m_additiveSymbols;
};
}

#endif
