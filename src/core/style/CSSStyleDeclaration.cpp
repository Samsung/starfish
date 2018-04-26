/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "core/dom/DOMException.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBox.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameDocument.h"
#include "core/page/Window.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/style/CSSCounterFunction.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSStyleLookupTrie.h"
#include "core/style/CSSStyleRule.h"
#include "core/style/CSSStyleSheet.h"
#include "core/style/GradientData.h"
#include "core/style/StyleRule.h"

namespace StarFish {

#define TOKEN_IS_STRING(str) \
    (strlen(str) == strlen(token) && (memcmp(token, str, strlen(str))) == 0)

#define VALUE_IS_STRING(str) \
    (strlen(str) == strlen(value) && (memcmp(value, str, strlen(str))) == 0)

#define VALUE_IS_INHERIT() VALUE_IS_STRING("inherit")

#define VALUE_IS_INITIAL() VALUE_IS_STRING("initial")

#define VALUE_IS_UNSET() VALUE_IS_STRING("unset")

#define VALUE_IS_NONE() VALUE_IS_STRING("none")

#define VALUE_IS_AUTO() VALUE_IS_STRING("auto")

#define STRING_VALUE_IS_STRING(str) (value.equals(str))

#define STRING_VALUE_IS_INHERIT() STRING_VALUE_IS_STRING("inherit")

#define STRING_VALUE_IS_INITIAL() STRING_VALUE_IS_STRING("initial")

#define STRING_VALUE_IS_UNSET() STRING_VALUE_IS_STRING("unset")

#define STRING_VALUE_IS_NONE() STRING_VALUE_IS_STRING("none")

#define STRING_VALUE_IS_AUTO() STRING_VALUE_IS_STRING("auto")

#define GEN_ATTRIBUTE_GETTER_FOURSIDE(PRE, ...)                           \
    {                                                                     \
        String* top = PRE##Top##__VA_ARGS__();                            \
        if (!top->equals(String::emptyString)) {                          \
            String* right = PRE##Right##__VA_ARGS__();                    \
            if (!right->equals(String::emptyString)) {                    \
                String* bottom = PRE##Bottom##__VA_ARGS__();              \
                if (!bottom->equals(String::emptyString)) {               \
                    String* left = PRE##Left##__VA_ARGS__();              \
                    if (!left->equals(String::emptyString)) {             \
                        return combineBoxString(top, right, bottom, left, \
                                                isCombined);              \
                    }                                                     \
                }                                                         \
            }                                                             \
        }                                                                 \
        return String::emptyString;                                       \
    }

#define ADD_PAIRS(PRE, ...)                                                  \
    addCSSValuePair(CSSStyleValuePair::KeyKind::PRE##Top##__VA_ARGS__, top); \
    addCSSValuePair(CSSStyleValuePair::KeyKind::PRE##Right##__VA_ARGS__,     \
                    right);                                                  \
    addCSSValuePair(CSSStyleValuePair::KeyKind::PRE##Bottom##__VA_ARGS__,    \
                    bottom);                                                 \
    addCSSValuePair(CSSStyleValuePair::KeyKind::PRE##Left##__VA_ARGS__, left);

#define RM_PAIRS(PRE, ...)                                                    \
    removeCSSValuePair(CSSStyleValuePair::KeyKind::PRE##Top##__VA_ARGS__);    \
    removeCSSValuePair(CSSStyleValuePair::KeyKind::PRE##Right##__VA_ARGS__);  \
    removeCSSValuePair(CSSStyleValuePair::KeyKind::PRE##Bottom##__VA_ARGS__); \
    removeCSSValuePair(CSSStyleValuePair::KeyKind::PRE##Left##__VA_ARGS__);

#define GEN_ATTRIBUTE_SETTER_FOURSIDE(PRE, ...)                    \
    {                                                              \
        if (length == 0) {                                         \
            RM_PAIRS(PRE, __VA_ARGS__);                            \
            return;                                                \
        }                                                          \
        CSSTokenVector tokens;                                     \
        tokenizeCSSValue(tokens, value, length);                   \
                                                                   \
        CSSStyleValuePair c, top, right, bottom, left;             \
        if (c.updateValueCommon(tokens)) {                         \
            c.setFlagImportant(isImportant);                       \
            top = right = bottom = left = c;                       \
            ADD_PAIRS(PRE, __VA_ARGS__);                           \
            return;                                                \
        }                                                          \
        size_t len = tokens.size();                                \
        if (len < 1 || len > 4) {                                  \
            return;                                                \
        }                                                          \
                                                                   \
        GCVector<CSSStyleValuePair> result;                        \
        for (size_t i = 0; i < len; i++) {                         \
            CSSStyleValuePair v;                                   \
            v.setFlagImportant(isImportant);                       \
            if (!v.updateValueUnit##PRE##__VA_ARGS__(tokens[i])) { \
                return;                                            \
            }                                                      \
            result.push_back(v);                                   \
        }                                                          \
        top = result[0];                                           \
        right = len < 2 ? top : result[1];                         \
        bottom = len < 3 ? top : result[2];                        \
        left = len < 4 ? right : result[3];                        \
        top.setFlagImportant(isImportant);                         \
        right.setFlagImportant(isImportant);                       \
        bottom.setFlagImportant(isImportant);                      \
        left.setFlagImportant(isImportant);                        \
        ADD_PAIRS(PRE, __VA_ARGS__);                               \
    }

#define GEN_ATTRIBUTE_GETTER_BORDER(POS)                               \
    {                                                                  \
        String* width = Border##POS##Width();                          \
        String* style = Border##POS##Style();                          \
        String* color = Border##POS##Color();                          \
        return BorderString(width, false, style, false, color, false); \
    }

#define GEN_ATTRIBUTE_SETTER_BORDER(POS)                                   \
    {                                                                      \
        if (len == 0) {                                                    \
            removeBorder##POS##CSSValuePairs(this);                        \
            return;                                                        \
        }                                                                  \
                                                                           \
        CSSTokenVector tokens;                                             \
        tokenizeCSSValue(tokens, value, len);                              \
                                                                           \
        CSSStyleValuePair v, width, style, color;                          \
        if (v.updateValueCommon(tokens)) {                                 \
            v.setFlagImportant(isImportant);                               \
            addBorder##POS##CSSValuePairs(this, v, v, v);                  \
        } else if (parseBorderShorthand(tokens, &width, &style, &color)) { \
            width.setFlagImportant(isImportant);                           \
            style.setFlagImportant(isImportant);                           \
            color.setFlagImportant(isImportant);                           \
            addBorder##POS##CSSValuePairs(this, width, style, color);      \
        }                                                                  \
    }

static bool seperatorContains(const char* seperator, size_t seperatorCount,
                              char ch)
{
    for (size_t i = 0; i < seperatorCount; i++) {
        if (seperator[i] == ch) {
            return true;
        }
    }
    return false;
}

static void removeBackgroundCSSValuePairs(CSSStyleDeclaration* target)
{
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundColor);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundImage);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatX);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatY);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundPositionX);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundPositionY);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundSize);
    target->removeCSSValuePair(
        CSSStyleValuePair::KeyKind::BackgroundAttachment);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundOrigin);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundClip);
}

static void addBackgroundCSSValuePairs(
    CSSStyleDeclaration* target, CSSStyleValuePair color,
    CSSStyleValuePair image, CSSStyleValuePair repeatX,
    CSSStyleValuePair repeatY, CSSStyleValuePair positionX,
    CSSStyleValuePair positionY, CSSStyleValuePair size,
    CSSStyleValuePair attachment, CSSStyleValuePair origin,
    CSSStyleValuePair clip)
{
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundColor, color);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundImage, image);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatX,
                            repeatX);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatY,
                            repeatY);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundPositionX,
                            positionX);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundPositionY,
                            positionY);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundSize, size);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundAttachment,
                            attachment);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundOrigin,
                            origin);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundClip, clip);
}

// Parse backgroundRepeat string in case it has single token
static bool parseBackgroundRepeatShorhand(const CSSTokenValue& tok,
                                          CSSStyleValuePair* retx,
                                          CSSStyleValuePair* rety)
{
    if (retx->updateValueUnitBackgroundRepeat(tok)) {
        *rety = *retx;
    } else if (tok.equals("repeat-x")) {
        *retx = CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind,
            RepeatRepeatValue);
        *rety = CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind,
            NoRepeatRepeatValue);
    } else if (tok.equals("repeat-y")) {
        *retx = CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind,
            NoRepeatRepeatValue);
        *rety = CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::BackgroundRepeatValueKind,
            RepeatRepeatValue);
    } else {
        return false;
    }
    return true;
}

static bool parseBackgroundRepeatShorhand(const CSSTokenVector& tokens,
                                          CSSStyleValuePair* retx,
                                          CSSStyleValuePair* rety,
                                          bool allowComma = true)
{
    // <repeat-style> = repeat-x | repeat-y | [repeat | no-repeat]{1,2}
    // <repeat-style> [, <repeat-style>]*
    size_t len = 0;
    retx->setValueList(new ValueList(ValueList::Separator::CommaSeparator));
    rety->setValueList(new ValueList(ValueList::Separator::CommaSeparator));
    for (unsigned int i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& value = tokens[i];
        if (value.equals(",")) {
            if (!allowComma || i == 0 || i == tokens.size() - 1) {
                return false;
            }
        } else if (i == tokens.size() - 1) {
            len++;
            i++;
        } else {
            len++;
            continue;
        }
        if (len == 1) {
            CSSStyleValuePair x, y;
            if (parseBackgroundRepeatShorhand(tokens[i - 1], &x, &y)) {
                retx->multiValue()->push_back(x);
                rety->multiValue()->push_back(y);
            } else {
                return false;
            }
        } else if (len == 2) {
            CSSStyleValuePair x, y;
            if (x.updateValueUnitBackgroundRepeat(tokens[i - 2]) &&
                y.updateValueUnitBackgroundRepeat(tokens[i - 1])) {
                retx->multiValue()->push_back(x);
                rety->multiValue()->push_back(y);
            } else {
                return false;
            }
        } else {
            return false;
        }
        len = 0;
    }
    return true;
}

static bool parseBackgroundShorthand(
    const CSSTokenVector& tokens, CSSStyleValuePair* _Color,
    CSSStyleValuePair* _Image, CSSStyleValuePair* _RepeatX,
    CSSStyleValuePair* _RepeatY, CSSStyleValuePair* _PositionX,
    CSSStyleValuePair* _PositionY, CSSStyleValuePair* _Size,
    CSSStyleValuePair* _Attachment, CSSStyleValuePair* _Origin,
    CSSStyleValuePair* _Clip, bool allowColor)
{
    // - ACCEPT only 1-word_ : bg-color, bg-image, bg-attachment, bg-origin,
    // bg-clip
    // - ACCEPT 1 to 2 words : bg-repeat, position, bg-size

    size_t len = tokens.size();
    if (len < 1 || len > 11) {
        return false;
    }

    _Color->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _Image->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _RepeatX->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _RepeatY->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _PositionX->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _PositionY->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _Size->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _Attachment->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _Origin->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _Clip->setValueKind(CSSStyleValuePair::ValueKind::Initial);

    bool hasColor = false, hasImage = false, hasRepeat = false,
         hasPosition = false, hasSize = false, hasAttachment = false,
         hasOrigin = false, hasClip = false;
    bool hasPositionPrev = false, shouldSize = false;
    CSSStyleValuePair temp, tempX, tempY;
    CSSTokenValue* tok;
    CSSTokenVector toks;

#define SET_SINGLE_PROP(PROP) \
    *_##PROP = temp;          \
    has##PROP = true;
#define SET_DOUBLE_PROP(PROP) \
    *_##PROP##X = tempX;      \
    *_##PROP##Y = tempY;      \
    has##PROP = true;
#define SINGLE_CONTINUE() continue;
#define DOUBLE_CONTINUE() \
    i++;                  \
    continue;

    for (size_t i = 0; i < len; i++) {
        if (hasPositionPrev) {
            hasPositionPrev = false;
            if (tokens[i].equals("/")) {
                shouldSize = true;
                continue;
            }
        }
        // 1. Verify 2 tokens (current and next token at once)
        // e.g. background: top center; -> means background-position(x:top,
        // y:center)
        if (i + 1 < len) {
            toks.clear();
            toks.push_back(tokens[i]);
            toks.push_back(tokens[i + 1]);
            // 1-1. BackgroundSize
            // 1-2. BackgroundPosition, BackgroundRepeat
            if (shouldSize) {
                STARFISH_ASSERT(!hasSize);
                if (temp.updateValueBackgroundSize(toks, false)) {
                    shouldSize = false;
                    SET_SINGLE_PROP(Size)
                    DOUBLE_CONTINUE()
                }
            } else if (!hasPosition &&
                       CSSStyleDeclaration::parseBackgroundPositionShorthand(
                           toks, &tempX, &tempY, false)) {
                hasPositionPrev = true;
                SET_DOUBLE_PROP(Position)
                DOUBLE_CONTINUE()
            } else if (!hasRepeat && parseBackgroundRepeatShorhand(
                                         toks, &tempX, &tempY, false)) {
                SET_DOUBLE_PROP(Repeat)
                DOUBLE_CONTINUE()
            }
        }
        // 2. Verify single token
        tok = const_cast<CSSTokenValue*>(&tokens[i]);
        toks.clear();
        toks.push_back(*tok);
        // 2-1. BackgroundSize
        // 2-2. BackgroundImage, BackgroundPosition,
        // BackgroundRepeat, BackgroundAttachment,
        // BackgroundOrigin, BackgroundClip, BackgroundColor
        if (shouldSize) {
            STARFISH_ASSERT(!hasSize);
            if (temp.updateValueBackgroundSize(toks, false)) {
                shouldSize = false;
                SET_SINGLE_PROP(Size)
                SINGLE_CONTINUE()
            }
        } else if (!hasImage && temp.updateValueBackgroundImage(toks, false)) {
            SET_SINGLE_PROP(Image)
            SINGLE_CONTINUE()
        } else if (!hasPosition &&
                   CSSStyleDeclaration::parseBackgroundPositionShorthand(
                       toks, &tempX, &tempY, false)) {
            hasPositionPrev = true;
            SET_DOUBLE_PROP(Position)
            SINGLE_CONTINUE()
        } else if (!hasRepeat &&
                   parseBackgroundRepeatShorhand(toks, &tempX, &tempY, false)) {
            SET_DOUBLE_PROP(Repeat)
            SINGLE_CONTINUE()
        } else if (!hasAttachment &&
                   temp.updateValueBackgroundAttachment(toks, false)) {
            SET_SINGLE_PROP(Attachment)
            SINGLE_CONTINUE()
        } else if (!hasOrigin && temp.updateValueBox(toks, false)) {
            SET_SINGLE_PROP(Origin)
            SINGLE_CONTINUE()
        } else if (!hasClip && temp.updateValueBox(toks, false)) {
            SET_SINGLE_PROP(Clip)
            SINGLE_CONTINUE()
        } else if (!hasColor && temp.updateValueUnitColor(*tok)) {
            if (!allowColor) {
                return false;
            }
            SET_SINGLE_PROP(Color)
            SINGLE_CONTINUE()
        }

        return false;
    }
#undef SINGLE_CONTINUE
#undef DOUBLE_CONTINUE
#undef SET_SINGLE_PROP
#undef SET_DOUBLE_PROP

    return true;
}

static String* printBackground(String* image, String* position, String* size,
                               String* repeat, String* attachment,
                               String* origin, String* clip, String* color)
{
    if (image->equals(String::inheritString) ||
        position->equals(String::inheritString) ||
        size->equals(String::inheritString) ||
        repeat->equals(String::inheritString) ||
        attachment->equals(String::inheritString) ||
        origin->equals(String::inheritString) ||
        clip->equals(String::inheritString) ||
        color->equals(String::inheritString)) {
        return String::emptyString;
    }

    StringBuilder builder;

    if (image->length() != 0 && !image->equals(String::initialString)) {
        builder.appendString(image);
    }
    if (position->length() != 0 && !position->equals(String::initialString)) {
        if (builder.contentLength() > 0) {
            builder.appendString(String::spaceString);
        }
        builder.appendString(position);
    }
    if (size->length() != 0 && !size->equals(String::initialString)) {
        if (builder.contentLength() > 0) {
            builder.appendString(String::spaceString);
        }
        if (position->length() == 0) {
            builder.appendString("0% 0%");
        }
        builder.appendString(" / ");
        builder.appendString(size);
    }
    if (repeat->length() != 0 && !repeat->equals(String::initialString)) {
        if (builder.contentLength() > 0) {
            builder.appendString(String::spaceString);
        }
        builder.appendString(repeat);
    }
    if (attachment->length() != 0 &&
        !attachment->equals(String::initialString)) {
        if (builder.contentLength() > 0) {
            builder.appendString(String::spaceString);
        }
        builder.appendString(attachment);
    }
    if (origin->length() != 0 && !origin->equals(String::initialString)) {
        if (builder.contentLength() > 0) {
            builder.appendString(String::spaceString);
        }
        builder.appendString(origin);
    }
    if (clip->length() != 0 && !clip->equals(String::initialString)) {
        if (builder.contentLength() > 0) {
            builder.appendString(String::spaceString);
        }
        builder.appendString(clip);
    }
    if (color->length() != 0 && !color->equals(String::initialString)) {
        if (builder.contentLength() > 0) {
            builder.appendString(String::spaceString);
        }
        builder.appendString(color);
    }

    return builder.finalize();
}

static String* BorderString(String* width, bool isWidthCombined, String* style,
                            bool isStyleCombined, String* color,
                            bool isColorCombined)
{
    String* space = String::spaceString;
    StringBuilder builder;

    if (width->equals(style) && style->equals(color)) {
        STARFISH_ASSERT(width->equals(String::emptyString) ||
                        width->equals(String::initialString) ||
                        width->equals(String::inheritString));
        return width;
    }

    if (!width->equals(String::emptyString) &&
        !width->equals(String::initialString) && !isWidthCombined) {
        builder.appendString(width);
    }

    if (!style->equals(String::emptyString) &&
        !style->equals(String::initialString) && !isStyleCombined) {
        if (builder.contentLength() > 0) {
            builder.appendString(space);
        }
        builder.appendString(style);
    }

    if (!color->equals(String::emptyString) &&
        !color->equals(String::initialString) && !isColorCombined) {
        if (builder.contentLength() > 0) {
            builder.appendString(space);
        }
        builder.appendString(color);
    }

    return builder.finalize();
}

static void mergeBordeRadiusString(StringBuilder& sb, String* tl, String* tr,
                                   String* br, String* bl)
{
    if (tl->equals(tr) && tr->equals(br) && br->equals(bl)) {
        sb.appendString(tl);
    } else if (tl->equals(br) && tr->equals(bl)) {
        sb.appendString(tl);
        sb.appendChar(' ');
        sb.appendString(tr);
    } else if (tr->equals(bl)) {
        sb.appendString(tl);
        sb.appendChar(' ');
        sb.appendString(tr);
        sb.appendChar(' ');
        sb.appendString(br);
    } else {
        sb.appendString(tl);
        sb.appendChar(' ');
        sb.appendString(tr);
        sb.appendChar(' ');
        sb.appendString(br);
        sb.appendChar(' ');
        sb.appendString(bl);
    }
}

static bool parseBorderShorthand(const CSSTokenVector& tokens,
                                 CSSStyleValuePair* width,
                                 CSSStyleValuePair* style,
                                 CSSStyleValuePair* color)
{
    size_t len = tokens.size();
    if (len < 1 || len > 3) {
        return false;
    }

    width->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    style->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    color->setValueKind(CSSStyleValuePair::ValueKind::Initial);

    bool hasWidth = false, hasStyle = false, hasColor = false;
    CSSStyleValuePair temp;
    for (size_t i = 0; i < len; i++) {
        const CSSTokenValue& tok = tokens[i];
        if (!hasWidth && temp.updateValueUnitBorderWidth(tok)) {
            *width = temp;
            hasWidth = true;
        } else if (!hasStyle && temp.updateValueUnitBorderStyle(tok)) {
            *style = temp;
            hasStyle = true;
        } else if (!hasColor && temp.updateValueUnitColor(tok)) {
            *color = temp;
            hasColor = true;
        } else {
            return false;
        }
    }
    return true;
}

#define ADD_REMOVE_BORDER_CSSVALUES(POS, ...)                                 \
    static void removeBorder##POS##CSSValuePairs(CSSStyleDeclaration* target) \
    {                                                                         \
        target->removeCSSValuePair(                                           \
            CSSStyleValuePair::KeyKind::Border##POS##Width);                  \
        target->removeCSSValuePair(                                           \
            CSSStyleValuePair::KeyKind::Border##POS##Style);                  \
        target->removeCSSValuePair(                                           \
            CSSStyleValuePair::KeyKind::Border##POS##Color);                  \
    }

GEN_FOURSIDE(ADD_REMOVE_BORDER_CSSVALUES)
#undef ADD_REMOVE_BORDER_CSSVALUES

#define ADD_ADD_BORDER_CSSVALUES(POS, ...)                          \
    static void addBorder##POS##CSSValuePairs(                      \
        CSSStyleDeclaration* target, CSSStyleValuePair width,       \
        CSSStyleValuePair style, CSSStyleValuePair color)           \
    {                                                               \
        target->addCSSValuePair(                                    \
            CSSStyleValuePair::KeyKind::Border##POS##Width, width); \
        target->addCSSValuePair(                                    \
            CSSStyleValuePair::KeyKind::Border##POS##Style, style); \
        target->addCSSValuePair(                                    \
            CSSStyleValuePair::KeyKind::Border##POS##Color, color); \
    }

GEN_FOURSIDE(ADD_ADD_BORDER_CSSVALUES)
#undef ADD_ADD_BORDER_CSSVALUES

static void removeBorderCSSValuePairs(CSSStyleDeclaration* target)
{
    removeBorderTopCSSValuePairs(target);
    removeBorderRightCSSValuePairs(target);
    removeBorderBottomCSSValuePairs(target);
    removeBorderLeftCSSValuePairs(target);
}

static void addBorderCSSValuePairs(CSSStyleDeclaration* target,
                                   CSSStyleValuePair width,
                                   CSSStyleValuePair style,
                                   CSSStyleValuePair color)
{
    addBorderTopCSSValuePairs(target, width, style, color);
    addBorderRightCSSValuePairs(target, width, style, color);
    addBorderBottomCSSValuePairs(target, width, style, color);
    addBorderLeftCSSValuePairs(target, width, style, color);
}

static void removeBorderImageCSSValuePairs(CSSStyleDeclaration* target)
{
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageSource);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageSlice);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageWidth);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageOutset);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageRepeat);
}

static void addBorderImageCSSValuePairs(CSSStyleDeclaration* target,
                                        CSSStyleValuePair source,
                                        CSSStyleValuePair slice,
                                        CSSStyleValuePair width,
                                        CSSStyleValuePair outset,
                                        CSSStyleValuePair repeat)
{
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageSource,
                            source);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageSlice,
                            slice);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageWidth,
                            width);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageOutset,
                            outset);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageRepeat,
                            repeat);
}

static bool isRepeatTokenValue(const CSSTokenValue& token)
{
    if (token.equals("stretch") || token.equals("repeat") ||
        token.equals("round") || token.equals("space")) {
        return true;
    }
    return false;
}

static bool parseBorderImageShorthand(const CSSTokenVector& tokens,
                                      CSSStyleValuePair* _source,
                                      CSSStyleValuePair* _slice,
                                      CSSStyleValuePair* _width,
                                      CSSStyleValuePair* _outset,
                                      CSSStyleValuePair* _repeat)
{
    size_t len = tokens.size();
    if (len < 1 || len > 18) {
        return false;
    }

    _source->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _slice->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _width->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _outset->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    _repeat->setValueKind(CSSStyleValuePair::ValueKind::Initial);

    bool hasSource = false, hasSlice = false, hasWidth = false,
         hasOutset = false, hasRepeat = false;
    bool hasSlicePrev = false, shouldWidth = false;
    bool hasWidthPrev = false, shouldOutset = false;
    CSSStyleValuePair temp;
    CSSTokenVector toks;

    size_t pos = 0;
    while (pos < len) {
        CSSTokenValue token = tokens[pos++];
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);
        if (hasSlicePrev) {
            hasSlicePrev = false;
            if (token.equals("/")) {
                shouldWidth = true;
                continue;
            }
        }
        if (hasWidthPrev) {
            hasWidthPrev = false;
            if (token.equals("/")) {
                shouldOutset = true;
                continue;
            }
        }

        if (shouldWidth) {
            shouldWidth = false;
            if (token.equals("/")) {
                shouldOutset = true;
                continue;
            }

            toks.clear();
            toks.push_back(token);
            while (pos < len) {
                token = tokens[pos++];
                if (token.equals("/")) {
                    shouldOutset = true;
                    pos--;
                    break;
                } else if (isRepeatTokenValue(token)) {
                    pos--;
                    break;
                }
                toks.push_back(token);
            }

            if (temp.updateValueUnitBorderImageWidth(toks)) {
                hasWidthPrev = true;
                hasWidth = true;
                *_width = temp;
                continue;
            }
        } else if (shouldOutset) {
            shouldOutset = false;
            toks.clear();
            toks.push_back(token);
            while (pos < len) {
                token = tokens[pos++];
                if (isRepeatTokenValue(token)) {
                    pos--;
                    break;
                }
                toks.push_back(token);
            }

            if (temp.updateValueUnitBorderImageOutset(toks)) {
                hasOutset = true;
                *_outset = temp;
                continue;
            }
        } else if (!hasSource && temp.updateValueUnitBorderImageSource(token)) {
            hasSource = true;
            *_source = temp;
            continue;
        } else if (!hasSlice || !hasRepeat) {
            toks.clear();
            toks.push_back(token);
            bool isRepeat = isRepeatTokenValue(token) ? true : false;

            while (pos < len) {
                token = tokens[pos++];
                if (token.equals("/")) {
                    pos--;
                    break;
                } else if (isRepeatTokenValue(token)) {
                    if (isRepeat) {
                        toks.push_back(token);
                    } else {
                        pos--;
                    }
                    break;
                }
                toks.push_back(token);
            }

            if (isRepeat && temp.updateValueUnitBorderImageRepeat(toks)) {
                hasRepeat = true;
                *_repeat = temp;
                continue;
            } else if (temp.updateValueUnitBorderImageSlice(toks)) {
                hasSlicePrev = true;
                hasSlice = true;
                *_slice = temp;
                continue;
            }
        }
        return false;
    }
    return true;
}

static void removeFlexCSSValuePairs(CSSStyleDeclaration* target)
{
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::FlexGrow);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::FlexShrink);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::FlexBasis);
}

static void addFlexCSSValuePairs(CSSStyleDeclaration* target,
                                 CSSStyleValuePair flexGrow,
                                 CSSStyleValuePair flexShrink,
                                 CSSStyleValuePair flexBasis)
{
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::FlexGrow, flexGrow);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::FlexShrink, flexShrink);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::FlexBasis, flexBasis);
}

static bool parseFlexShorthand(const CSSTokenVector& tokens,
                               CSSStyleValuePair* flexGrow,
                               CSSStyleValuePair* flexShrink,
                               CSSStyleValuePair* flexBasis)
{
    size_t len = tokens.size();
    if (len < 1) {
        return false;
    }

    flexGrow->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    flexShrink->setNumberValue(1);
    flexBasis->setPercentageValue(0);

    bool hasFlexGrow = false, hasFlexShrink = false, hasFlexBasis = false;
    CSSStyleValuePair temp;
    size_t pos = 0;

    while (pos < len) {
        const CSSTokenValue& token = tokens[pos++];
        if (!hasFlexGrow && temp.updateValueUnitFlexGrow(token)) {
            hasFlexGrow = true;
            *flexGrow = temp;
            continue;
        } else if (hasFlexGrow && !hasFlexShrink &&
                   temp.updateValueUnitFlexShrink(token)) {
            hasFlexShrink = true;
            *flexShrink = temp;
            continue;
        } else if (temp.updateValueUnitFlexBasis(token)) {
            hasFlexBasis = true;
            *flexBasis = temp;
            break;
        }
        return false;
    }

    return hasFlexGrow || hasFlexShrink || hasFlexBasis;
}

static void removeFlexFlowCSSValuePairs(CSSStyleDeclaration* target)
{
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::FlexDirection);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::FlexWrap);
}

static void addFlexFlowCSSValuePairs(CSSStyleDeclaration* target,
                                     CSSStyleValuePair flexDirection,
                                     CSSStyleValuePair flexWrap)
{
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::FlexDirection,
                            flexDirection);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::FlexWrap, flexWrap);
}

static bool parseFlexFlowShorthand(const CSSTokenVector& tokens,
                                   CSSStyleValuePair* flexDirection,
                                   CSSStyleValuePair* flexWrap)
{
    size_t len = tokens.size();
    if (len < 1) {
        return false;
    }

    flexDirection->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    flexWrap->setValueKind(CSSStyleValuePair::ValueKind::Initial);

    bool hasFlexDirection = false, hasFlexWrap = false;
    CSSStyleValuePair temp;
    size_t pos = 0;

    while (pos < len) {
        const CSSTokenValue& token = tokens[pos++];
        if (!hasFlexDirection && temp.updateValueUnitFlexDirection(token)) {
            hasFlexDirection = true;
            *flexDirection = temp;
            continue;
        } else if (temp.updateValueUnitFlexWrap(token)) {
            hasFlexWrap = true;
            *flexWrap = temp;
            break;
        }
        return false;
    }

    return hasFlexDirection || hasFlexWrap;
}

static bool parseFontShorthand(const CSSTokenVector& tokens,
                               CSSStyleValuePair* _Style,
                               // UNSUPPORTED CSSStyleValuePair* _Variant,
                               CSSStyleValuePair* _Weight,
                               // UNSUPPORTED CSSStyleValuePair* _Stretch,
                               CSSStyleValuePair* _Size,
                               CSSStyleValuePair* _LineHeight,
                               CSSStyleValuePair* _Family)
{
    // [font-style|font-weight] font-size[/line-height] font-family
    size_t len = tokens.size();
    if (len < 1) {
        return false;
    }

    _Style->setValueKind(CSSStyleValuePair::ValueKind::FontStyleValueKind);
    _Style->setValue(FontStyleValue::NormalFontStyleValue);
    _Weight->setValueKind(CSSStyleValuePair::ValueKind::FontWeightValueKind);
    _Weight->setValue(FontWeightValue::NormalFontWeightValue);
    _LineHeight->setValueKind(CSSStyleValuePair::ValueKind::Normal);

    bool hasStyle = false, hasWeight = false, hasSize = false,
         hasLineHeight = false, hasFamily = false;
    CSSStyleValuePair temp;
    bool hasSizePrev = false, shouldLineHeight = false;
    size_t pos = 0;
    CSSTokenVector fontFamilyCandidate;

    while (pos < len) {
        const CSSTokenValue& orgToken = tokens[pos];
        CSSTokenValue token = tokens[pos++];
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);

        if (hasSizePrev) {
            hasSizePrev = false;
            if (token.equals("/")) {
                shouldLineHeight = true;
                continue;
            }
        }
        if (shouldLineHeight) {
            shouldLineHeight = false;
            if (temp.updateValueUnitLineHeight(token)) {
                hasLineHeight = true;
                *_LineHeight = temp;
                continue;
            }
        } else if (!hasSize && !hasStyle &&
                   temp.updateValueUnitFontStyle(token)) {
            hasStyle = true;
            *_Style = temp;
            continue;
        } else if (!hasSize && !hasWeight &&
                   temp.updateValueUnitFontWeight(token)) {
            hasWeight = true;
            *_Weight = temp;
            continue;
        } else if (!hasSize && temp.updateValueUnitFontSize(token)) {
            hasSizePrev = true;
            hasSize = true;
            *_Size = temp;
            continue;
        } else if (hasSize) {
            if (hasFamily && token == ",") {
                continue;
            }
            if (token[0] == '\'' && token.length() > 2 &&
                token.back() == '\'') {
                fontFamilyCandidate.push_back(
                    orgToken.substr(1, orgToken.length() - 2));
            } else if (token[0] == '"' && token.length() > 2 &&
                       token.back() == '"') {
                fontFamilyCandidate.push_back(
                    orgToken.substr(1, orgToken.length() - 2));
            } else {
                fontFamilyCandidate.push_back(orgToken);
            }
            hasFamily = true;
            continue;
        }
        return false;
    }
    if (!hasSize || !hasFamily) {
        return false;
    }
    if (fontFamilyCandidate.size() == 1) {
        _Family->setKeywordValue(String::fromUTF8(
            fontFamilyCandidate[0].data(), fontFamilyCandidate[0].length()));
    } else {
        ValueList* val = new ValueList(
            ValueList::Separator::CommaSeparatorAppendQuoteWhenMeetWhiteSpace);
        for (size_t i = 0; i < fontFamilyCandidate.size(); i++) {
            auto str = fontFamilyCandidate[i];
            val->emplace_back(CSSStyleValuePair::ValueKind::KeywordValueKind,
                              String::fromUTF8(str.data(), str.length()));
        }
        _Family->setValueList(val);
    }
    return true;
}

static bool parseListStyleShorhand(Document* document,
                                   const CSSTokenVector& tokens,
                                   CSSStyleValuePair* type,
                                   CSSStyleValuePair* position,
                                   CSSStyleValuePair* image)
{
    size_t size = tokens.size();
    if (size == 1 && tokens[0].equals("none")) {
        type->setValueKind(CSSStyleValuePair::None);
        image->setValueKind(CSSStyleValuePair::None);
        position->setValueKind(CSSStyleValuePair::ListStylePositionValueKind);
        position->setValue(ListStylePositionValue::ListStylePositionOutside);
        return true;
    }
    if (size != 3) {
        return false;
    }
    bool foundType = false, foundPos = false, foundImg = false;
    size_t noneCount = 0;
    // NOTE Consider corner cases
    //   none none inside       (O)
    //   url(...) inside inside (O)
    //   none url(...) inside   (O)
    for (size_t i = 0; i < size; i++) {
        const CSSTokenValue& token = tokens[i];
        if (!foundPos && position->updateValueUnitListStylePosition(token)) {
            foundPos = true;
            continue;
        }
        if (!foundImg && image->updateValueUnitListStyleImage(token)) {
            if (image->valueKind() != CSSStyleValuePair::None) {
                foundImg = true;
            } else {
                noneCount++;
            }
            continue;
        }
        if (!foundType && type->updateValueUnitListStyleType(document, token)) {
            foundType = true;
            continue;
        }
        return false;
    }
    if (!foundImg && noneCount > 0) {
        noneCount--;
        image->setValueKind(CSSStyleValuePair::None);
        foundImg = true;
    }
    if (!foundType && noneCount > 0) {
        noneCount--;
        type->setValueKind(CSSStyleValuePair::None);
        foundType = true;
    }
    return foundPos && foundImg && foundType;
}

static void removeOverflowCSSValuePairs(CSSStyleDeclaration* target)
{
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::OverflowX);
    target->removeCSSValuePair(CSSStyleValuePair::KeyKind::OverflowY);
}

static void addOverflowCSSValuePairs(CSSStyleDeclaration* target,
                                     CSSStyleValuePair overflowX,
                                     CSSStyleValuePair overflowY)
{
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::OverflowX, overflowX);
    target->addCSSValuePair(CSSStyleValuePair::KeyKind::OverflowY, overflowY);
}

static bool parseOverflowShorthand(const CSSTokenVector& tokens,
                                   CSSStyleValuePair* overflowX,
                                   CSSStyleValuePair* overflowY)
{
    size_t len = tokens.size();
    if (len < 1) {
        return false;
    }

    overflowX->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    overflowY->setValueKind(CSSStyleValuePair::ValueKind::Initial);

    CSSStyleValuePair temp;

    const CSSTokenValue& tok = tokens[0];

    if (temp.updateValueUnitOverflowX(tokens[0])) {
        *overflowX = temp;
        *overflowY = temp;
    } else {
        return false;
    }
    return true;
}

static bool parseTransitionShorthand(const CSSTokenVector& tokens,
                                     CSSStyleValuePair* property,
                                     CSSStyleValuePair* duration,
                                     CSSStyleValuePair* timingFunction,
                                     CSSStyleValuePair* delay)
{
    size_t len = tokens.size();
    if (len < 1) {
        return false;
    }

    property->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    duration->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    timingFunction->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    delay->setValueKind(CSSStyleValuePair::ValueKind::Initial);

    CSSStyleValuePair temp;
    bool foundProperty = false;
    bool foundDuration = false;
    bool foundTimingFunction = false;
    bool foundDelay = false;

    for (size_t i = 0; i < len; i++) {
        const CSSTokenValue& tok = tokens[i];
        if (!foundProperty && temp.updateValueUnitTransitionProperty(tok)) {
            foundProperty = true;
            *property = temp;
            continue;
        }
        if ((!foundDuration || !foundDelay) &&
            temp.updateValueUnitTimeOrCalc(tok, 0)) {
            if (!foundDuration) {
                foundDuration = true;
                *duration = temp;
                continue;
            }
            if (!foundDelay) {
                foundDelay = true;
                *delay = temp;
                continue;
            }
        }
        if (!foundTimingFunction &&
            temp.updateValueUnitTransitionTimingFunction(tok)) {
            foundTimingFunction = true;
            *timingFunction = temp;
            continue;
        }
        return false;
    }
    return true;
}

// helper function to convert Length to CSSStyleValuePair format
CSSStyleValuePair CSSStyleDeclaration::lengthToCSSStyleValue(Length len)
{
    CSSStyleValuePair p;
    if (len.isFixed()) {
        p.setValueKind(CSSStyleValuePair::ValueKind::Length);
        p.setValue(CSSLength(len.fixed()));
    } else if (len.isPercent()) {
        p.setValueKind(CSSStyleValuePair::ValueKind::Percentage);
        p.setValue(len.percent());
    } else if (len.isFontPercent()) {
        p.setValueKind(CSSStyleValuePair::ValueKind::Length);
        Length::Type t = len.type();
        CSSLength::Kind k;
        if (t == Length::Em) {
            k = CSSLength::EM;
        } else if (t == Length::Ex) {
            k = CSSLength::EX;
        } else {
            k = CSSLength::REM;
        }
        p.setValue(CSSLength(k, len.fontPercent()));
    } else if (len.isViewportPercent()) {
        p.setValueKind(CSSStyleValuePair::ValueKind::Length);
        Length::Type t = len.type();
        CSSLength::Kind k;
        if (t == Length::Vw) {
            k = CSSLength::VW;
        } else if (t == Length::Vh) {
            k = CSSLength::VH;
        } else if (t == Length::Vmin) {
            k = CSSLength::VMIN;
        } else {
            k = CSSLength::VMAX;
        }
        p.setValue(CSSLength(k, len.viewportPercent()));
    } else if (len.isAuto()) {
        p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
    } else if (len.isCalc()) {
        p.setCalcValue(len.calcData());
    } else if (len.isInheritableNumber()) {
        p.setNumberValue(len.inheritableNumber());
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    return p;
}

void CSSStyleDeclaration::rootPointerValueIfExists(const CSSStyleValuePair& v)
{
    auto p = v.pointerValue();
    if (p) {
        m_pointerRooter.push_back(p);
    }
}

void CSSStyleDeclaration::addValuePair(CSSStyleValuePair p)
{
    for (size_t i = 0; i < m_cssValues.size(); i++) {
        CSSStyleValuePair v = m_cssValues[i];
        if (v.keyKind() == p.keyKind()) {
            m_cssValues[i] = p;
            rootPointerValueIfExists(p);
            return;
        }
    }

    m_cssValues.push_back(p);
    rootPointerValueIfExists(p);
}

void CSSStyleDeclaration::clear()
{
    m_cssValues.clear();
    m_pointerRooter.clear();
}

ScriptBindingInstance* CSSStyleDeclaration::scriptBindingInstance()
{
    STARFISH_ASSERT(m_node);
    return m_node->scriptBindingInstance();
}

CSSStyleDeclaration* CSSStyleDeclaration::clone(Element* element)
{
    CSSStyleDeclaration* newStyle = new CSSStyleDeclaration(element);
    newStyle->m_cssValues = m_cssValues;

    return newStyle;
}

String* CSSStyleDeclaration::customProperty(String* key)
{
    String* val = String::emptyString;

    for (size_t i = 0; i < m_cssCustomValues.size(); i++) {
        MutablePropertyValue customProperty = m_cssCustomValues[i];
        if (customProperty.name()->equals(key)) {
            val = customProperty.value();
            break;
        }
    }

    return val;
}

void CSSStyleDeclaration::setCustomProperty(String* key, String* value,
                                            size_t len)
{
    for (size_t i = 0; i < m_cssCustomValues.size(); i++) {
        MutablePropertyValue property = m_cssCustomValues[i];
        if (property.name()->equals(key)) {
            property.setValue(value);
            return;
        }
    }
    MutablePropertyValue custom(key, value);
    m_cssCustomValues.push_back(custom);
}

void CSSStyleDeclaration::tokenizeCSSValue(CSSTokenVector& tokens,
                                           const char* data, size_t length,
                                           const char* seperator,
                                           size_t seperatorCount,
                                           bool isCaseSensitive,
                                           bool preserveContentWS)
{
    CSSTokenValue str;
    bool inParenthesis = false;
    size_t numberOfnesting = 0;
    bool inQuotes = false;
    bool isWhiteSpaceState = false;
    for (size_t i = 0; i < length; i++) {
        if (data[i] == '(') {
            inParenthesis = true;
            numberOfnesting++;
        } else if (data[i] == ')') {
            if (numberOfnesting) {
                numberOfnesting--;
            }
        } else if (data[i] == '"' || data[i] == '\'') {
            inQuotes = !inQuotes;
        }

        if (!preserveContentWS && isWhiteSpaceState &&
            String::isSpaceOrNewline(data[i])) {
            continue;
        }

        isWhiteSpaceState = false;
        str += data[i];
        if (!preserveContentWS && (inParenthesis || inQuotes) &&
            String::isSpaceOrNewline(data[i])) {
            str[str.length() - 1] = ' ';
            isWhiteSpaceState = true;
            continue;
        }
        bool hasSepChar = false;
        if (seperatorCount > 0 && !inParenthesis) {
            hasSepChar = seperatorContains(seperator, seperatorCount, data[i]);
        }

        if ((!inParenthesis && !inQuotes &&
             (String::isSpaceOrNewline(data[i]) || hasSepChar)) ||
            (data[i] == '(' && hasSepChar) || (data[i] == ')' && hasSepChar)) {
            str.pop_back();
            bool onlyWhiteSpace = true;
            for (size_t i = 0; i < str.length(); i++) {
                if (!isCaseSensitive)
                    str[i] = ::tolower(str[i]);
                if (!String::isASCIISpace(str[i])) {
                    onlyWhiteSpace = false;
                }
            }

            if (!onlyWhiteSpace && !numberOfnesting) {
                tokens.push_back(CSSTokenValue(std::move(str)));
            }
            isWhiteSpaceState = true;
            if (hasSepChar && !numberOfnesting) {
                tokens.push_back(CSSTokenValue(std::string(data + i, 1)));
            }
        } else if (((inParenthesis && !numberOfnesting) && data[i] == ')') ||
                   i == length - 1) {
            if (str.length() > 3 && (str[0] == 'u' || str[0] == 'U') &&
                (str[1] == 'r' || str[1] == 'R') &&
                (str[2] == 'l' || str[2] == 'L')) {
                std::transform(str.begin(), str.begin() + 3, str.begin(),
                               ::tolower);
                tokens.push_back(std::move(str));
            } else if (str.length() != 0) {
                if (!isCaseSensitive && !inQuotes) {
                    std::transform(str.begin(), str.end(), str.begin(),
                                   ::tolower);
                }
                if (!numberOfnesting) {
                    tokens.push_back(std::move(str));
                }
            }
            inParenthesis = false;
            isWhiteSpaceState = true;
        }
    }
}

void CSSStyleDeclaration::addCSSValuePair(CSSStyleValuePair::KeyKind name,
                                          const CSSStyleValuePair& ret)
{
    for (unsigned i = 0; i < m_cssValues.size(); i++) {
        if (m_cssValues[i].keyKind() == name) {
            if (isInlineStyle() || ret.flagImportant() == true ||
                (ret.flagImportant() == false &&
                 m_cssValues[i].flagImportant() == false)) {
                m_cssValues[i].setValueKind(ret.valueKind());
                m_cssValues[i].setValue(ret.value());
                m_cssValues[i].setFlagImportant(ret.flagImportant());
                m_cssValues[i].setTemporaryKeyKind(ret.temporaryKeyKind());
                rootPointerValueIfExists(ret);
                notifyNeedsStyleRecalc();
            }

            return;
        }
    }
    m_cssValues.push_back(CSSStyleValuePair(ret));
    m_cssValues.back().setKeyKind(name);
    rootPointerValueIfExists(ret);
    notifyNeedsStyleRecalc();
}

void CSSStyleDeclaration::removeCSSValuePair(CSSStyleValuePair::KeyKind name)
{
    unsigned len = m_cssValues.size();
    for (unsigned i = 0; i < len; i++) {
        if (m_cssValues[i].keyKind() == name) {
            m_cssValues.erase(m_cssValues.begin() + i);
            notifyNeedsStyleRecalc();
            return;
        }
    }
}

bool CSSStyleDeclaration::hasCSSValuePair(CSSStyleValuePair::KeyKind name)
{
    unsigned len = m_cssValues.size();
    for (unsigned i = 0; i < len; i++) {
        if (m_cssValues[i].keyKind() == name) {
            return true;
        }
    }
    return false;
}

CSSStyleValuePair CSSStyleDeclaration::getCSSValuePair(
    CSSStyleValuePair::KeyKind name)
{
    unsigned len = m_cssValues.size();
    for (unsigned i = 0; i < len; i++) {
        if (m_cssValues[i].keyKind() == name) {
            return m_cssValues[i];
        }
    }

    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

void CSSStyleDeclaration::notifyNeedsStyleRecalc()
{
    if (m_node->isElement()) {
        m_node->asElement()->notifyInlineStyleChanged();
    }
}

String* CSSStyleDeclaration::combineBoxString(String* t, String* r, String* b,
                                              String* l, bool* isCombined)
{
    if (isCombined) {
        *isCombined = true;
    }
    // [NOTICE]
    // All initial --> return "initial"
    // Not all, but more than 1 initial --> return ""
    size_t initialCount = 0;
    initialCount += t->equals(String::initialString) ? 1 : 0;
    initialCount += r->equals(String::initialString) ? 1 : 0;
    initialCount += b->equals(String::initialString) ? 1 : 0;
    initialCount += l->equals(String::initialString) ? 1 : 0;
    if (initialCount > 0 && initialCount < 4) {
    }

    String* space = String::spaceString;
    if (!r->equals(l)) {
        return t->concat(space)
            ->concat(r)
            ->concat(space)
            ->concat(b)
            ->concat(space)
            ->concat(l);
    } else if (!t->equals(b)) {
        return t->concat(space)->concat(r)->concat(space)->concat(b);
    } else if (!t->equals(r)) {
        return t->concat(space)->concat(r);
    } else {
        if (isCombined) {
            *isCombined = false;
        }
        return t;
    }
}

#define DEFINE_ATTRIBUTE_GETTER(name, ...)                                    \
    String* CSSStyleDeclaration::name()                                       \
    {                                                                         \
        if (isComputedStyle()) {                                              \
            ComputedStyleCSSStyleDeclaration::Stage stage =                   \
                requiredStage(CSSStyleValuePair::KeyKind::name);              \
            if (stage ==                                                      \
                ComputedStyleCSSStyleDeclaration::Stage::frameTreeBuild) {    \
                buildFrameTreeIfNeeds();                                      \
            } else if (stage ==                                               \
                       ComputedStyleCSSStyleDeclaration::Stage::layout) {     \
                layoutIfNeeds();                                              \
            } else {                                                          \
                resolveStyleIfNeeds();                                        \
            }                                                                 \
            updateValue(CSSStyleValuePair::KeyKind::name);                    \
        }                                                                     \
        for (unsigned i = 0; i < m_cssValues.size(); i++) {                   \
            if (m_cssValues[i].keyKind() == CSSStyleValuePair::KeyKind::name) \
                return m_cssValues[i].toString();                             \
        }                                                                     \
        return String::emptyString;                                           \
    }
FOR_EACH_STYLE_ATTRIBUTE_BASIC(DEFINE_ATTRIBUTE_GETTER)
FOR_EACH_STYLE_ATTRIBUTE_STICKY(DEFINE_ATTRIBUTE_GETTER)
#undef DEFINE_ATTRIBUTE_GETTER

#define DEFINE_ATTRIBUTE_SETTER(name, ...)                              \
    void CSSStyleDeclaration::set##name(const char* value, size_t len,  \
                                        bool isImportant)               \
    {                                                                   \
        if (len == 0) {                                                 \
            removeCSSValuePair(CSSStyleValuePair::KeyKind::name);       \
            return;                                                     \
        }                                                               \
        CSSTokenVector tokens;                                          \
        if (UNLIKELY(CSSStyleValuePair::KeyKind::name ==                \
                     CSSStyleValuePair::KeyKind::Content)) {            \
            tokenizeCSSValue(tokens, value, len, "", 0, true, true);    \
        } else {                                                        \
            tokenizeCSSValue(tokens, value, len, ",", 1);               \
        }                                                               \
        CSSStyleValuePair ret;                                          \
        if (ret.updateVarValue(value, tokens)) {                        \
            ret.setFlagImportant(isImportant);                          \
            ret.setTemporaryKeyKind(CSSStyleValuePair::KeyKind::name);  \
            addCSSValuePair(CSSStyleValuePair::KeyKind::VarValue, ret); \
            return;                                                     \
        }                                                               \
        if (ret.updateValueCommon(tokens) ||                            \
            ret.updateValue##name(m_node->document(), tokens)) {        \
            ret.setFlagImportant(isImportant);                          \
            addCSSValuePair(CSSStyleValuePair::KeyKind::name, ret);     \
        }                                                               \
    }

FOR_EACH_STYLE_ATTRIBUTE_BASIC(DEFINE_ATTRIBUTE_SETTER)
#undef DEFINE_ATTRIBUTE_SETTER

uint32_t CSSStyleDeclaration::length() const
{
    return m_cssValues.size();
}

String* CSSStyleDeclaration::item(uint32_t index)
{
    if (index < m_cssValues.size()) {
        return m_cssValues[index].keyName();
    }
    return String::emptyString;
}

String* CSSStyleDeclaration::getPropertyValue(String* name)
{
    auto str = name->toNullableUTF8String();
    CSSStyleValuePair::KeyKind kind =
        lookupCSSStyle(str.m_buffer, str.m_bufferSize);
    String* val = String::emptyString;
    switch (kind) {
#define MATCH_KEY(Name, ...)               \
    case CSSStyleValuePair::KeyKind::Name: \
        val = Name();                      \
        break;
        FOR_EACH_STYLE_ATTRIBUTE_TOTAL(MATCH_KEY)
#undef MATCH_KEY
    default:
        val = customProperty(name);
        break;
    }
    return val;
}

void CSSStyleDeclaration::setProperty(String* name, String* value,
                                      String* prior)
{
    bool isImportant = false;
    auto str = name->toNullableUTF8String();
    CSSStyleValuePair::KeyKind kind =
        lookupCSSStyle(str.m_buffer, str.m_bufferSize);

    if (prior->length() > 0) {
        if (prior->equalsIgnoreCase("important")) {
            isImportant = true;
        } else {
            if (kind == CSSStyleValuePair::KeyKind::CustomProperty) {
                setCustomProperty(name, value, str.m_bufferSize);
            }
            return;
        }
    }

    struct Sender {
        CSSStyleDeclaration* self;
        CSSStyleValuePair::KeyKind kind;
        bool isImportant;
    } sender;
    sender.self = this;
    sender.kind = kind;
    sender.isImportant = isImportant;
    value->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
            CSSStyleValuePair::KeyKind kind = ((Sender*)data)->kind;
            CSSStyleDeclaration* self = ((Sender*)data)->self;
            bool isImportant = ((Sender*)data)->isImportant;
            if (kind == CSSStyleValuePair::KeyKind::Unknown) {
            } else {
                if (false) {
                }
#define SET_ATTR(name, nameLower, nameCSSCase)         \
    else if (kind == CSSStyleValuePair::KeyKind::name) \
    {                                                  \
        self->set##name(buf, len, isImportant);        \
    }
                FOR_EACH_STYLE_ATTRIBUTE_TOTAL(SET_ATTR)
#undef SET_ATTR
            }
            return 0;
        },
        &sender);
}

String* CSSStyleDeclaration::cssText() const
{
    return generateCSSText();
}

void CSSStyleDeclaration::setCssText(String* text)
{
}

Nullable<String*> CSSStyleDeclaration::defaultNamedGetter(String* name)
{
    auto str = name->toNullableUTF8String();
    CSSStyleValuePair::KeyKind kind =
        lookupCSSStyleCamelCase(str.m_buffer, str.m_bufferSize);

    if (kind == CSSStyleValuePair::KeyKind::Unknown) {
        kind = lookupCSSStyle(str.m_buffer, str.m_bufferSize);
    }
    if (kind == CSSStyleValuePair::KeyKind::Unknown) {
        return Nullable<String*>();
    }
    if (false) {
    }
#define GET_ATTR(name, ...)                            \
    else if (kind == CSSStyleValuePair::KeyKind::name) \
    {                                                  \
        return Nullable<String*>(name());              \
    }
    FOR_EACH_STYLE_ATTRIBUTE_TOTAL(GET_ATTR)
#undef GET_ATTR
    return Nullable<String*>();
}

void CSSStyleDeclaration::defaultNamedEnumerator(GCVector<String*>& enums)
{
#define ENUM_ATTR(name, nameLower, ...) \
    enums.push_back(String::createASCIIString(#nameLower));
    FOR_EACH_STYLE_ATTRIBUTE_TOTAL(ENUM_ATTR)
#undef ENUM_ATTR
}

bool CSSStyleDeclaration::defaultNamedSetter(String* name,
                                             Nullable<String*> value)
{
    auto str = name->toNullableUTF8String();
    CSSStyleValuePair::KeyKind kind =
        lookupCSSStyleCamelCase(str.m_buffer, str.m_bufferSize);

    if (kind == CSSStyleValuePair::KeyKind::Unknown) {
        kind = lookupCSSStyle(str.m_buffer, str.m_bufferSize);
    }
    if (kind == CSSStyleValuePair::KeyKind::Unknown) {
        return false;
    }
    if (UNLIKELY(isComputedStyle())) {
        throw new DOMException(m_node->document(), DOMException::DOM_EXCEPTION,
                               "Computed property is read-only");
    }
    // Empty string let setter remove its value
    String* valueTo = String::emptyString;
    if (value.hasValue()) {
        valueTo = value.getValue();
    }

    struct Sender {
        CSSStyleDeclaration* self;
        CSSStyleValuePair::KeyKind kind;
    } sender;
    sender.self = this;
    sender.kind = kind;

    valueTo->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
            CSSStyleValuePair::KeyKind kind = ((Sender*)data)->kind;
            CSSStyleDeclaration* self = ((Sender*)data)->self;
            if (false) {
            }
#define SET_ATTR(name, ...)                            \
    else if (kind == CSSStyleValuePair::KeyKind::name) \
    {                                                  \
        self->set##name(buf, len, false);              \
    }
            FOR_EACH_STYLE_ATTRIBUTE_TOTAL(SET_ATTR)
#undef SET_ATTR
            return 0;
        },
        &sender);
    return true;
}

String* CSSStyleDeclaration::cssTextAffectedByAllProperty(
    const size_t& pos) const
{
    String* value;
    CSSStyleValuePair pair = m_cssValues[pos];
    switch (pair.valueKind()) {
    case CSSStyleValuePair::ValueKind::Initial:
        value = String::createASCIIString("initial");
        break;
    case CSSStyleValuePair::ValueKind::Inherit:
        value = String::createASCIIString("inherit");
        break;
    case CSSStyleValuePair::ValueKind::Unset:
        value = String::createASCIIString("unset");
        break;
    default:
        value = String::emptyString;
        break;
    }
    bool isImportant = pair.flagImportant();
    StringBuilder txt;
#define APPEND_CSS_VALUES(Name, name, cssname)                                 \
    {                                                                          \
        CSSStyleValuePair::KeyKind kind =                                      \
            lookupCSSStyle(cssname, strlen(cssname));                          \
        if (kind != CSSStyleValuePair::KeyKind::All &&                         \
            kind != CSSStyleValuePair::KeyKind::Direction &&                   \
            kind != CSSStyleValuePair::KeyKind::UnicodeBidi) {                 \
            txt.appendString(cssname);                                         \
            txt.appendString(": ");                                            \
            auto iter =                                                        \
                std::find_if(m_cssValues.begin() + pos, m_cssValues.end(),     \
                             [kind](CSSStyleValuePair p) {                     \
                                 return (static_cast<int>(p.keyKind()) - 1) == \
                                        (static_cast<int>(kind) - 2);          \
                             });                                               \
            if (iter != m_cssValues.end()) {                                   \
                txt.appendString(iter->toString());                            \
            } else {                                                           \
                txt.appendString(value);                                       \
            }                                                                  \
            if (isImportant || iter->flagImportant()) {                        \
                txt.appendString(" !important");                               \
            }                                                                  \
            txt.appendString("; ");                                            \
        }                                                                      \
    }
    FOR_EACH_STYLE_ATTRIBUTE_TOTAL(APPEND_CSS_VALUES)
#undef APPEND_CSS_VALUES

    return txt.finalize();
}

String* CSSStyleDeclaration::generateCSSText() const
{
    size_t pos =
        std::find_if(m_cssValues.begin(), m_cssValues.end(),
                     [](CSSStyleValuePair p) {
                         return p.keyKind() == CSSStyleValuePair::KeyKind::All;
                     }) -
        m_cssValues.begin();

    bool hasAllProperty = false;
    bool hasImportantAllProperty = false;
    if (pos < m_cssValues.size()) {
        hasAllProperty = true;
        hasImportantAllProperty = m_cssValues[pos].flagImportant();
    }

    if (hasAllProperty && (pos + 1) < m_cssValues.size() &&
        !hasImportantAllProperty) {
        return cssTextAffectedByAllProperty(pos);
    }

    StringBuilder txt;
    auto itValue = m_cssValues.begin();
    if (hasAllProperty) {
        itValue += pos;
    }
    for (; itValue != m_cssValues.end(); itValue++) {
        if (hasImportantAllProperty) {
            if (itValue->flagImportant()) {
                txt.appendString(itValue->keyName());
                txt.appendString(": ");
                txt.appendString(itValue->toString());
                if (itValue->flagImportant()) {
                    txt.appendString(" !important");
                }
                txt.appendString("; ");
            }
        } else {
            txt.appendString(itValue->keyName());
            txt.appendString(": ");
            txt.appendString(itValue->toString());
            if (itValue->flagImportant()) {
                txt.appendString(" !important");
            }
            txt.appendString("; ");
        }
    }
    return txt.finalize();
}

String* CSSStyleDeclaration::Background()
{
    StringBuilder builder;
    String* images = BackgroundImage();
    String* positions = BackgroundPosition();
    String* sizes = BackgroundSize();
    String* repeats = BackgroundRepeat();
    String* attachments = BackgroundAttachment();
    String* origins = BackgroundOrigin();
    String* clips = BackgroundClip();
    String* color = BackgroundColor();
    GCVector<StringView> vImages, vPositions, vSizes, vRepeats, vAttachments,
        vOrigins, vClips;
    StringUtils::tokenize(images, ",", 1, vImages);
    StringUtils::tokenize(positions, ",", 1, vPositions);
    StringUtils::tokenize(sizes, ",", 1, vSizes);
    StringUtils::tokenize(repeats, ",", 1, vRepeats);
    StringUtils::tokenize(attachments, ",", 1, vAttachments);
    StringUtils::tokenize(origins, ",", 1, vOrigins);
    StringUtils::tokenize(clips, ",", 1, vClips);

    size_t max = vImages.size();
    if (max < vPositions.size()) {
        max = vPositions.size();
    }
    if (max < vSizes.size()) {
        max = vSizes.size();
    }
    if (max < vRepeats.size()) {
        max = vRepeats.size();
    }
    if (max < vAttachments.size()) {
        max = vAttachments.size();
    }
    if (max < vOrigins.size()) {
        max = vOrigins.size();
    }
    if (max < vClips.size()) {
        max = vClips.size();
    }

    for (unsigned int i = 0; i < max; i++) {
        String* image =
            (i < vImages.size()) ? vImages[i].trim() : String::emptyString;
        String* position = (i < vPositions.size()) ? vPositions[i].trim()
                                                   : String::emptyString;
        String* size =
            (i < vSizes.size()) ? vSizes[i].trim() : String::emptyString;
        String* repeat =
            (i < vRepeats.size()) ? vRepeats[i].trim() : String::emptyString;
        String* attachment = (i < vAttachments.size()) ? vAttachments[i].trim()
                                                       : String::emptyString;
        String* origin =
            (i < vOrigins.size()) ? vOrigins[i].trim() : String::emptyString;
        String* clip =
            (i < vClips.size()) ? vClips[i].trim() : String::emptyString;
        builder.appendString(
            printBackground(image, position, size, repeat, attachment, origin,
                            clip, i == max - 1 ? color : String::emptyString));
        if (i != max - 1) {
            builder.appendString(", ");
        }
    }
    return builder.finalize();
}

void CSSStyleDeclaration::setBackground(const char* value, size_t length,
                                        bool isImportant)
{
    if (length == 0) {
        removeBackgroundCSSValuePairs(this);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length, ",/", 2);
    if (tokens.size() == 0) {
        return;
    }

    // TODO: should check comma-separated input
    CSSStyleValuePair v, color, image, repeatX, repeatY, positionX, positionY,
        size, attachment, origin, clip;
    if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addBackgroundCSSValuePairs(this, v, v, v, v, v, v, v, v, v, v);
    } else {
#define APPEND_NEW_LAYER(PROP, NEWPROP)                                     \
    if (PROP.valueKind() != CSSStyleValuePair::ValueKind::ValueListKind) {  \
        CSSStyleValuePair tmp = PROP;                                       \
        PROP.setValueList(                                                  \
            new ValueList(ValueList::Separator::CommaSeparator));           \
        PROP.multiValue()->push_back(tmp);                                  \
    }                                                                       \
    if (NEWPROP.valueKind() == CSSStyleValuePair::ValueKind::ValueListKind) \
        PROP.multiValue()->push_back((*NEWPROP.multiValue())[0]);           \
    else {                                                                  \
        PROP.multiValue()->push_back(NEWPROP);                              \
    }

        CSSTokenVector layer;
        unsigned int cntLayer = 0;
        for (unsigned int t = 0; t < tokens.size(); t++) {
            const CSSTokenValue& val = tokens[t];
            if (val.equals(",")) {
                if (t == 0 || t == tokens.size() - 1)
                    return;
            } else if (t == tokens.size() - 1) {
                layer.push_back(tokens[t]);
            } else {
                layer.push_back(tokens[t]);
                continue;
            }

            if (layer.size() == 0) {
                return;
            }
            CSSStyleValuePair layerImage, layerRepeatX, layerRepeatY,
                layerPositionX, layerPositionY, layerSize, layerAttachment,
                layerOrigin, layerClip;
            // NOTE: Color is allowed only for the last background layer (t ==
            // tokens->size() - 1)
            if (!parseBackgroundShorthand(
                    layer, &color, &layerImage, &layerRepeatX, &layerRepeatY,
                    &layerPositionX, &layerPositionY, &layerSize,
                    &layerAttachment, &layerOrigin, &layerClip,
                    t == tokens.size() - 1)) {
                return;
            }
            if (cntLayer == 0) {
                image = layerImage;
                repeatX = layerRepeatX;
                repeatY = layerRepeatY;
                positionX = layerPositionX;
                positionY = layerPositionY;
                size = layerSize;
                attachment = layerAttachment;
                origin = layerOrigin;
                clip = layerClip;
            } else {
                APPEND_NEW_LAYER(image, layerImage);
                APPEND_NEW_LAYER(repeatX, layerRepeatX);
                APPEND_NEW_LAYER(repeatY, layerRepeatY);
                APPEND_NEW_LAYER(positionX, layerPositionX);
                APPEND_NEW_LAYER(positionY, layerPositionY);
                APPEND_NEW_LAYER(size, layerSize);
                APPEND_NEW_LAYER(attachment, layerAttachment);
                APPEND_NEW_LAYER(origin, layerOrigin);
                APPEND_NEW_LAYER(clip, layerClip);
            }
            cntLayer++;
            layer.clear();
        }
        color.setFlagImportant(isImportant);
        image.setFlagImportant(isImportant);
        repeatX.setFlagImportant(isImportant);
        repeatY.setFlagImportant(isImportant);
        positionX.setFlagImportant(isImportant);
        positionY.setFlagImportant(isImportant);
        size.setFlagImportant(isImportant);
        attachment.setFlagImportant(isImportant);
        origin.setFlagImportant(isImportant);
        clip.setFlagImportant(isImportant);
        addBackgroundCSSValuePairs(this, color, image, repeatX, repeatY,
                                   positionX, positionY, size, attachment,
                                   origin, clip);
    }
#undef APPEND_NEW_LAYER
}

bool CSSStyleDeclaration::parseBackgroundPositionShorthand(
    const CSSTokenVector& tokens, CSSStyleValuePair* retx,
    CSSStyleValuePair* rety, bool allowComma)
{
    // [ [ <percentage> | <length> | left | center | right ] [ <percentage> |
    // <length> | top | center | bottom ]? ] | [ [ left | center | right ] || [
    // top | center | bottom ] ] | inherit
    size_t len = 0;
    retx->setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
    rety->setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
    retx->setValueList(new ValueList(ValueList::Separator::SpaceSeparator));
    rety->setValueList(new ValueList(ValueList::Separator::SpaceSeparator));

    for (unsigned int i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& value = tokens[i];
        if (value.equals(",")) {
            if (!allowComma || i == 0 || i == tokens.size() - 1) {
                return false;
            }
        } else if (i == tokens.size() - 1) {
            len++;
            i++;
        } else {
            len++;
            continue;
        }

        CSSStyleValuePair ret;
        CSSStyleValuePair x, y;
        if (len == 1) {
            const CSSTokenValue& tok = tokens[i - 1];
            if (x.updateValueUnitBackgroundPositionX(tok)) {
                y = CSSStyleValuePair(
                    CSSStyleValuePair::ValueKind::SideValueKind,
                    SideValue::CenterSideValue);
            } else if (y.updateValueUnitBackgroundPositionY(tok)) {
                x = CSSStyleValuePair(
                    CSSStyleValuePair::ValueKind::SideValueKind,
                    SideValue::CenterSideValue);
            } else {
                return false;
            }
        } else if (len == 2) {
            // !!! NOTICE !!!
            // background-position: right bottom <- (O) [enum-x enum-y]
            // background-position: bottom right <- (O) [enum-y enum-x] !!!
            // background-position: 20px 20px    <- (O) [length length]
            // background-position: right 20px   <- (O) [enum-x length]
            // background-position: 20px bottom  <- (O) [length enum-y]
            // background-position: bottom 20px  <- (X) [enum-y length] !!!
            // background-position: 20px right   <- (X) [length enum-x] !!!
            bool checker = true;
            const CSSTokenValue& tok1 = tokens[i - 2];
            const CSSTokenValue& tok2 = tokens[i - 1];
            CSSStyleValuePair::ValueKind sideKind =
                CSSStyleValuePair::ValueKind::SideValueKind;
            checker &= x.updateValueUnitBackgroundPositionX(tok1);
            checker &= y.updateValueUnitBackgroundPositionY(tok2);

            if (!checker && x.valueKind() == sideKind &&
                y.valueKind() == sideKind) {
                checker = true;
                checker &= x.updateValueUnitBackgroundPositionX(tok2);
                checker &= y.updateValueUnitBackgroundPositionY(tok1);
            }
            if (!checker) {
                return false;
            }
        } else {
            return false;
        }
        retx->multiValue()->push_back(x);
        rety->multiValue()->push_back(y);
        len = 0;
    }

    return true;
}

String* CSSStyleDeclaration::BackgroundPosition()
{
    String* positionX = BackgroundPositionX();
    String* positionY = BackgroundPositionY();

    if (positionX->equals(String::emptyString) ||
        positionY->equals(String::emptyString)) {
        return String::emptyString;
    }

    GCVector<StringView> vPositionX, vPositionY;
    StringUtils::tokenize(positionX, ",", 1, vPositionX);
    StringUtils::tokenize(positionY, ",", 1, vPositionY);

    StringBuilder builder;
    for (size_t i = 0; i < vPositionX.size(); i++) {
        String* pX = vPositionX[i].trim();
        String* pY = vPositionY[i].trim();
        if (pX->equals(String::initialString)) {
            if (pY->equals(String::initialString)) {
                builder.appendString(String::initialString);
            } else {
                builder.appendString(String::emptyString);
            }
        } else if (pX->equals(String::inheritString)) {
            if (pY->equals(String::inheritString)) {
                builder.appendString(String::inheritString);
            } else {
                builder.appendString(String::emptyString);
            }
        } else {
            builder.appendString(pX);
            builder.appendString(String::spaceString);
            builder.appendString(pY);
        }
        if (i != vPositionX.size() - 1) {
            builder.appendChar(',');
            builder.appendString(String::spaceString);
        }
    }
    return builder.finalize();
}

void CSSStyleDeclaration::setBackgroundPosition(const char* value,
                                                size_t length, bool isImportant)
{
    if (value == 0) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundPositionX);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundPositionY);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length, ",", 1);

    CSSStyleValuePair c, x, y;
    if (c.updateValueCommon(tokens)) {
        c.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundPositionX, c);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundPositionY, c);
    } else if (parseBackgroundPositionShorthand(tokens, &x, &y)) {
        x.setFlagImportant(isImportant);
        y.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundPositionX, x);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundPositionY, y);
    }
}

String* CSSStyleDeclaration::BackgroundRepeat()
{
    String* repeatX = BackgroundRepeatX();
    String* repeatY = BackgroundRepeatY();
    GCVector<StringView> vRepeatX, vRepeatY;
    StringUtils::tokenize(repeatX, ",", 1, vRepeatX);
    StringUtils::tokenize(repeatY, ",", 1, vRepeatY);

    StringBuilder builder;
    size_t size = std::min(vRepeatX.size(), vRepeatY.size());
    for (size_t i = 0; i < size; i++) {
        String* rX = vRepeatX[i].trim();
        String* rY = vRepeatY[i].trim();
        if (rX->equals(rY)) {
            builder.appendString(rX);
        } else if (rX->equals("repeat") && rY->equals("no-repeat")) {
            builder.appendString("repeat-x");
        } else if (rX->equals("no-repeat") && rY->equals("repeat")) {
            builder.appendString("repeat-y");
        } else {
            return builder.finalize();
        }
        if (i != size - 1) {
            builder.appendChar(',');
            builder.appendString(String::spaceString);
        }
    }
    return builder.finalize();
}

void CSSStyleDeclaration::setBackgroundRepeat(const char* value, size_t length,
                                              bool isImportant)
{
    if (value == 0) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatX);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatY);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length, ",", 1);

    CSSStyleValuePair c, x, y;
    if (c.updateValueCommon(tokens)) {
        c.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatX, c);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatY, c);
    } else if (parseBackgroundRepeatShorhand(tokens, &x, &y)) {
        x.setFlagImportant(isImportant);
        y.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatX, x);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatY, y);
    }
}

String* CSSStyleDeclaration::Border()
{
    bool isWidthCombined;
    String* width = BorderWidth(&isWidthCombined);
    bool isStyleCombined;
    String* style = BorderStyle(&isStyleCombined);
    bool isColorCombined;
    String* color = BorderColor(&isColorCombined);
    return BorderString(width, isWidthCombined, style, isStyleCombined, color,
                        isColorCombined);
}

void CSSStyleDeclaration::setBorder(const char* value, size_t len,
                                    bool isImportant)
{
    if (len == 0) {
        removeBorderCSSValuePairs(this);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len);

    CSSStyleValuePair v, width, style, color;
    if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addBorderCSSValuePairs(this, v, v, v);
    } else if (parseBorderShorthand(tokens, &width, &style, &color)) {
        width.setFlagImportant(isImportant);
        style.setFlagImportant(isImportant);
        color.setFlagImportant(isImportant);
        addBorderCSSValuePairs(this, width, style, color);
    }
}

String* CSSStyleDeclaration::BorderColor(bool* isCombined)
{
    GEN_ATTRIBUTE_GETTER_FOURSIDE(Border, Color)
}

void CSSStyleDeclaration::setBorderColor(const char* value, size_t length,
                                         bool isImportant)
{
    GEN_ATTRIBUTE_SETTER_FOURSIDE(Border, Color)
}

String* CSSStyleDeclaration::BorderStyle(bool* isCombined)
{
    GEN_ATTRIBUTE_GETTER_FOURSIDE(Border, Style)
}

void CSSStyleDeclaration::setBorderStyle(const char* value, size_t length,
                                         bool isImportant)
{
    GEN_ATTRIBUTE_SETTER_FOURSIDE(Border, Style)
}

String* CSSStyleDeclaration::BorderWidth(bool* isCombined)
{
    GEN_ATTRIBUTE_GETTER_FOURSIDE(Border, Width)
}

void CSSStyleDeclaration::setBorderWidth(const char* value, size_t length,
                                         bool isImportant)
{
    GEN_ATTRIBUTE_SETTER_FOURSIDE(Border, Width)
}

String* CSSStyleDeclaration::BorderTop()
{
    GEN_ATTRIBUTE_GETTER_BORDER(Top)
}

void CSSStyleDeclaration::setBorderTop(const char* value, size_t len,
                                       bool isImportant)
{
    GEN_ATTRIBUTE_SETTER_BORDER(Top)
}

String* CSSStyleDeclaration::BorderRight()
{
    GEN_ATTRIBUTE_GETTER_BORDER(Right)
}

void CSSStyleDeclaration::setBorderRight(const char* value, size_t len,
                                         bool isImportant)
{
    GEN_ATTRIBUTE_SETTER_BORDER(Right)
}

String* CSSStyleDeclaration::BorderBottom()
{
    GEN_ATTRIBUTE_GETTER_BORDER(Bottom)
}

void CSSStyleDeclaration::setBorderBottom(const char* value, size_t len,
                                          bool isImportant)
{
    GEN_ATTRIBUTE_SETTER_BORDER(Bottom)
}

String* CSSStyleDeclaration::BorderLeft()
{
    GEN_ATTRIBUTE_GETTER_BORDER(Left)
}

void CSSStyleDeclaration::setBorderLeft(const char* value, size_t len,
                                        bool isImportant)
{
    GEN_ATTRIBUTE_SETTER_BORDER(Left)
}

String* CSSStyleDeclaration::BorderRadius()
{
    String* tl = BorderTopLeftRadius();
    String* tr = BorderTopRightRadius();
    String* br = BorderBottomRightRadius();
    String* bl = BorderBottomLeftRadius();

    size_t stl = tl->indexOf(' ');
    size_t str = tr->indexOf(' ');
    size_t sbr = br->indexOf(' ');
    size_t sbl = bl->indexOf(' ');

    bool needsSlash = (stl != SIZE_MAX) || (str != SIZE_MAX) ||
                      (sbr != SIZE_MAX) || (sbl != SIZE_MAX);
    StringBuilder sb;

    if (needsSlash) {
        String* tl2;
        if (stl != SIZE_MAX) {
            tl2 = tl->substring(0, stl);
        } else {
            tl2 = tl;
        }

        String* tr2;
        if (str != SIZE_MAX) {
            tr2 = tr->substring(0, str);
        } else {
            tr2 = tr;
        }

        String* br2;
        if (sbr != SIZE_MAX) {
            br2 = br->substring(0, sbr);
        } else {
            br2 = br;
        }

        String* bl2;
        if (sbl != SIZE_MAX) {
            bl2 = bl->substring(0, sbl);
        } else {
            bl2 = bl;
        }
        mergeBordeRadiusString(sb, tl2, tr2, br2, bl2);
        sb.appendString(" / ");

        if (stl != SIZE_MAX) {
            tl2 = tl->substring(stl + 1, tl->length() - (stl + 1));
        } else {
            tl2 = tl;
        }

        if (str != SIZE_MAX) {
            tr2 = tr->substring(str + 1, tr->length() - (str + 1));
        } else {
            tr2 = tr;
        }

        if (sbr != SIZE_MAX) {
            br2 = br->substring(sbr + 1, br->length() - (sbr + 1));
        } else {
            br2 = br;
        }

        if (sbl != SIZE_MAX) {
            bl2 = bl->substring(sbl + 1, bl->length() - (sbl + 1));
        } else {
            bl2 = bl;
        }
        mergeBordeRadiusString(sb, tl2, tr2, br2, bl2);
    } else {
        mergeBordeRadiusString(sb, tl, tr, br, bl);
    }

    return sb.finalize();
}

void CSSStyleDeclaration::setBorderRadius(const char* value, size_t len,
                                          bool isImportant)
{
    removeCSSValuePair(CSSStyleValuePair::BorderTopLeftRadius);
    removeCSSValuePair(CSSStyleValuePair::BorderTopRightRadius);
    removeCSSValuePair(CSSStyleValuePair::BorderBottomRightRadius);
    removeCSSValuePair(CSSStyleValuePair::BorderBottomLeftRadius);
    if (len == 0) {
        return;
    }
    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len, "/", 1);

    // {1~4} / {1~4}
    if (tokens.size() < 1 || tokens.size() > 9) {
        return;
    }

    bool seenSlash = false;
    size_t beforeSlash = 0;
    size_t afterSlash = 0;

    for (size_t i = 0; i < tokens.size(); i++) {
        const auto& t = tokens[i];
        if (t.equals("/")) {
            if (seenSlash) {
                return;
            }
            if (i == 0) {
                return;
            }
            seenSlash = true;
        } else {
            if (seenSlash) {
                afterSlash++;
            } else {
                beforeSlash++;
            }
            if (beforeSlash > 4 || afterSlash > 4) {
                return;
            }
        }
    }

    if (seenSlash && afterSlash == 0) {
        return;
    }

    CSSTokenVector topLeftV;
    CSSTokenVector topRightV;
    CSSTokenVector bottomRightV;
    CSSTokenVector bottomLeftV;

    if (beforeSlash == 1) {
        topLeftV.push_back(tokens[0]);
        topRightV.push_back(tokens[0]);
        bottomRightV.push_back(tokens[0]);
        bottomLeftV.push_back(tokens[0]);
    } else if (beforeSlash == 2) {
        topLeftV.push_back(tokens[0]);
        topRightV.push_back(tokens[1]);
        bottomRightV.push_back(tokens[0]);
        bottomLeftV.push_back(tokens[1]);
    } else if (beforeSlash == 3) {
        topLeftV.push_back(tokens[0]);
        topRightV.push_back(tokens[1]);
        bottomRightV.push_back(tokens[2]);
        bottomLeftV.push_back(tokens[1]);
    } else {
        STARFISH_ASSERT(beforeSlash == 4);
        topLeftV.push_back(tokens[0]);
        topRightV.push_back(tokens[1]);
        bottomRightV.push_back(tokens[2]);
        bottomLeftV.push_back(tokens[3]);
    }

    size_t base = 1 + beforeSlash;
    if (afterSlash == 1) {
        topLeftV.push_back(tokens[base]);
        topRightV.push_back(tokens[base]);
        bottomRightV.push_back(tokens[base]);
        bottomLeftV.push_back(tokens[base]);
    } else if (afterSlash == 2) {
        topLeftV.push_back(tokens[base]);
        topRightV.push_back(tokens[base + 1]);
        bottomRightV.push_back(tokens[base]);
        bottomLeftV.push_back(tokens[base + 1]);
    } else if (afterSlash == 3) {
        topLeftV.push_back(tokens[base]);
        topRightV.push_back(tokens[base + 1]);
        bottomRightV.push_back(tokens[base + 2]);
        bottomLeftV.push_back(tokens[base + 1]);
    } else if (afterSlash == 4) {
        topLeftV.push_back(tokens[base + 0]);
        topRightV.push_back(tokens[base + 1]);
        bottomRightV.push_back(tokens[base + 2]);
        bottomLeftV.push_back(tokens[base + 3]);
    }

    CSSStyleValuePair topLeft;
    CSSStyleValuePair topRight;
    CSSStyleValuePair bottomRight;
    CSSStyleValuePair bottomLeft;

    if (!topLeft.updateValueBorderRadius(topLeftV) ||
        !topRight.updateValueBorderRadius(topRightV) ||
        !bottomRight.updateValueBorderRadius(bottomRightV) ||
        !bottomLeft.updateValueBorderRadius(bottomLeftV)) {
        return;
    }

    topLeft.setFlagImportant(isImportant);
    topRight.setFlagImportant(isImportant);
    bottomRight.setFlagImportant(isImportant);
    bottomLeft.setFlagImportant(isImportant);
    addCSSValuePair(CSSStyleValuePair::BorderTopLeftRadius, topLeft);
    addCSSValuePair(CSSStyleValuePair::BorderTopRightRadius, topRight);
    addCSSValuePair(CSSStyleValuePair::BorderBottomRightRadius, bottomRight);
    addCSSValuePair(CSSStyleValuePair::BorderBottomLeftRadius, bottomLeft);
}

String* CSSStyleDeclaration::BorderImage()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return String::emptyString;
}

void CSSStyleDeclaration::setBorderImage(const char* value, size_t length,
                                         bool isImportant)
{
    if (length == 0) {
        removeBorderImageCSSValuePairs(this);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length, "/", 1);

    CSSStyleValuePair v, source, slice, width, outset, repeat;
    if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addBorderImageCSSValuePairs(this, v, v, v, v, v);
    } else if (parseBorderImageShorthand(tokens, &source, &slice, &width,
                                         &outset, &repeat)) {
        source.setFlagImportant(isImportant);
        slice.setFlagImportant(isImportant);
        width.setFlagImportant(isImportant);
        outset.setFlagImportant(isImportant);
        repeat.setFlagImportant(isImportant);
        addBorderImageCSSValuePairs(this, source, slice, width, outset, repeat);
    }
}

void CSSStyleDeclaration::setD(const char* value, size_t len, bool isImportant)
{
    if (len == 0) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::D);
        return;
    }

    if (VALUE_IS_INHERIT()) {
        CSSStyleValuePair pair;
        pair.setFlagImportant(isImportant);
        pair.setKeyKind(CSSStyleValuePair::KeyKind::D);
        pair.setValueKind(CSSStyleValuePair::ValueKind::Inherit);
        addCSSValuePair(CSSStyleValuePair::KeyKind::D, pair);
        return;
    }

    Nullable<CSSTokenValue> mayFunctionBlock =
        CSSPropertyParser::parseFunctionBlock(value, "path");
    if (!mayFunctionBlock.hasValue()) {
        return;
    }
    Nullable<CSSTokenValue> mayQuoteBlock =
        CSSPropertyParser::parseQuoteBlock(mayFunctionBlock.getValue().data());
    if (!mayQuoteBlock.hasValue()) {
        return;
    }
    // TODO validate function content for D property
    CSSStyleValuePair pair;
    pair.setPathFunctionValue(mayQuoteBlock.getValue().toGCString());
    addCSSValuePair(CSSStyleValuePair::KeyKind::D, pair);
}

String* CSSStyleDeclaration::Flex()
{
    String* flexGrow = FlexGrow();
    String* flexShrink = FlexShrink();
    String* flexBasis = FlexBasis();

    String* space = String::spaceString;
    StringBuilder builder;
    builder.appendString(flexGrow);
    builder.appendString(space);
    builder.appendString(flexShrink);
    builder.appendString(space);
    builder.appendString(flexBasis);

    return builder.finalize();
}

void CSSStyleDeclaration::setFlex(const char* str, size_t length,
                                  bool isImportant)
{
    if (length == 0) {
        removeFlexCSSValuePairs(this);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, str, length);
    const CSSTokenValue& value = tokens[0];

    // TODO comma separation
    CSSStyleValuePair v, flexGrow, flexShrink, flexBasis;
    float f;
    if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addFlexCSSValuePairs(this, v, v, v);
    } else if (STRING_VALUE_IS_AUTO()) {
        flexGrow.setFlagImportant(isImportant);
        flexShrink.setFlagImportant(isImportant);
        flexBasis.setFlagImportant(isImportant);
        flexGrow.setValueKind(CSSStyleValuePair::ValueKind::Number);
        flexGrow.setValue(1.0f);
        flexShrink.setValueKind(CSSStyleValuePair::ValueKind::Number);
        flexShrink.setValue(1.0f);
        flexBasis.setValueKind(CSSStyleValuePair::ValueKind::Auto);
        addFlexCSSValuePairs(this, flexGrow, flexShrink, flexBasis);
    } else if (STRING_VALUE_IS_NONE()) {
        flexGrow.setFlagImportant(isImportant);
        flexShrink.setFlagImportant(isImportant);
        flexBasis.setFlagImportant(isImportant);
        flexGrow.setValueKind(CSSStyleValuePair::ValueKind::Number);
        flexGrow.setValue(0.0f);
        flexShrink.setValueKind(CSSStyleValuePair::ValueKind::Number);
        flexShrink.setValue(0.0f);
        flexBasis.setValueKind(CSSStyleValuePair::ValueKind::Auto);
        addFlexCSSValuePairs(this, flexGrow, flexShrink, flexBasis);
    } else if (parseFlexShorthand(tokens, &flexGrow, &flexShrink, &flexBasis)) {
        flexGrow.setFlagImportant(isImportant);
        flexShrink.setFlagImportant(isImportant);
        flexBasis.setFlagImportant(isImportant);
        addFlexCSSValuePairs(this, flexGrow, flexShrink, flexBasis);
    }
}

void CSSStyleDeclaration::setFlexFlow(const char* value, size_t length,
                                      bool isImportant)
{
    if (length == 0) {
        removeFlexFlowCSSValuePairs(this);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length);

    // TODO comma separation
    CSSStyleValuePair v, flexDirection, flexWrap;
    if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addFlexFlowCSSValuePairs(this, v, v);
    } else if (parseFlexFlowShorthand(tokens, &flexDirection, &flexWrap)) {
        flexDirection.setFlagImportant(isImportant);
        flexWrap.setFlagImportant(isImportant);
        addFlexFlowCSSValuePairs(this, flexDirection, flexWrap);
    }
}

String* CSSStyleDeclaration::FlexFlow()
{
    String* flexDirection = FlexDirection();
    String* flexWrap = FlexWrap();

    String* space = String::spaceString;
    StringBuilder builder;
    builder.appendString(flexDirection);
    builder.appendString(space);
    builder.appendString(flexWrap);

    return builder.finalize();
}

String* CSSStyleDeclaration::Font()
{
    String* style = FontStyle();
    String* weight = FontWeight();
    String* size = FontSize();
    String* lineHeight = LineHeight();

    if (style->length() == 0 || weight->length() == 0 || size->length() == 0 ||
        lineHeight->length() == 0) {
        return String::emptyString;
    }
    int maxCount = 4;
    int initialCount = 0;
    int inheritCount = 0;
    initialCount += (style->equals(String::initialString) ? 1 : 0);
    initialCount += (weight->equals(String::initialString) ? 1 : 0);
    initialCount += (size->equals(String::initialString) ? 1 : 0);
    initialCount += (lineHeight->equals(String::inheritString) ? 1 : 0);
    inheritCount += (style->equals(String::inheritString) ? 1 : 0);
    inheritCount += (weight->equals(String::inheritString) ? 1 : 0);
    inheritCount += (size->equals(String::inheritString) ? 1 : 0);
    inheritCount += (lineHeight->equals(String::inheritString) ? 1 : 0);

    if (initialCount == maxCount) {
        return String::initialString;
    }
    if (inheritCount == maxCount) {
        return String::inheritString;
    }
    if (initialCount > 0 || inheritCount > 0) {
        return String::emptyString;
    }

    StringBuilder builder;
    // 1. style
    if (!style->equals("normal")) {
        builder.appendString(style);
    }
    // 2. weight
    if (!weight->equals("normal")) {
        if (builder.contentLength() > 0) {
            builder.appendString(String::spaceString);
        }
        builder.appendString(weight);
    }
    // 3. size
    if (builder.contentLength() > 0) {
        builder.appendString(String::spaceString);
    }
    builder.appendString(size);
    // 4. lineHeight
    if (!lineHeight->equals("normal")) {
        builder.appendString("/");
        builder.appendString(lineHeight);
    }
    return builder.finalize();
}

void CSSStyleDeclaration::setFont(const char* value, size_t length,
                                  bool isImportant)
{
    if (length == 0) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::FontFamily);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::FontStyle);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::FontWeight);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::FontSize);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::LineHeight);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length, "/,", 2);
    if (tokens.size() == 0) {
        return;
    }

    CSSStyleValuePair v, style /*, variant*/, weight /*, stretch*/, size,
        lineHeight, fontFamily;
    if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::FontFamily, v);
        addCSSValuePair(CSSStyleValuePair::KeyKind::FontStyle, v);
        addCSSValuePair(CSSStyleValuePair::KeyKind::FontWeight, v);
        addCSSValuePair(CSSStyleValuePair::KeyKind::FontSize, v);
        addCSSValuePair(CSSStyleValuePair::KeyKind::LineHeight, v);
        return;
    }

    tokens.clear();
    tokenizeCSSValue(tokens, value, length, "/,", 2, true);
    if (parseFontShorthand(tokens, &style, &weight, &size, &lineHeight,
                           &fontFamily)) {
        fontFamily.setFlagImportant(isImportant);
        style.setFlagImportant(isImportant);
        weight.setFlagImportant(isImportant);
        size.setFlagImportant(isImportant);
        lineHeight.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::FontFamily, fontFamily);
        addCSSValuePair(CSSStyleValuePair::KeyKind::FontStyle, style);
        addCSSValuePair(CSSStyleValuePair::KeyKind::FontWeight, weight);
        addCSSValuePair(CSSStyleValuePair::KeyKind::FontSize, size);
        addCSSValuePair(CSSStyleValuePair::KeyKind::LineHeight, lineHeight);
    }
}

void CSSStyleDeclaration::setFontFamily(const char* value, size_t len,
                                        bool isImportant)
{
    if (len == 0) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::FontFamily);
        return;
    }
    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len, ",", 1, true);
    CSSStyleValuePair ret;
    if (ret.updateValueCommon(tokens) || ret.updateValueFontFamily(tokens)) {
        ret.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::FontFamily, ret);
    }
}

String* CSSStyleDeclaration::ListStyle()
{
    String* t = ListStyleType();
    String* p = ListStylePosition();
    String* i = ListStyleImage();

    if (!t->length() || !p->length() || !i->length()) {
        return String::emptyString;
    }

    bool isTGlobal = (t == String::initialString) ||
                     (t == String::inheritString) || (t == String::unsetString);
    bool isPGlobal = (p == String::initialString) ||
                     (p == String::inheritString) || (p == String::unsetString);
    bool isIGlobal = (i == String::initialString) ||
                     (i == String::inheritString) || (i == String::unsetString);

    if (isTGlobal && isPGlobal && isIGlobal) {
        if (t == p && p == i) {
            // e.g. initial initial initial -> intial
            // e.g. inherit inherit inherit -> inherit
            // e.g. unset unset unset -> unset
            return t;
        }
        return String::emptyString;
    }

    // Sequence: position image type
    StringBuilder builder;
    builder.appendString(p);
    builder.appendString(String::spaceString);
    builder.appendString(i);
    builder.appendString(String::spaceString);
    builder.appendString(t);
    return builder.finalize();
}

void CSSStyleDeclaration::setListStyle(const char* value, size_t len,
                                       bool isImportant)
{
    if (len == 0) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::ListStyleType);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::ListStylePosition);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::ListStyleImage);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len, ",", 1);
    CSSStyleValuePair c, t, p, i;
    if (c.updateValueCommon(tokens)) {
        c.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::ListStyleType, c);
        addCSSValuePair(CSSStyleValuePair::KeyKind::ListStylePosition, c);
        addCSSValuePair(CSSStyleValuePair::KeyKind::ListStyleImage, c);
    } else if (parseListStyleShorhand(m_node->document(), tokens, &t, &p, &i)) {
        t.setFlagImportant(isImportant);
        p.setFlagImportant(isImportant);
        i.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::ListStyleType, t);
        addCSSValuePair(CSSStyleValuePair::KeyKind::ListStylePosition, p);
        addCSSValuePair(CSSStyleValuePair::KeyKind::ListStyleImage, i);
    }
}

String* CSSStyleDeclaration::Margin(bool* isCombined)
{
    GEN_ATTRIBUTE_GETTER_FOURSIDE(Margin)
}

void CSSStyleDeclaration::setMargin(const char* value, size_t length,
                                    bool isImportant)
{
    GEN_ATTRIBUTE_SETTER_FOURSIDE(Margin)
}

String* CSSStyleDeclaration::Outline()
{
    String* width = OutlineWidth();
    String* style = OutlineStyle();
    String* color = OutlineColor();
    return BorderString(width, false, style, false, color, false);
}

void CSSStyleDeclaration::setOutline(const char* value, size_t length,
                                     bool isImportant)
{
    if (length == 0) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::OutlineWidth);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::OutlineStyle);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::OutlineColor);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length);

    CSSStyleValuePair v, width, style, color;
    if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::OutlineWidth, v);
        addCSSValuePair(CSSStyleValuePair::KeyKind::OutlineStyle, v);
        addCSSValuePair(CSSStyleValuePair::KeyKind::OutlineColor, v);
    } else if (parseBorderShorthand(tokens, &width, &style, &color)) {
        width.setFlagImportant(isImportant);
        style.setFlagImportant(isImportant);
        color.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::OutlineWidth, width);
        addCSSValuePair(CSSStyleValuePair::KeyKind::OutlineStyle, style);
        addCSSValuePair(CSSStyleValuePair::KeyKind::OutlineColor, color);
    }
}

String* CSSStyleDeclaration::Overflow()
{
    // TODO: Should find the specific rule for composing overflow
    String* overflowX = OverflowX();
    String* overflowY = OverflowY();

    if (overflowX->equals(overflowY)) {
        return overflowX;
    } else {
        return String::createASCIIString("auto");
    }
}

void CSSStyleDeclaration::setOverflow(const char* value, size_t length,
                                      bool isImportant)
{
    if (length == 0) {
        removeOverflowCSSValuePairs(this);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length);

    // TODO comma separation
    CSSStyleValuePair v, overflowX, overflowY;
    if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addOverflowCSSValuePairs(this, v, v);
    } else if (parseOverflowShorthand(tokens, &overflowX, &overflowY)) {
        overflowX.setFlagImportant(isImportant);
        overflowY.setFlagImportant(isImportant);
        addOverflowCSSValuePairs(this, overflowX, overflowY);
    }
}

String* CSSStyleDeclaration::Padding(bool* isCombined)
{
    GEN_ATTRIBUTE_GETTER_FOURSIDE(Padding)
}

void CSSStyleDeclaration::setPadding(const char* value, size_t length,
                                     bool isImportant)
{
    GEN_ATTRIBUTE_SETTER_FOURSIDE(Padding)
}

void CSSStyleDeclaration::setSrc(const char* value, size_t len,
                                 bool isImportant)
{
    if (len == 0) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::Src);
        return;
    }
    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len, ",", 1, true);
    CSSStyleValuePair ret;
    if (ret.updateValueSrc(tokens)) {
        ret.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::Src, ret);
    }
}

String* CSSStyleDeclaration::TextDecoration()
{
    if (isComputedStyle()) {
        updateValue(CSSStyleValuePair::TextDecorationLine);
        updateValue(CSSStyleValuePair::TextDecorationStyle);
        updateValue(CSSStyleValuePair::TextDecorationColor);
    }

    StringBuilder b;
    for (unsigned i = 0; i < m_cssValues.size(); i++) {
        CSSStyleValuePair& p = m_cssValues[i];
        switch (p.keyKind()) {
        case CSSStyleValuePair::KeyKind::TextDecorationLine:
            b.appendString(p.toString());
            b.appendString(String::spaceString);
            break;
        case CSSStyleValuePair::KeyKind::TextDecorationStyle:
        case CSSStyleValuePair::KeyKind::TextDecorationColor:
            b.appendString(p.toString());
            b.appendString(String::spaceString);
            break;
        default:
            break;
        }
    }

    return b.finalize()->trim();
}

void CSSStyleDeclaration::setTextDecoration(const char* value, size_t len,
                                            bool isImportant)
{
    if (len == 0) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationLine);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationStyle);
        removeCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationColor);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len, "", 0);

    CSSStyleValuePair ret;
    if (ret.updateValueCommon(tokens)) {
        ret.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationLine, ret);
        addCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationStyle, ret);
        addCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationColor, ret);
        return;
    }

    bool doneStyle = false;
    bool doneColor = false;
    bool doneLine = false;
    CSSTokenVector singleTokenList;
    for (size_t i = 0; i < tokens.size(); i++) {
        singleTokenList.clear();
        singleTokenList.push_back(tokens[i]);

        CSSStyleValuePair p;
        p.setFlagImportant(isImportant);
        if (!doneStyle &&
            p.updateValueTextDecorationStyle(nullptr, singleTokenList)) {
            addCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationStyle, p);
            doneStyle = true;
        } else if (!doneColor &&
                   p.updateValueTextDecorationColor(nullptr, singleTokenList)) {
            addCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationColor, p);
            doneColor = true;
        } else if (!doneLine &&
                   p.updateValueTextDecorationLine(nullptr, singleTokenList)) {
            // text-decoration-line can accept consecutive multi-values
            std::set<TextDecorationLineValue> set;
            const CSSStyleValuePair& lineVal = p.multiValue()->at(0);
            set.insert(lineVal.textDecorationLineValue());
            for (size_t j = i + 1; j < tokens.size(); j++) {
                singleTokenList.clear();
                singleTokenList.push_back(tokens[j]);
                bool ok = false;
                CSSStyleValuePair tmpVal;
                if (tmpVal.updateValueTextDecorationLine(nullptr,
                                                         singleTokenList)) {
                    const CSSStyleValuePair& nextVal =
                        tmpVal.multiValue()->at(0);
                    if (set.find(nextVal.textDecorationLineValue()) ==
                        set.end()) {
                        i = j;
                        set.insert(nextVal.textDecorationLineValue());
                        p.multiValue()->push_back(nextVal);
                        ok = true;
                    }
                }

                if (!ok) {
                    break;
                }
            }
            addCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationLine, p);
            doneLine = true;
        } else {
            // Error: each style, color, and line can appear only once OR
            // an unknown token received
            removeCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationLine);
            removeCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationStyle);
            removeCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationColor);
            return;
        }
    }
}

void CSSStyleDeclaration::setTransitionProperty(const char* value,
                                                size_t length, bool isImportant)
{
    if (length == 0) {
        removeCSSValuePair(CSSStyleValuePair::TransitionProperty);
        return;
    }
    // TODO handle var() case
    CSSTokenVector layers;
    if (!CSSPropertyParser::parseLayers(value, length, layers)) {
        return;
    }
    ValueList* list = new ValueList(ValueList::CommaSeparator);
    size_t layerSize = layers.size();
    for (size_t i = 0; i < layerSize; i++) {
        CSSStyleValuePair sub;
        CSSTokenVector tokens;
        tokenizeCSSValue(tokens, layers[i].data(), layers[i].length());
        if (!(layerSize == 1 && sub.updateValueCommon(tokens)) &&
            !sub.updateValueLayerTransitionProperty(tokens)) {
            return;
        }
        list->push_back(sub);
    }
    CSSStyleValuePair result;
    result.setFlagImportant(isImportant);
    result.setValueList(list);
    addCSSValuePair(CSSStyleValuePair::TransitionProperty, result);
}

void CSSStyleDeclaration::setTransitionDuration(const char* value,
                                                size_t length, bool isImportant)
{
    if (length == 0) {
        removeCSSValuePair(CSSStyleValuePair::TransitionDuration);
        return;
    }
    // TODO handle var() case
    CSSTokenVector layers;
    if (!CSSPropertyParser::parseLayers(value, length, layers)) {
        return;
    }
    ValueList* list = new ValueList(ValueList::CommaSeparator);
    size_t layerSize = layers.size();
    for (size_t i = 0; i < layerSize; i++) {
        CSSStyleValuePair sub;
        CSSTokenVector tokens;
        tokenizeCSSValue(tokens, layers[i].data(), layers[i].length());
        if (!(layerSize == 1 && sub.updateValueCommon(tokens)) &&
            !sub.updateValueLayerTransitionDuration(tokens)) {
            return;
        }
        list->push_back(sub);
    }
    CSSStyleValuePair result;
    result.setFlagImportant(isImportant);
    result.setValueList(list);
    addCSSValuePair(CSSStyleValuePair::TransitionDuration, result);
}

void CSSStyleDeclaration::setTransitionTimingFunction(const char* value,
                                                      size_t length,
                                                      bool isImportant)
{
    if (length == 0) {
        removeCSSValuePair(CSSStyleValuePair::TransitionTimingFunction);
        return;
    }
    // TODO handle var() case
    CSSTokenVector layers;
    if (!CSSPropertyParser::parseLayers(value, length, layers)) {
        return;
    }
    ValueList* list = new ValueList(ValueList::CommaSeparator);
    size_t layerSize = layers.size();
    for (size_t i = 0; i < layerSize; i++) {
        CSSStyleValuePair sub;
        CSSTokenVector tokens;
        tokenizeCSSValue(tokens, layers[i].data(), layers[i].length());
        if (!(layerSize == 1 && sub.updateValueCommon(tokens)) &&
            !sub.updateValueLayerTransitionTimingFunction(tokens)) {
            return;
        }
        list->push_back(sub);
    }
    CSSStyleValuePair result;
    result.setFlagImportant(isImportant);
    result.setValueList(list);
    addCSSValuePair(CSSStyleValuePair::TransitionTimingFunction, result);
}

void CSSStyleDeclaration::setTransitionDelay(const char* value, size_t length,
                                             bool isImportant)
{
    if (length == 0) {
        removeCSSValuePair(CSSStyleValuePair::TransitionDelay);
        return;
    }
    // TODO handle var() case
    CSSTokenVector layers;
    if (!CSSPropertyParser::parseLayers(value, length, layers)) {
        return;
    }
    ValueList* list = new ValueList(ValueList::CommaSeparator);
    size_t layerSize = layers.size();
    for (size_t i = 0; i < layerSize; i++) {
        CSSStyleValuePair sub;
        CSSTokenVector tokens;
        tokenizeCSSValue(tokens, layers[i].data(), layers[i].length());
        if (!(layerSize == 1 && sub.updateValueCommon(tokens)) &&
            !sub.updateValueLayerTransitionDelay(tokens)) {
            return;
        }
        list->push_back(sub);
    }
    CSSStyleValuePair result;
    result.setFlagImportant(isImportant);
    result.setValueList(list);
    addCSSValuePair(CSSStyleValuePair::TransitionDelay, result);
}

String* CSSStyleDeclaration::Transition()
{
    const size_t kKeySize = 4;
    const CSSStyleValuePair::KeyKind kKeys[kKeySize] = {
        CSSStyleValuePair::TransitionProperty,
        CSSStyleValuePair::TransitionDuration,
        CSSStyleValuePair::TransitionTimingFunction,
        CSSStyleValuePair::TransitionDelay
    };

    if (isComputedStyle()) {
        for (size_t i = 0; i < kKeySize; i++) {
            updateValue(kKeys[i]);
        }
    }

    CSSStyleValuePair v[4];
    size_t size[4] = { 0, 0, 0, 0 };
    size_t maxLayer = 0;
    for (size_t k = 0; k < kKeySize; k++) {
        if (hasCSSValuePair(kKeys[k])) {
            v[k] = getCSSValuePair(kKeys[k]);
            // ASSERT inside
            if (v[k].valueKind() == CSSStyleValuePair::ValueListKind) {
                size[k] = v[k].multiValue()->size();
            } else {
                size[k] = 1;
            }
            maxLayer = std::max(maxLayer, size[k]);
        }
    }

    StringBuilder builder;
    for (size_t i = 0; i < maxLayer; i++) {
        if (i != 0) {
            builder.appendString(", ");
        }
        for (size_t k = 0; k < kKeySize; k++) {
            if (k != 0) {
                builder.appendString(String::spaceString);
            }
            if (i < size[k]) {
                if (v[k].valueKind() == CSSStyleValuePair::ValueListKind) {
                    builder.appendString(v[k].multiValue()->at(i).toString());
                } else {
                    builder.appendString(v[k].toString());
                }
            } else {
                builder.appendString(String::initialString);
            }
        }
    }
    return builder.finalize();
}

void CSSStyleDeclaration::setTransition(const char* value, size_t length,
                                        bool isImportant)
{
    if (length == 0) {
        removeCSSValuePair(CSSStyleValuePair::TransitionProperty);
        removeCSSValuePair(CSSStyleValuePair::TransitionDuration);
        removeCSSValuePair(CSSStyleValuePair::TransitionDelay);
        removeCSSValuePair(CSSStyleValuePair::TransitionTimingFunction);
        return;
    }
    // TODO handle var() case
    CSSTokenVector layers;
    if (!CSSPropertyParser::parseLayers(value, length, layers)) {
        return;
    }

    ValueList* properties = new ValueList(ValueList::CommaSeparator);
    ValueList* durations = new ValueList(ValueList::CommaSeparator);
    ValueList* timingFns = new ValueList(ValueList::CommaSeparator);
    ValueList* delays = new ValueList(ValueList::CommaSeparator);

    size_t layerSize = layers.size();
    for (size_t i = 0; i < layerSize; i++) {
        CSSStyleValuePair v0, v1, v2, v3;
        CSSTokenVector tokens;
        tokenizeCSSValue(tokens, layers[i].data(), layers[i].length());
        if (layerSize == 1 && v0.updateValueCommon(tokens)) {
            v1 = v2 = v3 = v0;
        } else if (!parseTransitionShorthand(tokens, &v0, &v1, &v2, &v3)) {
            return;
        }
        properties->push_back(v0);
        durations->push_back(v1);
        timingFns->push_back(v2);
        delays->push_back(v3);
    }

    CSSStyleValuePair r0, r1, r2, r3;
    r0.setValueList(properties);
    r1.setValueList(durations);
    r2.setValueList(timingFns);
    r3.setValueList(delays);

    r0.setFlagImportant(isImportant);
    r1.setFlagImportant(isImportant);
    r2.setFlagImportant(isImportant);
    r3.setFlagImportant(isImportant);

    addCSSValuePair(CSSStyleValuePair::TransitionProperty, r0);
    addCSSValuePair(CSSStyleValuePair::TransitionDuration, r1);
    addCSSValuePair(CSSStyleValuePair::TransitionTimingFunction, r2);
    addCSSValuePair(CSSStyleValuePair::TransitionDelay, r3);
}

StyleRuleCSSStyleDeclaration::StyleRuleCSSStyleDeclaration(
    CSSStyleDeclaration* src, CSSRule* parentRule)
    : CSSStyleDeclaration(parentRule->parentStyleSheet()
                              ->scriptBindingInstance()
                              ->ownerDocument())
{
    m_cssValues = src->m_cssValues;
    m_pointerRooter = src->m_pointerRooter;
    m_parentRule = parentRule;
}

CSSStyleSheet* StyleRuleCSSStyleDeclaration::parentStyleSheet() const
{
    STARFISH_ASSERT(m_parentRule);
    return m_parentRule->parentStyleSheet();
}

ScriptBindingInstance* StyleRuleCSSStyleDeclaration::scriptBindingInstance()
{
    STARFISH_ASSERT(m_parentRule);
    STARFISH_ASSERT(m_parentRule->parentStyleSheet());
    return m_parentRule->parentStyleSheet()->scriptBindingInstance();
}

void StyleRuleCSSStyleDeclaration::setCssText(String* text)
{
    STARFISH_ASSERT(m_parentRule);
    CSSStyleDeclaration* decl =
        ((CSSStyleRule*)m_parentRule)->styleRule()->styleDeclaration();
    decl->clear();

    CSSParser parser(scriptBindingInstance()->ownerDocument());
    parser.parseStyleDeclaration(text, decl);
    m_cssValues = decl->cssValues();
    m_pointerRooter = decl->m_pointerRooter;

    scriptBindingInstance()
        ->ownerWindow()
        ->browsingContext()
        ->setNeedsStyleSheetsRecalc();
}

void InlineCSSStyleDeclaration::setCssText(String* text)
{
    STARFISH_ASSERT(m_node->isElement());
    m_node->asElement()->setStyleAttr(text);
}

#undef ADD_PAIRS
#undef RM_PAIRS
#undef GEN_ATTRIBUTE_GETTER_FOURSIDE
#undef GEN_ATTRIBUTE_SETTER_FOURSIDE
#undef GEN_ATTRIBUTE_GETTER_BORDER
#undef GEN_ATTRIBUTE_SETTER_BORDER
}
