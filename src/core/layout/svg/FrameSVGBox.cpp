/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"
#include "FrameSVGBox.h"
#include "core/dom/Node.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"

namespace StarFish {

void* FrameSVGBox::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FrameSVGBox)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameSVGBox, m_node));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameSVGBox, m_layoutParent));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameSVGBox, m_treeItemModel.m_parent));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameSVGBox, m_treeItemModel.m_previous));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameSVGBox, m_treeItemModel.m_next));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameSVGBox, m_treeItemModel.m_firstChild));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameSVGBox, m_treeItemModel.m_lastChild));
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FrameSVGBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void FrameSVGBox::layout(LayoutContext& ctx,
                         Frame::LayoutWantToResolve resolveWhat)
{
    FrameBox* cb = layoutParent()->asFrameBox();
    if (node()->asSVGElement()->needsGeometryAttributes()) {
        if (resolveWhat & Frame::ResolveWidth) {
            auto styleWidth = style()->width();
            LayoutUnit width;
            if (!styleWidth.isAuto()) {
                width = styleWidth.specifiedValue(cb->width(), this);
            }
            setWidth(width);
        }

        if (resolveWhat & Frame::ResolveHeight) {
            auto styleHeight = style()->height();
            LayoutUnit height;
            if (!styleHeight.isAuto()) {
                height = styleHeight.specifiedValue(cb->height(), this);
            }
            setHeight(height);
        }
    } else {
        if (resolveWhat & Frame::ResolveWidth) {
            setWidth(cb->width());
        }

        if (resolveWhat & Frame::ResolveHeight) {
            setHeight(cb->height());
        }
    }

    Frame* f = firstChild();
    while (f) {
        f->asFrameSVGBox()->resolvePosition(ctx);
        f->layout(ctx, Frame::LayoutWantToResolve::ResolveAll);
        f = f->next();
    }
}

void FrameSVGBox::resolvePosition(LayoutContext& ctx)
{
    if (node()->asSVGElement()->needsGeometryAttributes()) {
        FrameBox* cb = layoutParent()->asFrameBox();
        LayoutUnit xResult;
        String* x = node()->asElement()->getAttributeOrEmpty(
            node()->starFish()->staticStrings()->m_x);
        if (x->length()) {
            auto s = x->toUTF8NonGCString();
            CSSStyleValuePair pair;
            if (CSSPropertyParser::parseLength(
                    s.data(), CSSPropertyParser::AllowPercent |
                                  CSSPropertyParser::AllowWithoutUnit,
                    &pair)) {
                Length ll = pair.lengthValue();
                xResult = ll.specifiedValue(cb->width(), this);
            }
        }

        setX(xResult);

        LayoutUnit yResult;
        String* y = node()->asElement()->getAttributeOrEmpty(
            node()->starFish()->staticStrings()->m_y);
        if (y->length()) {
            auto s = y->toUTF8NonGCString();
            CSSStyleValuePair pair;
            if (CSSPropertyParser::parseLength(
                    s.data(), CSSPropertyParser::AllowPercent |
                                  CSSPropertyParser::AllowWithoutUnit,
                    &pair)) {
                Length ll = pair.lengthValue();
                yResult = ll.specifiedValue(cb->height(), this);
            }
        }

        setY(yResult);
    }
}

void FrameSVGBox::paintContent(PaintingContext& ctx)
{
    ctx.m_canvas->save();

    if (style()->visibility() == VisibilityValue::HiddenVisibilityValue) {
        ctx.m_canvas->setVisible(false);
    } else {
        ctx.m_canvas->setVisible(true);
    }

    if (style()->hasTransforms()) {
        FrameBox* cb = layoutParent()->asFrameBox();
        auto matrix =
            style()->transformsToMatrix(cb->width(), cb->height(), this, true);
        if (!matrix.isIdentity()) {
            ctx.m_canvas->postMatrix(matrix);
        }
    }

    ctx.m_canvas->save();
    paintSVG(ctx);
    ctx.m_canvas->restore();
    paintChildrenWith(ctx);

    ctx.m_canvas->restore();
}

std::vector<std::pair<double, double>> FrameSVGBox::parsePointsFromString(
    String* str)
{
    std::vector<std::pair<double, double>> result;
    auto utf8Str = str->toUTF8NonGCString();
    CSSTokenVector tokensInput;
    const char* sep = ",-";
    CSSStyleDeclaration::tokenizeCSSValue(tokensInput, utf8Str.data(),
                                          utf8Str.length(), sep, 2, true);
    std::vector<CSSTokenValue> tokens;
    tokens.reserve(tokensInput.size());
    for (size_t i = 0; i < tokensInput.size(); i++) {
        tokens.push_back(std::move(tokensInput[i]));
    }
    enum Mode {
        WaitCoordsX,
        WaitCoordsY,
    };
    Mode mode = Mode::WaitCoordsX;
    bool gotMinus = false;
    float x, y;

#define READ_NUMBER(n)                                             \
    if (!CSSPropertyParser::parseNumber(                           \
            token.data(), CSSPropertyParser::AllowNegative, &n)) { \
        break;                                                     \
    }                                                              \
    if (gotMinus) {                                                \
        n = -n;                                                    \
    }                                                              \
    gotMinus = false;

    for (size_t i = 0; i < tokens.size(); i++) {
        const auto& token = tokens[i];
        if (token.equals(",")) {
            continue;
        }

        if (token.equals("-")) {
            if (gotMinus) {
                // error
                break;
            }
            gotMinus = true;
            continue;
        }

        {
            auto token = tokens[i];
            bool hasMultipleDot = false;
            bool seenDot = false;
            for (size_t k = 0; k < token.size(); k++) {
                if (token[k] == '.') {
                    if (!seenDot) {
                        seenDot = true;
                    } else {
                        hasMultipleDot = true;
                        tokens.erase(tokens.begin() + i);
                        CSSTokenValue s1 = token.substr(0, k);
                        CSSTokenValue s2 = token.substr(k, token.size() - k);
                        tokens.insert(tokens.begin() + i, s1);
                        tokens.insert(tokens.begin() + i + 1, s2);
                        i--;
                        break;
                    }
                }
            }
            if (hasMultipleDot) {
                continue;
            }
        }

        if (mode == Mode::WaitCoordsX) {
            READ_NUMBER(x);
            mode = Mode::WaitCoordsY;
        } else {
            READ_NUMBER(y);
            mode = Mode::WaitCoordsX;

            result.push_back(std::make_pair(x, y));
        }
    }

    return result;
}

double FrameSVGBox::resolveLengthFromAttribute(QualifiedName attr)
{
    double result = 0;
    String* str = node()->asElement()->getAttributeOrEmpty(attr);
    if (str->length()) {
        auto s = str->toUTF8NonGCString();
        CSSStyleValuePair pair;
        if (CSSPropertyParser::parseLength(
                s.data(), CSSPropertyParser::AllowPercent |
                              CSSPropertyParser::AllowWithoutUnit,
                &pair)) {
            Length ll = pair.lengthValue();
            FrameBox* cb = layoutParent()->asFrameBox();
            result = ll.specifiedValue(cb->width(), this);
        }
    }

    return result;
}
}
