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
#include "core/style/StyleRule.h"

namespace StarFish {

// helper function to convert Length to CSSStyleValuePair format
static CSSStyleValuePair lengthToCSSStyleValue(Length len)
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

static CSSStyleValuePair stylePaintDataToCSSStyleValue(StylePaintData paintData)
{
    CSSStyleValuePair ret;
    if (paintData.color().isTransparent()) {
        ret.setValueKind(CSSStyleValuePair::ValueKind::None);
    } else {
        ret.setValueKind(CSSStyleValuePair::ValueKind::ColorValueKind);
        ret.setColorValue(paintData.color());
    }
    return ret;
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

#define DEFINE_ATTRIBUTE_GETTER_FOURSIDE(PRE, ...)                        \
    String* CSSStyleDeclaration::PRE##__VA_ARGS__(bool* isCombined)       \
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
DEFINE_ATTRIBUTE_GETTER_FOURSIDE(Margin);
DEFINE_ATTRIBUTE_GETTER_FOURSIDE(Padding);
DEFINE_ATTRIBUTE_GETTER_FOURSIDE(Border, Width);
DEFINE_ATTRIBUTE_GETTER_FOURSIDE(Border, Style);
DEFINE_ATTRIBUTE_GETTER_FOURSIDE(Border, Color);
#undef DEFINE_ATTRIBUTE_GETTER_FOURSIDE

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

void ComputedStyleCSSStyleDeclaration::layoutIfNeeds()
{
    if (m_node->isDocument()) {
        return;
    }

    m_node->window()->browsingContext()->webView()->layoutIfNeeds();
}

void ComputedStyleCSSStyleDeclaration::buildFrameTreeIfNeeds()
{
    if (m_node->isDocument()) {
        return;
    }

    m_node->window()->browsingContext()->buildFrameTreeIfNeeds();
}

void ComputedStyleCSSStyleDeclaration::resolveStyleIfNeeds()
{
    if (m_node->isDocument()) {
        return;
    }

    m_node->window()->browsingContext()->resolveStyleIfNeeds();
}

ComputedStyleCSSStyleDeclaration::Stage
ComputedStyleCSSStyleDeclaration::requiredStage(
    CSSStyleValuePair::KeyKind keyKind)
{
    ComputedStyleCSSStyleDeclaration::Stage result =
        ComputedStyleCSSStyleDeclaration::Stage::resolveStyle;

    if (keyKind >= CSSStyleValuePair::KeyKind::PaddingTop &&
        keyKind <= CSSStyleValuePair::KeyKind::PaddingLeft) {
        result = ComputedStyleCSSStyleDeclaration::Stage::layout;
    } else if (keyKind >= CSSStyleValuePair::KeyKind::MarginTop &&
               keyKind <= CSSStyleValuePair::KeyKind::MarginLeft) {
        result = ComputedStyleCSSStyleDeclaration::Stage::layout;
    } else if (keyKind >= CSSStyleValuePair::KeyKind::BorderTopWidth &&
               keyKind <= CSSStyleValuePair::KeyKind::BorderLeftWidth) {
        result = ComputedStyleCSSStyleDeclaration::Stage::layout;
    } else if (keyKind >= CSSStyleValuePair::KeyKind::Top &&
               keyKind <= CSSStyleValuePair::KeyKind::Left) {
        result = ComputedStyleCSSStyleDeclaration::Stage::layout;
    } else if (keyKind >= CSSStyleValuePair::KeyKind::Width &&
               keyKind <= CSSStyleValuePair::KeyKind::Height) {
        result = ComputedStyleCSSStyleDeclaration::Stage::layout;
    } else if (keyKind == CSSStyleValuePair::KeyKind::MinWidth ||
               keyKind == CSSStyleValuePair::KeyKind::MinHeight) {
        result = ComputedStyleCSSStyleDeclaration::Stage::frameTreeBuild;
    }
    return result;
}

void ComputedStyleCSSStyleDeclaration::updateValue(
    CSSStyleValuePair::KeyKind keyKind)
{
    if (m_node->isDocument() || m_node->style() == nullptr) {
        return;
    }

    Frame* frame = m_node->frame();
    ComputedStyle* style = m_node->style();

    if ((keyKind >= CSSStyleValuePair::KeyKind::PaddingTop &&
         keyKind <= CSSStyleValuePair::KeyKind::PaddingLeft) ||
        (keyKind >= CSSStyleValuePair::KeyKind::MarginTop &&
         keyKind <= CSSStyleValuePair::KeyKind::MarginLeft) ||
        (keyKind >= CSSStyleValuePair::KeyKind::BorderTopWidth &&
         keyKind <= CSSStyleValuePair::KeyKind::BorderLeftWidth)) {
// length properties
#define ADD_ABSOLUTE_LENGTH_PAIR(keyKind, getter)                     \
    {                                                                 \
        CSSStyleValuePair p;                                          \
        p.setKeyKind(CSSStyleValuePair::KeyKind::keyKind);            \
        if (frame && frame->isFrameBox()) {                           \
            p.setValueKind(CSSStyleValuePair::ValueKind::Length);     \
            p.setValue(CSSLength(frame->asFrameBox()->getter()));     \
        } else if (frame && frame->isFrameInline()) {                 \
            InlineNonReplacedBox* inb =                               \
                blockContainer(frame)->firstInlineNonReplacedBox(     \
                    frame->asFrameInline());                          \
            if (inb != nullptr) {                                     \
                p.setValue(CSSLength(inb->getter()));                 \
                p.setValueKind(CSSStyleValuePair::ValueKind::Length); \
            } else {                                                  \
                p.setValueKind(CSSStyleValuePair::ValueKind::Auto);   \
            }                                                         \
        } else {                                                      \
            p.setValueKind(CSSStyleValuePair::ValueKind::Auto);       \
        }                                                             \
        addValuePair(p);                                              \
    }
        if (keyKind == CSSStyleValuePair::KeyKind::MarginTop) {
            ADD_ABSOLUTE_LENGTH_PAIR(MarginTop, marginTop);
        } else if (keyKind == CSSStyleValuePair::KeyKind::MarginRight) {
            ADD_ABSOLUTE_LENGTH_PAIR(MarginRight, marginRight);
        } else if (keyKind == CSSStyleValuePair::KeyKind::MarginBottom) {
            ADD_ABSOLUTE_LENGTH_PAIR(MarginBottom, marginBottom);
        } else if (keyKind == CSSStyleValuePair::KeyKind::MarginLeft) {
            ADD_ABSOLUTE_LENGTH_PAIR(MarginLeft, marginLeft);
        } else if (keyKind == CSSStyleValuePair::KeyKind::PaddingTop) {
            ADD_ABSOLUTE_LENGTH_PAIR(PaddingTop, paddingTop);
        } else if (keyKind == CSSStyleValuePair::KeyKind::PaddingRight) {
            ADD_ABSOLUTE_LENGTH_PAIR(PaddingRight, paddingRight);
        } else if (keyKind == CSSStyleValuePair::KeyKind::PaddingBottom) {
            ADD_ABSOLUTE_LENGTH_PAIR(PaddingBottom, paddingBottom);
        } else if (keyKind == CSSStyleValuePair::KeyKind::PaddingLeft) {
            ADD_ABSOLUTE_LENGTH_PAIR(PaddingLeft, paddingLeft);
        } else if (keyKind == CSSStyleValuePair::KeyKind::BorderTopWidth) {
            ADD_ABSOLUTE_LENGTH_PAIR(BorderTopWidth, borderTop);
        } else if (keyKind == CSSStyleValuePair::KeyKind::BorderRightWidth) {
            ADD_ABSOLUTE_LENGTH_PAIR(BorderRightWidth, borderRight);
        } else if (keyKind == CSSStyleValuePair::KeyKind::BorderBottomWidth) {
            ADD_ABSOLUTE_LENGTH_PAIR(BorderBottomWidth, borderBottom);
        } else if (keyKind == CSSStyleValuePair::KeyKind::BorderLeftWidth) {
            ADD_ABSOLUTE_LENGTH_PAIR(BorderLeftWidth, borderLeft)
        } else {
            STARFISH_ASSERT_NOT_REACHED();
        }
    } else if (keyKind >= CSSStyleValuePair::KeyKind::Top &&
               keyKind <= CSSStyleValuePair::KeyKind::Right) {
        CSSStyleValuePair t, b, l, r;
        t.setKeyKind(CSSStyleValuePair::KeyKind::Top);
        b.setKeyKind(CSSStyleValuePair::KeyKind::Bottom);
        l.setKeyKind(CSSStyleValuePair::KeyKind::Left);
        r.setKeyKind(CSSStyleValuePair::KeyKind::Right);
        if (frame && frame->isFrameBox() && frame->isPositioned()) {
            LayoutContext ctx(m_node->starFish(),
                              m_node->document()->frame()->asFrameDocument());
            FrameBox* cb = containingBlock(frame);
            FrameBox* self = frame->asFrameBox();
            LayoutUnit parentContentWidth = cb->contentWidth();
            LayoutUnit parentContentHeight = cb->contentHeight();
            if (frame->isAbsolutePositioned()) {
                FrameBox* parent = frame->layoutParent()->asFrameBox();

                LayoutLocation l1, l2;
                if (cb->isAncestorOf(parent)) {
                    l2 = parent->absolutePoint(cb);
                } else {
                    l1 = cb->absolutePoint(ctx.frameDocument());
                    l2 = parent->absolutePoint(ctx.frameDocument());
                }

                if (keyKind == CSSStyleValuePair::KeyKind::Top ||
                    keyKind == CSSStyleValuePair::KeyKind::Bottom) {
                    LayoutUnit absY =
                        self->y() + l2.y() - l1.y() - cb->borderTop();
                    LayoutUnit top = absY - self->marginTop();
                    parentContentHeight += cb->paddingHeight();
                    t.setLengthValue(CSSLength(top));
                    b.setLengthValue(CSSLength(parentContentHeight - top -
                                               self->outerHeight()));
                } else {
                    LayoutUnit absX =
                        self->x() + l2.x() - l1.x() - cb->borderLeft();
                    LayoutUnit left = absX - self->marginLeft();
                    parentContentWidth += cb->paddingWidth();
                    l.setLengthValue(CSSLength(left));
                    r.setLengthValue(CSSLength(parentContentWidth - left -
                                               self->outerWidth()));
                }
            } else {
                STARFISH_ASSERT(style->position() == RelativePositionValue);
                LengthData offset = style->offset();

                if (keyKind == CSSStyleValuePair::KeyKind::Top ||
                    keyKind == CSSStyleValuePair::KeyKind::Bottom) {
                    Length top = offset.top();
                    Length bottom = offset.bottom();
                    if (!top.isAuto() && !bottom.isAuto()) {
                        t.setLengthValue(CSSLength(
                            top.specifiedValue(parentContentHeight, self)));
                        b.setLengthValue(CSSLength(
                            bottom.specifiedValue(parentContentHeight, self)));
                    } else if (!top.isAuto()) {
                        t.setLengthValue(CSSLength(
                            top.specifiedValue(parentContentHeight, self)));
                        b.setLengthValue(-1 * t.cssLengthValue());
                    } else if (!bottom.isAuto()) {
                        b.setLengthValue(CSSLength(
                            bottom.specifiedValue(parentContentHeight, self)));
                        t.setLengthValue(-1 * b.cssLengthValue());
                    } else {
                        t.setLengthValue(CSSLength(0));
                        b.setLengthValue(CSSLength(0));
                    }
                } else {
                    Length left = offset.left();
                    Length right = offset.right();
                    if (!left.isAuto() && !right.isAuto()) {
                        l.setLengthValue(CSSLength(
                            left.specifiedValue(parentContentWidth, self)));
                        r.setLengthValue(CSSLength(
                            right.specifiedValue(parentContentWidth, self)));
                    } else if (!left.isAuto()) {
                        l.setLengthValue(CSSLength(
                            left.specifiedValue(parentContentWidth, self)));
                        r.setLengthValue(-1 * l.cssLengthValue());
                    } else if (!right.isAuto()) {
                        r.setLengthValue(CSSLength(
                            right.specifiedValue(parentContentWidth, self)));
                        l.setLengthValue(-1 * r.cssLengthValue());
                    } else {
                        l.setLengthValue(CSSLength(0));
                        r.setLengthValue(CSSLength(0));
                    }
                }
            }
        }

        if (keyKind == CSSStyleValuePair::KeyKind::Top) {
            addValuePair(t);
        } else if (keyKind == CSSStyleValuePair::KeyKind::Bottom) {
            addValuePair(b);
        } else if (keyKind == CSSStyleValuePair::KeyKind::Left) {
            addValuePair(l);
        } else if (keyKind == CSSStyleValuePair::KeyKind::Right) {
            addValuePair(r);
        }
    } else if (keyKind >= CSSStyleValuePair::KeyKind::Width &&
               keyKind <= CSSStyleValuePair::KeyKind::Height) {
        if (keyKind == CSSStyleValuePair::KeyKind::Width) {
            CSSStyleValuePair w;
            w.setKeyKind(CSSStyleValuePair::KeyKind::Width);
            if (frame && style->width().isDefinite(true)) {
                LayoutContext ctx(
                    m_node->starFish(),
                    m_node->document()->frame()->asFrameDocument());
                w.setLengthValue(CSSLength(style->width().specifiedValue(
                    ctx.parentContentWidth(frame), m_node)));
            } else {
                if (frame && frame->isFrameBox()) {
                    w.setLengthValue(
                        CSSLength(frame->asFrameBox()->contentWidth()));
                } else {
                    w.setValueKind(CSSStyleValuePair::ValueKind::Auto);
                }
            }
            addValuePair(w);
        } else {
            CSSStyleValuePair h;
            h.setKeyKind(CSSStyleValuePair::KeyKind::Height);
            if (frame && frame->isFrameBox()) {
                LayoutContext ctx(
                    m_node->starFish(),
                    m_node->document()->frame()->asFrameDocument());
                bool parentHasFixedHeight = ctx.parentHasFixedHeight(frame);
                if (style->height().isDefinite(parentHasFixedHeight)) {
                    LayoutUnit parentContentHeight;
                    if (parentHasFixedHeight) {
                        parentContentHeight = ctx.parentFixedHeight(frame);
                    }
                    h.setLengthValue(CSSLength(style->height().specifiedValue(
                        parentContentHeight, m_node)));
                } else {
                    h.setLengthValue(
                        CSSLength(frame->asFrameBox()->contentHeight()));
                }
            } else {
                h.setValueKind(CSSStyleValuePair::ValueKind::Auto);
            }
            addValuePair(h);
        }
    } else if (keyKind == CSSStyleValuePair::KeyKind::FontFamily) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::FontFamily);
        if (style->fontFamily()[0].m_length == 1) {
            p.setValueKind(CSSStyleValuePair::ValueKind::KeywordValueKind);
            p.setValue(style->fontFamily()[1].m_familyName);
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
            ValueList* val =
                new ValueList(ValueList::Separator::
                                  CommaSeparatorAppendQuoteWhenMeetWhiteSpace);
            size_t len = style->fontFamily()[0].m_length;
            for (size_t i = 0; i < len; i++) {
                val->emplace_back(
                    CSSStyleValuePair::ValueKind::KeywordValueKind,
                    style->fontFamily()[i + 1].m_familyName);
            }
            p.setValueList(val);
        }
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::ZIndex) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::ZIndex);
        if (style->isSpecifiedZIndex()) {
            p.setValueKind(CSSStyleValuePair::ValueKind::Int32);
            p.setValue(style->zIndex());
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
        }
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::All) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::All);
        p.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
        p.setValue(String::emptyString);
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::WhiteSpace) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::WhiteSpace);
        p.setValue(CSSStyleValuePair::ValueData(style->whiteSpace()));
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::FlexBasis) {
        CSSStyleValuePair p;
        FlexBasisData flexBasis = style->flexBasis();
        if (flexBasis.isContent()) {
            p.setValueKind(CSSStyleValuePair::ValueKind::FlexBasisValueKind);
            p.setValue(FlexBasisValue::ContentFlexBasisValue);
        } else {
            Length len = flexBasis.width();
            p = lengthToCSSStyleValue(len);
        }
        p.setKeyKind(CSSStyleValuePair::KeyKind::FlexBasis);
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::TextIndent) {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->textIndent());
        p.setKeyKind(CSSStyleValuePair::KeyKind::TextIndent);
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::LineHeight) {
        CSSStyleValuePair lh;
        lh.setKeyKind(CSSStyleValuePair::KeyKind::LineHeight);

        if (style->hasNormalLineHeight() || !frame) {
            lh.setValueKind(CSSStyleValuePair::ValueKind::Normal);
        } else {
            lh.setLengthValue(CSSLength(CSSLength::PX, frame->lineHeight()));
        }
        addValuePair(lh);
    } else if (keyKind == CSSStyleValuePair::KeyKind::FontSize) {
        CSSStyleValuePair fs;
        fs.setKeyKind(CSSStyleValuePair::KeyKind::FontSize);

        fs.setLengthValue(CSSLength(CSSLength::PX, style->fixedFontSize()));

        addValuePair(fs);
    } else if (keyKind == CSSStyleValuePair::KeyKind::LetterSpacing) {
        CSSStyleValuePair fs;
        fs.setKeyKind(CSSStyleValuePair::KeyKind::LetterSpacing);

        fs.setLengthValue(
            CSSLength(CSSLength::PX, style->letterSpacing().fixed()));

        addValuePair(fs);
    } else if (keyKind == CSSStyleValuePair::KeyKind::WordSpacing) {
        CSSStyleValuePair fs;
        fs.setKeyKind(CSSStyleValuePair::KeyKind::WordSpacing);

        fs.setLengthValue(
            CSSLength(CSSLength::PX, style->wordSpacing().fixed()));

        addValuePair(fs);
    } else if (keyKind == CSSStyleValuePair::KeyKind::MinWidth) {
        CSSStyleValuePair minW;
        minW.setKeyKind(CSSStyleValuePair::KeyKind::MinWidth);
        Length minWidth = style->minWidth();

        if (minWidth.isAuto()) {
            if (frame && frame->isFlexItem()) {
                minW.setValueKind(CSSStyleValuePair::ValueKind::Auto);
            } else {
                minW.setLengthValue(CSSLength(CSSLength::PX, 0));
            }
        } else if (minWidth.isDefinite(false)) {
            CSSStyleValuePair p = lengthToCSSStyleValue(minWidth);
            p.setKeyKind(CSSStyleValuePair::KeyKind::MinWidth);
            minW = p;
        } else if (minWidth.isPercent()) {
            minW.setPercentageValue(minWidth.percent());
        } else if (minWidth.isCalc()) {
            minW.setCalcValue(minWidth.calcData());
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }

        addValuePair(minW);
    } else if (keyKind == CSSStyleValuePair::KeyKind::MinHeight) {
        CSSStyleValuePair minH;
        minH.setKeyKind(CSSStyleValuePair::KeyKind::MinHeight);
        Length minHeight = style->minHeight();

        if (minHeight.isAuto()) {
            if (frame && frame->isFlexItem()) {
                minH.setValueKind(CSSStyleValuePair::ValueKind::Auto);
            } else {
                minH.setLengthValue(CSSLength(CSSLength::PX, 0));
            }
        } else if (minHeight.isDefinite(false)) {
            CSSStyleValuePair p = lengthToCSSStyleValue(minHeight);
            p.setKeyKind(CSSStyleValuePair::KeyKind::MinHeight);
            minH = p;
        } else if (minHeight.isPercent()) {
            minH.setPercentageValue(minHeight.percent());
        } else if (minHeight.isCalc()) {
            minH.setCalcValue(minHeight.calcData());
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }

        addValuePair(minH);
    } else if (keyKind == CSSStyleValuePair::KeyKind::MaxWidth) {
        CSSStyleValuePair maxW;
        maxW.setKeyKind(CSSStyleValuePair::KeyKind::MaxWidth);
        Length maxWidth = style->maxWidth();

        if (maxWidth.isAuto()) {
            maxW.setValueKind(CSSStyleValuePair::ValueKind::None);
        } else if (maxWidth.isDefinite(false)) {
            CSSStyleValuePair p = lengthToCSSStyleValue(maxWidth);
            p.setKeyKind(CSSStyleValuePair::KeyKind::MaxWidth);
            maxW = p;
        } else if (maxWidth.isPercent()) {
            maxW.setPercentageValue(maxWidth.percent());
        } else if (maxWidth.isCalc()) {
            maxW.setCalcValue(maxWidth.calcData());
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }

        addValuePair(maxW);
    } else if (keyKind == CSSStyleValuePair::KeyKind::MaxHeight) {
        CSSStyleValuePair maxH;
        maxH.setKeyKind(CSSStyleValuePair::KeyKind::MaxHeight);
        Length maxHeight = style->maxHeight();

        if (maxHeight.isAuto()) {
            maxH.setValueKind(CSSStyleValuePair::ValueKind::None);
        } else if (maxHeight.isDefinite(false)) {
            CSSStyleValuePair p = lengthToCSSStyleValue(maxHeight);
            p.setKeyKind(CSSStyleValuePair::KeyKind::MaxHeight);
            maxH = p;
        } else if (maxHeight.isPercent()) {
            maxH.setPercentageValue(maxHeight.percent());
        } else if (maxHeight.isCalc()) {
            maxH.setCalcValue(maxHeight.calcData());
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        addValuePair(maxH);
    } else if (keyKind == CSSStyleValuePair::KeyKind::BackgroundImage) {
        CSSStyleValuePair bgImage;

        bgImage.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundImage);
        bgImage.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);

        ValueList* bgImageValues;
        bgImageValues = new ValueList(ValueList::Separator::CommaSeparator);

        for (unsigned int i = 0; i < style->backgroundLayerSize(); i++) {
            CSSStyleValuePair item;
            if (style->backgroundImage(i)->length() != 0) {
                item.setUrlValue(style->backgroundImage(i));
                bgImageValues->push_back(item);
            }
        }
        bgImage.setValueList(bgImageValues);
        addValuePair(bgImage);

    } else if (keyKind == CSSStyleValuePair::KeyKind::BackgroundSize) {
        CSSStyleValuePair bgSize;

        bgSize.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundSize);
        bgSize.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);

        ValueList* bgSizeValues;

        bgSizeValues = new ValueList(ValueList::Separator::CommaSeparator);

        for (unsigned int i = 0; i < style->backgroundLayerSize(); i++) {
            CSSStyleValuePair item;
            if (style->backgroundSizeIsLength(i)) {
                item.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
                ValueList* vals =
                    new ValueList(ValueList::Separator::SpaceSeparator);
                LengthSize lengthSize = style->backgroundSizeLengthValue(i);

                CSSStyleValuePair w = lengthToCSSStyleValue(lengthSize.width());
                vals->emplace_back(w.valueKind(), w.value());

                CSSStyleValuePair h =
                    lengthToCSSStyleValue(lengthSize.height());
                vals->emplace_back(h.valueKind(), h.value());

                item.setValue(vals);
            } else {
                item.setBackgroundSizeValue(style->backgroundSizeTypeValue(i));
                item.setValueKind(
                    CSSStyleValuePair::ValueKind::BackgroundSizeValueKind);
                item.setValue(style->backgroundSizeTypeValue(i));
            }
            bgSizeValues->push_back(item);
        }

        bgSize.setValueList(bgSizeValues);
        addValuePair(bgSize);

    } else if (keyKind == CSSStyleValuePair::KeyKind::BackgroundAttachment) {
        CSSStyleValuePair bgAttachment;

        bgAttachment.setKeyKind(
            CSSStyleValuePair::KeyKind::BackgroundAttachment);
        bgAttachment.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        ValueList* bgAttachmentValues;

        bgAttachmentValues =
            new ValueList(ValueList::Separator::CommaSeparator);

        for (unsigned int i = 0; i < style->backgroundLayerSize(); i++) {
            CSSStyleValuePair item;

            item.setBackgroundAttachmentValue(style->backgroundAttachment(i));
            bgAttachmentValues->push_back(item);
        }
        bgAttachment.setValueList(bgAttachmentValues);
        addValuePair(bgAttachment);

    } else if (keyKind == CSSStyleValuePair::KeyKind::BackgroundClip) {
        CSSStyleValuePair bgClip;

        bgClip.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundClip);
        bgClip.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);

        ValueList* bgClipValues;

        bgClipValues = new ValueList(ValueList::Separator::CommaSeparator);
        for (unsigned int i = 0; i < style->backgroundLayerSize(); i++) {
            CSSStyleValuePair item;

            item.setBoxValue(style->backgroundClip(i));
            bgClipValues->push_back(item);
        }
        bgClip.setValueList(bgClipValues);
        addValuePair(bgClip);

    } else if (keyKind == CSSStyleValuePair::KeyKind::BackgroundOrigin) {
        CSSStyleValuePair bgOrigin;

        bgOrigin.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundOrigin);
        bgOrigin.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);

        ValueList* bgOriginValues;

        bgOriginValues = new ValueList(ValueList::Separator::CommaSeparator);

        for (unsigned int i = 0; i < style->backgroundLayerSize(); i++) {
            CSSStyleValuePair item;
            item.setBoxValue(style->backgroundOrigin(i));
            bgOriginValues->push_back(item);
        }
        bgOrigin.setValueList(bgOriginValues);
        addValuePair(bgOrigin);

    } else if (keyKind == CSSStyleValuePair::KeyKind::BackgroundRepeatX) {
        CSSStyleValuePair bgRepeatX;

        bgRepeatX.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundRepeatX);
        bgRepeatX.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);

        ValueList* bgRepeatXValues;

        bgRepeatXValues = new ValueList(ValueList::Separator::CommaSeparator);

        for (unsigned int i = 0; i < style->backgroundLayerSize(); i++) {
            CSSStyleValuePair item;

            item.setBackgroundRepeatValue(style->backgroundRepeatX(i));
            bgRepeatXValues->push_back(item);
        }

        bgRepeatX.setValueList(bgRepeatXValues);
        addValuePair(bgRepeatX);

    } else if (keyKind == CSSStyleValuePair::KeyKind::BackgroundRepeatY) {
        CSSStyleValuePair bgRepeatY;

        bgRepeatY.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundRepeatY);
        bgRepeatY.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);

        ValueList* bgRepeatYValues;

        bgRepeatYValues = new ValueList(ValueList::Separator::CommaSeparator);

        for (unsigned int i = 0; i < style->backgroundLayerSize(); i++) {
            CSSStyleValuePair item;
            item.setBackgroundRepeatValue(style->backgroundRepeatY(i));
            bgRepeatYValues->push_back(item);
        }

        bgRepeatY.setValueList(bgRepeatYValues);
        addValuePair(bgRepeatY);

    } else if (keyKind == CSSStyleValuePair::KeyKind::BackgroundPositionX) {
        CSSStyleValuePair bgPositionX;

        bgPositionX.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundPositionX);
        bgPositionX.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);

        ValueList* bgPositionXValues;

        bgPositionXValues = new ValueList(ValueList::Separator::CommaSeparator);

        for (unsigned int i = 0; i < style->backgroundLayerSize(); i++) {
            CSSStyleValuePair item;

            item = lengthToCSSStyleValue(style->backgroundPositionX(i));
            bgPositionXValues->push_back(item);
        }

        bgPositionX.setValueList(bgPositionXValues);
        addValuePair(bgPositionX);

    } else if (keyKind == CSSStyleValuePair::KeyKind::BackgroundPositionY) {
        CSSStyleValuePair bgPositionY;

        bgPositionY.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundPositionY);
        bgPositionY.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);

        ValueList* bgPositionYValues;

        bgPositionYValues = new ValueList(ValueList::Separator::CommaSeparator);

        for (unsigned int i = 0; i < style->backgroundLayerSize(); i++) {
            CSSStyleValuePair item;

            item = lengthToCSSStyleValue(style->backgroundPositionY(i));
            bgPositionYValues->push_back(item);
        }

        bgPositionY.setValueList(bgPositionYValues);
        addValuePair(bgPositionY);

    } else if (keyKind == CSSStyleValuePair::KeyKind::BorderImageSource) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderImageSource);
        BorderData border = style->border();
        if (border.image().url()->length() == 0) {
            p.setValueKind(CSSStyleValuePair::ValueKind::None);
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::UrlValueKind);
            p.setValue(border.image().url());
        }
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::BorderImageSlice) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderImageSlice);
        p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        ValueList* vals = new ValueList(ValueList::Separator::SpaceSeparator);
        BorderData border = style->border();
        BorderImageLengthBox box = border.image().slices();

        if (box.top().isLength()) {
            CSSStyleValuePair t = lengthToCSSStyleValue(box.top().length());
            vals->emplace_back(t.valueKind(), t.value());
        } else {
            vals->emplace_back(CSSStyleValuePair::ValueKind::Number,
                               (float)box.top().number());
        }
        if (box.right().isLength()) {
            CSSStyleValuePair r = lengthToCSSStyleValue(box.right().length());
            vals->emplace_back(r.valueKind(), r.value());
        } else {
            vals->emplace_back(CSSStyleValuePair::ValueKind::Number,
                               (float)box.right().number());
        }
        if (box.bottom().isLength()) {
            CSSStyleValuePair b = lengthToCSSStyleValue(box.bottom().length());
            vals->emplace_back(b.valueKind(), b.value());
        } else {
            vals->emplace_back(CSSStyleValuePair::ValueKind::Number,
                               (float)box.bottom().number());
        }
        if (box.left().isLength()) {
            CSSStyleValuePair l = lengthToCSSStyleValue(box.left().length());
            vals->emplace_back(l.valueKind(), l.value());
        } else {
            vals->emplace_back(CSSStyleValuePair::ValueKind::Number,
                               (float)box.left().number());
        }
        if (border.image().sliceFill()) {
            vals->emplace_back(CSSStyleValuePair::ValueKind::KeywordValueKind,
                               String::fromUTF8("fill"));
        }

        p.setValue(vals);
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::BorderImageRepeat) {
        BorderImageData bImage = style->border().image();

        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderImageRepeat);

        if (bImage.repeatX() == bImage.repeatY()) {
            p.setBorderImageRepeatValue(bImage.repeatX());
        } else {
            ValueList* vals =
                new ValueList(ValueList::Separator::SpaceSeparator);
            vals->emplace_back(
                CSSStyleValuePair::ValueKind::BorderImageRepeatValueKind,
                bImage.repeatX());
            vals->emplace_back(
                CSSStyleValuePair::ValueKind::BorderImageRepeatValueKind,
                bImage.repeatY());
            p.setValueList(vals);
        }
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::BorderImageWidth) {
        // FIXME: Need to refactor BorderImageLength.h, and update the lines
        // below

        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderImageWidth);
        p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        ValueList* vals = new ValueList(ValueList::Separator::SpaceSeparator);
        BorderData border = style->border();
        BorderImageLengthBox box = border.image().widths();

        if (box.top().isLength()) {
            CSSStyleValuePair t = lengthToCSSStyleValue(box.top().length());
            vals->emplace_back(t.valueKind(), t.value());
        } else {
            vals->emplace_back(CSSStyleValuePair::ValueKind::Number,
                               (float)box.top().number());
        }
        if (box.right().isLength()) {
            CSSStyleValuePair r = lengthToCSSStyleValue(box.right().length());
            vals->emplace_back(r.valueKind(), r.value());
        } else {
            vals->emplace_back(CSSStyleValuePair::ValueKind::Number,
                               (float)box.right().number());
        }
        if (box.bottom().isLength()) {
            CSSStyleValuePair b = lengthToCSSStyleValue(box.bottom().length());
            vals->emplace_back(b.valueKind(), b.value());
        } else {
            vals->emplace_back(CSSStyleValuePair::ValueKind::Number,
                               (float)box.bottom().number());
        }
        if (box.left().isLength()) {
            CSSStyleValuePair l = lengthToCSSStyleValue(box.left().length());
            vals->emplace_back(l.valueKind(), l.value());
        } else {
            vals->emplace_back(CSSStyleValuePair::ValueKind::Number,
                               (float)box.bottom().number());
        }

        p.setValue(vals);
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::BorderImageOutset) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderImageOutset);
        p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        ValueList* vals = new ValueList(ValueList::Separator::SpaceSeparator);
        BorderData border = style->border();
        BorderImageLengthBox box = border.image().outsets();

        if (box.top().isLength()) {
            CSSStyleValuePair t = lengthToCSSStyleValue(box.top().length());
            vals->emplace_back(t.valueKind(), t.value());
        } else {
            vals->emplace_back(CSSStyleValuePair::ValueKind::Number,
                               (float)box.top().number());
        }
        if (box.right().isLength()) {
            CSSStyleValuePair r = lengthToCSSStyleValue(box.right().length());
            vals->emplace_back(r.valueKind(), r.value());
        } else {
            vals->emplace_back(CSSStyleValuePair::ValueKind::Number,
                               (float)box.right().number());
        }
        if (box.bottom().isLength()) {
            CSSStyleValuePair b = lengthToCSSStyleValue(box.bottom().length());
            vals->emplace_back(b.valueKind(), b.value());
        } else {
            vals->emplace_back(CSSStyleValuePair::ValueKind::Number,
                               (float)box.bottom().number());
        }
        if (box.left().isLength()) {
            CSSStyleValuePair l = lengthToCSSStyleValue(box.left().length());
            vals->emplace_back(l.valueKind(), l.value());
        } else {
            vals->emplace_back(CSSStyleValuePair::ValueKind::Number,
                               (float)box.bottom().number());
        }

        p.setValue(vals);
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::TransformOrigin) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::TransformOrigin);

        if (!style->hasTransformOrigin()) {
            p.setValueKind(CSSStyleValuePair::ValueKind::None);
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
            ValueList* vals = new ValueList();

            CSSStyleValuePair x = lengthToCSSStyleValue(
                style->transformOrigin()->originValue()->getXAxis());
            vals->emplace_back(x.valueKind(), x.value());

            CSSStyleValuePair y = lengthToCSSStyleValue(
                style->transformOrigin()->originValue()->getYAxis());
            vals->emplace_back(y.valueKind(), y.value());

            p.setValue(vals);
        }
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::Transform) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::Transform);
        if (!style->hasTransforms()) {
            p.setValueKind(CSSStyleValuePair::ValueKind::None);
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::TransformFunctions);
            FrameDocument* doc = frame->asFrameDocument();
            FrameBox* box = frame->findNearestAssociateBox();
            SkMatrix m = style->transformsToMatrix(
                box->width(), box->height(), box, style->hasTransforms(frame));

            CSSTransformFunctions* transforms = new CSSTransformFunctions();

            ValueList* values =
                new ValueList(ValueList::Separator::CommaSeparator);

            values->emplace_back(CSSStyleValuePair::ValueKind::Number,
                                 m.getScaleX());
            values->emplace_back(CSSStyleValuePair::ValueKind::Number,
                                 m.getSkewY());
            values->emplace_back(CSSStyleValuePair::ValueKind::Number,
                                 m.getSkewX());
            values->emplace_back(CSSStyleValuePair::ValueKind::Number,
                                 m.getScaleY());
            values->emplace_back(CSSStyleValuePair::ValueKind::Number,
                                 m.getTranslateX());
            values->emplace_back(CSSStyleValuePair::ValueKind::Number,
                                 m.getTranslateY());

            transforms->emplace_back(CSSTransformFunction::Matrix, values);
            p.setValue(transforms);
        }
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::BorderTopStyle) {
        CSSStyleValuePair tStyle;
        tStyle.setKeyKind(CSSStyleValuePair::KeyKind::BorderTopStyle);
        tStyle.setBorderStyleValue(style->border().top().style());
        addValuePair(tStyle);
    } else if (keyKind == CSSStyleValuePair::KeyKind::BorderBottomStyle) {
        CSSStyleValuePair bStyle;
        bStyle.setKeyKind(CSSStyleValuePair::KeyKind::BorderBottomStyle);
        bStyle.setBorderStyleValue(style->border().bottom().style());
        addValuePair(bStyle);
    } else if (keyKind == CSSStyleValuePair::KeyKind::BorderLeftStyle) {
        CSSStyleValuePair lStyle;
        lStyle.setKeyKind(CSSStyleValuePair::KeyKind::BorderLeftStyle);
        lStyle.setBorderStyleValue(style->border().left().style());
        addValuePair(lStyle);
    } else if (keyKind == CSSStyleValuePair::KeyKind::BorderRightStyle) {
        CSSStyleValuePair rStyle;
        rStyle.setKeyKind(CSSStyleValuePair::KeyKind::BorderRightStyle);
        rStyle.setBorderStyleValue(style->border().right().style());
        addValuePair(rStyle);
    } else if (keyKind == CSSStyleValuePair::KeyKind::BorderTopColor) {
        CSSStyleValuePair tColor;
        tColor.setKeyKind(CSSStyleValuePair::KeyKind::BorderTopColor);
        tColor.setColorValue(style->border().top().color());
        addValuePair(tColor);
    } else if (keyKind == CSSStyleValuePair::KeyKind::BorderBottomColor) {
        CSSStyleValuePair bColor;
        bColor.setKeyKind(CSSStyleValuePair::KeyKind::BorderBottomColor);
        bColor.setColorValue(style->border().bottom().color());
        addValuePair(bColor);
    } else if (keyKind == CSSStyleValuePair::KeyKind::BorderLeftColor) {
        CSSStyleValuePair lColor;
        lColor.setKeyKind(CSSStyleValuePair::KeyKind::BorderLeftColor);
        lColor.setColorValue(style->border().left().color());
        addValuePair(lColor);
    } else if (keyKind == CSSStyleValuePair::KeyKind::BorderRightColor) {
        CSSStyleValuePair rColor;
        rColor.setKeyKind(CSSStyleValuePair::KeyKind::BorderRightColor);
        rColor.setColorValue(style->border().right().color());
        addValuePair(rColor);
    } else if (keyKind == CSSStyleValuePair::KeyKind::ObjectPosition) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::ObjectPosition);

        if (!style->hasObjectSizing()) {
            p.setValueKind(CSSStyleValuePair::ValueKind::None);
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
            ValueList* vals =
                new ValueList(ValueList::Separator::SpaceSeparator);
            CSSStyleValuePair x =
                lengthToCSSStyleValue(style->objectPositionX());
            vals->emplace_back(x.valueKind(), x.value());
            CSSStyleValuePair y =
                lengthToCSSStyleValue(style->objectPositionY());
            vals->emplace_back(y.valueKind(), y.value());
            p.setValue(vals);
        }
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::Clip) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::Clip);
        if (style->clip()) {
            p.setValueKind(CSSStyleValuePair::ValueKind::RectValueKind);
            p.setValue(style->clip());
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
        }
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::ListStyleImage) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::ListStyleImage);
        const ListStyleData& listStyle = style->listStyleData();
        if (listStyle.image()->length() == 0) {
            p.setValueKind(CSSStyleValuePair::ValueKind::None);
        } else {
            p.setUrlValue(listStyle.image());
        }
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::TextShadow) {
        CSSStyleValuePair shadows;
        shadows.setKeyKind(CSSStyleValuePair::KeyKind::TextShadow);
        if (!style->textShadow().size()) {
            shadows.setValueKind(CSSStyleValuePair::ValueKind::None);
        } else {
            shadows.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
            shadows.setValueList(
                new ValueList(ValueList::Separator::CommaSeparator));

            for (auto& sd : style->textShadow()) {
                CSSStyleValuePair s;
                s.setValueList(
                    new ValueList(ValueList::Separator::SpaceSeparator));
                {
                    CSSStyleValuePair color;
                    if (sd.hasColor()) {
                        color.setColorValue(sd.color());
                    } else {
                        color.setColorValue(style->color());
                    }
                    s.multiValue()->emplace_back(color.valueKind(),
                                                 color.value());
                }

                CSSStyleValuePair lengths;
                lengths.setValueList(
                    new ValueList(ValueList::Separator::SpaceSeparator));

                CSSStyleValuePair l1 = lengthToCSSStyleValue(sd.offsetX());
                lengths.multiValue()->emplace_back(l1.valueKind(), l1.value());

                CSSStyleValuePair l2 = lengthToCSSStyleValue(sd.offsetY());
                lengths.multiValue()->emplace_back(l2.valueKind(), l2.value());

                CSSStyleValuePair l3 = lengthToCSSStyleValue(sd.radius());
                lengths.multiValue()->emplace_back(l3.valueKind(), l3.value());

                s.multiValue()->emplace_back(lengths.valueKind(),
                                             lengths.value());

                shadows.multiValue()->emplace_back(s.valueKind(), s.value());
            }
        }
        addValuePair(shadows);
    } else if (keyKind == CSSStyleValuePair::KeyKind::BorderCollapse) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderCollapse);
        p.setValueKind(CSSStyleValuePair::ValueKind::BorderCollapseValueKind);
        p.setValue(CSSStyleValuePair::ValueData(style->borderCollapse()));
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::BorderSpacing) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderSpacing);
        auto h = style->horizontalBorderSpacing();
        auto v = style->verticalBorderSpacing();
        if (h == v) {
            p.setValueKind(CSSStyleValuePair::ValueKind::Length);
            p.setValue(lengthToCSSStyleValue(h).value());
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
            ValueList* list =
                new ValueList(ValueList::Separator::SpaceSeparator);
            list->emplace_back(CSSStyleValuePair::ValueKind::Length,
                               lengthToCSSStyleValue(h).value());
            list->emplace_back(CSSStyleValuePair::ValueKind::Length,
                               lengthToCSSStyleValue(v).value());
        }
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::TableLayout) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::TableLayout);
        p.setValueKind(CSSStyleValuePair::ValueKind::TableLayoutValueKind);
        p.setValue(CSSStyleValuePair::ValueData(style->tableLayout()));
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::Content) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::Content);
        p.setValueKind(CSSStyleValuePair::ValueKind::None);
        ContentDataGroup* contentData = style->content();
        if (contentData) {
            ValueList* values = new ValueList();
            for (size_t i = 0; i < contentData->size(); i++) {
                ContentData& c = contentData->at(i);
                if (c.type() == ContentData::Text) {
                    CSSStyleValuePair t;
                    t.setStringValue(c.text()->text());
                    values->pushBack(t);
                } else if (c.type() == ContentData::Image) {
                    CSSStyleValuePair t;
                    t.setUrlValue(c.image()->image());
                    values->pushBack(t);
                } else if (c.type() == ContentData::Counter) {
                    CSSStyleValuePair t;
                    CounterContentData* data = c.counter();
                    CSSCounterFunction* f = new CSSCounterFunction(data->id());
                    f->setSeparator(data->separator());
                    f->setStyle(AtomicString::createAtomicString(
                        m_node->starFish(), data->counterStyle()->name()));
                    t.setCounterFunctionValue(f);
                    values->pushBack(t);
                } else if (c.type() == ContentData::Quote) {
                    CSSStyleValuePair t;
                    t.setQuoteValue(c.quote());
                    values->pushBack(t);
                }
            }
            p.setValueList(values);
        }
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::TransitionProperty) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::TransitionProperty);
        p.setValueKind(
            CSSStyleValuePair::ValueKind::TransitionPropertyValueKind);
        p.setValue(CSSStyleValuePair::ValueData(style->transitionProperty()));
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::TransitionDuration) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::TransitionDuration);
        p.setValueKind(CSSStyleValuePair::ValueKind::Time);
        p.setValue(CSSStyleValuePair::ValueData(style->transitionDuration()));
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::TransitionDelay) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::TransitionDelay);
        p.setValueKind(CSSStyleValuePair::ValueKind::Time);
        p.setValue(CSSStyleValuePair::ValueData(style->transitionDelay()));
        addValuePair(p);
    } else if (keyKind ==
               CSSStyleValuePair::KeyKind::TransitionTimingFunction) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::TransitionTimingFunction);
        p.setValueKind(
            CSSStyleValuePair::ValueKind::TransitionTimingFunctionValueKind);
        p.setValue(
            CSSStyleValuePair::ValueData(style->transitionTimingFunction()));
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::BoxShadow) {
        CSSStyleValuePair shadows;
        shadows.setKeyKind(CSSStyleValuePair::KeyKind::BoxShadow);
        if (!style->boxShadow().size()) {
            shadows.setValueKind(CSSStyleValuePair::ValueKind::None);
        } else {
            shadows.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
            shadows.setValueList(
                new ValueList(ValueList::Separator::CommaSeparator));

            for (auto& sd : style->boxShadow()) {
                CSSStyleValuePair s;
                s.setValueList(
                    new ValueList(ValueList::Separator::SpaceSeparator));
                {
                    CSSStyleValuePair color;
                    if (sd.hasColor()) {
                        color.setColorValue(sd.color());
                    } else {
                        color.setColorValue(style->color());
                    }
                    s.multiValue()->emplace_back(color.valueKind(),
                                                 color.value());
                }

                CSSStyleValuePair lengths;
                lengths.setValueList(
                    new ValueList(ValueList::Separator::SpaceSeparator));

                CSSStyleValuePair l1 = lengthToCSSStyleValue(sd.offsetX());
                lengths.multiValue()->emplace_back(l1.valueKind(), l1.value());

                CSSStyleValuePair l2 = lengthToCSSStyleValue(sd.offsetY());
                lengths.multiValue()->emplace_back(l2.valueKind(), l2.value());

                CSSStyleValuePair l3 = lengthToCSSStyleValue(sd.radius());
                lengths.multiValue()->emplace_back(l3.valueKind(), l3.value());

                CSSStyleValuePair l4 =
                    lengthToCSSStyleValue(sd.spreadDistance());
                lengths.multiValue()->emplace_back(l4.valueKind(), l4.value());

                s.multiValue()->emplace_back(lengths.valueKind(),
                                             lengths.value());

                if (sd.inset()) {
                    CSSStyleValuePair inset;
                    inset.setValueKind(
                        CSSStyleValuePair::ValueKind::StringValueKind);
                    inset.setStringValue(String::createASCIIString("inset"));
                    s.multiValue()->emplace_back(inset.valueKind(),
                                                 inset.value());
                }

                shadows.multiValue()->emplace_back(s.valueKind(), s.value());
            }
        }
        addValuePair(shadows);
    } else if (keyKind == CSSStyleValuePair::KeyKind::Fill) {
        CSSStyleValuePair p = stylePaintDataToCSSStyleValue(style->fill());
        p.setKeyKind(CSSStyleValuePair::KeyKind::Fill);
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::FillOpacity) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::FillOpacity);
        p.setValueKind(CSSStyleValuePair::ValueKind::Number);
        p.setNumberValue(style->fillOpacity());
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::FillRule) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::FillRule);
        p.setValueKind(CSSStyleValuePair::ValueKind::FillRuleValueKind);
        p.setValue(CSSStyleValuePair::ValueData(style->fillRule()));
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::Stroke) {
        CSSStyleValuePair p = stylePaintDataToCSSStyleValue(style->stroke());
        p.setKeyKind(CSSStyleValuePair::KeyKind::Stroke);
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::StrokeWidth) {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->strokeWidth());
        p.setKeyKind(CSSStyleValuePair::KeyKind::StrokeWidth);
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::X) {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->x());
        p.setKeyKind(CSSStyleValuePair::KeyKind::X);
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::Y) {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->y());
        p.setKeyKind(CSSStyleValuePair::KeyKind::Y);
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::CX) {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->cx());
        p.setKeyKind(CSSStyleValuePair::KeyKind::CX);
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::CY) {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->cy());
        p.setKeyKind(CSSStyleValuePair::KeyKind::CY);
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::R) {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->r());
        p.setKeyKind(CSSStyleValuePair::KeyKind::R);
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::RX) {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->rx());
        p.setKeyKind(CSSStyleValuePair::KeyKind::RX);
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::RY) {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->ry());
        p.setKeyKind(CSSStyleValuePair::KeyKind::RY);
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::OutlineColor) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::OutlineColor);
        p.setValueKind(CSSStyleValuePair::ValueKind::ColorValueKind);
        p.setColorValue(style->color());
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::OutlineWidth) {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->outlineWidth());
        p.setKeyKind(CSSStyleValuePair::KeyKind::OutlineWidth);
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::OutlineOffset) {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->outlineOffset());
        p.setKeyKind(CSSStyleValuePair::KeyKind::OutlineOffset);
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::OutlineStyle) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::OutlineStyle);
        p.setValueKind(CSSStyleValuePair::ValueKind::BorderStyleValueKind);
        p.setBorderStyleValue(style->outlineStyle());
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::Cursor) {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::Cursor);
        // when cursor value type is implemented, add cursor value type instead
        // of string `auto`
        p.setKeywordValue(String::createASCIIString("auto"));
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::MaskImage) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::MaskImage);
        String* url = style->maskImage();
        if (url->length()) {
            p.setUrlValue(url);
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::None);
        }
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::MaskSize) {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::MaskSize);
        p.setValueKind(CSSStyleValuePair::ValueKind::None);
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::GridTemplateColumns) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::GridTemplateColumns);
        if (style->gridTemplateColumns()) {
            p.setValueKind(CSSStyleValuePair::ValueKind::GridTemplateUnits);
            p.setGridTemplateUnits(style->gridTemplateColumns());
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::None);
        }
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::GridTemplateRows) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::GridTemplateRows);
        if (style->gridTemplateRows()) {
            p.setValueKind(CSSStyleValuePair::ValueKind::GridTemplateUnits);
            p.setGridTemplateUnits(style->gridTemplateRows());
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::None);
        }
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::GridGap) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::GridGap);
        p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        ValueList* values = new ValueList(ValueList::Separator::SpaceSeparator);
        Length row = style->gridRowGap();
        Length column = style->gridColumnGap();
        CSSStyleValuePair ret1;
        if (row.isFixed()) {
            ret1.setLengthValue(CSSLength(row.numberData()));
            values->push_back(ret1);
        }

        CSSStyleValuePair ret2;
        if (column.isFixed()) {
            ret2.setLengthValue(CSSLength(column.numberData()));
            values->push_back(ret2);
        }
        p.setValueList(values);

        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::GridRowGap) {
        CSSStyleValuePair p;
        Length row = style->gridRowGap();
        if (row.isFixed()) {
            p.setLengthValue(CSSLength(row.fixed()));
        }
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::GridColumnGap) {
        CSSStyleValuePair p;
        Length column = style->gridColumnGap();
        if (column.isFixed()) {
            p.setLengthValue(CSSLength(column.fixed()));
        }
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::GridTemplateAreas) {
        CSSStyleValuePair p;
        String* areas = style->gridTemplateAreas();
        if (areas) {
            p.setKeyKind(CSSStyleValuePair::KeyKind::GridTemplateAreas);
            p.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            p.setStringValue(areas);
        }
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::TextOverflow) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::TextOverflow);
        p.setValueKind(CSSStyleValuePair::ValueKind::TextOverflowValueKind);
        p.setValue(new TextOverflowData(style->textOverflow()));
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::CounterReset) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::CounterReset);
        CounterBaseList* list = style->counterReset();
        if (list) {
            ValueList* v = new ValueList(ValueList::SpaceSeparator);
            size_t size = list->size();
            for (size_t i = 0; i < size; i++) {
                v->emplace_back(CSSStyleValuePair::AtomicStringValueKind,
                                (*list)[i].first);
                v->emplace_back(CSSStyleValuePair::Int32, (*list)[i].second);
            }
            p.setValueList(v);
        } else {
            p.setValueKind(CSSStyleValuePair::None);
        }
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::CounterIncrement) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::CounterIncrement);
        CounterBaseList* list = style->counterIncrement();
        if (list) {
            ValueList* v = new ValueList(ValueList::SpaceSeparator);
            size_t size = list->size();
            for (size_t i = 0; i < size; i++) {
                v->emplace_back(CSSStyleValuePair::AtomicStringValueKind,
                                (*list)[i].first);
                v->emplace_back(CSSStyleValuePair::Int32, (*list)[i].second);
            }
            p.setValueList(v);
        } else {
            p.setValueKind(CSSStyleValuePair::None);
        }
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::GridRowStart) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::GridRowStart);
        p.setValueKind(CSSStyleValuePair::ValueKind::Int32);
        p.setInt32Value(style->gridRowStart());
    } else if (keyKind == CSSStyleValuePair::KeyKind::GridRowEnd) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::GridRowEnd);
        p.setValueKind(CSSStyleValuePair::ValueKind::Int32);
        p.setInt32Value(style->gridRowEnd());
    } else if (keyKind == CSSStyleValuePair::KeyKind::GridColumnStart) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::GridColumnStart);
        p.setValueKind(CSSStyleValuePair::ValueKind::Int32);
        p.setInt32Value(style->gridColumnStart());
    } else if (keyKind == CSSStyleValuePair::KeyKind::GridColumnEnd) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::GridColumnStart);
        p.setValueKind(CSSStyleValuePair::ValueKind::Int32);
        p.setInt32Value(style->gridColumnStart());
    } else if (keyKind == CSSStyleValuePair::KeyKind::WillChange) {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::WillChange);
        WillChangeData* data = style->willChange();
        if (!data) {
            p.setValueKind(CSSStyleValuePair::Auto);
        } else {
            ValueList* list = new ValueList(ValueList::CommaSeparator);
            if (data->contents()) {
                list->emplace_back(CSSStyleValuePair::KeywordValueKind,
                                   String::fromUTF8("contents"));
            }
            if (data->scrollPosition()) {
                list->emplace_back(CSSStyleValuePair::KeywordValueKind,
                                   String::fromUTF8("scroll-position"));
            }
            size_t size = data->size();
            for (size_t i = 0; i < size; i++) {
                list->emplace_back(CSSStyleValuePair::AtomicStringValueKind,
                                   data->at(i));
            }
            p.setValueList(list);
        }
        addValuePair(p);
    }
#define ADD_VALUE_PAIR_BORDER_RADIUS(Name1Name2, name1Name2)                  \
    else if (keyKind ==                                                       \
             CSSStyleValuePair::KeyKind::Border##Name1Name2##Radius)          \
    {                                                                         \
        CSSStyleValuePair p;                                                  \
        p.setKeyKind(CSSStyleValuePair::KeyKind::Border##Name1Name2##Radius); \
        p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);          \
        ValueList* valueList = new ValueList(ValueList::SpaceSeparator);      \
        if (style->hasBorderRadius()) {                                       \
            if (style->borderRadius().m_##name1Name2##Vertical ==             \
                style->borderRadius().m_##name1Name2##Horizontal) {           \
                CSSStyleValuePair pair;                                       \
                if (style->borderRadius()                                     \
                        .m_##name1Name2##Vertical.isPercent()) {              \
                    pair.setValueKind(                                        \
                        CSSStyleValuePair::ValueKind::Percentage);            \
                    pair.setPercentageValue(                                  \
                        style->borderRadius()                                 \
                            .m_##name1Name2##Vertical.percent());             \
                } else {                                                      \
                    pair.setValueKind(CSSStyleValuePair::ValueKind::Length);  \
                    pair.setLengthValue(                                      \
                        CSSLength(style->borderRadius()                       \
                                      .m_##name1Name2##Vertical.fixed()));    \
                }                                                             \
                valueList->pushBack(pair);                                    \
            } else {                                                          \
                CSSStyleValuePair pair;                                       \
                if (style->borderRadius()                                     \
                        .m_##name1Name2##Horizontal.isPercent()) {            \
                    pair.setValueKind(                                        \
                        CSSStyleValuePair::ValueKind::Percentage);            \
                    pair.setPercentageValue(                                  \
                        style->borderRadius()                                 \
                            .m_##name1Name2##Horizontal.percent());           \
                } else {                                                      \
                    pair.setValueKind(CSSStyleValuePair::ValueKind::Length);  \
                    pair.setLengthValue(                                      \
                        CSSLength(style->borderRadius()                       \
                                      .m_##name1Name2##Horizontal.fixed()));  \
                }                                                             \
                valueList->pushBack(pair);                                    \
                if (style->borderRadius()                                     \
                        .m_##name1Name2##Vertical.isPercent()) {              \
                    pair.setValueKind(                                        \
                        CSSStyleValuePair::ValueKind::Percentage);            \
                    pair.setPercentageValue(                                  \
                        style->borderRadius()                                 \
                            .m_##name1Name2##Vertical.percent());             \
                } else {                                                      \
                    pair.setValueKind(CSSStyleValuePair::ValueKind::Length);  \
                    pair.setLengthValue(                                      \
                        CSSLength(style->borderRadius()                       \
                                      .m_##name1Name2##Vertical.fixed()));    \
                }                                                             \
                valueList->pushBack(pair);                                    \
            }                                                                 \
        } else {                                                              \
            CSSStyleValuePair pair;                                           \
            pair.setValueKind(CSSStyleValuePair::ValueKind::Length);          \
            pair.setLengthValue(CSSLength(0.f));                              \
            valueList->pushBack(pair);                                        \
        }                                                                     \
        p.setValueList(valueList);                                            \
        addValuePair(p);                                                      \
    }

    ADD_VALUE_PAIR_BORDER_RADIUS(TopLeft, topLeft)
    ADD_VALUE_PAIR_BORDER_RADIUS(BottomLeft, bottomLeft)
    ADD_VALUE_PAIR_BORDER_RADIUS(TopRight, topRight)
    ADD_VALUE_PAIR_BORDER_RADIUS(BottomRight, bottomRight)

#define ADD_VALUE_PAIR(KEY, VALUE, GETTER)                   \
    else if (keyKind == CSSStyleValuePair::KeyKind::KEY)     \
    {                                                        \
        CSSStyleValuePair p;                                 \
        p.setKeyKind(CSSStyleValuePair::KeyKind::KEY);       \
        p.setValueKind(CSSStyleValuePair::ValueKind::VALUE); \
        p.setValue(style->GETTER());                         \
        addValuePair(p);                                     \
    }
    ADD_VALUE_PAIR(Display, DisplayValueKind, display)
    ADD_VALUE_PAIR(Position, PositionValueKind, position)
    ADD_VALUE_PAIR(Float, FloatValueKind, floating)
    ADD_VALUE_PAIR(Clear, ClearValueKind, clear)
    ADD_VALUE_PAIR(VerticalAlign, VerticalAlignValueKind, verticalAlign)
    ADD_VALUE_PAIR(TextAlign, SideValueKind, textAlign)
    ADD_VALUE_PAIR(TextDecorationLine, ValueListKind, textDecorationLine)
    ADD_VALUE_PAIR(TextDecorationStyle, TextDecorationStyleValueKind,
                   textDecorationStyle)
    ADD_VALUE_PAIR(TextUnderlinePosition, TextUnderlinePositionValueKind,
                   textUnderlinePosition)
    ADD_VALUE_PAIR(Resize, ResizeValueKind, resize)
    ADD_VALUE_PAIR(TextTransform, TextTransformValueKind, textTransform)
    ADD_VALUE_PAIR(Direction, DirectionValueKind, direction)
    ADD_VALUE_PAIR(Visibility, VisibilityValueKind, visibility)
    ADD_VALUE_PAIR(FontStyle, FontStyleValueKind, fontStyle)
    ADD_VALUE_PAIR(FontWeight, FontWeightValueKind, fontWeight)
    ADD_VALUE_PAIR(FontKerning, FontKerningValueKind, fontKerning)
    ADD_VALUE_PAIR(WordWrap, WordWrapValueKind, wordWrap)
    ADD_VALUE_PAIR(OverflowWrap, WordWrapValueKind, wordWrap)
    ADD_VALUE_PAIR(OverflowX, OverflowValueKind, overflowX)
    ADD_VALUE_PAIR(OverflowY, OverflowValueKind, overflowY)
    ADD_VALUE_PAIR(UnicodeBidi, UnicodeBidiValueKind, unicodeBidi)
    ADD_VALUE_PAIR(Opacity, Number, opacity)
    ADD_VALUE_PAIR(BoxSizing, BoxSizingValueKind, boxSizing)
    ADD_VALUE_PAIR(FlexDirection, FlexDirectionValueKind, flexDirection)
    ADD_VALUE_PAIR(FlexWrap, FlexWrapValueKind, flexWrap)
    ADD_VALUE_PAIR(Order, Int32, order)
    ADD_VALUE_PAIR(JustifyContent, JustifyContentValueKind, justifyContent)
    ADD_VALUE_PAIR(AlignItems, AlignItemValueKind, alignItems)
    ADD_VALUE_PAIR(AlignSelf, AlignItemValueKind, alignSelf)
    ADD_VALUE_PAIR(AlignContent, AlignContentValueKind, alignContent)
    ADD_VALUE_PAIR(FlexGrow, Number, flexGrow)
    ADD_VALUE_PAIR(FlexShrink, Number, flexShrink)
    ADD_VALUE_PAIR(FillOpacity, Number, fillOpacity)
    ADD_VALUE_PAIR(ObjectFit, ObjectFitValueKind, objectFit)
    ADD_VALUE_PAIR(ListStylePosition, ListStylePositionValueKind,
                   listStylePosition)
    ADD_VALUE_PAIR(ListStyleType, KeywordValueKind, listStyleType)
    ADD_VALUE_PAIR(UserSelect, UserSelectValueKind, userSelect)
    ADD_VALUE_PAIR(Hyphens, HyphensValueKind, hyphens)
    ADD_VALUE_PAIR(LineBreak, LineBreakValueKind, lineBreak)
    ADD_VALUE_PAIR(WordBreak, WordBreakValueKind, wordBreak)
    ADD_VALUE_PAIR(ImageRendering, ImageRenderingValueKind, imageRendering)
    ADD_VALUE_PAIR(PointerEvents, PointerEventsValueKind, pointerEvents)

#undef ADD_VALUE_PAIR
#define ADD_COLOR_PAIR(KEY, GETTER)                                     \
    else if (keyKind == CSSStyleValuePair::KeyKind::KEY)                \
    {                                                                   \
        CSSStyleValuePair p;                                            \
        p.setKeyKind(CSSStyleValuePair::KeyKind::KEY);                  \
        p.setValueKind(CSSStyleValuePair::ValueKind::KeywordValueKind); \
        p.setValue(style->GETTER().toString());                         \
        addValuePair(p);                                                \
    }
    ADD_COLOR_PAIR(Color, color)
    ADD_COLOR_PAIR(BackgroundColor, backgroundColor)
    ADD_COLOR_PAIR(TextDecorationColor, textDecorationColor)
    ADD_COLOR_PAIR(CaretColor, caretColor)
#undef ADD_COLOR_PAIR
    else
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
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

String* CSSStyleDeclaration::getPropertyValue(String* name)
{
    auto str = name->toNullableUTF8String();
    CSSStyleKind kind = lookupCSSStyle(str.m_buffer, str.m_bufferSize);
    String* val = String::emptyString;
    switch (kind) {
#define MATCH_KEY(Name, ...) \
    case CSSStyleKind::Name: \
        val = Name();        \
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
    CSSStyleKind kind = lookupCSSStyle(str.m_buffer, str.m_bufferSize);

    if (prior->length() > 0) {
        if (prior->equalsIgnoreCase("important")) {
            isImportant = true;
        } else {
            if (kind == CSSStyleKind::CustomProperty) {
                setCustomProperty(name, value, str.m_bufferSize);
            }
            return;
        }
    }

    struct Sender {
        CSSStyleDeclaration* self;
        CSSStyleKind kind;
        bool isImportant;
    } sender;
    sender.self = this;
    sender.kind = kind;
    sender.isImportant = isImportant;
    value->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
            CSSStyleKind kind = ((Sender*)data)->kind;
            CSSStyleDeclaration* self = ((Sender*)data)->self;
            bool isImportant = ((Sender*)data)->isImportant;
            if (kind == CSSStyleKind::Unknown) {
            } else {
                if (false) {
                }
#define SET_ATTR(name, nameLower, nameCSSCase)  \
    else if (kind == CSSStyleKind::name)        \
    {                                           \
        self->set##name(buf, len, isImportant); \
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
    CSSStyleKind kind = lookupCSSStyleCamelCase(str.m_buffer, str.m_bufferSize);

    if (kind == CSSStyleKind::Unknown) {
        kind = lookupCSSStyle(str.m_buffer, str.m_bufferSize);
    }
    if (kind == CSSStyleKind::Unknown) {
        return Nullable<String*>();
    }
    if (false) {
    }
#define GET_ATTR(name, ...)               \
    else if (kind == CSSStyleKind::name)  \
    {                                     \
        return Nullable<String*>(name()); \
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
    CSSStyleKind kind = lookupCSSStyleCamelCase(str.m_buffer, str.m_bufferSize);

    if (kind == CSSStyleKind::Unknown) {
        kind = lookupCSSStyle(str.m_buffer, str.m_bufferSize);
    }
    if (kind == CSSStyleKind::Unknown) {
        return false;
    }
    // Empty string let setter remove its value
    String* valueTo = String::emptyString;
    if (value.hasValue()) {
        valueTo = value.getValue();
    }

    struct Sender {
        CSSStyleDeclaration* self;
        CSSStyleKind kind;
    } sender;
    sender.self = this;
    sender.kind = kind;

    valueTo->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
            CSSStyleKind kind = ((Sender*)data)->kind;
            CSSStyleDeclaration* self = ((Sender*)data)->self;
            if (false) {
            }
#define SET_ATTR(name, ...)               \
    else if (kind == CSSStyleKind::name)  \
    {                                     \
        self->set##name(buf, len, false); \
    }
            FOR_EACH_STYLE_ATTRIBUTE_TOTAL(SET_ATTR)
#undef SET_ATTR
            return 0;
        },
        &sender);
    return true;
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

void ComputedStyleCSSStyleDeclaration::setCssText(String* text)
{
    STARFISH_ASSERT(m_node->isElement());
    throw new DOMException(
        m_node->asElement()->document(),
        DOMException::NO_MODIFICATION_ALLOWED_ERR,
        "These styles are computed, and therefore read-only.");
}
}
