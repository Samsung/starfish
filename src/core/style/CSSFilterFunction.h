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

#ifndef __StarFishCSSFilterFunction__
#define __StarFishCSSFilterFunction__

namespace StarFish {
enum FilterFunctionType ENSURE_ENUM_UNSIGNED {
    // NOTE Sequence is matter in CSSFilterFunction.cpp and FilterFunctions.cpp
    BlurFilterFunctionType = 0,
    DropShadowFilterFunctionType,
    HueRotateFilterFunctionType,
    BrightnessFilterFunctionType,
    ContrastFilterFunctionType,
    GrayScaleFilterFunctionType,
    InvertFilterFunctionType,
    OpacityFilterFunctionType,
    SaturateFilterFunctionType,
    SepiaFilterFunctionType,
    SVGUrlFilterFunctionType,
};

class CSSFilterFunction : public gc {
public:
    CSSFilterFunction(FilterFunctionType type, const CSSStyleValuePair& data)
        : m_type(type)
        , m_data(data)
    {
    }

    static CSSFilterFunction* parse(const CSSTokenValue& from);

    FilterFunctionType type() const
    {
        return m_type;
    }

    const CSSStyleValuePair& data() const
    {
        return m_data;
    }

    static const char* typeToString(FilterFunctionType type);
    String* toString() const;

    static bool isLengthType(FilterFunctionType type)
    {
        return type == BlurFilterFunctionType;
    }

    static bool isPercentType(FilterFunctionType type)
    {
        return type >= BrightnessFilterFunctionType &&
               type <= SepiaFilterFunctionType;
    }

    static bool isDegreeType(FilterFunctionType type)
    {
        return type == HueRotateFilterFunctionType;
    }

    static bool isDropShadowDataType(FilterFunctionType type)
    {
        return type >= DropShadowFilterFunctionType;
    }

private:
    FilterFunctionType m_type;
    CSSStyleValuePair m_data;
};
}

#endif
