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
};

void CSSStyleDeclaration::rootPointerValueIfExists(CSSStyleValuePair v)
{
    auto p = v.pointerValue();
    if (p) {
        m_pointerRooter.insert(p);
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
    STARFISH_ASSERT(m_element);
    return m_element->scriptBindingInstance();
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

#define DEFINE_ATTRIBUTE_SETTER(name, ...)                                    \
    void CSSStyleDeclaration::set##name(const char* value, size_t len,        \
                                        bool isImportant)                     \
    {                                                                         \
        if (len == 0) {                                                       \
            removeCSSValuePair(CSSStyleValuePair::KeyKind::name);             \
            return;                                                           \
        }                                                                     \
        CSSTokenVector tokens;                                                \
        if (UNLIKELY(CSSStyleValuePair::KeyKind::name ==                      \
                     CSSStyleValuePair::KeyKind::Content)) {                  \
            tokenizeCSSValue(tokens, value, len, "", 0, true);                \
        } else {                                                              \
            tokenizeCSSValue(tokens, value, len, ",", 1);                     \
        }                                                                     \
        CSSStyleValuePair ret;                                                \
        if (ret.updateVarValue(value, tokens)) {                              \
            ret.setFlagImportant(isImportant);                                \
            ret.setTemporaryKeyKind(CSSStyleValuePair::KeyKind::name);        \
            addCSSValuePair(CSSStyleValuePair::KeyKind::VarValue, ret);       \
            return;                                                           \
        }                                                                     \
        if (ret.updateValueCommon(tokens) || ret.updateValue##name(tokens)) { \
            ret.setFlagImportant(isImportant);                                \
            addCSSValuePair(CSSStyleValuePair::KeyKind::name, ret);           \
        }                                                                     \
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
    if (m_element == nullptr) {
        return;
    }

    m_element->window()->browsingContext()->webView()->layoutIfNeeds();
}

void ComputedStyleCSSStyleDeclaration::buildFrameTreeIfNeeds()
{
    if (m_element == nullptr) {
        return;
    }

    m_element->window()->browsingContext()->buildFrameTreeIfNeeds();
}

void ComputedStyleCSSStyleDeclaration::resolveStyleIfNeeds()
{
    if (m_element == nullptr) {
        return;
    }

    m_element->window()->browsingContext()->resolveStyleIfNeeds();
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
    if (m_element == nullptr || m_element->style() == nullptr) {
        return;
    }

    Frame* frame = m_element->frame();
    ComputedStyle* style = m_element->style();

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
            LayoutContext ctx(
                m_element->starFish(),
                m_element->document()->frame()->asFrameDocument());
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
                    m_element->starFish(),
                    m_element->document()->frame()->asFrameDocument());
                w.setLengthValue(CSSLength(style->width().specifiedValue(
                    ctx.parentContentWidth(frame), m_element)));
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
                    m_element->starFish(),
                    m_element->document()->frame()->asFrameDocument());
                bool parentHasFixedHeight = ctx.parentHasFixedHeight(frame);
                if (style->height().isDefinite(parentHasFixedHeight)) {
                    LayoutUnit parentContentHeight;
                    if (parentHasFixedHeight) {
                        parentContentHeight = ctx.parentFixedHeight(frame);
                    }
                    h.setLengthValue(CSSLength(style->height().specifiedValue(
                        parentContentHeight, m_element)));
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
            p.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            p.setValue(style->fontFamily()[1].m_familyName);
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
            ValueList* val =
                new ValueList(ValueList::Separator::
                                  CommaSeparatorAppendQuoteWhenMeetWhiteSpace);
            size_t len = style->fontFamily()[0].m_length;
            for (size_t i = 0; i < len; i++) {
                val->emplace_back(CSSStyleValuePair::ValueKind::StringValueKind,
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
        ValueList* vals = new ValueList();
        BorderData border = style->border();
        LengthBox box = border.image().slices();

        CSSStyleValuePair t = lengthToCSSStyleValue(box.top());
        vals->emplace_back(t.valueKind(), t.value());

        CSSStyleValuePair r = lengthToCSSStyleValue(box.right());
        vals->emplace_back(r.valueKind(), r.value());

        CSSStyleValuePair b = lengthToCSSStyleValue(box.bottom());
        vals->emplace_back(b.valueKind(), b.value());

        CSSStyleValuePair l = lengthToCSSStyleValue(box.left());
        vals->emplace_back(l.valueKind(), l.value());

        p.setValue(vals);
        addValuePair(p);
    } else if (keyKind == CSSStyleValuePair::KeyKind::BorderImageWidth) {
        // FIXME: Need to refactor BorderImageLength.h, and update the lines
        // below

        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderImageWidth);
        p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        ValueList* vals = new ValueList();
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
    }
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
    ADD_VALUE_PAIR(TextDecoration, TextDecorationValueKind, textDecoration)
    ADD_VALUE_PAIR(TextTransform, TextTransformValueKind, textTransform)
    ADD_VALUE_PAIR(Direction, DirectionValueKind, direction)
    ADD_VALUE_PAIR(Visibility, VisibilityValueKind, visibility)
    ADD_VALUE_PAIR(FontStyle, FontStyleValueKind, fontStyle)
    ADD_VALUE_PAIR(FontWeight, FontWeightValueKind, fontWeight)
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
#undef ADD_VALUE_PAIR
#define ADD_COLOR_PAIR(KEY, GETTER)                                    \
    else if (keyKind == CSSStyleValuePair::KeyKind::KEY)               \
    {                                                                  \
        CSSStyleValuePair p;                                           \
        p.setKeyKind(CSSStyleValuePair::KeyKind::KEY);                 \
        p.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind); \
        p.setValue(style->GETTER().toString());                        \
        addValuePair(p);                                               \
    }
    ADD_COLOR_PAIR(Color, color)
    ADD_COLOR_PAIR(BackgroundColor, backgroundColor)
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
    m_element->setStyleAttr(text);
}

void ComputedStyleCSSStyleDeclaration::setCssText(String* text)
{
    throw new DOMException(
        m_element->document(), DOMException::NO_MODIFICATION_ALLOWED_ERR,
        "These styles are computed, and therefore read-only.");
}
}
