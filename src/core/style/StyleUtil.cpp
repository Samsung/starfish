/*
 * Copyright (c) 2021-present Samsung Electronics Co., Ltd
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
#include "core/style/StyleUtil.h"

namespace Starfish {

String* StyleUtil::fontWeightToString(FontWeightValue weight)
{
    switch (weight) {
    case FontWeightValue::NormalFontWeightValue:
        return String::fromUTF8("normal");
    case FontWeightValue::BoldFontWeightValue:
        return String::fromUTF8("bold");
    case FontWeightValue::BolderFontWeightValue:
        return String::fromUTF8("bolder");
    case FontWeightValue::LighterFontWeightValue:
        return String::fromUTF8("lighter");
    case FontWeightValue::OneHundredFontWeightValue:
        return String::fromUTF8("100");
    case FontWeightValue::TwoHundredsFontWeightValue:
        return String::fromUTF8("200");
    case FontWeightValue::ThreeHundredsFontWeightValue:
        return String::fromUTF8("300");
    case FontWeightValue::FourHundredsFontWeightValue:
        return String::fromUTF8("400");
    case FontWeightValue::FiveHundredsFontWeightValue:
        return String::fromUTF8("500");
    case FontWeightValue::SixHundredsFontWeightValue:
        return String::fromUTF8("600");
    case FontWeightValue::SevenHundredsFontWeightValue:
        return String::fromUTF8("700");
    case FontWeightValue::EightHundredsFontWeightValue:
        return String::fromUTF8("800");
    case FontWeightValue::NineHundredsFontWeightValue:
        return String::fromUTF8("900");
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return String::emptyString;
    }
}

String* StyleUtil::fontStyleToString(FontStyleValue fontStyle)
{
    switch (fontStyle) {
    case FontStyleValue::NormalFontStyleValue:
        return String::fromUTF8("normal");
    case FontStyleValue::ItalicFontStyleValue:
        return String::fromUTF8("italic");
    case FontStyleValue::ObliqueFontStyleValue:
        return String::fromUTF8("oblique");
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return String::emptyString;
    }
}

String* StyleUtil::textTransformToString(TextTransformValue textTransform)
{
    switch (textTransform) {
    case TextTransformValue::NoneTextTransformValue:
        return String::fromUTF8("none");
    case TextTransformValue::CapitalizeTextTransformValue:
        return String::fromUTF8("capitalize");
    case TextTransformValue::UppercaseTextTransformValue:
        return String::fromUTF8("uppercase");
    case TextTransformValue::LowercaseTextTransformValue:
        return String::fromUTF8("lowercase");
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return String::emptyString;
    }
}
} // namespace Starfish
