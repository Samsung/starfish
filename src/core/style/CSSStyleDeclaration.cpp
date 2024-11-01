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
#include "binding/ScriptBindingInstance.h"
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
#include "core/dom/MutationObserver.h"
#include "core/dom/MutationRecord.h"
#include "core/dom/MutationObservationScope.h"
#include "core/dom/Node.h"

namespace Starfish {

static bool UnitPositionShorthandToLongHand(
    const CSSStyleValuePair::KeyKind& shorthand,
    CSSStyleValuePair::KeyKind& longhandX,
    CSSStyleValuePair::KeyKind& longhandY)
{
    if (shorthand == CSSStyleValuePair::KeyKind::BackgroundPosition) {
        longhandX = CSSStyleValuePair::KeyKind::BackgroundPositionX;
        longhandY = CSSStyleValuePair::KeyKind::BackgroundPositionY;
        return true;
    } else if (shorthand == CSSStyleValuePair::KeyKind::MaskPosition) {
        longhandX = CSSStyleValuePair::KeyKind::MaskPositionX;
        longhandY = CSSStyleValuePair::KeyKind::MaskPositionY;
        return true;
    }
    return false;
}

static bool UnitRepeatStyleShorthandToLongHand(
    const CSSStyleValuePair::KeyKind& shorthand,
    CSSStyleValuePair::KeyKind& longhandX,
    CSSStyleValuePair::KeyKind& longhandY)
{
    if (shorthand == CSSStyleValuePair::KeyKind::BackgroundRepeat) {
        longhandX = CSSStyleValuePair::KeyKind::BackgroundRepeatX;
        longhandY = CSSStyleValuePair::KeyKind::BackgroundRepeatY;
        return true;
    } else if (shorthand == CSSStyleValuePair::KeyKind::MaskRepeat) {
        longhandX = CSSStyleValuePair::KeyKind::MaskRepeatX;
        longhandY = CSSStyleValuePair::KeyKind::MaskRepeatY;
        return true;
    }
    return false;
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

// Parse repeat-style value string in case it has single token
static bool parseUnitRepeatStyle(const CSSTokenValue& tok,
                                 CSSStyleValuePair* retx,
                                 CSSStyleValuePair* rety)
{
    if (retx->updateValueUnitRepeatStyle(tok)) {
        *rety = *retx;
    } else if (tok.equals("repeat-x")) {
        *retx = CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::RepeatStyleValueKind,
            RepeatRepeatValue);
        *rety = CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::RepeatStyleValueKind,
            NoRepeatRepeatValue);
    } else if (tok.equals("repeat-y")) {
        *retx = CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::RepeatStyleValueKind,
            NoRepeatRepeatValue);
        *rety = CSSStyleValuePair(
            CSSStyleValuePair::ValueKind::RepeatStyleValueKind,
            RepeatRepeatValue);
    } else {
        return false;
    }
    return true;
}

static bool parseUnitRepeatStyle(const CSSTokenVector& tokens,
                                 CSSStyleValuePair* retx,
                                 CSSStyleValuePair* rety,
                                 bool allowComma = true)
{
    // <repeat-style> = repeat-x | repeat-y | [repeat | no-repeat]{1,2}
    // <repeat-style> [, <repeat-style>]*
    size_t len = 0;
    retx->setValueList(new ValueList(Separator::CommaSeparator));
    rety->setValueList(new ValueList(Separator::CommaSeparator));
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
            if (parseUnitRepeatStyle(tokens[i - 1], &x, &y)) {
                retx->multiValue()->push_back(x);
                rety->multiValue()->push_back(y);
            } else {
                return false;
            }
        } else if (len == 2) {
            CSSStyleValuePair x, y;
            if (x.updateValueUnitRepeatStyle(tokens[i - 2]) &&
                y.updateValueUnitRepeatStyle(tokens[i - 1])) {
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
    const CSSTokenVector& tokens, CSSStyleValuePair* color,
    CSSStyleValuePair* image, CSSStyleValuePair* repeatX,
    CSSStyleValuePair* repeatY, CSSStyleValuePair* positionX,
    CSSStyleValuePair* positionY, CSSStyleValuePair* size,
    CSSStyleValuePair* attachment, CSSStyleValuePair* origin,
    CSSStyleValuePair* clip, bool allowColor)
{
    // - ACCEPT only 1-word_ : bg-color, bg-image, bg-attachment, bg-origin,
    // bg-clip
    // - ACCEPT 1 to 2 words : bg-repeat, position, bg-size

    size_t len = tokens.size();
    if (len < 1 || len > 11) {
        return false;
    }

    color->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    image->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    repeatX->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    repeatY->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    positionX->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    positionY->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    size->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    attachment->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    origin->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    clip->setValueKind(CSSStyleValuePair::ValueKind::Initial);

    bool hasColor = false, hasImage = false, hasRepeat = false,
         hasPosition = false, hasSize = false, hasAttachment = false,
         hasOrigin = false, hasClip = false;
    bool hasPositionPrev = false, shouldSize = false;
    CSSStyleValuePair temp, tempX, tempY;
    CSSTokenValue* tok;
    CSSTokenVector toks;

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
                    *size = temp;
                    hasSize = true;
                    i++;
                    continue;
                }
            } else if (!hasPosition &&
                       CSSStyleDeclaration::parseUnitPositionShorthand(
                           toks, CSSStyleValuePair::KeyKind::BackgroundPosition,
                           &tempX, &tempY, false)) {
                hasPositionPrev = true;
                *positionX = tempX;
                *positionY = tempY;
                hasPosition = true;
                i++;
                continue;
            } else if (!hasRepeat &&
                       parseUnitRepeatStyle(toks, &tempX, &tempY, false)) {
                *repeatX = tempX;
                *repeatY = tempY;
                hasRepeat = true;
                i++;
                continue;
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
                *size = temp;
                hasSize = true;
                continue;
            }
        } else if (!hasImage && temp.updateValueBackgroundImage(toks, false)) {
            *image = temp;
            hasImage = true;
            continue;
        } else if (!hasPosition &&
                   CSSStyleDeclaration::parseUnitPositionShorthand(
                       toks, CSSStyleValuePair::KeyKind::BackgroundPosition,
                       &tempX, &tempY, false)) {
            hasPositionPrev = true;
            *positionX = tempX;
            *positionY = tempY;
            hasPosition = true;
            continue;
        } else if (!hasRepeat &&
                   parseUnitRepeatStyle(toks, &tempX, &tempY, false)) {
            *repeatX = tempX;
            *repeatY = tempY;
            hasRepeat = true;
            continue;
        } else if (!hasAttachment &&
                   temp.updateValueBackgroundAttachment(toks, false)) {
            *attachment = temp;
            hasAttachment = true;
            continue;
        } else if (!hasOrigin && temp.updateValueBox(toks, false)) {
            *origin = temp;
            hasOrigin = true;
            continue;
        } else if (!hasClip && temp.updateValueBox(toks, false)) {
            *clip = temp;
            hasClip = true;
            continue;
        } else if (!hasColor && temp.updateValueUnitColor(*tok)) {
            if (!allowColor) {
                return false;
            }
            *color = temp;
            hasColor = true;
            continue;
        }

        return false;
    }

    return true;
}

static String* printBackground(String* image, String* position, String* size,
                               String* repeat, String* attachment,
                               String* origin, String* clip, String* color)
{
    const int maxCount = 8;
    int initialCount = 0, inheritCount = 0;
    initialCount += (image->equals(String::initialString) ? 1 : 0);
    initialCount += (position->equals(String::initialString) ? 1 : 0);
    initialCount += (size->equals(String::initialString) ? 1 : 0);
    initialCount += (repeat->equals(String::initialString) ? 1 : 0);
    initialCount += (attachment->equals(String::initialString) ? 1 : 0);
    initialCount += (origin->equals(String::initialString) ? 1 : 0);
    initialCount += (clip->equals(String::initialString) ? 1 : 0);
    initialCount += (color->equals(String::initialString) ? 1 : 0);

    inheritCount += (image->equals(String::inheritString) ? 1 : 0);
    inheritCount += (position->equals(String::inheritString) ? 1 : 0);
    inheritCount += (size->equals(String::inheritString) ? 1 : 0);
    inheritCount += (repeat->equals(String::inheritString) ? 1 : 0);
    inheritCount += (attachment->equals(String::inheritString) ? 1 : 0);
    inheritCount += (origin->equals(String::inheritString) ? 1 : 0);
    inheritCount += (clip->equals(String::inheritString) ? 1 : 0);
    inheritCount += (color->equals(String::inheritString) ? 1 : 0);

    if (initialCount == maxCount) {
        return String::initialString;
    }
    if (inheritCount == maxCount) {
        return String::inheritString;
    }
    if (inheritCount > 0) {
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

void CSSStyleDeclaration::addBorderTopCSSValuePairs(
    const CSSStyleValuePair& width, const CSSStyleValuePair& style,
    const CSSStyleValuePair& color)
{
    if (shouldKeepAppearanceOrder(CSSStyleValuePair::KeyKind::BorderTopWidth)) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderTopWidth);
    }
    addCSSValuePair(CSSStyleValuePair::KeyKind::BorderTopWidth, width);

    if (shouldKeepAppearanceOrder(CSSStyleValuePair::KeyKind::BorderTopStyle)) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderTopStyle);
    }
    addCSSValuePair(CSSStyleValuePair::KeyKind::BorderTopStyle, style);

    if (shouldKeepAppearanceOrder(CSSStyleValuePair::KeyKind::BorderTopColor)) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderTopColor);
    }
    addCSSValuePair(CSSStyleValuePair::KeyKind::BorderTopColor, color);
}

void CSSStyleDeclaration::addBorderRightCSSValuePairs(
    const CSSStyleValuePair& width, const CSSStyleValuePair& style,
    const CSSStyleValuePair& color)
{
    addCSSValuePair(CSSStyleValuePair::KeyKind::BorderRightWidth, width);
    addCSSValuePair(CSSStyleValuePair::KeyKind::BorderRightStyle, style);
    addCSSValuePair(CSSStyleValuePair::KeyKind::BorderRightColor, color);
}

void CSSStyleDeclaration::addBorderBottomCSSValuePairs(
    const CSSStyleValuePair& width, const CSSStyleValuePair& style,
    const CSSStyleValuePair& color)
{
    addCSSValuePair(CSSStyleValuePair::KeyKind::BorderBottomWidth, width);
    addCSSValuePair(CSSStyleValuePair::KeyKind::BorderBottomStyle, style);
    addCSSValuePair(CSSStyleValuePair::KeyKind::BorderBottomColor, color);
}

void CSSStyleDeclaration::addBorderLeftCSSValuePairs(
    const CSSStyleValuePair& width, const CSSStyleValuePair& style,
    const CSSStyleValuePair& color)
{
    addCSSValuePair(CSSStyleValuePair::KeyKind::BorderLeftWidth, width);
    addCSSValuePair(CSSStyleValuePair::KeyKind::BorderLeftStyle, style);
    addCSSValuePair(CSSStyleValuePair::KeyKind::BorderLeftColor, color);
}

void CSSStyleDeclaration::addBorderCSSValuePairs(const CSSStyleValuePair& width,
                                                 const CSSStyleValuePair& style,
                                                 const CSSStyleValuePair& color)
{
    addBorderTopCSSValuePairs(width, style, color);
    addBorderRightCSSValuePairs(width, style, color);
    addBorderBottomCSSValuePairs(width, style, color);
    addBorderLeftCSSValuePairs(width, style, color);
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
                                      CSSStyleValuePair* source,
                                      CSSStyleValuePair* slice,
                                      CSSStyleValuePair* width,
                                      CSSStyleValuePair* outset,
                                      CSSStyleValuePair* repeat)
{
    size_t len = tokens.size();
    if (len < 1 || len > 18) {
        return false;
    }

    source->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    slice->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    width->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    outset->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    repeat->setValueKind(CSSStyleValuePair::ValueKind::Initial);

    bool hasSource = false, hasSlice = false, hasWidth = false,
         hasOutset = false, hasRepeat = false;
    bool hasSlicePrev = false, shouldWidth = false;
    bool hasWidthPrev = false, shouldOutset = false;
    CSSStyleValuePair temp;
    CSSTokenVector toks;

    size_t pos = 0;
    while (pos < len) {
        CSSTokenValue token = tokens[pos++];
        std::transform(token.begin(), token.end(), token.begin(), tolower);
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
                *width = temp;
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
                *outset = temp;
                continue;
            }
        } else if (!hasSource && temp.updateValueUnitBorderImageSource(token)) {
            hasSource = true;
            *source = temp;
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
                *repeat = temp;
                continue;
            } else if (temp.updateValueUnitBorderImageSlice(toks)) {
                hasSlicePrev = true;
                hasSlice = true;
                *slice = temp;
                continue;
            }
        }
        return false;
    }
    return true;
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

static void addMultiValueToOwner(CSSStyleValuePair& owner,
                                 CSSStyleValuePair& layerValue)
{
    if (owner.valueKind() != CSSStyleValuePair::ValueKind::ValueListKind) {
        CSSStyleValuePair tmp = owner;
        owner.setValueList(new ValueList(Separator::CommaSeparator));
        owner.multiValue()->push_back(tmp);
    }
    if (layerValue.valueKind() == CSSStyleValuePair::ValueKind::ValueListKind) {
        owner.multiValue()->push_back((*layerValue.multiValue())[0]);
    } else {
        owner.multiValue()->push_back(layerValue);
    }
}

// To help setting value for shorthand properties which has a four sides.
// |sides| must be [top, right, bottom, left].
void CSSStyleDeclaration::setFourSidedShorthandProperty(
    CSSStyleValuePair::KeyKind fourSidedShorthand,
    const CSSStyleValuePair::KeyKind sides[4], const char* value, size_t length,
    bool isImportant)
{
    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length);

    CSSStyleValuePair c;
    if (c.updateValueVarReferences(tokens)) {
        c.setValue(String::fromUTF8(value, length));
        c.setFlagImportant(isImportant);
        addCSSValuePair(fourSidedShorthand, c);
    } else if (c.updateValueCommon(tokens)) {
        c.setFlagImportant(isImportant);
        for (int i = 0; i < 4; i++) {
            if (shouldKeepAppearanceOrder(sides[i])) {
                removeCSSValuePair(sides[i]);
            }
            addCSSValuePair(sides[i], c);
        }
        return;
    }

    size_t len = tokens.size();
    if (len < 1 || len > 4) {
        return;
    }

    GCVector<CSSStyleValuePair> result;
    for (size_t i = 0; i < len; i++) {
        CSSStyleValuePair v;
        v.setFlagImportant(isImportant);
        if (!v.updateValueUnitFourSidedShorthandProperty(fourSidedShorthand,
                                                         tokens[i])) {
            return;
        }
        result.push_back(v);
    }

    CSSStyleValuePair* fourSides[4];
    fourSides[0] = &result[0];
    fourSides[1] = len < 2 ? fourSides[0] : &result[1];
    fourSides[2] = len < 3 ? fourSides[0] : &result[2];
    fourSides[3] = len < 4 ? fourSides[1] : &result[3];

    for (int i = 0; i < 4; i++) {
        if (shouldKeepAppearanceOrder(sides[i])) {
            removeCSSValuePair(sides[i]);
        }
        addCSSValuePair(sides[i], *fourSides[i]);
    }
}

bool CSSStyleDeclaration::parseFontShorthand(
    const CSSTokenVector& tokens, CSSStyleValuePair* style,
    // UNSUPPORTED CSSStyleValuePair* variant,
    CSSStyleValuePair* weight,
    // UNSUPPORTED CSSStyleValuePair* stretch,
    CSSStyleValuePair* size, CSSStyleValuePair* lineHeight,
    CSSStyleValuePair* family)
{
    // [font-style|font-weight] font-size[/line-height] font-family
    size_t len = tokens.size();
    if (len < 1) {
        return false;
    }

    style->setValueKind(CSSStyleValuePair::ValueKind::FontStyleValueKind);
    style->setValue(FontStyleValue::NormalFontStyleValue);
    weight->setValueKind(CSSStyleValuePair::ValueKind::FontWeightValueKind);
    weight->setValue(FontWeightValue::NormalFontWeightValue);
    lineHeight->setValueKind(CSSStyleValuePair::ValueKind::Normal);

    bool hasStyle = false, hasWeight = false, hasSize = false,
         hasLineHeight = false, hasFamily = false;
    CSSStyleValuePair temp;
    bool hasSizePrev = false, shouldLineHeight = false;
    size_t pos = 0;
    CSSTokenVector fontFamilyCandidate;

    while (pos < len) {
        const CSSTokenValue& orgToken = tokens[pos];
        CSSTokenValue token = tokens[pos++];
        std::transform(token.begin(), token.end(), token.begin(), tolower);

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
                *lineHeight = temp;
                continue;
            }
        } else if (!hasSize && !hasStyle &&
                   temp.updateValueUnitFontStyle(token)) {
            hasStyle = true;
            *style = temp;
            continue;
        } else if (!hasSize && !hasWeight &&
                   temp.updateValueUnitFontWeight(token)) {
            hasWeight = true;
            *weight = temp;
            continue;
        } else if (!hasSize && temp.updateValueUnitFontSize(token)) {
            hasSizePrev = true;
            hasSize = true;
            *size = temp;
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
        family->setKeywordValue(String::fromUTF8(
            fontFamilyCandidate[0].data(), fontFamilyCandidate[0].length()));
    } else {
        ValueList* val = new ValueList(
            Separator::CommaSeparatorAppendQuoteWhenMeetWhiteSpace);
        for (size_t i = 0; i < fontFamilyCandidate.size(); i++) {
            const auto& str = fontFamilyCandidate[i];
            val->emplace_back(CSSStyleValuePair::ValueKind::KeywordValueKind,
                              String::fromUTF8(str.data(), str.length()));
        }
        family->setValueList(val);
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

    bool foundProperty = false;
    bool foundDuration = false;
    bool foundTimingFunction = false;
    bool foundDelay = false;

    for (size_t i = 0; i < len; i++) {
        CSSStyleValuePair temp;
        const CSSTokenValue& tok = tokens[i];
        if (!foundProperty && temp.updateValueUnitTransitionProperty(tok)) {
            foundProperty = true;
            *property = temp;
            continue;
        }

        // TODO: This coide is for ValueUnitTransitionProperty of 'var'.

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

        // TODO: This code is for Duration and Delay of 'var'.
        CSSTokenVector toks;
        toks.push_back(tok);
        if (!foundTimingFunction &&
            temp.updateValueUnitTransitionTimingFunction(tok)) {
            foundTimingFunction = true;
            *timingFunction = temp;
            continue;
        } else if (!foundTimingFunction &&
                   temp.updateValueVarReferences(toks)) {
            temp.setValue(String::fromUTF8(tok.data(), tok.length()));
            foundTimingFunction = true;
            *timingFunction = temp;
            continue;
        }

        return false;
    }
    return true;
}

static bool parseAnimationShorthand(
    const CSSTokenVector& tokens, CSSStyleValuePair* name,
    CSSStyleValuePair* duration, CSSStyleValuePair* timingFunction,
    CSSStyleValuePair* delay, CSSStyleValuePair* iteration,
    CSSStyleValuePair* direction, CSSStyleValuePair* playState,
    CSSStyleValuePair* fillMode)
{
    size_t len = tokens.size();
    if (len < 1) {
        return false;
    }

    name->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    duration->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    timingFunction->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    delay->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    iteration->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    direction->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    playState->setValueKind(CSSStyleValuePair::ValueKind::Initial);
    fillMode->setValueKind(CSSStyleValuePair::ValueKind::Initial);

    bool foundName = false;
    bool foundDuration = false;
    bool foundTimingFunction = false;
    bool foundDelay = false;
    bool foundIteration = false;
    bool foundDirection = false;
    bool foundPlayState = false;
    bool foundFillMode = false;

    for (size_t i = 0; i < len; i++) {
        CSSStyleValuePair temp;
        const CSSTokenValue& tok = tokens[i];

        // delay can be negative
        int timeParsingOption =
            foundDuration ? CSSPropertyParser::AllowNegative : 0;
        if ((!foundDuration || !foundDelay) &&
            temp.updateValueUnitTimeOrCalc(tok, timeParsingOption)) {
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
        // TODO: Handle animation-duration and animation-delay using 'var'.

        CSSTokenVector toks;
        toks.push_back(tok);
        if (!foundTimingFunction &&
            temp.updateValueUnitAnimationTimingFunction(tok)) {
            foundTimingFunction = true;
            *timingFunction = temp;
            continue;
        } else if (!foundTimingFunction &&
                   temp.updateValueVarReferences(toks)) {
            temp.setValue(String::fromUTF8(tok.data(), tok.length()));
            foundTimingFunction = true;
            *timingFunction = temp;
            continue;
        }

        if (!foundIteration &&
            temp.updateValueUnitAnimationIterationCount(tok)) {
            foundIteration = true;
            *iteration = temp;
            continue;
        }
        // TODO: Handle animation-iteration-count using 'var'.

        if (!foundDirection && temp.updateValueUnitAnimationDirection(tok)) {
            foundDirection = true;
            *direction = temp;
            continue;
        }
        // TODO: Handle animation-direction of 'var'.

        if (!foundPlayState && temp.updateValueUnitAnimationPlayState(tok)) {
            foundPlayState = true;
            *playState = temp;
            continue;
        }
        // TODO: Handle animation-play-state using 'var'.

        if (!foundFillMode && temp.updateValueUnitAnimationFillMode(tok)) {
            foundFillMode = true;
            *fillMode = temp;
            continue;
        }
        // TODO: Handle animation-fill-mode using 'var'.

        if (!foundName && temp.updateValueUnitAnimationName(tok)) {
            foundName = true;
            *name = temp;
            continue;
        }
        // TODO: Handle animation-name using 'var'.

        return false;
    }
    return true;
}

CSSStyleDeclaration::CSSStyleDeclaration(Element* element)
    : ScriptWrappable(this)
    , m_node(element)
{
}

CSSStyleDeclaration::CSSStyleDeclaration(Document* document)
    : ScriptWrappable(this)
    , m_node(document)
{
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
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    return p;
}

void CSSStyleDeclaration::rootPointerValueIfExists(const CSSStyleValuePair& v)
{
    v.rootPointerValue(m_pointerRooter);
}

void CSSStyleDeclaration::removeRootPointerValue(const CSSStyleValuePair& v)
{
    v.unrootPointerValue(m_pointerRooter);
}

void CSSStyleDeclaration::clear()
{
    m_cssValues.clear();
    m_pointerRooter.clear();
    if (m_cssCustomValues) {
        m_cssCustomValues->clear();
    }
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
    if (m_cssCustomValues) {
        newStyle->m_cssCustomValues =
            new MutablePropertyValueList(*m_cssCustomValues);
    }
    return newStyle;
}

String* CSSStyleDeclaration::customProperty(String* key)
{
    AtomicString atomicKey =
        AtomicString::createAtomicString(m_node->starfish(), key);
    if (m_cssCustomValues) {
        auto ret = m_cssCustomValues->property(atomicKey);
        if (ret) {
            return ret.value();
        }
        return String::emptyString;
    }
    return String::emptyString;
}

void CSSStyleDeclaration::setCustomProperty(AtomicString key, String* value)
{
    if (!m_cssCustomValues) {
        m_cssCustomValues = new MutablePropertyValueList();
    }

    m_cssCustomValues->setProperty(key, value);
    notifyNeedsStyleRecalc();
}

void CSSStyleDeclaration::removeCustomProperty(String* key)
{
    AtomicString atomicKey =
        AtomicString::createAtomicString(m_node->starfish(), key);
    if (m_cssCustomValues) {
        m_cssCustomValues->removeProperty(atomicKey);
        notifyNeedsStyleRecalc();
    }
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
        if (data[i] == '(' && !inQuotes) {
            inParenthesis = true;
            numberOfnesting++;
        } else if (data[i] == ')' && !inQuotes) {
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
            // Added to cover the following cases.
            // "1.37916809e-13 58.2301959 58.2301959 58.2301959 58.2301959 0
            // 1.37916809e-13 0"
            if (data[i] == '-' && i >= 1 && data[i - 1] == 'e') {
                hasSepChar = false;
            }
        }

        if ((!inParenthesis && !inQuotes &&
             (String::isSpaceOrNewline(data[i]) || hasSepChar)) ||
            (data[i] == '(' && hasSepChar) || (data[i] == ')' && hasSepChar)) {
            str.pop_back();
            bool onlyWhiteSpace = true;
            for (size_t i = 0; i < str.length(); i++) {
                if (!isCaseSensitive)
                    str[i] = tolower(str[i]);
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
                               tolower);
                tokens.push_back(std::move(str));
            } else if (str.length() > 3 && (str[0] == 'v' || str[0] == 'V') &&
                       (str[1] == 'a' || str[1] == 'a') &&
                       (str[2] == 'r' || str[2] == 'R')) {
                std::transform(str.begin(), str.begin() + 3, str.begin(),
                               tolower);
                tokens.push_back(std::move(str));
            } else if (str.length() != 0) {
                if (!isCaseSensitive && !inQuotes) {
                    std::transform(str.begin(), str.end(), str.begin(),
                                   tolower);
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

void CSSStyleDeclaration::addValuePair(const CSSStyleValuePair& p)
{
    for (size_t i = 0; i < m_cssValues.size(); i++) {
        CSSStyleValuePair v = m_cssValues[i];
        if (v.keyKind() == p.keyKind()) {
            removeRootPointerValue(m_cssValues[i]);
            m_cssValues[i] = p;
            rootPointerValueIfExists(p);
            return;
        }
    }

    m_cssValues.push_back(p);
    rootPointerValueIfExists(p);
}

void CSSStyleDeclaration::addCSSValuePair(CSSStyleValuePair::KeyKind keyKind,
                                          const CSSStyleValuePair& value)
{
    for (unsigned i = 0; i < m_cssValues.size(); i++) {
        if (m_cssValues[i].keyKind() == keyKind) {
            if (isInlineStyle() || value.flagImportant() == true ||
                (value.flagImportant() == false &&
                 m_cssValues[i].flagImportant() == false)) {
                if (!m_cssValues[i].valueEquals(value)) {
                    removeRootPointerValue(m_cssValues[i]);
                    m_cssValues[i].setValueKind(value.valueKind());
                    m_cssValues[i].setValue(value.value());
                    m_cssValues[i].setFlagImportant(value.flagImportant());
                    rootPointerValueIfExists(value);
                    notifyNeedsStyleRecalc();
                }
            }
            return;
        }
    }
    m_cssValues.push_back(value);
    m_cssValues.back().setKeyKind(keyKind);
    rootPointerValueIfExists(value);
    notifyNeedsStyleRecalc();
}

void CSSStyleDeclaration::removeCSSValuePair(CSSStyleValuePair::KeyKind keyKind)
{
    unsigned len = m_cssValues.size();
    for (unsigned i = 0; i < len; i++) {
        if (m_cssValues[i].keyKind() == keyKind) {
            m_cssValues.erase(m_cssValues.begin() + i);
            notifyNeedsStyleRecalc();
            return;
        }
    }
}

bool CSSStyleDeclaration::hasCSSValuePair(CSSStyleValuePair::KeyKind keyKind)
{
    unsigned len = m_cssValues.size();
    for (unsigned i = 0; i < len; i++) {
        if (m_cssValues[i].keyKind() == keyKind) {
            return true;
        }
    }
    return false;
}

CSSStyleValuePair CSSStyleDeclaration::getCSSValuePair(
    CSSStyleValuePair::KeyKind keyKind)
{
    unsigned len = m_cssValues.size();
    for (unsigned i = 0; i < len; i++) {
        if (m_cssValues[i].keyKind() == keyKind) {
            return m_cssValues[i];
        }
    }

    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
}

bool CSSStyleDeclaration::shouldKeepAppearanceOrder(
    CSSStyleValuePair::KeyKind keyKind)
{
    if (keyKind == CSSStyleValuePair::KeyKind::All) {
        return hasCSSValuePair(CSSStyleValuePair::KeyKind::All);
    }

    switch (keyKind) {
    case CSSStyleValuePair::KeyKind::MarginBlockStart:
    case CSSStyleValuePair::KeyKind::MarginBlockEnd:
        if (hasCSSValuePair(CSSStyleValuePair::KeyKind::MarginTop) ||
            hasCSSValuePair(CSSStyleValuePair::KeyKind::MarginBottom)) {
            return true;
        }
        break;
    case CSSStyleValuePair::KeyKind::MarginInlineStart:
    case CSSStyleValuePair::KeyKind::MarginInlineEnd:
        if (hasCSSValuePair(CSSStyleValuePair::KeyKind::MarginLeft) ||
            hasCSSValuePair(CSSStyleValuePair::KeyKind::MarginRight)) {
            return true;
        }
        break;
    case CSSStyleValuePair::KeyKind::PaddingBlockStart:
    case CSSStyleValuePair::KeyKind::PaddingBlockEnd:
        if (hasCSSValuePair(CSSStyleValuePair::KeyKind::PaddingTop) ||
            hasCSSValuePair(CSSStyleValuePair::KeyKind::PaddingBottom)) {
            return true;
        }
        break;
    case CSSStyleValuePair::KeyKind::PaddingInlineStart:
    case CSSStyleValuePair::KeyKind::PaddingInlineEnd:
        if (hasCSSValuePair(CSSStyleValuePair::KeyKind::PaddingLeft) ||
            hasCSSValuePair(CSSStyleValuePair::KeyKind::PaddingRight)) {
            return true;
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderBlockStartColor:
    case CSSStyleValuePair::KeyKind::BorderBlockEndColor:
        if (hasCSSValuePair(CSSStyleValuePair::KeyKind::BorderTopColor) ||
            hasCSSValuePair(CSSStyleValuePair::KeyKind::BorderBottomColor)) {
            return true;
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderBlockStartStyle:
    case CSSStyleValuePair::KeyKind::BorderBlockEndStyle:
        if (hasCSSValuePair(CSSStyleValuePair::KeyKind::BorderTopStyle) ||
            hasCSSValuePair(CSSStyleValuePair::KeyKind::BorderBottomStyle)) {
            return true;
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderBlockStartWidth:
    case CSSStyleValuePair::KeyKind::BorderBlockEndWidth:
        if (hasCSSValuePair(CSSStyleValuePair::KeyKind::BorderTopWidth) ||
            hasCSSValuePair(CSSStyleValuePair::KeyKind::BorderBottomWidth)) {
            return true;
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderInlineStartColor:
    case CSSStyleValuePair::KeyKind::BorderInlineEndColor:
        if (hasCSSValuePair(CSSStyleValuePair::KeyKind::BorderLeftColor) ||
            hasCSSValuePair(CSSStyleValuePair::KeyKind::BorderRightColor)) {
            return true;
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderInlineStartWidth:
    case CSSStyleValuePair::KeyKind::BorderInlineEndWidth:
        if (hasCSSValuePair(CSSStyleValuePair::KeyKind::BorderLeftWidth) ||
            hasCSSValuePair(CSSStyleValuePair::KeyKind::BorderRightWidth)) {
            return true;
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderInlineStartStyle:
    case CSSStyleValuePair::KeyKind::BorderInlineEndStyle:
        if (hasCSSValuePair(CSSStyleValuePair::KeyKind::BorderLeftStyle) ||
            hasCSSValuePair(CSSStyleValuePair::KeyKind::BorderRightStyle)) {
            return true;
        }
        break;
    default:
        break;
    }

    switch (keyKind) {
    case CSSStyleValuePair::KeyKind::MarginTop:
    case CSSStyleValuePair::KeyKind::MarginBottom:
        if (hasCSSValuePair(CSSStyleValuePair::KeyKind::MarginBlockStart) ||
            hasCSSValuePair(CSSStyleValuePair::KeyKind::MarginBlockEnd)) {
            return true;
        }
        break;
    case CSSStyleValuePair::KeyKind::MarginLeft:
    case CSSStyleValuePair::KeyKind::MarginRight:
        if (hasCSSValuePair(CSSStyleValuePair::KeyKind::MarginInlineStart) ||
            hasCSSValuePair(CSSStyleValuePair::KeyKind::MarginInlineEnd)) {
            return true;
        }
        break;
    case CSSStyleValuePair::KeyKind::PaddingTop:
    case CSSStyleValuePair::KeyKind::PaddingBottom:
        if (hasCSSValuePair(CSSStyleValuePair::KeyKind::PaddingBlockStart) ||
            hasCSSValuePair(CSSStyleValuePair::KeyKind::PaddingBlockEnd)) {
            return true;
        }
        break;
    case CSSStyleValuePair::KeyKind::PaddingLeft:
    case CSSStyleValuePair::KeyKind::PaddingRight:
        if (hasCSSValuePair(CSSStyleValuePair::KeyKind::PaddingInlineStart) ||
            hasCSSValuePair(CSSStyleValuePair::KeyKind::PaddingInlineEnd)) {
            return true;
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderTopColor:
    case CSSStyleValuePair::KeyKind::BorderBottomColor:
        if (hasCSSValuePair(
                CSSStyleValuePair::KeyKind::BorderBlockStartColor) ||
            hasCSSValuePair(CSSStyleValuePair::KeyKind::BorderBlockEndColor)) {
            return true;
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderTopStyle:
    case CSSStyleValuePair::KeyKind::BorderBottomStyle:
        if (hasCSSValuePair(
                CSSStyleValuePair::KeyKind::BorderBlockStartStyle) ||
            hasCSSValuePair(CSSStyleValuePair::KeyKind::BorderBlockEndStyle)) {
            return true;
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderTopWidth:
    case CSSStyleValuePair::KeyKind::BorderBottomWidth:
        if (hasCSSValuePair(
                CSSStyleValuePair::KeyKind::BorderBlockStartWidth) ||
            hasCSSValuePair(CSSStyleValuePair::KeyKind::BorderBlockEndWidth)) {
            return true;
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderLeftColor:
    case CSSStyleValuePair::KeyKind::BorderRightColor:
        if (hasCSSValuePair(
                CSSStyleValuePair::KeyKind::BorderInlineStartColor) ||
            hasCSSValuePair(CSSStyleValuePair::KeyKind::BorderInlineEndColor)) {
            return true;
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderLeftWidth:
    case CSSStyleValuePair::KeyKind::BorderRightWidth:
        if (hasCSSValuePair(
                CSSStyleValuePair::KeyKind::BorderInlineStartWidth) ||
            hasCSSValuePair(CSSStyleValuePair::KeyKind::BorderInlineEndWidth)) {
            return true;
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderLeftStyle:
    case CSSStyleValuePair::KeyKind::BorderRightStyle:
        if (hasCSSValuePair(
                CSSStyleValuePair::KeyKind::BorderInlineStartStyle) ||
            hasCSSValuePair(CSSStyleValuePair::KeyKind::BorderInlineEndStyle)) {
            return true;
        }
        break;
    default:
        break;
    }
    return false;
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

CSSStyleValuePair::KeyKind lookupName(const char* buf, size_t len)
{
    if (len > 2 && buf[0] == '-' && buf[1] == '-') {
        return CSSStyleValuePair::KeyKind::CustomProperty;
    } else {
        char* mutableBuf = ALLOCA(len, char);
        for (size_t i = 0; i < len; i++) {
            mutableBuf[i] = tolower(buf[i]);
        }

        return CSSStyleLookupTrie::lookupCSSStyle(mutableBuf, len);
    }
}

Optional<String*> CSSStyleDeclaration::defaultNamedGetter(String* name)
{
    CSSStyleValuePair::KeyKind keyKind;
    name->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
            CSSStyleValuePair::KeyKind* keykind =
                reinterpret_cast<CSSStyleValuePair::KeyKind*>(data);
            // defaultNamedGetter allows camel-case name.
            *keykind = CSSStyleLookupTrie::lookupCSSStyleCamelCase(buf, len);
            if (*keykind == CSSStyleValuePair::KeyKind::Unknown) {
                *keykind = CSSStyleLookupTrie::lookupCSSStyle(buf, len);
            }
            return 0;
        },
        &keyKind);

    if (keyKind == CSSStyleValuePair::KeyKind::Unknown) {
        return Optional<String*>();
    }

    return getPropertyValueInternal(keyKind);
}

String* CSSStyleDeclaration::getPropertyValue(String* name)
{
    CSSStyleValuePair::KeyKind keykind;
    name->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
            CSSStyleValuePair::KeyKind* keykind =
                reinterpret_cast<CSSStyleValuePair::KeyKind*>(data);
            *keykind = lookupName(buf, len);
            return 0;
        },
        &keykind);

    String* val = String::emptyString;
    if (keykind == CSSStyleValuePair::KeyKind::CustomProperty) {
        updateValue(keykind, name);
        val = customProperty(name);
    } else {
        val = getPropertyValueInternal(keykind);
    }

    return val;
}

bool CSSStyleDeclaration::isShorthandProperty(
    CSSStyleValuePair::KeyKind keyKind)
{
    // Use macros to prevent missing shorthanded properties.
    switch (keyKind) {
#define IS_SHORTHAND(Name, ...)            \
    case CSSStyleValuePair::KeyKind::Name: \
        return true;
        FOR_EACH_STYLE_ATTRIBUTE_SHORTHAND(IS_SHORTHAND)
#undef IS_SHORTHAND
    default:
        return false;
    }
    return false;
}

bool CSSStyleDeclaration::isStickyProperty(CSSStyleValuePair::KeyKind keyKind)
{
    // Use macros to prevent missing sticky properties.
    switch (keyKind) {
#define IS_STICKY(Name, ...)               \
    case CSSStyleValuePair::KeyKind::Name: \
        return true;
        FOR_EACH_STYLE_ATTRIBUTE_STICKY(IS_STICKY)
#undef IS_STICKY
    default:
        return false;
    }
    return false;
}

template <>
String* CSSStyleDeclaration::getPropertyValueInternalFor<
    CSSStyleDeclaration::PropertyType::kLonghand>(
    CSSStyleValuePair::KeyKind keyKind)
{
    updateValue(keyKind);
    for (unsigned i = 0; i < m_cssValues.size(); i++) {
        if (m_cssValues[i].keyKind() == keyKind)
            return m_cssValues[i].toString();
    }
    return String::emptyString;
}

template <>
String* CSSStyleDeclaration::getPropertyValueInternalFor<
    CSSStyleDeclaration::PropertyType::kShorthand>(
    CSSStyleValuePair::KeyKind keyKind)
{
    // Use macros to prevent missing shorthanded properties.
    switch (keyKind) {
#define GET_ATTR(name, ...)                                                \
    case CSSStyleValuePair::KeyKind::name: {                               \
        if (isInInlineStyleWithVarFunctionValueKind(                       \
                CSSStyleValuePair::KeyKind::name)) {                       \
            auto pair = getCSSValuePair(CSSStyleValuePair::KeyKind::name); \
            STARFISH_ASSERT(                                               \
                pair.valueKind() ==                                        \
                CSSStyleValuePair::ValueKind::VarFunctionValueKind);       \
            return pair.toString();                                        \
        }                                                                  \
        return name();                                                     \
        break;                                                             \
    }
        FOR_EACH_STYLE_ATTRIBUTE_SHORTHAND(GET_ATTR)
#undef GET_ATTR
    default:
        STARFISH_UNIMPLEMENTED();
        break;
    }
    return nullptr;
}

String* CSSStyleDeclaration::getPropertyValueInternal(
    CSSStyleValuePair::KeyKind keyKind)
{
    if (isShorthandProperty(keyKind)) {
        return getPropertyValueInternalFor<PropertyType::kShorthand>(keyKind);
    } else {
        return getPropertyValueInternalFor<PropertyType::kLonghand>(keyKind);
    }
}

// https://drafts.csswg.org/cssom/#dom-cssstyledeclaration-getpropertypriority
String* CSSStyleDeclaration::getPropertyPriority(String* name)
{
    CSSStyleValuePair::KeyKind keyKind;
    name->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
            CSSStyleValuePair::KeyKind* keykind =
                reinterpret_cast<CSSStyleValuePair::KeyKind*>(data);
            *keykind = lookupName(buf, len);
            return 0;
        },
        &keyKind);

    auto iter = std::find_if(
        m_cssValues.begin(), m_cssValues.end(),
        [keyKind](CSSStyleValuePair p) { return p.keyKind() == keyKind; });

    return iter != m_cssValues.end() && iter->flagImportant()
               ? String::fromUTF8("important")
               : String::emptyString;
}

template <>
bool CSSStyleDeclaration::setPropertyInternalFor<
    CSSStyleDeclaration::PropertyType::kLonghand>(
    CSSStyleValuePair::KeyKind keyKind, const char* value, size_t valueLength,
    bool isImportant)
{
    if (valueLength == 0) {
        removeCSSValuePair(keyKind);
        return true;
    }

    CSSTokenVector tokens;
    if (UNLIKELY(keyKind == CSSStyleValuePair::KeyKind::Content)) {
        tokenizeCSSValue(tokens, value, valueLength, "", 0, true, true);
    } else if (UNLIKELY(keyKind == CSSStyleValuePair::KeyKind::Transform)) {
        tokenizeCSSValue(tokens, value, valueLength, ",", 1, true);
    } else {
        tokenizeCSSValue(tokens, value, valueLength, ",", 1);
    }

    bool needToRemoveAndUpdate = shouldKeepAppearanceOrder(keyKind);
    CSSStyleValuePair cssStyleValuePair;
    if (cssStyleValuePair.updateValueCommon(tokens) ||
        cssStyleValuePair.updateValueForAttributeBasic(m_node->document(),
                                                       keyKind, tokens)) {
        cssStyleValuePair.setFlagImportant(isImportant);
        if (needToRemoveAndUpdate) {
            removeCSSValuePair(keyKind);
        }
        addCSSValuePair(keyKind, cssStyleValuePair);
        return true;
    } else if (cssStyleValuePair.updateValueVarReferences(tokens)) {
        cssStyleValuePair.setValue(String::fromUTF8(value, valueLength));
        cssStyleValuePair.setFlagImportant(isImportant);
        if (needToRemoveAndUpdate) {
            removeCSSValuePair(keyKind);
        }
        addCSSValuePair(keyKind, cssStyleValuePair);
        return true;
    }

    return false;
}

template <>
bool CSSStyleDeclaration::setPropertyInternalFor<
    CSSStyleDeclaration::PropertyType::kShorthand>(
    CSSStyleValuePair::KeyKind keyKind, const char* value, size_t valueLength,
    bool isImportant)
{
    // Use macros to prevent missing shorthanded properties.
    switch (keyKind) {
#define SET_ATTR(name, ...)                         \
    case CSSStyleValuePair::KeyKind::name: {        \
        set##name(value, valueLength, isImportant); \
        return true;                                \
    }
        FOR_EACH_STYLE_ATTRIBUTE_SHORTHAND(SET_ATTR)
#undef SET_ATTR
    default:
        STARFISH_LOG_WARN("Keykind is not shorthand.");
        return false;
    }
    return false;
}

template <>
bool CSSStyleDeclaration::setPropertyInternalFor<
    CSSStyleDeclaration::PropertyType::kSticky>(
    CSSStyleValuePair::KeyKind keyKind, const char* value, size_t valueLength,
    bool isImportant)
{
    // Use macros to prevent missing shorthanded properties.
    switch (keyKind) {
#define SET_ATTR(name, ...)                         \
    case CSSStyleValuePair::KeyKind::name: {        \
        set##name(value, valueLength, isImportant); \
        return true;                                \
    }
        FOR_EACH_STYLE_ATTRIBUTE_STICKY(SET_ATTR)
#undef SET_ATTR
    default:
        STARFISH_LOG_WARN("Keykind is not shorthand.");
        return false;
    }
    return false;
}

bool CSSStyleDeclaration::setPropertyInternal(
    CSSStyleValuePair::KeyKind keyKind, const char* value, size_t valueLength,
    bool isImportant)
{
    MutationObservationScope scope;
    if (isInlineStyle() && m_node->document()->hasMutationObserversOfType(
                               MutationObserverOptionType::kAttributes)) {
        String* old = nullptr;
        QualifiedName qname(m_node->starfish()->staticStrings()->m_style);
        if (m_node->isElement()) {
            auto maybeOld = m_node->asElement()->getAttribute(qname);
            if (maybeOld) {
                old = maybeOld.getValue();
            }
        }
        scope.startAttributeMutationScope(m_node, qname, old);
    }

    if (isShorthandProperty(keyKind)) {
        return setPropertyInternalFor<PropertyType::kShorthand>(
            keyKind, value, valueLength, isImportant);
    } else if (isStickyProperty(keyKind)) {
        return setPropertyInternalFor<PropertyType::kSticky>(
            keyKind, value, valueLength, isImportant);
    } else {
        return setPropertyInternalFor<PropertyType::kLonghand>(
            keyKind, value, valueLength, isImportant);
    }
}

void CSSStyleDeclaration::setProperty(String* name, String* value,
                                      String* prior)
{
    bool isImportant = false;
    if (prior && !prior->equals("undefined") && prior->length() > 0) {
        if (prior->equalsIgnoreCase("important")) {
            isImportant = true;
        }
    }

    setProperty(name, value, isImportant, true);
}

String* CSSStyleDeclaration::removeProperty(String* name)
{
    if (isComputedStyle()) {
        throw new DOMException(m_node->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "Computed property is read-only");
    }

    CSSStyleValuePair::KeyKind keyKind;
    name->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
            CSSStyleValuePair::KeyKind* keyKind =
                reinterpret_cast<CSSStyleValuePair::KeyKind*>(data);
            *keyKind = lookupName(buf, len);
            return 0;
        },
        &keyKind);

    String* value = String::emptyString;
    if (keyKind == CSSStyleValuePair::KeyKind::CustomProperty) {
        value = customProperty(name);
        removeCustomProperty(name);
    } else {
        value = removePropertyInternal(keyKind);
    }

    return value;
}

String* CSSStyleDeclaration::removePropertyInternal(
    CSSStyleValuePair::KeyKind keyKind)
{
    String* value = String::emptyString;
    if (isShorthandProperty(keyKind)) {
        switch (keyKind) {
            // Use macros to prevent missing shorthanded properties.
#define MATCH_KEY(Name, ...)                       \
    case CSSStyleValuePair::KeyKind::Name:         \
        value = getPropertyValueInternal(keyKind); \
        remove##Name();                            \
        break;
            FOR_EACH_STYLE_ATTRIBUTE_SHORTHAND(MATCH_KEY)
#undef MATCH_KEY
        default:
            STARFISH_ASSERT_NOT_REACHED();
            break;
        }
    } else {
        value = getPropertyValueInternal(keyKind);
        removeCSSValuePair(keyKind);
    }
    return value;
}

String* CSSStyleDeclaration::cssText() const
{
    return generateCSSText();
}

void CSSStyleDeclaration::setCssText(String* text)
{
}

void CSSStyleDeclaration::defaultNamedEnumerator(GCVector<String*>& enums)
{
#define ENUM_ATTR(name, nameLower, ...) \
    enums.push_back(String::createASCIIString(#nameLower));
    FOR_EACH_STYLE_ATTRIBUTE_TOTAL(ENUM_ATTR)
#undef ENUM_ATTR
}

bool CSSStyleDeclaration::defaultNamedSetter(String* name,
                                             Optional<String*> value)
{
    CSSStyleValuePair::KeyKind keyKind;
    name->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
            CSSStyleValuePair::KeyKind* keykind =
                reinterpret_cast<CSSStyleValuePair::KeyKind*>(data);
            // defaultNamedSetter allows camel-case name.
            *keykind = CSSStyleLookupTrie::lookupCSSStyleCamelCase(buf, len);
            if (*keykind == CSSStyleValuePair::KeyKind::Unknown) {
                *keykind = CSSStyleLookupTrie::lookupCSSStyle(buf, len);
            }
            return 0;
        },
        &keyKind);

    if (keyKind == CSSStyleValuePair::KeyKind::Unknown) {
        return false;
    }

    if (UNLIKELY(isComputedStyle())) {
        throw new DOMException(m_node->executionContext(),
                               DOMException::DOM_EXCEPTION,
                               "Computed property is read-only");
    }
    // Empty string let setter remove its value
    String* valueTo = String::emptyString;
    if (value.hasValue()) {
        valueTo = value.getValue();
    }

    struct Sender2 {
        CSSStyleDeclaration* self;
        CSSStyleValuePair::KeyKind kind;
    } sender2;
    sender2.self = this;
    sender2.kind = keyKind;

    valueTo->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
            CSSStyleValuePair::KeyKind kind = ((Sender2*)data)->kind;
            CSSStyleDeclaration* self = ((Sender2*)data)->self;
            self->setPropertyInternal(kind, buf, len, false);
            return 0;
        },
        &sender2);
    return true;
}

void CSSStyleDeclaration::appendCSSText(StringBuilder& txtBuilder,
                                        const size_t pos, const char* cssName,
                                        String* value, bool isImportant) const
{
    CSSStyleValuePair::KeyKind kind =
        CSSStyleLookupTrie::lookupCSSStyle(cssName, strlen(cssName));
    if (kind != CSSStyleValuePair::KeyKind::All &&
        kind != CSSStyleValuePair::KeyKind::Direction &&
        kind != CSSStyleValuePair::KeyKind::UnicodeBidi) {
        txtBuilder.appendString(cssName, strlen(cssName));
        txtBuilder.appendString(": ");
        auto iter = std::find_if(m_cssValues.begin() + pos, m_cssValues.end(),
                                 [kind](CSSStyleValuePair p) {
                                     return (static_cast<int>(p.keyKind()) -
                                             1) == (static_cast<int>(kind) - 2);
                                 });
        if (iter != m_cssValues.end()) {
            txtBuilder.appendString(iter->toString());
        } else {
            txtBuilder.appendString(value);
        }
        if (isImportant || iter->flagImportant()) {
            txtBuilder.appendString(" !important");
        }
        txtBuilder.appendString("; ");
    }
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
    StringBuilder textBuilder;
#define APPEND_CSS_VALUES(Name, name, cssName) \
    appendCSSText(textBuilder, pos, cssName, value, isImportant);
    FOR_EACH_STYLE_ATTRIBUTE_TOTAL(APPEND_CSS_VALUES)
#undef APPEND_CSS_VALUES
    return textBuilder.finalize();
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
    return txt.finalize()->trim();
}

bool CSSStyleDeclaration::isInInlineStyleWithVarFunctionValueKind(
    CSSStyleValuePair::KeyKind keyKind)
{
    STARFISH_ASSERT(isShorthandProperty(keyKind));
    // Shorthand is used as the key value only when it has VarFunctionValueKind
    // as the value.
    return isInlineStyle() && hasCSSValuePair(keyKind);
}

String* CSSStyleDeclaration::Background()
{
    StringBuilder builder;
    String* images = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BackgroundImage);
    String* sizes = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BackgroundSize);
    String* attachments = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BackgroundAttachment);
    String* origins = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BackgroundOrigin);
    String* clips = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BackgroundClip);
    String* color = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BackgroundColor);

    String* positions = getPropertyValueInternalFor<PropertyType::kShorthand>(
        CSSStyleValuePair::KeyKind::BackgroundPosition);
    String* repeats = getPropertyValueInternalFor<PropertyType::kShorthand>(
        CSSStyleValuePair::KeyKind::BackgroundRepeat);

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
        removeBackground();
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
    if (v.updateValueVarReferences(tokens)) {
        v.setValue(String::fromUTF8(value, length));
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::Background, v);
        return;
    } else if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addBackgroundCSSValuePairs(this, v, v, v, v, v, v, v, v, v, v);
    } else {
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
                addMultiValueToOwner(image, layerImage);
                addMultiValueToOwner(repeatX, layerRepeatX);
                addMultiValueToOwner(repeatY, layerRepeatY);
                addMultiValueToOwner(positionX, layerPositionX);
                addMultiValueToOwner(positionY, layerPositionY);
                addMultiValueToOwner(size, layerSize);
                addMultiValueToOwner(attachment, layerAttachment);
                addMultiValueToOwner(origin, layerOrigin);
                addMultiValueToOwner(clip, layerClip);
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
}

void CSSStyleDeclaration::removeBackground()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundColor);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundImage);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatX);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundRepeatY);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundPositionX);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundPositionY);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundSize);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundAttachment);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundOrigin);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BackgroundClip);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::Background);
}

bool CSSStyleDeclaration::parseUnitPositionShorthand(
    const CSSTokenVector& tokens, CSSStyleValuePair::KeyKind keykind,
    CSSStyleValuePair* retx, CSSStyleValuePair* rety, bool allowComma)
{
    // [ [ <percentage> | <length> | left | center | right ] [ <percentage> |
    // <length> | top | center | bottom ]? ] | [ [ left | center | right ] || [
    // top | center | bottom ] ] | inherit
    size_t len = 0;
    retx->setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
    rety->setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
    retx->setValueList(new ValueList(Separator::SpaceSeparator));
    rety->setValueList(new ValueList(Separator::SpaceSeparator));

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
        if (keykind == CSSStyleValuePair::KeyKind::BackgroundPosition) {
            x.setKeyKind(CSSStyleValuePair::BackgroundPositionX);
            y.setKeyKind(CSSStyleValuePair::BackgroundPositionY);
        } else if (keykind == CSSStyleValuePair::KeyKind::MaskPosition) {
            x.setKeyKind(CSSStyleValuePair::MaskPositionX);
            y.setKeyKind(CSSStyleValuePair::MaskPositionY);
        }

        if (len == 1) {
            const CSSTokenValue& tok = tokens[i - 1];
            if (x.updateValueUnitPositionX(tok)) {
                y = CSSStyleValuePair(
                    CSSStyleValuePair::ValueKind::SideValueKind,
                    SideValue::CenterSideValue);
            } else if (y.updateValueUnitPositionY(tok)) {
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
            checker &= x.updateValueUnitPositionX(tok1);
            checker &= y.updateValueUnitPositionY(tok2);

            if (!checker && x.valueKind() == sideKind &&
                y.valueKind() == sideKind) {
                checker = true;
                checker &= x.updateValueUnitPositionX(tok2);
                checker &= y.updateValueUnitPositionY(tok1);
            }
            if (!checker) {
                return false;
            }
        } else if (len == 4) {
            // The first value and third values one of the keyword values top,
            // left, bottom, right The second and fourth values are <length> or
            // <percentage> values.
            CSSStyleValuePair first, second, third, fourth;
            const CSSTokenValue& tok1 = tokens[i - 4];
            const CSSTokenValue& tok2 = tokens[i - 3];
            const CSSTokenValue& tok3 = tokens[i - 2];
            const CSSTokenValue& tok4 = tokens[i - 1];

            bool firstIsX = true;
            // Try to parse X.
            if (!(first.updateValueUnitPositionX(tok1) &&
                  first.valueKind() ==
                      CSSStyleValuePair::ValueKind::SideValueKind &&
                  first.sideValue() != SideValue::CenterSideValue)) {
                firstIsX = false;
                // Try to parse Y.
                if (!(first.updateValueUnitPositionY(tok1) &&
                      first.valueKind() ==
                          CSSStyleValuePair::ValueKind::SideValueKind &&
                      first.sideValue() != SideValue::CenterSideValue)) {
                    return false;
                }
            }

            bool thirdIsX = true;
            // Try to parse X.
            if (!(third.updateValueUnitPositionX(tok3) &&
                  third.valueKind() ==
                      CSSStyleValuePair::ValueKind::SideValueKind &&
                  third.sideValue() != SideValue::CenterSideValue)) {
                thirdIsX = false;
                // Try to parse Y.
                if (!(third.updateValueUnitPositionY(tok3) &&
                      third.valueKind() ==
                          CSSStyleValuePair::ValueKind::SideValueKind &&
                      third.sideValue() != SideValue::CenterSideValue)) {
                    return false;
                }
            }

            if (firstIsX == thirdIsX) {
                return false;
            }

            // Parse offset.
            uint8_t option = CSSPropertyParser::AllowNegative |
                             CSSPropertyParser::AllowPercent;
            if (!(second.updateValueUnitLengthOrCalc(tok2, option) &&
                  fourth.updateValueUnitLengthOrCalc(tok4, option))) {
                return false;
            }

            ValuePair* firstPair = new ValuePair(first, second);
            ValuePair* secondPair = new ValuePair(third, fourth);
            if (firstIsX) {
                x.setValuePair(firstPair);
                y.setValuePair(secondPair);
            } else {
                x.setValuePair(secondPair);
                y.setValuePair(firstPair);
            }
        } else {
            // TODO: 3 value, 4 value case.
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
    return UnitPosition(CSSStyleValuePair::KeyKind::BackgroundPosition);
}

void CSSStyleDeclaration::setBackgroundPosition(const char* value,
                                                size_t length, bool isImportant)
{
    setUnitPosition(value, length, isImportant,
                    CSSStyleValuePair::KeyKind::BackgroundPosition);
}

void CSSStyleDeclaration::removeBackgroundPosition()
{
    removeUnitPosition(CSSStyleValuePair::KeyKind::BackgroundPosition);
}

String* CSSStyleDeclaration::BackgroundRepeat()
{
    return UnitRepeatStyle(CSSStyleValuePair::KeyKind::BackgroundRepeat);
}

void CSSStyleDeclaration::setBackgroundRepeat(const char* value, size_t length,
                                              bool isImportant)
{
    setUnitRepeatStyle(value, length, isImportant,
                       CSSStyleValuePair::KeyKind::BackgroundRepeat);
}

void CSSStyleDeclaration::removeBackgroundRepeat()
{
    removeUnitRepeatStyle(CSSStyleValuePair::KeyKind::BackgroundRepeat);
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
        removeBorder();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len);

    CSSStyleValuePair v, width, style, color;
    if (v.updateValueVarReferences(tokens)) {
        v.setValue(String::fromUTF8(value, len));
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::Border, v);
    } else if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        removeBorderBlockStart();
        removeBorderBlockEnd();
        removeBorderInlineStart();
        removeBorderInlineEnd();
        addBorderCSSValuePairs(v, v, v);
    } else if (parseBorderShorthand(tokens, &width, &style, &color)) {
        width.setFlagImportant(isImportant);
        style.setFlagImportant(isImportant);
        color.setFlagImportant(isImportant);
        removeBorderBlockStart();
        removeBorderBlockEnd();
        removeBorderInlineStart();
        removeBorderInlineEnd();
        addBorderCSSValuePairs(width, style, color);
    }
}

void CSSStyleDeclaration::removeBorder()
{
    removeBorderTop();
    removeBorderRight();
    removeBorderBottom();
    removeBorderLeft();
    removeCSSValuePair(CSSStyleValuePair::KeyKind::Border);
}

String* CSSStyleDeclaration::BorderBlockStart()
{
    String* width = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderBlockStartWidth);
    String* style = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderBlockStartStyle);
    String* color = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderBlockStartColor);
    return BorderString(width, false, style, false, color, false);
}

void CSSStyleDeclaration::setBorderBlockStart(const char* value, size_t len,
                                              bool isImportant)
{
    if (len == 0) {
        removeBorderBlockStart();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len);

    CSSStyleValuePair v;

    std::pair<CSSStyleValuePair::KeyKind, CSSStyleValuePair> longhands[3] = {
        { CSSStyleValuePair::KeyKind::BorderBlockStartWidth,
          CSSStyleValuePair() },
        { CSSStyleValuePair::KeyKind::BorderBlockStartStyle,
          CSSStyleValuePair() },
        { CSSStyleValuePair::KeyKind::BorderBlockStartColor,
          CSSStyleValuePair() },
    };

    if (v.updateValueVarReferences(tokens)) {
        v.setValue(String::fromUTF8(value, len));
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderBlockStart, v);
    } else if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        for (int i = 0; i < 3; i++) {
            longhands[i].second = v;
        }
        addFlowRelativeBorderCSSValuePairs(longhands);
        // Add dummy value to mark that above longhands are derived from
        // shorthand.
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderBlockStart,
                        CSSStyleValuePair());
    } else if (parseBorderShorthand(tokens, &longhands[0].second,
                                    &longhands[1].second,
                                    &longhands[2].second)) {
        for (int i = 0; i < 3; i++) {
            longhands[i].second.setFlagImportant(isImportant);
        }
        addFlowRelativeBorderCSSValuePairs(longhands);
        // Add dummy value to mark that above longhands are derived from
        // shorthand.
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderBlockStart,
                        CSSStyleValuePair());
    }
}

void CSSStyleDeclaration::removeBorderBlockStart()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderBlockStartWidth);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderBlockStartStyle);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderBlockStartColor);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderBlockStart);
}

String* CSSStyleDeclaration::BorderBlockEnd()
{
    String* width = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderBlockEndWidth);
    String* style = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderBlockEndStyle);
    String* color = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderBlockEndColor);
    return BorderString(width, false, style, false, color, false);
}

void CSSStyleDeclaration::setBorderBlockEnd(const char* value, size_t len,
                                            bool isImportant)
{
    if (len == 0) {
        removeBorderBlockEnd();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len);

    CSSStyleValuePair v;

    std::pair<CSSStyleValuePair::KeyKind, CSSStyleValuePair> longhands[3] = {
        { CSSStyleValuePair::KeyKind::BorderBlockEndWidth,
          CSSStyleValuePair() },
        { CSSStyleValuePair::KeyKind::BorderBlockEndStyle,
          CSSStyleValuePair() },
        { CSSStyleValuePair::KeyKind::BorderBlockEndColor,
          CSSStyleValuePair() },
    };

    if (v.updateValueVarReferences(tokens)) {
        v.setValue(String::fromUTF8(value, len));
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderBlockEnd, v);
    } else if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        for (int i = 0; i < 3; i++) {
            longhands[i].second = v;
        }
        addFlowRelativeBorderCSSValuePairs(longhands);
        // Add dummy value to mark that above longhands are derived from
        // shorthand.
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderBlockEnd,
                        CSSStyleValuePair());
    } else if (parseBorderShorthand(tokens, &longhands[0].second,
                                    &longhands[1].second,
                                    &longhands[2].second)) {
        for (int i = 0; i < 3; i++) {
            longhands[i].second.setFlagImportant(isImportant);
        }
        addFlowRelativeBorderCSSValuePairs(longhands);
        // Add dummy value to mark that above longhands are derived from
        // shorthand.
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderBlockEnd,
                        CSSStyleValuePair());
    }
}

void CSSStyleDeclaration::removeBorderBlockEnd()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderBlockEndWidth);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderBlockEndStyle);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderBlockEndColor);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderBlockEnd);
}

String* CSSStyleDeclaration::BorderInlineStart()
{
    String* width = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderInlineStartWidth);
    String* style = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderInlineStartStyle);
    String* color = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderInlineStartColor);
    return BorderString(width, false, style, false, color, false);
}

void CSSStyleDeclaration::setBorderInlineStart(const char* value, size_t len,
                                               bool isImportant)
{
    if (len == 0) {
        removeBorderInlineStart();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len);

    CSSStyleValuePair v;

    std::pair<CSSStyleValuePair::KeyKind, CSSStyleValuePair> longhands[3] = {
        { CSSStyleValuePair::KeyKind::BorderInlineStartWidth,
          CSSStyleValuePair() },
        { CSSStyleValuePair::KeyKind::BorderInlineStartStyle,
          CSSStyleValuePair() },
        { CSSStyleValuePair::KeyKind::BorderInlineStartColor,
          CSSStyleValuePair() },
    };

    if (v.updateValueVarReferences(tokens)) {
        v.setValue(String::fromUTF8(value, len));
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderInlineStart, v);
    } else if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        for (int i = 0; i < 3; i++) {
            longhands[i].second = v;
        }
        addFlowRelativeBorderCSSValuePairs(longhands);
        // Add dummy value to mark that above longhands are derived from
        // shorthand.
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderInlineStart,
                        CSSStyleValuePair());
    } else if (parseBorderShorthand(tokens, &longhands[0].second,
                                    &longhands[1].second,
                                    &longhands[2].second)) {
        for (int i = 0; i < 3; i++) {
            longhands[i].second.setFlagImportant(isImportant);
        }
        addFlowRelativeBorderCSSValuePairs(longhands);
        // Add dummy value to mark that above longhands are derived from
        // shorthand.
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderInlineStart,
                        CSSStyleValuePair());
    }
}

void CSSStyleDeclaration::removeBorderInlineStart()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderInlineStartWidth);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderInlineStartStyle);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderInlineStartColor);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderInlineStart);
}

String* CSSStyleDeclaration::BorderInlineEnd()
{
    String* width = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderInlineEndWidth);
    String* style = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderInlineEndStyle);
    String* color = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderInlineEndColor);
    return BorderString(width, false, style, false, color, false);
}

void CSSStyleDeclaration::setBorderInlineEnd(const char* value, size_t len,
                                             bool isImportant)
{
    if (len == 0) {
        removeBorderInlineEnd();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len);

    CSSStyleValuePair v;

    std::pair<CSSStyleValuePair::KeyKind, CSSStyleValuePair> longhands[3] = {
        { CSSStyleValuePair::KeyKind::BorderInlineEndWidth,
          CSSStyleValuePair() },
        { CSSStyleValuePair::KeyKind::BorderInlineEndStyle,
          CSSStyleValuePair() },
        { CSSStyleValuePair::KeyKind::BorderInlineEndColor,
          CSSStyleValuePair() },
    };

    if (v.updateValueVarReferences(tokens)) {
        v.setValue(String::fromUTF8(value, len));
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderInlineEnd, v);
    } else if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        for (int i = 0; i < 3; i++) {
            longhands[i].second = v;
        }
        addFlowRelativeBorderCSSValuePairs(longhands);
        // Add dummy value to mark that above longhands are derived from
        // shorthand.
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderInlineEnd,
                        CSSStyleValuePair());
    } else if (parseBorderShorthand(tokens, &longhands[0].second,
                                    &longhands[1].second,
                                    &longhands[2].second)) {
        for (int i = 0; i < 3; i++) {
            longhands[i].second.setFlagImportant(isImportant);
        }
        addFlowRelativeBorderCSSValuePairs(longhands);
        // Add dummy value to mark that above longhands are derived from
        // shorthand.
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderInlineEnd,
                        CSSStyleValuePair());
    }
}

void CSSStyleDeclaration::removeBorderInlineEnd()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderInlineEndWidth);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderInlineEndStyle);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderInlineEndColor);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderInlineEnd);
}

void CSSStyleDeclaration::addFlowRelativeBorderCSSValuePairs(
    std::pair<CSSStyleValuePair::KeyKind, CSSStyleValuePair> longhands[3])
{
    for (int i = 0; i < 3; i++) {
        if (shouldKeepAppearanceOrder(longhands[i].first)) {
            removeCSSValuePair(longhands[i].first);
        }
        addCSSValuePair(longhands[i].first, longhands[i].second);
    }
}

String* CSSStyleDeclaration::BorderColor(bool* isCombined)
{
    String* top = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderTopColor);
    String* right = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderRightColor);
    String* bottom = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderBottomColor);
    String* left = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderLeftColor);

    if (top && right && bottom && left) {
        return combineBoxString(top, right, bottom, left, isCombined);
    }
    return String::emptyString;
}

void CSSStyleDeclaration::setBorderColor(const char* value, size_t length,
                                         bool isImportant)
{
    if (length == 0) {
        removeBorderColor();
        return;
    }

    constexpr CSSStyleValuePair::KeyKind sides[4] = {
        CSSStyleValuePair::KeyKind::BorderTopColor,
        CSSStyleValuePair::KeyKind::BorderRightColor,
        CSSStyleValuePair::KeyKind::BorderBottomColor,
        CSSStyleValuePair::KeyKind::BorderLeftColor
    };

    setFourSidedShorthandProperty(CSSStyleValuePair::KeyKind::BorderColor,
                                  sides, value, length, isImportant);
}

void CSSStyleDeclaration::removeBorderColor()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderTopColor);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderRightColor);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderBottomColor);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderLeftColor);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderColor);
}

String* CSSStyleDeclaration::BorderStyle(bool* isCombined)
{
    String* top = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderTopStyle);
    String* right = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderRightStyle);
    String* bottom = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderBottomStyle);
    String* left = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderLeftStyle);

    if (top && right && bottom && left) {
        return combineBoxString(top, right, bottom, left, isCombined);
    }
    return String::emptyString;
}

void CSSStyleDeclaration::setBorderStyle(const char* value, size_t length,
                                         bool isImportant)
{
    if (length == 0) {
        removeBorderStyle();
        return;
    }

    constexpr CSSStyleValuePair::KeyKind sides[4] = {
        CSSStyleValuePair::KeyKind::BorderTopStyle,
        CSSStyleValuePair::KeyKind::BorderRightStyle,
        CSSStyleValuePair::KeyKind::BorderBottomStyle,
        CSSStyleValuePair::KeyKind::BorderLeftStyle
    };

    setFourSidedShorthandProperty(CSSStyleValuePair::KeyKind::BorderStyle,
                                  sides, value, length, isImportant);
}

void CSSStyleDeclaration::removeBorderStyle()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderTopStyle);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderRightStyle);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderBottomStyle);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderLeftStyle);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderStyle);
}

String* CSSStyleDeclaration::BorderWidth(bool* isCombined)
{
    String* top = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderTopWidth);
    String* right = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderRightWidth);
    String* bottom = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderBottomWidth);
    String* left = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderLeftWidth);

    if (top && right && bottom && left) {
        return combineBoxString(top, right, bottom, left, isCombined);
    }
    return String::emptyString;
}

void CSSStyleDeclaration::setBorderWidth(const char* value, size_t length,
                                         bool isImportant)
{
    if (length == 0) {
        removeBorderWidth();
        return;
    }

    constexpr CSSStyleValuePair::KeyKind sides[4] = {
        CSSStyleValuePair::KeyKind::BorderTopWidth,
        CSSStyleValuePair::KeyKind::BorderRightWidth,
        CSSStyleValuePair::KeyKind::BorderBottomWidth,
        CSSStyleValuePair::KeyKind::BorderLeftWidth
    };

    setFourSidedShorthandProperty(CSSStyleValuePair::KeyKind::BorderWidth,
                                  sides, value, length, isImportant);
}

void CSSStyleDeclaration::removeBorderWidth()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderTopWidth);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderRightWidth);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderBottomWidth);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderLeftWidth);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderWidth);
}

String* CSSStyleDeclaration::BorderTop()
{
    String* width = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderTopWidth);
    String* style = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderTopStyle);
    String* color = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderTopColor);
    return BorderString(width, false, style, false, color, false);
}

void CSSStyleDeclaration::setBorderTop(const char* value, size_t len,
                                       bool isImportant)
{
    if (len == 0) {
        removeBorderTop();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len);

    CSSStyleValuePair v, width, style, color;
    if (v.updateValueVarReferences(tokens)) {
        v.setValue(String::fromUTF8(value, len));
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderTop, v);
    } else if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addBorderTopCSSValuePairs(v, v, v);
    } else if (parseBorderShorthand(tokens, &width, &style, &color)) {
        width.setFlagImportant(isImportant);
        style.setFlagImportant(isImportant);
        color.setFlagImportant(isImportant);
        addBorderTopCSSValuePairs(width, style, color);
    }
}

void CSSStyleDeclaration::removeBorderTop()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderTopWidth);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderTopStyle);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderTopColor);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderTop);
}

String* CSSStyleDeclaration::BorderRight()
{
    String* width = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderRightWidth);
    String* style = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderRightStyle);
    String* color = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderRightColor);
    return BorderString(width, false, style, false, color, false);
}

void CSSStyleDeclaration::setBorderRight(const char* value, size_t len,
                                         bool isImportant)
{
    if (len == 0) {
        removeBorderRight();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len);

    CSSStyleValuePair v, width, style, color;
    if (v.updateValueVarReferences(tokens)) {
        v.setValue(String::fromUTF8(value, len));
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderRight, v);
    } else if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addBorderRightCSSValuePairs(v, v, v);
    } else if (parseBorderShorthand(tokens, &width, &style, &color)) {
        width.setFlagImportant(isImportant);
        style.setFlagImportant(isImportant);
        color.setFlagImportant(isImportant);
        addBorderRightCSSValuePairs(width, style, color);
    }
}

void CSSStyleDeclaration::removeBorderRight()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderRightWidth);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderRightStyle);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderRightColor);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderRight);
}

String* CSSStyleDeclaration::BorderBottom()
{
    String* width = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderBottomWidth);
    String* style = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderBottomStyle);
    String* color = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderBottomColor);
    return BorderString(width, false, style, false, color, false);
}

void CSSStyleDeclaration::setBorderBottom(const char* value, size_t len,
                                          bool isImportant)
{
    if (len == 0) {
        removeBorderBottom();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len);
    CSSStyleValuePair v, width, style, color;

    if (v.updateValueVarReferences(tokens)) {
        v.setValue(String::fromUTF8(value, len));
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderBottom, v);
    } else if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addBorderBottomCSSValuePairs(v, v, v);
    } else if (parseBorderShorthand(tokens, &width, &style, &color)) {
        width.setFlagImportant(isImportant);
        style.setFlagImportant(isImportant);
        color.setFlagImportant(isImportant);
        addBorderBottomCSSValuePairs(width, style, color);
    }
}

void CSSStyleDeclaration::removeBorderBottom()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderBottomWidth);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderBottomStyle);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderBottomColor);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderBottom);
}

String* CSSStyleDeclaration::BorderLeft()
{
    String* width = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderLeftWidth);
    String* style = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderLeftStyle);
    String* color = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderLeftColor);
    return BorderString(width, false, style, false, color, false);
}

void CSSStyleDeclaration::setBorderLeft(const char* value, size_t len,
                                        bool isImportant)
{
    if (len == 0) {
        removeBorderLeft();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len);
    CSSStyleValuePair v, width, style, color;

    if (v.updateValueVarReferences(tokens)) {
        v.setValue(String::fromUTF8(value, len));
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderLeft, v);
    } else if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addBorderLeftCSSValuePairs(v, v, v);
    } else if (parseBorderShorthand(tokens, &width, &style, &color)) {
        width.setFlagImportant(isImportant);
        style.setFlagImportant(isImportant);
        color.setFlagImportant(isImportant);
        addBorderLeftCSSValuePairs(width, style, color);
    }
}

void CSSStyleDeclaration::removeBorderLeft()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderLeftWidth);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderLeftStyle);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderLeftColor);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderLeft);
}

String* CSSStyleDeclaration::BorderRadius()
{
    String* tl = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderTopLeftRadius);
    String* tr = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderTopRightRadius);
    String* br = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderBottomRightRadius);
    String* bl = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::BorderBottomLeftRadius);

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
    if (len == 0) {
        removeBorderRadius();
        return;
    }
    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len, "/", 1);

    // {1~4} / {1~4}
    if (tokens.size() < 1 || tokens.size() > 9) {
        return;
    }

    CSSStyleValuePair v;
    if (v.updateValueVarReferences(tokens)) {
        v.setValue(String::fromUTF8(value, len));
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderRadius, v);
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
    addCSSValuePair(CSSStyleValuePair::KeyKind::BorderTopLeftRadius, topLeft);
    addCSSValuePair(CSSStyleValuePair::KeyKind::BorderTopRightRadius, topRight);
    addCSSValuePair(CSSStyleValuePair::KeyKind::BorderBottomRightRadius,
                    bottomRight);
    addCSSValuePair(CSSStyleValuePair::KeyKind::BorderBottomLeftRadius,
                    bottomLeft);
}

void CSSStyleDeclaration::removeBorderRadius()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderTopLeftRadius);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderTopRightRadius);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderBottomRightRadius);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderBottomLeftRadius);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderRadius);
}

String* CSSStyleDeclaration::BorderImage()
{
    STARFISH_UNIMPLEMENTED();
    return String::emptyString;
}

void CSSStyleDeclaration::setBorderImage(const char* value, size_t length,
                                         bool isImportant)
{
    if (length == 0) {
        removeBorderImage();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length, "/", 1);

    CSSStyleValuePair v, source, slice, width, outset, repeat;
    if (v.updateValueVarReferences(tokens)) {
        v.setValue(String::fromUTF8(value, length));
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::BorderImage, v);
    } else if (v.updateValueCommon(tokens)) {
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

void CSSStyleDeclaration::removeBorderImage()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageSource);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageSlice);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageWidth);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageOutset);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderImageRepeat);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::BorderImage);
}

void CSSStyleDeclaration::setD(const char* value, size_t len, bool isImportant)
{
    if (len == 0) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::D);
        return;
    }

    if (strlen("inherit") == strlen(value) &&
        (memcmp(value, "inherit", strlen("inherit"))) == 0) {
        CSSStyleValuePair pair;
        pair.setFlagImportant(isImportant);
        pair.setKeyKind(CSSStyleValuePair::KeyKind::D);
        pair.setValueKind(CSSStyleValuePair::ValueKind::Inherit);
        addCSSValuePair(CSSStyleValuePair::KeyKind::D, pair);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len, ",", 1);

    CSSStyleValuePair v;
    if (v.updateValueVarReferences(tokens)) {
        v.setValue(String::fromUTF8(value, len));
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::D, v);
        return;
    }

    Optional<CSSTokenValue> mayFunctionBlock =
        CSSPropertyParser::parseFunctionBlock(value, "path");
    if (!mayFunctionBlock.hasValue()) {
        return;
    }
    Optional<CSSTokenValue> mayQuoteBlock =
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
    String* flexGrow = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::FlexGrow);
    String* flexShrink = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::FlexShrink);
    String* flexBasis = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::FlexBasis);

    if (flexGrow->isEmpty() && flexShrink->isEmpty() && flexBasis->isEmpty()) {
        return String::emptyString;
    }

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
        removeFlex();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, str, length);
    const CSSTokenValue& value = tokens[0];

    // TODO comma separation
    CSSStyleValuePair v, flexGrow, flexShrink, flexBasis;
    float f;
    if (v.updateValueVarReferences(tokens)) {
        v.setValue(String::fromUTF8(str, length));
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::Flex, v);
    } else if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addFlexCSSValuePairs(this, v, v, v);
    } else if (value.equals("auto")) {
        flexGrow.setFlagImportant(isImportant);
        flexShrink.setFlagImportant(isImportant);
        flexBasis.setFlagImportant(isImportant);
        flexGrow.setValueKind(CSSStyleValuePair::ValueKind::Number);
        flexGrow.setValue(1.0f);
        flexShrink.setValueKind(CSSStyleValuePair::ValueKind::Number);
        flexShrink.setValue(1.0f);
        flexBasis.setValueKind(CSSStyleValuePair::ValueKind::Auto);
        addFlexCSSValuePairs(this, flexGrow, flexShrink, flexBasis);
    } else if (value.equals("none")) {
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

void CSSStyleDeclaration::removeFlex()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::FlexGrow);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::FlexShrink);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::FlexBasis);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::Flex);
}

void CSSStyleDeclaration::setFlexFlow(const char* value, size_t length,
                                      bool isImportant)
{
    if (length == 0) {
        removeFlexFlow();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length);

    // TODO comma separation
    CSSStyleValuePair v, flexDirection, flexWrap;
    if (v.updateValueVarReferences(tokens)) {
        v.setValue(String::fromUTF8(value, length));
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::FlexFlow, v);
    } else if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addFlexFlowCSSValuePairs(this, v, v);
    } else if (parseFlexFlowShorthand(tokens, &flexDirection, &flexWrap)) {
        flexDirection.setFlagImportant(isImportant);
        flexWrap.setFlagImportant(isImportant);
        addFlexFlowCSSValuePairs(this, flexDirection, flexWrap);
    }
}

void CSSStyleDeclaration::removeFlexFlow()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::FlexDirection);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::FlexWrap);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::FlexFlow);
}

String* CSSStyleDeclaration::FlexFlow()
{
    String* flexDirection =
        getPropertyValueInternalFor<PropertyType::kLonghand>(
            CSSStyleValuePair::KeyKind::FlexDirection);
    String* flexWrap = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::FlexWrap);

    if (flexDirection->isEmpty() && flexWrap->isEmpty()) {
        return String::emptyString;
    }

    String* space = String::spaceString;
    StringBuilder builder;
    builder.appendString(flexDirection);
    builder.appendString(space);
    builder.appendString(flexWrap);

    return builder.finalize();
}

String* CSSStyleDeclaration::Font()
{
    String* style = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::FontStyle);
    String* weight = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::FontWeight);
    String* size = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::FontSize);
    String* lineHeight = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::LineHeight);

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
    initialCount += (lineHeight->equals(String::initialString) ? 1 : 0);
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
        removeFont();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length, "/,", 2);
    if (tokens.size() == 0) {
        return;
    }

    CSSStyleValuePair v, style /*, variant*/, weight /*, stretch*/, size,
        lineHeight, fontFamily;
    if (v.updateValueVarReferences(tokens)) {
        v.setValue(String::fromUTF8(value, length));
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::Font, v);
        return;
    } else if (v.updateValueCommon(tokens)) {
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

void CSSStyleDeclaration::removeFont()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::FontFamily);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::FontStyle);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::FontWeight);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::FontSize);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::LineHeight);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::Font);
}

void CSSStyleDeclaration::setFontFamily(const char* value, size_t len,
                                        bool isImportant)
{
    if (len == 0) {
        removeCSSValuePair(CSSStyleValuePair::KeyKind::FontFamily);
        return;
    }

    CSSTokenVector layers;
    if (!CSSPropertyParser::parseLayers(value, len, layers)) {
        return;
    }

    CSSStyleValuePair ret;
    if (ret.updateValueVarReferences(layers)) {
        ret.setValue(String::fromUTF8(value, len));
        ret.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::FontFamily, ret);
    } else if (ret.updateValueCommon(layers) ||
               ret.updateValueFontFamily(layers)) {
        ret.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::FontFamily, ret);
    }
}

String* CSSStyleDeclaration::ListStyle()
{
    String* t = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::ListStyleType);
    String* p = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::ListStylePosition);
    String* i = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::ListStyleImage);

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
        removeListStyle();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len, ",", 1);
    CSSStyleValuePair c, t, p, i;
    if (c.updateValueVarReferences(tokens)) {
        c.setValue(String::fromUTF8(value, len));
        c.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::ListStyle, c);
    } else if (c.updateValueCommon(tokens)) {
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

void CSSStyleDeclaration::removeListStyle()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::ListStyleType);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::ListStylePosition);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::ListStyleImage);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::ListStyle);
}

String* CSSStyleDeclaration::Margin(bool* isCombined)
{
    String* top = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::MarginTop);
    String* right = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::MarginRight);
    String* bottom = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::MarginBottom);
    String* left = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::MarginLeft);
    if (top && right && bottom && left) {
        return combineBoxString(top, right, bottom, left, isCombined);
    }
    return String::emptyString;
}

void CSSStyleDeclaration::setInset(const char* value, size_t length,
                                   bool isImportant)
{
    if (length == 0) {
        removeMargin();
        return;
    }

    constexpr CSSStyleValuePair::KeyKind sides[4] = {
        CSSStyleValuePair::KeyKind::Top, CSSStyleValuePair::KeyKind::Right,
        CSSStyleValuePair::KeyKind::Bottom, CSSStyleValuePair::KeyKind::Left
    };

    setFourSidedShorthandProperty(CSSStyleValuePair::KeyKind::Inset, sides,
                                  value, length, isImportant);
}

void CSSStyleDeclaration::removeInset()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::Top);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::Right);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::Bottom);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::Left);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::Inset);
}

String* CSSStyleDeclaration::Inset(bool* isCombined)
{
    String* top = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::Top);
    String* right = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::Right);
    String* bottom = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::Bottom);
    String* left = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::Left);
    if (top && right && bottom && left) {
        return combineBoxString(top, right, bottom, left, isCombined);
    }
    return String::emptyString;
}

void CSSStyleDeclaration::setMargin(const char* value, size_t length,
                                    bool isImportant)
{
    if (length == 0) {
        removeMargin();
        return;
    }

    constexpr CSSStyleValuePair::KeyKind sides[4] = {
        CSSStyleValuePair::KeyKind::MarginTop,
        CSSStyleValuePair::KeyKind::MarginRight,
        CSSStyleValuePair::KeyKind::MarginBottom,
        CSSStyleValuePair::KeyKind::MarginLeft
    };

    setFourSidedShorthandProperty(CSSStyleValuePair::KeyKind::Margin, sides,
                                  value, length, isImportant);
}

void CSSStyleDeclaration::removeMargin()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::MarginTop);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::MarginRight);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::MarginBottom);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::MarginLeft);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::Margin);
}

String* CSSStyleDeclaration::MarginBlock()
{
    String* start = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::MarginBlockStart);
    String* end = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::MarginBlockEnd);

    if (start->equals(end)) {
        return start;
    }

    return start->concat(" ")->concat(end);
}

void CSSStyleDeclaration::setMarginBlock(const char* value, size_t len,
                                         bool isImportant)
{
    if (len == 0) {
        removeMarginBlock();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len);

    CSSStyleValuePair start, end;
    bool canAdd = false;
    if (start.updateValueVarReferences(tokens)) {
        start.setValue(String::fromUTF8(value, len));
        start.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::MarginBlock, start);
        return;
    } else if (start.updateValueCommon(tokens)) {
        end = start;
        canAdd = true;
    } else if (tokens.size() == 1 &&
               start.updateValueMarginBlockStart(m_node->document(), tokens)) {
        end = start;
        canAdd = true;
    } else if (tokens.size() == 2 && start.updateValueUnitMargin(tokens[0]) &&
               end.updateValueUnitMargin(tokens[1])) {
        canAdd = true;
    }

    if (!canAdd) {
        return;
    }

    if (shouldKeepAppearanceOrder(
            CSSStyleValuePair::KeyKind::MarginBlockStart) ||
        shouldKeepAppearanceOrder(CSSStyleValuePair::KeyKind::MarginBlockEnd)) {
        removeMarginBlock();
    }

    start.setFlagImportant(isImportant);
    end.setFlagImportant(isImportant);
    addCSSValuePair(CSSStyleValuePair::KeyKind::MarginBlockStart, start);
    addCSSValuePair(CSSStyleValuePair::KeyKind::MarginBlockEnd, end);
}

void CSSStyleDeclaration::removeMarginBlock()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::MarginBlockEnd);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::MarginBlockStart);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::MarginBlock);
}

String* CSSStyleDeclaration::MarginInline()
{
    String* start = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::MarginInlineStart);
    String* end = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::MarginInlineEnd);

    if (start->equals(end)) {
        return start;
    }

    return start->concat(" ")->concat(end);
}

void CSSStyleDeclaration::setMarginInline(const char* value, size_t len,
                                          bool isImportant)
{
    if (len == 0) {
        removeMarginInline();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len);

    CSSStyleValuePair start, end;
    bool canAdd = false;
    if (start.updateValueVarReferences(tokens)) {
        start.setValue(String::fromUTF8(value, len));
        start.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::MarginInline, start);
        return;
    } else if (start.updateValueCommon(tokens)) {
        end = start;
        canAdd = true;
    } else if (tokens.size() == 1 &&
               start.updateValueMarginInlineStart(m_node->document(), tokens)) {
        end = start;
        canAdd = true;
    } else if (tokens.size() == 2 && start.updateValueUnitMargin(tokens[0]) &&
               end.updateValueUnitMargin(tokens[1])) {
        canAdd = true;
    }

    if (!canAdd) {
        return;
    }

    if (shouldKeepAppearanceOrder(
            CSSStyleValuePair::KeyKind::MarginInlineStart) ||
        shouldKeepAppearanceOrder(
            CSSStyleValuePair::KeyKind::MarginInlineEnd)) {
        removeMarginInline();
    }

    start.setFlagImportant(isImportant);
    end.setFlagImportant(isImportant);
    addCSSValuePair(CSSStyleValuePair::KeyKind::MarginInlineStart, start);
    addCSSValuePair(CSSStyleValuePair::KeyKind::MarginInlineEnd, end);
}

void CSSStyleDeclaration::removeMarginInline()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::MarginInlineEnd);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::MarginInlineStart);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::MarginInline);
}

String* CSSStyleDeclaration::Outline()
{
    String* width = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::OutlineWidth);
    String* style = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::OutlineStyle);
    String* color = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::OutlineColor);
    return BorderString(width, false, style, false, color, false);
}

void CSSStyleDeclaration::setOutline(const char* value, size_t length,
                                     bool isImportant)
{
    if (length == 0) {
        removeOutline();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length);

    CSSStyleValuePair v, width, style, color;
    if (v.updateValueVarReferences(tokens)) {
        v.setValue(String::fromUTF8(value, length));
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::Outline, v);
    } else if (v.updateValueCommon(tokens)) {
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

void CSSStyleDeclaration::removeOutline()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::OutlineWidth);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::OutlineStyle);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::OutlineColor);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::Outline);
}

String* CSSStyleDeclaration::Overflow()
{
    // TODO: Should find the specific rule for composing overflow
    String* overflowX = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::OverflowX);
    String* overflowY = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::OverflowY);

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
        removeOverflow();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length);

    // TODO comma separation
    CSSStyleValuePair v, overflowX, overflowY;
    if (v.updateValueVarReferences(tokens)) {
        v.setValue(String::fromUTF8(value, length));
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::Overflow, v);
    } else if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addOverflowCSSValuePairs(this, v, v);
    } else if (parseOverflowShorthand(tokens, &overflowX, &overflowY)) {
        overflowX.setFlagImportant(isImportant);
        overflowY.setFlagImportant(isImportant);
        addOverflowCSSValuePairs(this, overflowX, overflowY);
    }
}

void CSSStyleDeclaration::removeOverflow()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::OverflowX);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::OverflowY);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::Overflow);
}

String* CSSStyleDeclaration::Padding(bool* isCombined)
{
    String* top = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::PaddingTop);
    String* right = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::PaddingRight);
    String* bottom = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::PaddingBottom);
    String* left = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::PaddingLeft);
    if (top && right && bottom && left) {
        return combineBoxString(top, right, bottom, left, isCombined);
    }
    return String::emptyString;
}

void CSSStyleDeclaration::setPadding(const char* value, size_t length,
                                     bool isImportant)
{
    if (length == 0) {
        removePadding();
        return;
    }

    constexpr CSSStyleValuePair::KeyKind sides[4] = {
        CSSStyleValuePair::KeyKind::PaddingTop,
        CSSStyleValuePair::KeyKind::PaddingRight,
        CSSStyleValuePair::KeyKind::PaddingBottom,
        CSSStyleValuePair::KeyKind::PaddingLeft
    };

    setFourSidedShorthandProperty(CSSStyleValuePair::KeyKind::Padding, sides,
                                  value, length, isImportant);
}

void CSSStyleDeclaration::removePadding()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::PaddingTop);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::PaddingRight);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::PaddingBottom);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::PaddingLeft);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::Padding);
}

String* CSSStyleDeclaration::PaddingBlock()
{
    String* start = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::PaddingBlockStart);
    String* end = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::PaddingBlockEnd);

    if (start->equals(end)) {
        return start;
    }

    return start->concat(" ")->concat(end);
}

void CSSStyleDeclaration::setPaddingBlock(const char* value, size_t len,
                                          bool isImportant)
{
    if (len == 0) {
        removePaddingBlock();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len);

    CSSStyleValuePair start, end;
    bool canAdd = false;
    if (start.updateValueVarReferences(tokens)) {
        start.setValue(String::fromUTF8(value, len));
        start.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::PaddingBlock, start);
        return;
    } else if (start.updateValueCommon(tokens)) {
        end = start;
        canAdd = true;
    } else if (tokens.size() == 1 &&
               start.updateValuePaddingBlockStart(m_node->document(), tokens)) {
        end = start;
        canAdd = true;
    } else if (tokens.size() == 2 && start.updateValueUnitPadding(tokens[0]) &&
               end.updateValueUnitPadding(tokens[1])) {
        canAdd = true;
    }

    if (!canAdd) {
        return;
    }

    if (shouldKeepAppearanceOrder(
            CSSStyleValuePair::KeyKind::PaddingBlockStart) ||
        shouldKeepAppearanceOrder(
            CSSStyleValuePair::KeyKind::PaddingBlockEnd)) {
        removePaddingBlock();
    }

    start.setFlagImportant(isImportant);
    end.setFlagImportant(isImportant);
    addCSSValuePair(CSSStyleValuePair::KeyKind::PaddingBlockStart, start);
    addCSSValuePair(CSSStyleValuePair::KeyKind::PaddingBlockEnd, end);
}

void CSSStyleDeclaration::removePaddingBlock()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::PaddingBlockEnd);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::PaddingBlockStart);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::PaddingBlock);
}

String* CSSStyleDeclaration::PaddingInline()
{
    String* start = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::PaddingInlineStart);
    String* end = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::PaddingInlineEnd);

    if (start->equals(end)) {
        return start;
    } else {
        return start->concat(" ")->concat(end);
    }
    return String::emptyString;
}

void CSSStyleDeclaration::setPaddingInline(const char* value, size_t len,
                                           bool isImportant)
{
    if (len == 0) {
        removePaddingInline();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len);

    CSSStyleValuePair start, end;
    bool canAdd = false;
    if (start.updateValueVarReferences(tokens)) {
        start.setValue(String::fromUTF8(value, len));
        start.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::PaddingInline, start);
        return;
    } else if (start.updateValueCommon(tokens)) {
        end = start;
        canAdd = true;
    } else if (tokens.size() == 1 && start.updateValuePaddingInlineStart(
                                         m_node->document(), tokens)) {
        end = start;
        canAdd = true;
    } else if (tokens.size() == 2 && start.updateValueUnitPadding(tokens[0]) &&
               end.updateValueUnitPadding(tokens[1])) {
        canAdd = true;
    }

    if (!canAdd) {
        return;
    }

    if (shouldKeepAppearanceOrder(
            CSSStyleValuePair::KeyKind::PaddingInlineStart) ||
        shouldKeepAppearanceOrder(
            CSSStyleValuePair::KeyKind::PaddingInlineEnd)) {
        removePaddingInline();
    }

    start.setFlagImportant(isImportant);
    end.setFlagImportant(isImportant);
    addCSSValuePair(CSSStyleValuePair::KeyKind::PaddingInlineStart, start);
    addCSSValuePair(CSSStyleValuePair::KeyKind::PaddingInlineEnd, end);
}

void CSSStyleDeclaration::removePaddingInline()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::PaddingInlineEnd);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::PaddingInlineStart);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::PaddingInline);
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
    } else if (ret.updateValueVarReferences(tokens)) {
        ret.setValue(String::fromUTF8(value, len));
        ret.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::Src, ret);
    }
}

String* CSSStyleDeclaration::GridTemplate()
{
    String* gridTemplateRows =
        getPropertyValueInternalFor<PropertyType::kLonghand>(
            CSSStyleValuePair::KeyKind::GridTemplateRows);
    String* gridTemplateColumns =
        getPropertyValueInternalFor<PropertyType::kLonghand>(
            CSSStyleValuePair::KeyKind::GridTemplateColumns);
    String* gridTemplateAreas =
        getPropertyValueInternalFor<PropertyType::kLonghand>(
            CSSStyleValuePair::KeyKind::GridTemplateAreas);

    if (gridTemplateRows->isEmpty() && gridTemplateColumns->isEmpty() &&
        gridTemplateAreas->isEmpty()) {
        return String::emptyString;
    }

    StringBuilder builder;
    builder.appendString(gridTemplateRows);
    builder.appendString(" / ");
    builder.appendString(gridTemplateColumns);
    builder.appendString(" / ");
    builder.appendString(gridTemplateAreas);

    return builder.finalize();
}

void CSSStyleDeclaration::setGridTemplate(const char* value, size_t len,
                                          bool isImportant)
{
    if (len == 0) {
        removeGridTemplate();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len, "/", 1);

    CSSStyleValuePair v;
    if (v.updateValueVarReferences(tokens)) {
        v.setValue(String::fromUTF8(value, len));
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::GridTemplate, v);
        return;
    }

    if (v.updateValueCommon(tokens)) {
        v.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::GridTemplateAreas, v);
        addCSSValuePair(CSSStyleValuePair::KeyKind::GridTemplateRows, v);
        addCSSValuePair(CSSStyleValuePair::KeyKind::GridTemplateColumns, v);
        return;
    }

    bool seenSlash = false;
    size_t beforeSlash = 0;
    for (size_t i = 0; i < tokens.size(); i++) {
        auto& token = tokens[i];
        if (token == "/") {
            if (seenSlash || i == 0) {
                return;
            }
            seenSlash = true;
        } else {
            if (!seenSlash) {
                beforeSlash++;
            }
        }
    }

    CSSTokenVector tokensForGridTemplateAreas;
    CSSTokenVector tokensForGridTemplateRows;
    bool lastIsString = false;
    for (size_t i = 0; i < beforeSlash; i++) {
        CSSStyleValuePair temp;
        auto& token = tokens[i];
        bool isString = isQuote(token[0]) && isQuote(token[token.length() - 1]);
        if (isString) {
            if (lastIsString) {
                tokensForGridTemplateRows.push_back(CSSTokenValue("auto"));
            }
            tokensForGridTemplateAreas.push_back(tokens[i]);
            lastIsString = true;
        } else {
            tokensForGridTemplateRows.push_back(tokens[i]);
            lastIsString = false;
        }
    }

    if (lastIsString) {
        tokensForGridTemplateRows.push_back(CSSTokenValue("auto"));
    }

    if (!(tokensForGridTemplateAreas.size() ==
          tokensForGridTemplateRows.size()) &&
        !(!tokensForGridTemplateAreas.size() &&
          tokensForGridTemplateRows.size())) {
        return;
    }
    if (!tokensForGridTemplateAreas.size()) {
        // Set default value.
        tokensForGridTemplateAreas.push_back(CSSTokenValue("none"));
    }

    CSSTokenVector tokensForGridTemplateColumns;
    for (size_t i = beforeSlash + 1; i < tokens.size(); i++) {
        tokensForGridTemplateColumns.push_back(tokens[i]);
    }
    if (!tokensForGridTemplateColumns.size()) {
        // Set default value.
        tokensForGridTemplateColumns.push_back(CSSTokenValue("none"));
    }

    CSSStyleValuePair gridTemplateAreas, gridTemplateRows, gridTemplateColumns;
    gridTemplateAreas.setFlagImportant(isImportant);
    gridTemplateRows.setFlagImportant(isImportant);
    gridTemplateColumns.setFlagImportant(isImportant);
    if (gridTemplateAreas.updateValueGridTemplateAreas(
            m_node->document(), tokensForGridTemplateAreas) &&
        gridTemplateRows.updateValueGridTemplateRows(
            m_node->document(), tokensForGridTemplateRows) &&
        gridTemplateColumns.updateValueGridTemplateColumns(
            m_node->document(), tokensForGridTemplateColumns)) {
        addCSSValuePair(CSSStyleValuePair::KeyKind::GridTemplateAreas,
                        gridTemplateAreas);
        addCSSValuePair(CSSStyleValuePair::KeyKind::GridTemplateRows,
                        gridTemplateRows);
        addCSSValuePair(CSSStyleValuePair::KeyKind::GridTemplateColumns,
                        gridTemplateColumns);
    }
}

void CSSStyleDeclaration::removeGridTemplate()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::GridTemplateAreas);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::GridTemplateRows);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::GridTemplateColumns);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::GridTemplate);
}

String* CSSStyleDeclaration::TextDecoration()
{
    updateValue(CSSStyleValuePair::TextDecorationLine);
    updateValue(CSSStyleValuePair::TextDecorationStyle);
    updateValue(CSSStyleValuePair::TextDecorationColor);

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
        removeTextDecoration();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, len, "", 0);

    CSSStyleValuePair ret;
    if (ret.updateValueVarReferences(tokens)) {
        ret.setValue(String::fromUTF8(value, len));
        ret.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::TextDecoration, ret);
        return;
    } else if (ret.updateValueCommon(tokens)) {
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

void CSSStyleDeclaration::removeTextDecoration()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationLine);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationStyle);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::TextDecorationColor);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::TextDecoration);
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
    ValueList* list = new ValueList(Separator::CommaSeparator);
    size_t layerSize = layers.size();
    for (size_t i = 0; i < layerSize; i++) {
        CSSTokenVector tokens;
        tokenizeCSSValue(tokens, layers[i].data(), layers[i].length());

        CSSStyleValuePair sub;
        if (sub.updateValueVarReferences(tokens)) {
            sub.setValue(String::fromUTF8(value, length));
            sub.setFlagImportant(isImportant);
            addCSSValuePair(CSSStyleValuePair::KeyKind::TransitionProperty,
                            sub);
            return;
        } else if (!(layerSize == 1 && sub.updateValueCommon(tokens)) &&
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
    ValueList* list = new ValueList(Separator::CommaSeparator);
    size_t layerSize = layers.size();
    for (size_t i = 0; i < layerSize; i++) {
        CSSTokenVector tokens;
        tokenizeCSSValue(tokens, layers[i].data(), layers[i].length());

        CSSStyleValuePair sub;
        if (sub.updateValueVarReferences(tokens)) {
            sub.setValue(String::fromUTF8(value, length));
            sub.setFlagImportant(isImportant);
            addCSSValuePair(CSSStyleValuePair::KeyKind::TransitionDuration,
                            sub);
            return;
        } else if (!(layerSize == 1 && sub.updateValueCommon(tokens)) &&
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
    ValueList* list = new ValueList(Separator::CommaSeparator);
    size_t layerSize = layers.size();
    for (size_t i = 0; i < layerSize; i++) {
        CSSTokenVector tokens;
        tokenizeCSSValue(tokens, layers[i].data(), layers[i].length());

        CSSStyleValuePair sub;
        if (sub.updateValueVarReferences(tokens)) {
            sub.setValue(String::fromUTF8(value, length));
            sub.setFlagImportant(isImportant);
            addCSSValuePair(
                CSSStyleValuePair::KeyKind::TransitionTimingFunction, sub);
            return;
        } else if (!(layerSize == 1 && sub.updateValueCommon(tokens)) &&
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
    ValueList* list = new ValueList(Separator::CommaSeparator);
    size_t layerSize = layers.size();
    for (size_t i = 0; i < layerSize; i++) {
        CSSTokenVector tokens;
        tokenizeCSSValue(tokens, layers[i].data(), layers[i].length());

        CSSStyleValuePair sub;
        if (sub.updateValueVarReferences(tokens)) {
            sub.setValue(String::fromUTF8(value, length));
            sub.setFlagImportant(isImportant);
            addCSSValuePair(CSSStyleValuePair::KeyKind::TransitionDelay, sub);
            return;
        } else if (!(layerSize == 1 && sub.updateValueCommon(tokens)) &&
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

    StringBuilder builder;

    // Case that includes custom variables
    if (hasCSSValuePair(CSSStyleValuePair::Transition)) {
        CSSStyleValuePair v = getCSSValuePair(CSSStyleValuePair::Transition);

        STARFISH_ASSERT(v.valueKind() ==
                        CSSStyleValuePair::ValueKind::VarFunctionValueKind);
        builder.appendString(v.toString());
        return builder.finalize();
    }

    CSSStyleValuePair v[4];
    size_t size[4] = { 0, 0, 0, 0 };
    size_t maxLayer = 0;
    for (size_t k = 0; k < kKeySize; k++) {
        updateValue(kKeys[k]);

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
        // There are not arguments.
        removeTransition();
        return;
    }
    // TODO handle var() case
    CSSTokenVector layers;
    if (!CSSPropertyParser::parseLayers(value, length, layers)) {
        return;
    }

    ValueList* properties = new ValueList(Separator::CommaSeparator);
    ValueList* durations = new ValueList(Separator::CommaSeparator);
    ValueList* timingFns = new ValueList(Separator::CommaSeparator);
    ValueList* delays = new ValueList(Separator::CommaSeparator);

    size_t layerSize = layers.size();
    for (size_t i = 0; i < layerSize; i++) {
        CSSTokenVector tokens;
        tokenizeCSSValue(tokens, layers[i].data(), layers[i].length());

        CSSStyleValuePair v, v0, v1, v2, v3;
        if (v.updateValueVarReferences(tokens)) {
            v.setValue(String::fromUTF8(value, length));
            v.setFlagImportant(isImportant);
            addCSSValuePair(CSSStyleValuePair::KeyKind::Transition, v);
            return;
        } else if (layerSize == 1 && v0.updateValueCommon(tokens)) {
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
    r0.setFlagImportant(isImportant);
    addCSSValuePair(CSSStyleValuePair::TransitionProperty, r0);

    r1.setValueList(durations);
    r1.setFlagImportant(isImportant);
    addCSSValuePair(CSSStyleValuePair::TransitionDuration, r1);

    r2.setValueList(timingFns);
    r2.setFlagImportant(isImportant);
    addCSSValuePair(CSSStyleValuePair::TransitionTimingFunction, r2);

    r3.setValueList(delays);
    r3.setFlagImportant(isImportant);
    addCSSValuePair(CSSStyleValuePair::TransitionDelay, r3);
}

void CSSStyleDeclaration::removeTransition()
{
    // There are not arguments.
    removeCSSValuePair(CSSStyleValuePair::TransitionProperty);
    removeCSSValuePair(CSSStyleValuePair::TransitionDuration);
    removeCSSValuePair(CSSStyleValuePair::TransitionDelay);
    removeCSSValuePair(CSSStyleValuePair::TransitionTimingFunction);
    removeCSSValuePair(CSSStyleValuePair::Transition);
}

String* CSSStyleDeclaration::Animation()
{
    const size_t kKeySize = 7;
    const CSSStyleValuePair::KeyKind kKeys[kKeySize] = {
        CSSStyleValuePair::AnimationName,
        CSSStyleValuePair::AnimationDuration,
        CSSStyleValuePair::AnimationTimingFunction,
        CSSStyleValuePair::AnimationDelay,
        CSSStyleValuePair::AnimationIterationCount,
        CSSStyleValuePair::AnimationDirection,
        CSSStyleValuePair::AnimationPlayState,
    };

    for (size_t i = 0; i < kKeySize; i++) {
        updateValue(kKeys[i]);
    }

    CSSStyleValuePair v[7];
    size_t size[7] = { 0, 0, 0, 0, 0, 0, 0 };
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

void CSSStyleDeclaration::setAnimationName(const char* value, size_t length,
                                           bool isImportant)
{
    STARFISH_ASSERT(value != nullptr);
    if (length == 0) {
        removeCSSValuePair(CSSStyleValuePair::AnimationName);
        return;
    }

    // TODO handle var() case
    CSSTokenVector layers;
    if (CSSPropertyParser::parseLayers(value, length, layers) == false) {
        return;
    }
    ValueList* list = new ValueList(Separator::CommaSeparator);
    size_t layerSize = layers.size();
    for (size_t i = 0; i < layerSize; i++) {
        CSSStyleValuePair sub;
        CSSTokenVector tokens;
        tokenizeCSSValue(tokens, layers[i].data(), layers[i].length(), "", 0,
                         true);
        if (!(layerSize == 1 && sub.updateValueCommon(tokens) == true) &&
            sub.updateValueLayerAnimationName(tokens) == false) {
            return;
        }
        list->push_back(sub);
    }
    CSSStyleValuePair result;
    result.setFlagImportant(isImportant);
    result.setValueList(list);
    addCSSValuePair(CSSStyleValuePair::AnimationName, result);
}

void CSSStyleDeclaration::setAnimationDuration(const char* value, size_t length,
                                               bool isImportant)
{
    STARFISH_ASSERT(value != nullptr);
    if (length == 0) {
        removeCSSValuePair(CSSStyleValuePair::AnimationDuration);
        return;
    }
    // TODO handle var() case
    CSSTokenVector layers;
    if (CSSPropertyParser::parseLayers(value, length, layers) == false) {
        return;
    }
    ValueList* list = new ValueList(Separator::CommaSeparator);
    size_t layerSize = layers.size();
    for (size_t i = 0; i < layerSize; i++) {
        CSSStyleValuePair sub;
        CSSTokenVector tokens;
        tokenizeCSSValue(tokens, layers[i].data(), layers[i].length());
        if (!(layerSize == 1 && sub.updateValueCommon(tokens) == true) &&
            sub.updateValueLayerAnimationDuration(tokens) == false) {
            return;
        }
        list->push_back(sub);
    }
    CSSStyleValuePair result;
    result.setFlagImportant(isImportant);
    result.setValueList(list);
    addCSSValuePair(CSSStyleValuePair::AnimationDuration, result);
}

void CSSStyleDeclaration::setAnimationTimingFunction(const char* value,
                                                     size_t length,
                                                     bool isImportant)
{
    STARFISH_ASSERT(value != nullptr);
    if (length == 0) {
        removeCSSValuePair(CSSStyleValuePair::AnimationTimingFunction);
        return;
    }
    // TODO handle var() case
    CSSTokenVector layers;
    if (CSSPropertyParser::parseLayers(value, length, layers) == false) {
        return;
    }
    ValueList* list = new ValueList(Separator::CommaSeparator);
    size_t layerSize = layers.size();
    for (size_t i = 0; i < layerSize; i++) {
        CSSStyleValuePair sub;
        CSSTokenVector tokens;
        tokenizeCSSValue(tokens, layers[i].data(), layers[i].length());
        if (!(layerSize == 1 && sub.updateValueCommon(tokens) == true) &&
            sub.updateValueLayerAnimationTimingFunction(tokens) == false) {
            return;
        }
        list->push_back(sub);
    }
    CSSStyleValuePair result;
    result.setFlagImportant(isImportant);
    result.setValueList(list);
    addCSSValuePair(CSSStyleValuePair::AnimationTimingFunction, result);
}

void CSSStyleDeclaration::setAnimationDelay(const char* value, size_t length,
                                            bool isImportant)
{
    STARFISH_ASSERT(value != nullptr);
    if (length == 0) {
        removeCSSValuePair(CSSStyleValuePair::AnimationDelay);
        return;
    }
    // TODO handle var() case
    CSSTokenVector layers;
    if (CSSPropertyParser::parseLayers(value, length, layers) == false) {
        return;
    }
    ValueList* list = new ValueList(Separator::CommaSeparator);
    size_t layerSize = layers.size();
    for (size_t i = 0; i < layerSize; i++) {
        CSSStyleValuePair sub;
        CSSTokenVector tokens;
        tokenizeCSSValue(tokens, layers[i].data(), layers[i].length());
        if (!(layerSize == 1 && sub.updateValueCommon(tokens) == true) &&
            sub.updateValueLayerAnimationDelay(tokens) == false) {
            return;
        }
        list->push_back(sub);
    }
    CSSStyleValuePair result;
    result.setFlagImportant(isImportant);
    result.setValueList(list);
    addCSSValuePair(CSSStyleValuePair::AnimationDelay, result);
}

void CSSStyleDeclaration::setAnimationIterationCount(const char* value,
                                                     size_t length,
                                                     bool isImportant)
{
    STARFISH_ASSERT(value != nullptr);
    if (length == 0) {
        removeCSSValuePair(CSSStyleValuePair::AnimationIterationCount);
        return;
    }
    // TODO handle var() case
    CSSTokenVector layers;
    if (CSSPropertyParser::parseLayers(value, length, layers) == false) {
        return;
    }
    ValueList* list = new ValueList(Separator::CommaSeparator);
    size_t layerSize = layers.size();
    for (size_t i = 0; i < layerSize; i++) {
        CSSStyleValuePair sub;
        CSSTokenVector tokens;
        tokenizeCSSValue(tokens, layers[i].data(), layers[i].length());
        if (!(layerSize == 1 && sub.updateValueCommon(tokens) == true) &&
            sub.updateValueLayerAnimationIterationCount(tokens) == false) {
            return;
        }
        list->push_back(sub);
    }
    CSSStyleValuePair result;
    result.setFlagImportant(isImportant);
    result.setValueList(list);
    addCSSValuePair(CSSStyleValuePair::AnimationIterationCount, result);
}

void CSSStyleDeclaration::setAnimationDirection(const char* value,
                                                size_t length, bool isImportant)
{
    STARFISH_ASSERT(value != nullptr);
    if (length == 0) {
        removeCSSValuePair(CSSStyleValuePair::AnimationDirection);
        return;
    }
    // TODO handle var() case
    CSSTokenVector layers;
    if (CSSPropertyParser::parseLayers(value, length, layers) == false) {
        return;
    }
    ValueList* list = new ValueList(Separator::CommaSeparator);
    size_t layerSize = layers.size();
    for (size_t i = 0; i < layerSize; i++) {
        CSSStyleValuePair sub;
        CSSTokenVector tokens;
        tokenizeCSSValue(tokens, layers[i].data(), layers[i].length());
        if (!(layerSize == 1 && sub.updateValueCommon(tokens) == true) &&
            sub.updateValueLayerAnimationDirection(tokens) == false) {
            return;
        }
        list->push_back(sub);
    }
    CSSStyleValuePair result;
    result.setFlagImportant(isImportant);
    result.setValueList(list);
    addCSSValuePair(CSSStyleValuePair::AnimationDirection, result);
}

void CSSStyleDeclaration::setAnimationPlayState(const char* value,
                                                size_t length, bool isImportant)
{
    STARFISH_ASSERT(value != nullptr);
    if (length == 0) {
        removeCSSValuePair(CSSStyleValuePair::AnimationPlayState);
        return;
    }
    // TODO handle var() case
    CSSTokenVector layers;
    if (CSSPropertyParser::parseLayers(value, length, layers) == false) {
        return;
    }
    ValueList* list = new ValueList(Separator::CommaSeparator);
    size_t layerSize = layers.size();
    for (size_t i = 0; i < layerSize; i++) {
        CSSStyleValuePair sub;
        CSSTokenVector tokens;
        tokenizeCSSValue(tokens, layers[i].data(), layers[i].length());
        if (!(layerSize == 1 && sub.updateValueCommon(tokens) == true) &&
            sub.updateValueLayerAnimationPlayState(tokens) == false) {
            return;
        }
        list->push_back(sub);
    }
    CSSStyleValuePair result;
    result.setFlagImportant(isImportant);
    result.setValueList(list);
    addCSSValuePair(CSSStyleValuePair::AnimationPlayState, result);
}

void CSSStyleDeclaration::setAnimationFillMode(const char* value, size_t length,
                                               bool isImportant)
{
    STARFISH_ASSERT(value != nullptr);
    if (length == 0) {
        removeCSSValuePair(CSSStyleValuePair::AnimationFillMode);
        return;
    }
    // TODO handle var() case
    CSSTokenVector layers;
    if (CSSPropertyParser::parseLayers(value, length, layers) == false) {
        return;
    }
    ValueList* list = new ValueList(Separator::CommaSeparator);
    size_t layerSize = layers.size();
    for (size_t i = 0; i < layerSize; i++) {
        CSSStyleValuePair sub;
        CSSTokenVector tokens;
        tokenizeCSSValue(tokens, layers[i].data(), layers[i].length());
        if (!(layerSize == 1 && sub.updateValueCommon(tokens) == true) &&
            sub.updateValueLayerAnimationFillMode(tokens) == false) {
            return;
        }
        list->push_back(sub);
    }
    CSSStyleValuePair result;
    result.setFlagImportant(isImportant);
    result.setValueList(list);
    addCSSValuePair(CSSStyleValuePair::AnimationFillMode, result);
}

void CSSStyleDeclaration::setAnimation(const char* value, size_t length,
                                       bool isImportant)
{
    if (length == 0) {
        // There are not arguments.
        removeAnimation();
        return;
    }
    // TODO handle var() case
    CSSTokenVector layers;
    if (!CSSPropertyParser::parseLayers(value, length, layers)) {
        return;
    }

    ValueList* names = new ValueList(Separator::CommaSeparator);
    ValueList* durations = new ValueList(Separator::CommaSeparator);
    ValueList* timingFns = new ValueList(Separator::CommaSeparator);
    ValueList* delays = new ValueList(Separator::CommaSeparator);
    ValueList* iterations = new ValueList(Separator::CommaSeparator);
    ValueList* directions = new ValueList(Separator::CommaSeparator);
    ValueList* playStates = new ValueList(Separator::CommaSeparator);
    ValueList* fillModes = new ValueList(Separator::CommaSeparator);

    size_t layerSize = layers.size();
    for (size_t i = 0; i < layerSize; i++) {
        CSSStyleValuePair v0, v1, v2, v3, v4, v5, v6, v7;
        CSSTokenVector tokens;
        tokenizeCSSValue(tokens, layers[i].data(), layers[i].length(), "", 0,
                         true);
        if (layerSize == 1 && v0.updateValueCommon(tokens)) {
            v1 = v2 = v3 = v4 = v5 = v6 = v7 = v0;
        } else if (!parseAnimationShorthand(tokens, &v0, &v1, &v2, &v3, &v4,
                                            &v5, &v6, &v7)) {
            return;
        }
        names->push_back(v0);
        durations->push_back(v1);
        timingFns->push_back(v2);
        delays->push_back(v3);
        iterations->push_back(v4);
        directions->push_back(v5);
        playStates->push_back(v6);
        fillModes->push_back(v7);
    }

    CSSStyleValuePair r0, r1, r2, r3, r4, r5, r6, r7;
    r0.setValueList(names);
    r0.setFlagImportant(isImportant);
    addCSSValuePair(CSSStyleValuePair::AnimationName, r0);

    r1.setValueList(durations);
    r1.setFlagImportant(isImportant);
    addCSSValuePair(CSSStyleValuePair::AnimationDuration, r1);

    r2.setValueList(timingFns);
    r2.setFlagImportant(isImportant);
    addCSSValuePair(CSSStyleValuePair::AnimationTimingFunction, r2);

    r3.setValueList(delays);
    r3.setFlagImportant(isImportant);
    addCSSValuePair(CSSStyleValuePair::AnimationDelay, r3);

    r4.setValueList(iterations);
    r4.setFlagImportant(isImportant);
    addCSSValuePair(CSSStyleValuePair::AnimationIterationCount, r4);

    r5.setValueList(directions);
    r5.setFlagImportant(isImportant);
    addCSSValuePair(CSSStyleValuePair::AnimationDirection, r5);

    r6.setValueList(playStates);
    r6.setFlagImportant(isImportant);
    addCSSValuePair(CSSStyleValuePair::AnimationPlayState, r6);

    r7.setValueList(fillModes);
    r7.setFlagImportant(isImportant);
    addCSSValuePair(CSSStyleValuePair::AnimationFillMode, r7);
}

void CSSStyleDeclaration::removeAnimation()
{
    // There are not arguments.
    removeCSSValuePair(CSSStyleValuePair::AnimationName);
    removeCSSValuePair(CSSStyleValuePair::AnimationDuration);
    removeCSSValuePair(CSSStyleValuePair::AnimationTimingFunction);
    removeCSSValuePair(CSSStyleValuePair::AnimationDelay);
    removeCSSValuePair(CSSStyleValuePair::AnimationIterationCount);
    removeCSSValuePair(CSSStyleValuePair::AnimationDirection);
    removeCSSValuePair(CSSStyleValuePair::AnimationPlayState);
    removeCSSValuePair(CSSStyleValuePair::AnimationFillMode);
    removeCSSValuePair(CSSStyleValuePair::Animation);
}

String* CSSStyleDeclaration::Mask()
{
    // Mask is only supported as SVG attribute.
    STARFISH_UNSUPPORTED("css property: mask");
    return String::emptyString;
}

void CSSStyleDeclaration::setMask(const char* value, size_t length,
                                  bool isImportant)
{
    // Mask is only supported as SVG attribute.
    STARFISH_ASSERT(value != nullptr);
}

void CSSStyleDeclaration::removeMask()
{
    // Mask is only supported as SVG attribute.
}

String* CSSStyleDeclaration::MaskPosition()
{
    return UnitPosition(CSSStyleValuePair::KeyKind::MaskPosition);
}

void CSSStyleDeclaration::setMaskPosition(const char* value, size_t length,
                                          bool isImportant)
{
    setUnitPosition(value, length, isImportant,
                    CSSStyleValuePair::KeyKind::MaskPosition);
}

void CSSStyleDeclaration::removeMaskPosition()
{
    removeUnitPosition(CSSStyleValuePair::KeyKind::MaskPosition);
}

String* CSSStyleDeclaration::MaskRepeat()
{
    return UnitRepeatStyle(CSSStyleValuePair::KeyKind::MaskRepeat);
}

void CSSStyleDeclaration::setMaskRepeat(const char* value, size_t length,
                                        bool isImportant)
{
    setUnitRepeatStyle(value, length, isImportant,
                       CSSStyleValuePair::KeyKind::MaskRepeat);
}

void CSSStyleDeclaration::removeMaskRepeat()
{
    removeUnitRepeatStyle(CSSStyleValuePair::KeyKind::MaskRepeat);
}

String* CSSStyleDeclaration::Gap()
{
    String* row = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::RowGap);
    String* column = getPropertyValueInternalFor<PropertyType::kLonghand>(
        CSSStyleValuePair::KeyKind::ColumnGap);

    if (row->equals(column)) {
        return row;
    }

    return row->concat(" ")->concat(column);
}

void CSSStyleDeclaration::setGap(const char* value, size_t length,
                                 bool isImportant)
{
    if (length == 0) {
        removeGap();
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length);

    CSSStyleValuePair row, column;
    if (row.updateValueVarReferences(tokens)) {
        row.setValue(String::fromUTF8(value, length));
        row.setFlagImportant(isImportant);
        addCSSValuePair(CSSStyleValuePair::KeyKind::Gap, row);
        return;
    } else if (row.updateValueCommon(tokens)) {
        column = row;
    } else if (tokens.size() == 1 &&
               row.updateValueRowGap(m_node->document(), tokens)) {
        column = row;
    } else if (tokens.size() == 2 && row.updateValueUnitGap(tokens[0]) &&
               column.updateValueUnitGap(tokens[1])) {
    } else {
        return;
    }

    row.setFlagImportant(isImportant);
    column.setFlagImportant(isImportant);
    addCSSValuePair(CSSStyleValuePair::KeyKind::RowGap, row);
    addCSSValuePair(CSSStyleValuePair::KeyKind::ColumnGap, column);
}

void CSSStyleDeclaration::removeGap()
{
    removeCSSValuePair(CSSStyleValuePair::KeyKind::RowGap);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::ColumnGap);
    removeCSSValuePair(CSSStyleValuePair::KeyKind::Gap);
}

String* CSSStyleDeclaration::UnitPosition(CSSStyleValuePair::KeyKind keyKind)
{
    CSSStyleValuePair::KeyKind xKind, yKind;
    if (!UnitPositionShorthandToLongHand(keyKind, xKind, yKind)) {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return String::emptyString;
    }

    String* positionX =
        getPropertyValueInternalFor<PropertyType::kLonghand>(xKind);
    String* positionY =
        getPropertyValueInternalFor<PropertyType::kLonghand>(yKind);

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

void CSSStyleDeclaration::setUnitPosition(const char* value, size_t length,
                                          bool isImportant,
                                          CSSStyleValuePair::KeyKind keyKind)
{
    CSSStyleValuePair::KeyKind xKind, yKind;
    if (!UnitPositionShorthandToLongHand(keyKind, xKind, yKind)) {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return;
    }

    if (length == 0) {
        removeUnitPosition(keyKind);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length, ",", 1);

    CSSStyleValuePair c, x, y;
    if (c.updateValueVarReferences(tokens)) {
        c.setValue(String::fromUTF8(value, length));
        c.setFlagImportant(isImportant);
        addCSSValuePair(keyKind, c);
    } else if (c.updateValueCommon(tokens)) {
        c.setFlagImportant(isImportant);
        addCSSValuePair(xKind, c);
        addCSSValuePair(yKind, c);
    } else if (parseUnitPositionShorthand(tokens, keyKind, &x, &y)) {
        x.setFlagImportant(isImportant);
        y.setFlagImportant(isImportant);
        addCSSValuePair(xKind, x);
        addCSSValuePair(yKind, y);
    }
}

void CSSStyleDeclaration::removeUnitPosition(CSSStyleValuePair::KeyKind keyKind)
{
    CSSStyleValuePair::KeyKind xKind, yKind;
    if (!UnitPositionShorthandToLongHand(keyKind, xKind, yKind)) {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return;
    }

    removeCSSValuePair(xKind);
    removeCSSValuePair(yKind);
    removeCSSValuePair(keyKind);
}

String* CSSStyleDeclaration::UnitRepeatStyle(CSSStyleValuePair::KeyKind keyKind)
{
    CSSStyleValuePair::KeyKind xKind, yKind;
    if (!UnitRepeatStyleShorthandToLongHand(keyKind, xKind, yKind)) {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return String::emptyString;
    }

    String* repeatX =
        getPropertyValueInternalFor<PropertyType::kLonghand>(xKind);
    String* repeatY =
        getPropertyValueInternalFor<PropertyType::kLonghand>(yKind);

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

void CSSStyleDeclaration::setUnitRepeatStyle(const char* value, size_t length,
                                             bool isImportant,
                                             CSSStyleValuePair::KeyKind keyKind)
{
    CSSStyleValuePair::KeyKind xKind, yKind;
    if (!UnitRepeatStyleShorthandToLongHand(keyKind, xKind, yKind)) {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return;
    }

    if (length == 0) {
        removeUnitRepeatStyle(keyKind);
        return;
    }

    CSSTokenVector tokens;
    tokenizeCSSValue(tokens, value, length, ",", 1);

    CSSStyleValuePair c, x, y;
    if (c.updateValueVarReferences(tokens)) {
        c.setValue(String::fromUTF8(value, length));
        c.setFlagImportant(isImportant);
        addCSSValuePair(keyKind, c);
    } else if (c.updateValueCommon(tokens)) {
        c.setFlagImportant(isImportant);
        addCSSValuePair(xKind, c);
        addCSSValuePair(yKind, c);
    } else if (parseUnitRepeatStyle(tokens, &x, &y)) {
        x.setFlagImportant(isImportant);
        y.setFlagImportant(isImportant);
        addCSSValuePair(xKind, x);
        addCSSValuePair(yKind, y);
    }
}

void CSSStyleDeclaration::removeUnitRepeatStyle(
    CSSStyleValuePair::KeyKind keyKind)
{
    CSSStyleValuePair::KeyKind xKind, yKind;
    if (!UnitRepeatStyleShorthandToLongHand(keyKind, xKind, yKind)) {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return;
    }

    removeCSSValuePair(xKind);
    removeCSSValuePair(yKind);
    removeCSSValuePair(keyKind);
}

StyleRuleCSSStyleDeclaration::StyleRuleCSSStyleDeclaration(
    CSSStyleDeclaration* src, CSSRule* parentRule)
    : CSSStyleDeclaration(parentRule->parentStyleSheet()
                              ->scriptBindingInstance()
                              ->ownerDocument())
{
    m_cssValues = src->m_cssValues;
    if (src->m_cssCustomValues) {
        m_cssCustomValues =
            new MutablePropertyValueList(*src->m_cssCustomValues);
    }
    m_pointerRooter = src->m_pointerRooter;
    m_parentRule = parentRule;
}

CSSStyleSheet* StyleRuleCSSStyleDeclaration::parentStyleSheet() const
{
    STARFISH_ASSERT(m_parentRule != nullptr);
    return m_parentRule->parentStyleSheet();
}

ScriptBindingInstance* StyleRuleCSSStyleDeclaration::scriptBindingInstance()
{
    STARFISH_ASSERT(m_parentRule != nullptr);
    STARFISH_ASSERT(m_parentRule->parentStyleSheet());
    return m_parentRule->parentStyleSheet()->scriptBindingInstance();
}

void StyleRuleCSSStyleDeclaration::setCssText(String* text)
{
    STARFISH_ASSERT(text != nullptr);
    STARFISH_ASSERT(m_parentRule != nullptr);

    CSSStyleDeclaration* decl;
    if (m_parentRule->type() == CSSRule::Type::KEYFRAME_RULE) {
        decl =
            ((CSSKeyframeRule*)m_parentRule)->styleRule()->styleDeclaration();
    } else {
        decl = ((CSSStyleRule*)m_parentRule)->styleRule()->styleDeclaration();
    }
    decl->clear();

    CSSParser parser(m_node);
    parser.parseStyleDeclaration(text, decl);
    m_cssValues = decl->cssValues();
    m_pointerRooter = decl->m_pointerRooter;

    m_node->styleResolver().setNeedsRecalcRuleSet();

    scriptBindingInstance()
        ->ownerWindow()
        ->browsingContext()
        ->setNeedsStyleSheetsRecalcAndWholeDocumentNeedsStyleRecalc();
}

void InlineCSSStyleDeclaration::setCssText(String* text)
{
    STARFISH_ASSERT(m_node->isElement());
    m_node->asElement()->setStyleAttr(text);
}

} // namespace Starfish
