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

#ifndef __StarFishFilterFunctions__
#define __StarFishFilterFunctions__

#include "core/style/CSSFilterFunction.h"

namespace StarFish {

class FilterFunction : public gc {
public:
    FilterFunction(FilterFunctionType type)
        : m_type(type)
    {
    }
    virtual ~FilterFunction()
    {
    }

    FilterFunctionType type() const
    {
        return m_type;
    }

    static FilterFunction* create(const CSSFilterFunction& from);

    virtual void checkComputed(Length currentFS, Length rootFS, Font* font,
                               LayoutSize windowSize, ComputedStyle* cs)
    {
    }

    virtual bool isInitialValue() const = 0;
    virtual String* toString() const = 0;
    virtual CSSFilterFunction* toCSSFilterFunction() const = 0;

    virtual bool compare(const FilterFunction* b) const
    {
        return b && m_type == b->type();
    }

protected:
    FilterFunctionType m_type;
};

class UnsupportedFilterFunction : public FilterFunction {
public:
    UnsupportedFilterFunction(FilterFunctionType type)
        : FilterFunction(type)
    {
    }

    bool isInitialValue() const override
    {
        return true;
    }

    String* toString() const override;
    CSSFilterFunction* toCSSFilterFunction() const override;
};

class BlurFilterFunction : public FilterFunction {
public:
    BlurFilterFunction(const Length& value)
        : FilterFunction(FilterFunctionType::BlurFilterFunctionType)
        , m_stdDeviation(value)
    {
    }

    BlurFilterFunction(const CSSFilterFunction& from);

    void checkComputed(Length currentFS, Length rootFS, Font* font,
                       LayoutSize windowSize, ComputedStyle* cs) override
    {
        m_stdDeviation.changeToFixedIfNeeded(currentFS, rootFS, font,
                                             windowSize.width(),
                                             windowSize.height(), cs);
    }

    bool isInitialValue() const override
    {
        return m_stdDeviation.isFixed() && m_stdDeviation.fixed() == 1.0;
    }

    Length standardDeviation()
    {
        return m_stdDeviation;
    }

    const Length& standardDeviation() const
    {
        return m_stdDeviation;
    }

    void setStandardDeviation(const Length& v)
    {
        m_stdDeviation = v;
    }

    String* toString() const override;
    CSSFilterFunction* toCSSFilterFunction() const override;

    bool compare(const FilterFunction* b) const override
    {
        if (!b || b->type() != FilterFunctionType::BlurFilterFunctionType) {
            return false;
        }
        return m_stdDeviation == ((BlurFilterFunction*)b)->standardDeviation();
    }

private:
    Length m_stdDeviation;
};

class FilterFunctions : public GCVector<FilterFunction*> {
public:
    void checkComputed(Length currentFS, Length rootFS, Font* font,
                       LayoutSize windowSize, ComputedStyle* cs)
    {
        const size_t length = size();
        for (size_t i = 0; i < length; i++) {
            at(i)->checkComputed(currentFS, rootFS, font, windowSize, cs);
        }
    }

    // TODO Use CSSValue instead of CSSStyleValuePair
    static FilterFunctions* create(const CSSStyleValuePair& from);
    void toCSSStyleValue(CSSStyleValuePair& result) const;

    String* toString() const;
    bool compare(const FilterFunctions* b) const
    {
        const size_t length = size();
        if (!b || length != b->size()) {
            return false;
        }
        for (size_t i = 0; i < length; i++) {
            if (!at(i)->compare(b->at(i))) {
                return false;
            }
        }
        return true;
    }

    bool getStandardDeviationOfBlurFilter(Length& out);
};
}

#endif
