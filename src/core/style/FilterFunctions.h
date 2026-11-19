/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishFilterFunctions__
#define __StarfishFilterFunctions__

#include "core/style/CSSFilterFunction.h"

namespace Starfish {

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

    virtual String* toString() const = 0;
    virtual CSSFilterFunction* toCSSFilterFunction() const = 0;

    virtual bool compare(const FilterFunction* b) const
    {
        return b && m_type == b->type();
    }

    // Filters a premultiplied buffer whose pixels are scale per CSS px.
    virtual void apply(WebView* webView, uint8_t* buffer, size_t width,
                       size_t height, size_t stride, float scale) const = 0;

protected:
    FilterFunctionType m_type;
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

    void apply(WebView* webView, uint8_t* buffer, size_t width, size_t height,
               size_t stride, float scale) const override;

private:
    Length m_stdDeviation;
};

// brightness() contrast() grayscale() hue-rotate() invert() opacity()
// saturate() sepia(): each is one feColorMatrix or feComponentTransfer
// primitive, so it is a 4x5 matrix on unpremultiplied sRGB colors
// (Filter Effects 1 "Supported Filter Functions").
class ColorFilterFunction : public FilterFunction {
public:
    // amount: a number (100% = 1), or degrees for hue-rotate().
    ColorFilterFunction(FilterFunctionType type, float amount);
    ColorFilterFunction(const CSSFilterFunction& from);

    float amount() const
    {
        return m_amount;
    }

    // Row-major 4x5, offsets in [0, 1] units.
    void colorMatrix(float out[20]) const;
    // The amount that leaves the input unchanged (Filter Effects 1
    // "Interpolation of Filters" initial value for interpolation).
    static float identityAmount(FilterFunctionType type);
    bool isIdentity() const
    {
        return m_amount == identityAmount(m_type);
    }

    String* toString() const override;
    CSSFilterFunction* toCSSFilterFunction() const override;
    bool compare(const FilterFunction* b) const override
    {
        return FilterFunction::compare(b) &&
               m_amount == static_cast<const ColorFilterFunction*>(b)->m_amount;
    }
    void apply(WebView* webView, uint8_t* buffer, size_t width, size_t height,
               size_t stride, float scale) const override;

private:
    float m_amount;
};

class DropShadowFilterFunction : public FilterFunction {
public:
    DropShadowFilterFunction(const Length& x, const Length& y,
                             const Length& stdDeviation, bool hasColor,
                             const Unit::Color& color)
        : FilterFunction(FilterFunctionType::DropShadowFilterFunctionType)
        , m_offsetX(x)
        , m_offsetY(y)
        , m_stdDeviation(stdDeviation)
        , m_hasColor(hasColor)
        , m_color(color)
    {
    }
    DropShadowFilterFunction(const CSSFilterFunction& from);

    void checkComputed(Length currentFS, Length rootFS, Font* font,
                       LayoutSize windowSize, ComputedStyle* cs) override;

    float offsetX() const
    {
        return m_offsetX.fixed();
    }
    float offsetY() const
    {
        return m_offsetY.fixed();
    }
    float stdDeviation() const
    {
        return m_stdDeviation.fixed();
    }
    const Unit::Color& color() const
    {
        return m_color;
    }
    bool isCurrentColor() const
    {
        return !m_hasColor;
    }
    // The input rect grown by where the shadow can paint.
    LayoutRect inkRect(const LayoutRect& input) const;

    String* toString() const override;
    CSSFilterFunction* toCSSFilterFunction() const override;
    bool compare(const FilterFunction* b) const override;
    void apply(WebView* webView, uint8_t* buffer, size_t width, size_t height,
               size_t stride, float scale) const override;

private:
    Length m_offsetX;
    Length m_offsetY;
    Length m_stdDeviation;
    // false: currentcolor, which checkComputed() resolves into m_color.
    bool m_hasColor;
    Unit::Color m_color;
};

class SVGUrlFilterFunction : public FilterFunction {
public:
    SVGUrlFilterFunction(String* url)
        : FilterFunction(FilterFunctionType::SVGUrlFilterFunctionType)
        , m_url(url)
    {
    }

    String* url() const
    {
        return m_url;
    }
    SVGUrlFilterFunction(const CSSFilterFunction& from);
    String* toString() const override;
    CSSFilterFunction* toCSSFilterFunction() const override;

    void apply(WebView* webView, uint8_t* buffer, size_t width, size_t height,
               size_t stride, float scale) const override;

private:
    String* m_url;
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
    // The list an element with 'filter: inherit' takes from its parent.
    FilterFunctions* forInheritance();
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

    // Whether any function changes what is drawn.
    bool hasEffect() const;
    // Whether every function maps each pixel on its own (no blur(),
    // drop-shadow() or url()).
    bool isColorOnly() const;
    // Whether filtering the layers of an image apart and compositing them
    // gives the filtered image: color functions that leave alpha alone and
    // never clamp, so they distribute over source-over.
    bool filtersLayersApart() const;
    // A plain fill of color through the color functions; a blur() leaves
    // one as large as the image as it is, and drop-shadow() and url() are
    // left out.
    Unit::Color filterColor(const Unit::Color& color) const;
    // The input rect grown by every function's ink overflow.
    LayoutRect inkRect(const LayoutRect& input) const;
};
} // namespace Starfish

#endif
