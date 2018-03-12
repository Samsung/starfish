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

#ifndef __StarFishStyleRuleCounterStyle__
#define __StarFishStyleRuleCounterStyle__

namespace StarFish {

class Document;

class StyleRuleCounterStyle : public gc {
public:
    enum System {
        NoneSystem,
        CyclicSystem,
        FixedSystem,
        SymbolicSystem,
        AlphabeticSystem,
        NumericSystem,
        AddictiveSystem,
        ExtendsSystem
    };

protected:
    union SystemExtra {
        int32_t m_firstSymbolValue;
        StyleRuleCounterStyle* m_extends;

        SystemExtra(int32_t v)
            : m_firstSymbolValue(v)
        {
        }
        SystemExtra(StyleRuleCounterStyle* v)
            : m_extends(v)
        {
        }
    };

    static const int32_t kUninitializedRangeValue = 1 << 16;
    static const int32_t kMaxRangeValue = (1 << 15) - 1;
    static const int32_t kMinRangeValue = -1 * kMaxRangeValue - 1;
    static const int32_t kPositiveInfinityRangeValue = kMaxRangeValue + 1;
    static const int32_t kNegativeInfinityRangeValue = kMinRangeValue - 1;

public:
    StyleRuleCounterStyle(String* v, System s)
        : m_name(v)
        , m_system(s)
        , m_systemExtra(0)
        , m_negativePrefix(String::createASCIIString('-'))
        , m_negativeSuffix(String::emptyString)
        , m_prefix(String::emptyString)
        , m_suffix(String::createASCIIString('.'))
        , m_lowerBound(kUninitializedRangeValue)
        , m_upperBound(kUninitializedRangeValue)
        , m_padFixedWidth(0)
        , m_padSymbol(String::emptyString)
        , m_fallback(nullptr)
    {
        if (s == FixedSystem) {
            m_systemExtra.m_firstSymbolValue = 1;
        } else if (s == ExtendsSystem) {
            m_systemExtra.m_extends = nullptr;
        }
    }

    static const StyleRuleCounterStyle* getKnownCounter(String* name);
    static const StyleRuleCounterStyle* getNoneCounter();
    static const StyleRuleCounterStyle* getDiscCounter();

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

    StyleRuleCounterStyle* extends() const
    {
        STARFISH_ASSERT(m_system == ExtendsSystem);
        STARFISH_ASSERT(m_systemExtra.m_extends);
        return m_systemExtra.m_extends;
    }

    void setExtends(StyleRuleCounterStyle* v)
    {
        STARFISH_ASSERT(m_system == ExtendsSystem);
        m_systemExtra.m_extends = v;
    }

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

    int32_t lowerBound() const
    {
        if (m_lowerBound == kUninitializedRangeValue) {
            switch (m_system) {
            case NoneSystem:
            case CyclicSystem:
            case NumericSystem:
            case FixedSystem:
                return kNegativeInfinityRangeValue;
            case AlphabeticSystem:
            case SymbolicSystem:
                return 1;
            case AddictiveSystem:
                return 0;
            case ExtendsSystem:
                STARFISH_ASSERT(m_systemExtra.m_extends);
                return m_systemExtra.m_extends->lowerBound();
            }
        }
        STARFISH_ASSERT(m_lowerBound >= kNegativeInfinityRangeValue);
        return m_lowerBound;
    }

    void setLowerBound(int32_t v)
    {
        m_lowerBound = v < kMinRangeValue ? kMinRangeValue : v;
    }

    void setLowerBoundToNegativeInfinity()
    {
        m_lowerBound = kNegativeInfinityRangeValue;
    }

    int32_t upperBound() const
    {
        if (m_upperBound == kUninitializedRangeValue) {
            switch (m_system) {
            case NoneSystem:
            case CyclicSystem:
            case NumericSystem:
            case FixedSystem:
            case AlphabeticSystem:
            case SymbolicSystem:
            case AddictiveSystem:
                return kPositiveInfinityRangeValue;
            case ExtendsSystem:
                STARFISH_ASSERT(m_systemExtra.m_extends);
                return m_systemExtra.m_extends->upperBound();
            }
        }
        STARFISH_ASSERT(m_upperBound <= kPositiveInfinityRangeValue);
        return m_upperBound;
    }

    void setUpperBound(int32_t v)
    {
        m_upperBound = v > kMaxRangeValue ? kMaxRangeValue : v;
    }

    void setUpperBoundToPositiveInfinity()
    {
        m_upperBound = kPositiveInfinityRangeValue;
    }

    const StyleRuleCounterStyle* fallback()
    {
        if (!m_fallback) {
            m_fallback = getDiscCounter();
        }
        return m_fallback;
    }

    void setFallback(StyleRuleCounterStyle* v)
    {
        STARFISH_ASSERT(v);
        m_fallback = v;
    }

    void addSymbol(String* s)
    {
        STARFISH_ASSERT(m_system != AddictiveSystem);
        m_symbols.push_back(s);
    }

    void addAdditiveSymbol(uint32_t w, String* s)
    {
        STARFISH_ASSERT(m_system == AddictiveSystem);
        m_additiveWeight.push_back(w);
        m_symbols.push_back(s);
    }

    bool equals(const StyleRuleCounterStyle* other) const;

protected:
    String* m_name;
    System m_system;
    SystemExtra m_systemExtra;
    String* m_negativePrefix;
    String* m_negativeSuffix;
    String* m_prefix;
    String* m_suffix;
    int32_t m_lowerBound;
    int32_t m_upperBound;
    uint32_t m_padFixedWidth;
    String* m_padSymbol;
    const StyleRuleCounterStyle* m_fallback;
    GCVector<String*> m_symbols;
    GCAtomicVector<uint32_t> m_additiveWeight;
};
}

#endif
