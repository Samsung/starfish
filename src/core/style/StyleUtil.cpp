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

void StyleUtil::appendCSSString(StringBuilder& builder, String* value)
{
    // CSSOM escapes delimiters and control characters so that parsing
    // the serialized string preserves its value.
    // https://drafts.csswg.org/cssom/#serialize-a-string
    builder.appendChar('"');
    size_t start = 0;
    for (size_t i = 0; i < value->length(); i++) {
        char32_t c = value->charAt(i);
        if (c > 0x1f && c != 0x7f && c != '"' && c != '\\') {
            continue;
        }
        builder.appendSubString(value, start, i);
        if (c == 0) {
            builder.appendChar(static_cast<char32_t>(0xfffd));
        } else if (c <= 0x1f || c == 0x7f) {
            const char* hex = "0123456789abcdef";
            builder.appendChar('\\');
            if (c >= 0x10) {
                builder.appendChar(hex[c >> 4]);
            }
            builder.appendChar(hex[c & 0xf]);
            builder.appendChar(' ');
        } else {
            builder.appendChar('\\');
            builder.appendChar(c);
        }
        start = i + 1;
    }
    builder.appendSubString(value, start, value->length());
    builder.appendChar('"');
}

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

FontWeightValue StyleUtil::bolderWeight(FontWeightValue weight)
{
    switch (weight) {
    case FontWeightValue::OneHundredFontWeightValue:
    case FontWeightValue::TwoHundredsFontWeightValue:
    case FontWeightValue::ThreeHundredsFontWeightValue:
        return FontWeightValue::NormalFontWeightValue; // 400
    case FontWeightValue::FourHundredsFontWeightValue:
    case FontWeightValue::NormalFontWeightValue:
    case FontWeightValue::FiveHundredsFontWeightValue:
        return FontWeightValue::BoldFontWeightValue; // 700
    case FontWeightValue::SixHundredsFontWeightValue:
    case FontWeightValue::SevenHundredsFontWeightValue:
    case FontWeightValue::BoldFontWeightValue:
    case FontWeightValue::EightHundredsFontWeightValue:
    case FontWeightValue::NineHundredsFontWeightValue:
        return FontWeightValue::NineHundredsFontWeightValue; // 900
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
}

FontWeightValue StyleUtil::lighterWeight(FontWeightValue weight)
{
    switch (weight) {
    case FontWeightValue::OneHundredFontWeightValue:
    case FontWeightValue::TwoHundredsFontWeightValue:
    case FontWeightValue::ThreeHundredsFontWeightValue:
    case FontWeightValue::FourHundredsFontWeightValue:
    case FontWeightValue::NormalFontWeightValue:
    case FontWeightValue::FiveHundredsFontWeightValue:
        return FontWeightValue::OneHundredFontWeightValue;
    case FontWeightValue::SixHundredsFontWeightValue:
    case FontWeightValue::SevenHundredsFontWeightValue:
    case FontWeightValue::BoldFontWeightValue:
        return FontWeightValue::NormalFontWeightValue; // 400
    case FontWeightValue::EightHundredsFontWeightValue:
    case FontWeightValue::NineHundredsFontWeightValue:
        return FontWeightValue::BoldFontWeightValue; // 700
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
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
