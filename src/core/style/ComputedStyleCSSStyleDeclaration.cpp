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

#include "StarFishConfig.h"

#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBox.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameDocument.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/style/ComputedStyle.h"
#include "core/style/CSSCounterFunction.h"
#include "core/style/GradientData.h"

#include "core/style/CSSStyleDeclaration.h"

namespace StarFish {

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

void ComputedStyleCSSStyleDeclaration::setCssText(String* text)
{
    STARFISH_ASSERT(m_node->isElement());
    throw new DOMException(
        m_node->asElement()->document(),
        DOMException::NO_MODIFICATION_ALLOWED_ERR,
        "These styles are computed, and therefore read-only.");
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

void ComputedStyleCSSStyleDeclaration::updateValue(
    CSSStyleValuePair::KeyKind keyKind)
{
    if (m_node->isDocument() || m_node->style() == nullptr) {
        return;
    }

    Frame* frame = m_node->frame();
    ComputedStyle* style = m_node->style();

    switch (keyKind) {
#define IGNORE_SHORTHANDS_AND_ETC(Name, ...) \
    case CSSStyleValuePair::KeyKind::Name:   \
        break;
        FOR_EACH_STYLE_ATTRIBUTE_SHORTHAND(IGNORE_SHORTHANDS_AND_ETC)
        IGNORE_SHORTHANDS_AND_ETC(CustomProperty)
        IGNORE_SHORTHANDS_AND_ETC(KeyKindSize)
        IGNORE_SHORTHANDS_AND_ETC(Src)
        IGNORE_SHORTHANDS_AND_ETC(Unknown)
        IGNORE_SHORTHANDS_AND_ETC(VarValue)
#undef IGNORE_SHORTHANDS_ETC

#define ADD_ABSOLUTE_LENGTH_PAIR(keyKind, getter)                     \
    case CSSStyleValuePair::KeyKind::keyKind: {                       \
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
    } break;

        ADD_ABSOLUTE_LENGTH_PAIR(MarginTop, marginTop);
        ADD_ABSOLUTE_LENGTH_PAIR(MarginRight, marginRight);
        ADD_ABSOLUTE_LENGTH_PAIR(MarginBottom, marginBottom);
        ADD_ABSOLUTE_LENGTH_PAIR(MarginLeft, marginLeft);
        ADD_ABSOLUTE_LENGTH_PAIR(PaddingTop, paddingTop);
        ADD_ABSOLUTE_LENGTH_PAIR(PaddingRight, paddingRight);
        ADD_ABSOLUTE_LENGTH_PAIR(PaddingBottom, paddingBottom);
        ADD_ABSOLUTE_LENGTH_PAIR(PaddingLeft, paddingLeft);
        ADD_ABSOLUTE_LENGTH_PAIR(BorderTopWidth, borderTop);
        ADD_ABSOLUTE_LENGTH_PAIR(BorderRightWidth, borderRight);
        ADD_ABSOLUTE_LENGTH_PAIR(BorderBottomWidth, borderBottom);
        ADD_ABSOLUTE_LENGTH_PAIR(BorderLeftWidth, borderLeft)
#undef ADD_ABSOLUTE_LENGTH_PAIR

    case CSSStyleValuePair::KeyKind::Top:
    case CSSStyleValuePair::KeyKind::Right:
    case CSSStyleValuePair::KeyKind::Bottom:
    case CSSStyleValuePair::KeyKind::Left: {
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
    } break;
    case CSSStyleValuePair::KeyKind::Width: {
        CSSStyleValuePair w;
        w.setKeyKind(CSSStyleValuePair::KeyKind::Width);
        if (frame && style->width().isDefinite(true)) {
            LayoutContext ctx(m_node->starFish(),
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
    } break;
    case CSSStyleValuePair::KeyKind::Height: {
        CSSStyleValuePair h;
        h.setKeyKind(CSSStyleValuePair::KeyKind::Height);
        if (frame && frame->isFrameBox()) {
            LayoutContext ctx(m_node->starFish(),
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
    } break;
    case CSSStyleValuePair::KeyKind::FontFamily: {
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
    } break;
    case CSSStyleValuePair::KeyKind::ZIndex: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::ZIndex);
        if (style->isSpecifiedZIndex()) {
            p.setValueKind(CSSStyleValuePair::ValueKind::Int32);
            p.setValue(style->zIndex());
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::All: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::All);
        p.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
        p.setValue(String::emptyString);
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::WhiteSpace: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::WhiteSpace);
        p.setValueKind(CSSStyleValuePair::WhiteSpaceValueKind);
        p.setValue(CSSStyleValuePair::ValueData(style->whiteSpace()));
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::FlexBasis: {
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
    } break;
    case CSSStyleValuePair::KeyKind::TextIndent: {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->textIndent());
        p.setKeyKind(CSSStyleValuePair::KeyKind::TextIndent);
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::LineHeight: {
        CSSStyleValuePair lh;
        lh.setKeyKind(CSSStyleValuePair::KeyKind::LineHeight);

        if (style->hasNormalLineHeight() || !frame) {
            lh.setValueKind(CSSStyleValuePair::ValueKind::Normal);
        } else {
            lh.setLengthValue(CSSLength(CSSLength::PX, frame->lineHeight()));
        }
        addValuePair(lh);
    } break;
    case CSSStyleValuePair::KeyKind::FontSize: {
        CSSStyleValuePair fs;
        fs.setKeyKind(CSSStyleValuePair::KeyKind::FontSize);

        fs.setLengthValue(CSSLength(CSSLength::PX, style->fixedFontSize()));

        addValuePair(fs);
    } break;
    case CSSStyleValuePair::KeyKind::LetterSpacing: {
        CSSStyleValuePair fs;
        fs.setKeyKind(CSSStyleValuePair::KeyKind::LetterSpacing);

        fs.setLengthValue(
            CSSLength(CSSLength::PX, style->letterSpacing().fixed()));

        addValuePair(fs);
    } break;
    case CSSStyleValuePair::KeyKind::WordSpacing: {
        CSSStyleValuePair fs;
        fs.setKeyKind(CSSStyleValuePair::KeyKind::WordSpacing);

        fs.setLengthValue(
            CSSLength(CSSLength::PX, style->wordSpacing().fixed()));

        addValuePair(fs);
    } break;
    case CSSStyleValuePair::KeyKind::MinWidth: {
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
    } break;
    case CSSStyleValuePair::KeyKind::MinHeight: {
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
    } break;
    case CSSStyleValuePair::KeyKind::MaxWidth: {
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
    } break;
    case CSSStyleValuePair::KeyKind::MaxHeight: {
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
    } break;
    case CSSStyleValuePair::KeyKind::BackgroundImage: {
        CSSStyleValuePair bgImage;
        bgImage.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundImage);
        if (!style->backgroundLayerSize()) {
            bgImage.setValueKind(CSSStyleValuePair::None);
            addValuePair(bgImage);
            return;
        }
        bgImage.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);

        ValueList* bgImageValues;
        bgImageValues = new ValueList(ValueList::Separator::CommaSeparator);

        for (unsigned int i = 0; i < style->backgroundLayerSize(); i++) {
            CSSStyleValuePair item;
            auto imageValue = style->backgroundImage();
            const auto& type = imageValue->type();
            if (type.isNone()) {
                item.setStringValue(String::emptyString);
            } else if (type.isURL()) {
                item.setUrlValue(imageValue->urlValue());
            } else if (type.isGradient()) {
                item.setGradientValue(
                    imageValue->gradientValue()->convertToCSSGradientValue());
            }
            bgImageValues->push_back(item);
        }
        bgImage.setValueList(bgImageValues);
        addValuePair(bgImage);
    } break;
    case CSSStyleValuePair::KeyKind::BackgroundSize: {
        CSSStyleValuePair bgSize;

        bgSize.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundSize);
        if (!style->backgroundLayerSize()) {
            bgSize.setValueKind(CSSStyleValuePair::Auto);
            addValuePair(bgSize);
            return;
        }
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

    } break;
    case CSSStyleValuePair::KeyKind::BackgroundAttachment: {
        CSSStyleValuePair bgAttachment;

        bgAttachment.setKeyKind(
            CSSStyleValuePair::KeyKind::BackgroundAttachment);
        if (!style->backgroundLayerSize()) {
            bgAttachment.setBackgroundAttachmentValue(
                ScrollBackgroundAttachmentValue);
            addValuePair(bgAttachment);
            return;
        }
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

    } break;
    case CSSStyleValuePair::KeyKind::BackgroundClip: {
        CSSStyleValuePair bgClip;

        bgClip.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundClip);
        if (!style->backgroundLayerSize()) {
            bgClip.setValueKind(CSSStyleValuePair::BoxValueKind);
            bgClip.setValue(BorderBoxBoxValue);
            addValuePair(bgClip);
            return;
        }
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

    } break;
    case CSSStyleValuePair::KeyKind::BackgroundOrigin: {
        CSSStyleValuePair bgOrigin;

        bgOrigin.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundOrigin);
        if (!style->backgroundLayerSize()) {
            bgOrigin.setValueKind(CSSStyleValuePair::BoxValueKind);
            bgOrigin.setValue(PaddingBoxBoxValue);
            addValuePair(bgOrigin);
            return;
        }
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

    } break;
    case CSSStyleValuePair::KeyKind::BackgroundRepeatX: {
        CSSStyleValuePair bgRepeatX;

        bgRepeatX.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundRepeatX);
        if (!style->backgroundLayerSize()) {
            bgRepeatX.setBackgroundRepeatValue(style->backgroundRepeatX());
            addValuePair(bgRepeatX);
            return;
        }
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

    } break;
    case CSSStyleValuePair::KeyKind::BackgroundRepeatY: {
        CSSStyleValuePair bgRepeatY;

        bgRepeatY.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundRepeatY);
        if (!style->backgroundLayerSize()) {
            bgRepeatY.setBackgroundRepeatValue(style->backgroundRepeatY());
            addValuePair(bgRepeatY);
            return;
        }
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

    } break;
    case CSSStyleValuePair::KeyKind::BackgroundPositionX: {
        CSSStyleValuePair bgPositionX;

        bgPositionX.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundPositionX);
        if (!style->backgroundLayerSize()) {
            bgPositionX.setPercentageValue(0);
            addValuePair(bgPositionX);
            return;
        }
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

    } break;
    case CSSStyleValuePair::KeyKind::BackgroundPositionY: {
        CSSStyleValuePair bgPositionY;

        bgPositionY.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundPositionY);
        if (!style->backgroundLayerSize()) {
            bgPositionY.setPercentageValue(0);
            addValuePair(bgPositionY);
            return;
        }
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

    } break;
    case CSSStyleValuePair::KeyKind::BorderImageSource: {
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
    } break;
    case CSSStyleValuePair::KeyKind::BorderImageSlice: {
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
    } break;
    case CSSStyleValuePair::KeyKind::BorderImageRepeat: {
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
    } break;
    case CSSStyleValuePair::KeyKind::BorderImageWidth: {
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
    } break;
    case CSSStyleValuePair::KeyKind::BorderImageOutset: {
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
    } break;
    case CSSStyleValuePair::KeyKind::TransformOrigin: {
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
    } break;
    case CSSStyleValuePair::KeyKind::Transform: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::Transform);
        if (!style->hasTransforms()) {
            p.setValueKind(CSSStyleValuePair::ValueKind::None);
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::TransformFunctions);
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
    } break;
    case CSSStyleValuePair::KeyKind::BorderTopStyle: {
        CSSStyleValuePair tStyle;
        tStyle.setKeyKind(CSSStyleValuePair::KeyKind::BorderTopStyle);
        tStyle.setBorderStyleValue(style->border().top().style());
        addValuePair(tStyle);
    } break;
    case CSSStyleValuePair::KeyKind::BorderBottomStyle: {
        CSSStyleValuePair bStyle;
        bStyle.setKeyKind(CSSStyleValuePair::KeyKind::BorderBottomStyle);
        bStyle.setBorderStyleValue(style->border().bottom().style());
        addValuePair(bStyle);
    } break;
    case CSSStyleValuePair::KeyKind::BorderLeftStyle: {
        CSSStyleValuePair lStyle;
        lStyle.setKeyKind(CSSStyleValuePair::KeyKind::BorderLeftStyle);
        lStyle.setBorderStyleValue(style->border().left().style());
        addValuePair(lStyle);
    } break;
    case CSSStyleValuePair::KeyKind::BorderRightStyle: {
        CSSStyleValuePair rStyle;
        rStyle.setKeyKind(CSSStyleValuePair::KeyKind::BorderRightStyle);
        rStyle.setBorderStyleValue(style->border().right().style());
        addValuePair(rStyle);
    } break;
    case CSSStyleValuePair::KeyKind::BorderTopColor: {
        CSSStyleValuePair tColor;
        tColor.setKeyKind(CSSStyleValuePair::KeyKind::BorderTopColor);
        tColor.setColorValue(style->border().top().color());
        addValuePair(tColor);
    } break;
    case CSSStyleValuePair::KeyKind::BorderBottomColor: {
        CSSStyleValuePair bColor;
        bColor.setKeyKind(CSSStyleValuePair::KeyKind::BorderBottomColor);
        bColor.setColorValue(style->border().bottom().color());
        addValuePair(bColor);
    } break;
    case CSSStyleValuePair::KeyKind::BorderLeftColor: {
        CSSStyleValuePair lColor;
        lColor.setKeyKind(CSSStyleValuePair::KeyKind::BorderLeftColor);
        lColor.setColorValue(style->border().left().color());
        addValuePair(lColor);
    } break;
    case CSSStyleValuePair::KeyKind::BorderRightColor: {
        CSSStyleValuePair rColor;
        rColor.setKeyKind(CSSStyleValuePair::KeyKind::BorderRightColor);
        rColor.setColorValue(style->border().right().color());
        addValuePair(rColor);
    } break;
    case CSSStyleValuePair::KeyKind::ObjectPosition: {
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
    } break;
    case CSSStyleValuePair::KeyKind::Clip: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::Clip);
        if (style->clip()) {
            p.setValueKind(CSSStyleValuePair::ValueKind::RectValueKind);
            p.setValue(style->clip());
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::ListStyleImage: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::ListStyleImage);
        const ListStyleData& listStyle = style->listStyleData();
        if (listStyle.image()->length() == 0) {
            p.setValueKind(CSSStyleValuePair::ValueKind::None);
        } else {
            p.setUrlValue(listStyle.image());
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::TextShadow: {
        CSSStyleValuePair shadows;
        shadows.setKeyKind(CSSStyleValuePair::KeyKind::TextShadow);
        if (!style->textShadow()) {
            shadows.setValueKind(CSSStyleValuePair::ValueKind::None);
        } else {
            shadows.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
            shadows.setValueList(
                new ValueList(ValueList::Separator::CommaSeparator));

            for (auto& sd : *style->textShadow()) {
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
    } break;
    case CSSStyleValuePair::KeyKind::BorderCollapse: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderCollapse);
        p.setValueKind(CSSStyleValuePair::ValueKind::BorderCollapseValueKind);
        p.setValue(CSSStyleValuePair::ValueData(style->borderCollapse()));
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::BorderSpacing: {
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
    } break;
    case CSSStyleValuePair::KeyKind::TableLayout: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::TableLayout);
        p.setValueKind(CSSStyleValuePair::ValueKind::TableLayoutValueKind);
        p.setValue(CSSStyleValuePair::ValueData(style->tableLayout()));
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::Content: {
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
    } break;
    case CSSStyleValuePair::KeyKind::TransitionProperty: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::TransitionProperty);
        p.setValueList(new ValueList(ValueList::CommaSeparator));
        size_t layerSize = style->transitionLayerSize();
        if (!layerSize) {
            p.multiValue()->emplace_back(
                CSSStyleValuePair::CSSPropertyNameValueKind,
                CSSStyleValuePair::All);
        } else {
            for (size_t i = 0; i < layerSize; i++) {
                p.multiValue()->emplace_back(
                    CSSStyleValuePair::CSSPropertyNameValueKind,
                    style->transitionProperty(i));
            }
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::TransitionDuration: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::TransitionDuration);
        p.setValueList(new ValueList(ValueList::CommaSeparator));
        size_t layerSize = style->transitionLayerSize();
        if (!layerSize) {
            p.multiValue()->emplace_back(CSSStyleValuePair::Time, CSSTime(0));
        } else {
            ValueList* list = new ValueList(ValueList::CommaSeparator);
            for (size_t i = 0; i < layerSize; i++) {
                p.multiValue()->emplace_back(CSSStyleValuePair::Time,
                                             style->transitionDuration(i));
            }
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::TransitionDelay: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::TransitionDelay);
        p.setValueList(new ValueList(ValueList::CommaSeparator));
        size_t layerSize = style->transitionLayerSize();
        if (!layerSize) {
            p.multiValue()->emplace_back(CSSStyleValuePair::Time, CSSTime(0));
        } else {
            ValueList* list = new ValueList(ValueList::CommaSeparator);
            for (size_t i = 0; i < layerSize; i++) {
                p.multiValue()->emplace_back(CSSStyleValuePair::Time,
                                             style->transitionDelay(i));
            }
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::TransitionTimingFunction: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::TransitionTimingFunction);
        p.setValueList(new ValueList(ValueList::CommaSeparator));
        size_t layerSize = style->transitionLayerSize();
        if (!layerSize) {
            p.multiValue()->emplace_back(
                CSSStyleValuePair::AnimationTimingFunctionValueKind,
                StyleTransitionData::defaultTimingFunction());
        } else {
            ValueList* list = new ValueList(ValueList::CommaSeparator);
            for (size_t i = 0; i < layerSize; i++) {
                p.multiValue()->emplace_back(
                    CSSStyleValuePair::AnimationTimingFunctionValueKind,
                    style->transitionTimingFunction(i));
            }
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::BoxShadow: {
        CSSStyleValuePair shadows;
        shadows.setKeyKind(CSSStyleValuePair::KeyKind::BoxShadow);
        if (!style->boxShadow()) {
            shadows.setValueKind(CSSStyleValuePair::ValueKind::None);
        } else {
            shadows.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
            shadows.setValueList(
                new ValueList(ValueList::Separator::CommaSeparator));

            for (auto& sd : *style->boxShadow()) {
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
    } break;
    case CSSStyleValuePair::KeyKind::Fill: {
        CSSStyleValuePair p = stylePaintDataToCSSStyleValue(style->fill());
        p.setKeyKind(CSSStyleValuePair::KeyKind::Fill);
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::FillOpacity: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::FillOpacity);
        p.setValueKind(CSSStyleValuePair::ValueKind::Number);
        p.setNumberValue(style->fillOpacity());
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::FillRule: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::FillRule);
        p.setValueKind(CSSStyleValuePair::ValueKind::FillRuleValueKind);
        p.setValue(CSSStyleValuePair::ValueData(style->fillRule()));
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::Stroke: {
        CSSStyleValuePair p = stylePaintDataToCSSStyleValue(style->stroke());
        p.setKeyKind(CSSStyleValuePair::KeyKind::Stroke);
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::StrokeWidth: {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->strokeWidth());
        p.setKeyKind(CSSStyleValuePair::KeyKind::StrokeWidth);
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::X: {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->x());
        p.setKeyKind(CSSStyleValuePair::KeyKind::X);
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::Y: {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->y());
        p.setKeyKind(CSSStyleValuePair::KeyKind::Y);
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::CX: {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->cx());
        p.setKeyKind(CSSStyleValuePair::KeyKind::CX);
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::CY: {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->cy());
        p.setKeyKind(CSSStyleValuePair::KeyKind::CY);
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::R: {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->r());
        p.setKeyKind(CSSStyleValuePair::KeyKind::R);
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::RX: {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->rx());
        p.setKeyKind(CSSStyleValuePair::KeyKind::RX);
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::RY: {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->ry());
        p.setKeyKind(CSSStyleValuePair::KeyKind::RY);
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::OutlineColor: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::OutlineColor);
        p.setValueKind(CSSStyleValuePair::ValueKind::ColorValueKind);
        p.setColorValue(style->color());
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::OutlineWidth: {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->outlineWidth());
        p.setKeyKind(CSSStyleValuePair::KeyKind::OutlineWidth);
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::OutlineOffset: {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->outlineOffset());
        p.setKeyKind(CSSStyleValuePair::KeyKind::OutlineOffset);
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::OutlineStyle: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::OutlineStyle);
        p.setValueKind(CSSStyleValuePair::ValueKind::BorderStyleValueKind);
        p.setBorderStyleValue(style->outlineStyle());
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::Cursor: {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::Cursor);
        // when cursor value type is implemented, add cursor value type instead
        // of string `auto`
        p.setKeywordValue(String::createASCIIString("auto"));
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::MaskImage: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::MaskImage);
        String* url = style->maskImage();
        if (url->length()) {
            p.setUrlValue(url);
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::None);
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::MaskSize: {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::MaskSize);
        p.setValueKind(CSSStyleValuePair::ValueKind::None);
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::GridTemplateColumns: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::GridTemplateColumns);
        if (style->gridTemplateColumns()) {
            p.setValueKind(CSSStyleValuePair::ValueKind::GridTemplateUnits);
            p.setGridTemplateUnits(style->gridTemplateColumns());
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::None);
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::GridColumn: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::GridColumn);
        p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        p.setValueList(new ValueList(ValueList::Separator::SlashSeparator));
        CSSStyleValuePair cs;
        cs.setKeyKind(CSSStyleValuePair::KeyKind::GridColumnStart);
        cs.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
        if (style->gridColumnStart() != String::emptyString) {
            cs.setStringValue(style->gridColumnStart());
        } else {
            cs.setStringValue(String::createASCIIString("auto"));
        }
        p.multiValue()->emplace_back(cs.valueKind(), cs.value());

        CSSStyleValuePair ce;
        ce.setKeyKind(CSSStyleValuePair::KeyKind::GridColumnEnd);
        ce.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
        if (style->gridColumnEnd() != String::emptyString) {
            ce.setStringValue(style->gridColumnEnd());
        } else {
            ce.setStringValue(String::createASCIIString("auto"));
        }
        p.multiValue()->emplace_back(ce.valueKind(), ce.value());
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::GridRow: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::GridRow);
        p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        p.setValueList(new ValueList(ValueList::Separator::SlashSeparator));
        CSSStyleValuePair rs;
        rs.setKeyKind(CSSStyleValuePair::KeyKind::GridRowStart);
        rs.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
        if (style->gridRowStart() != String::emptyString) {
            rs.setStringValue(style->gridRowStart());
        } else {
            rs.setStringValue(String::createASCIIString("auto"));
        }
        p.multiValue()->emplace_back(rs.valueKind(), rs.value());

        CSSStyleValuePair re;
        re.setKeyKind(CSSStyleValuePair::KeyKind::GridRowEnd);
        re.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
        if (style->gridRowEnd() != String::emptyString) {
            re.setStringValue(style->gridRowEnd());
        } else {
            re.setStringValue(String::createASCIIString("auto"));
        }
        p.multiValue()->emplace_back(re.valueKind(), re.value());
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::GridTemplateRows: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::GridTemplateRows);
        if (style->gridTemplateRows()) {
            p.setValueKind(CSSStyleValuePair::ValueKind::GridTemplateUnits);
            p.setGridTemplateUnits(style->gridTemplateRows());
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::None);
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::GridGap: {
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
    } break;
    case CSSStyleValuePair::KeyKind::GridRowGap: {
        CSSStyleValuePair p;
        Length row = style->gridRowGap();
        if (row.isFixed()) {
            p.setLengthValue(CSSLength(row.fixed()));
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::GridColumnGap: {
        CSSStyleValuePair p;
        Length column = style->gridColumnGap();
        if (column.isFixed()) {
            p.setLengthValue(CSSLength(column.fixed()));
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::GridTemplateAreas: {
        CSSStyleValuePair p;
        String* areas = style->gridTemplateAreas();
        if (areas) {
            p.setKeyKind(CSSStyleValuePair::KeyKind::GridTemplateAreas);
            p.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            p.setStringValue(areas);
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::None);
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::GridArea: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::GridArea);
        p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        p.setValueList(new ValueList(ValueList::Separator::SlashSeparator));

        // row start / column start / row end / column end
        CSSStyleValuePair rs;
        rs.setKeyKind(CSSStyleValuePair::KeyKind::GridRowStart);
        rs.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
        if (style->gridRowStart() != String::emptyString) {
            rs.setStringValue(style->gridRowStart());
        } else {
            rs.setStringValue(String::createASCIIString("auto"));
        }
        p.multiValue()->emplace_back(rs.valueKind(), rs.value());

        CSSStyleValuePair cs;
        cs.setKeyKind(CSSStyleValuePair::KeyKind::GridColumnStart);
        cs.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
        if (style->gridColumnStart() != String::emptyString) {
            cs.setStringValue(style->gridColumnStart());
        } else {
            cs.setStringValue(String::createASCIIString("auto"));
        }
        p.multiValue()->emplace_back(cs.valueKind(), cs.value());

        CSSStyleValuePair re;
        re.setKeyKind(CSSStyleValuePair::KeyKind::GridRowEnd);
        re.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
        if (style->gridRowEnd() != String::emptyString) {
            re.setStringValue(style->gridRowEnd());
        } else {
            re.setStringValue(String::createASCIIString("auto"));
        }
        p.multiValue()->emplace_back(re.valueKind(), re.value());

        CSSStyleValuePair ce;
        ce.setKeyKind(CSSStyleValuePair::KeyKind::GridColumnEnd);
        ce.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
        if (style->gridColumnEnd() != String::emptyString) {
            ce.setStringValue(style->gridColumnEnd());
        } else {
            ce.setStringValue(String::createASCIIString("auto"));
        }
        p.multiValue()->emplace_back(ce.valueKind(), ce.value());

        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::TextOverflow: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::TextOverflow);
        p.setValueKind(CSSStyleValuePair::ValueKind::TextOverflowValueKind);
        p.setValue(new TextOverflowData(style->textOverflow()));
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::CounterReset: {
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
    } break;
    case CSSStyleValuePair::KeyKind::CounterIncrement: {
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
    } break;
    case CSSStyleValuePair::KeyKind::GridRowStart: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::GridRowStart);
        p.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
        p.setStringValue(style->gridRowStart());
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::GridRowEnd: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::GridRowEnd);
        p.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
        p.setStringValue(style->gridRowEnd());
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::GridColumnStart: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::GridColumnStart);
        p.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
        p.setStringValue(style->gridColumnStart());
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::GridColumnEnd: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::GridColumnStart);
        p.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
        p.setStringValue(style->gridColumnStart());
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::WillChange: {
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
    } break;
    case CSSStyleValuePair::KeyKind::TextDecorationLine: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::TextDecorationLine);
        p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        if (style->textDecorationLine()) {
            p.setValue(style->textDecorationLine());
        } else {
            p.setValue(new ValueList(ValueList::Separator::SpaceSeparator));
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::D: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::D);
        String* dvalue = style->d();
        if (dvalue->length()) {
            p.setPathFunctionValue(dvalue);
        } else {
            p.setValueKind(CSSStyleValuePair::None);
        }
        addValuePair(p);
    } break;
#define ADD_VALUE_PAIR_BORDER_RADIUS(Name1Name2, name1Name2)                  \
    case CSSStyleValuePair::KeyKind::Border##Name1Name2##Radius: {            \
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
    } break;

        ADD_VALUE_PAIR_BORDER_RADIUS(TopLeft, topLeft)
        ADD_VALUE_PAIR_BORDER_RADIUS(BottomLeft, bottomLeft)
        ADD_VALUE_PAIR_BORDER_RADIUS(TopRight, topRight)
        ADD_VALUE_PAIR_BORDER_RADIUS(BottomRight, bottomRight)
#undef ADD_VALUE_PAIR_BORDER_RADIUS

#define ADD_VALUE_PAIR(KEY, VALUE, GETTER)                   \
    case CSSStyleValuePair::KeyKind::KEY: {                  \
        CSSStyleValuePair p;                                 \
        p.setKeyKind(CSSStyleValuePair::KeyKind::KEY);       \
        p.setValueKind(CSSStyleValuePair::ValueKind::VALUE); \
        p.setValue(style->GETTER());                         \
        addValuePair(p);                                     \
    } break;
        ADD_VALUE_PAIR(Display, DisplayValueKind, display)
        ADD_VALUE_PAIR(Position, PositionValueKind, position)
        ADD_VALUE_PAIR(Float, FloatValueKind, floating)
        ADD_VALUE_PAIR(CaptionSide, CaptionSideValueKind, captionSide)
        ADD_VALUE_PAIR(Clear, ClearValueKind, clear)
        ADD_VALUE_PAIR(EmptyCells, EmptyCellsValueKind, emptyCells)
        ADD_VALUE_PAIR(VerticalAlign, VerticalAlignValueKind, verticalAlign)
        ADD_VALUE_PAIR(TextAlign, SideValueKind, textAlign)
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
        ADD_VALUE_PAIR(BoxDecorationBreak, BoxDecorationBreakValueKind,
                       boxDecorationBreak)
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
    case CSSStyleValuePair::KeyKind::KEY: {                             \
        CSSStyleValuePair p;                                            \
        p.setKeyKind(CSSStyleValuePair::KeyKind::KEY);                  \
        p.setValueKind(CSSStyleValuePair::ValueKind::KeywordValueKind); \
        p.setValue(style->GETTER().toString());                         \
        addValuePair(p);                                                \
    } break;
        ADD_COLOR_PAIR(Color, color)
        ADD_COLOR_PAIR(BackgroundColor, backgroundColor)
        ADD_COLOR_PAIR(TextDecorationColor, textDecorationColor)
        ADD_COLOR_PAIR(CaretColor, caretColor)
#undef ADD_COLOR_PAIR

    } /* switch */
}
}
