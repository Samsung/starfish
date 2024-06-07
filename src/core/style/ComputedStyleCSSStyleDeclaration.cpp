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

#include "StarfishConfig.h"

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
#include "core/style/FilterFunctions.h"
#include "core/style/GradientData.h"
#include "core/style/WillChangeData.h"

#include "core/style/CSSStyleDeclaration.h"

namespace Starfish {

void ComputedStyleCSSStyleDeclaration::layoutIfNeeds()
{
    if (m_node->isDocument()) {
        return;
    }

    m_node->window()->browsingContext()->webView()->layoutIfNeeded(false);
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
        m_node->asElement()->executionContext(),
        DOMException::NO_MODIFICATION_ALLOWED_ERR,
        "These styles are computed, and therefore read-only.");
}

void ComputedStyleCSSStyleDeclaration::triggerResolveComputedStyleIfNeeds(
    CSSStyleValuePair::KeyKind keyKind)
{
    BrowsingContext* browsingContext = m_node->window()->browsingContext();

    // Perform the minimum action required to resolve the style about |keykind|.
    switch (requiredStage(keyKind)) {
    case RequreidStyleResolveStage::kStayleResolution:
        browsingContext->resolveStyleIfNeeds();
        break;
    case RequreidStyleResolveStage::kFrameTreeBuild:
        browsingContext->buildFrameTreeIfNeeds();
        break;
    case RequreidStyleResolveStage::kLayout:
        browsingContext->layoutIfNeeded();
        break;
    default:
        STARFISH_ASSERT_NOT_REACHED();
        break;
    }
}

ComputedStyleCSSStyleDeclaration::RequreidStyleResolveStage
ComputedStyleCSSStyleDeclaration::requiredStage(
    CSSStyleValuePair::KeyKind keyKind)
{
    switch (keyKind) {
    case CSSStyleValuePair::KeyKind::Top:
    case CSSStyleValuePair::KeyKind::Right:
    case CSSStyleValuePair::KeyKind::Bottom:
    case CSSStyleValuePair::KeyKind::Left:
    case CSSStyleValuePair::KeyKind::PaddingTop:
    case CSSStyleValuePair::KeyKind::PaddingRight:
    case CSSStyleValuePair::KeyKind::PaddingBottom:
    case CSSStyleValuePair::KeyKind::PaddingLeft:
    case CSSStyleValuePair::KeyKind::MarginTop:
    case CSSStyleValuePair::KeyKind::MarginRight:
    case CSSStyleValuePair::KeyKind::MarginBottom:
    case CSSStyleValuePair::KeyKind::MarginLeft:
    case CSSStyleValuePair::KeyKind::BorderTop:
    case CSSStyleValuePair::KeyKind::BorderRight:
    case CSSStyleValuePair::KeyKind::BorderBottom:
    case CSSStyleValuePair::KeyKind::BorderLeft:
    case CSSStyleValuePair::KeyKind::Width:
    case CSSStyleValuePair::KeyKind::Height:
    case CSSStyleValuePair::KeyKind::PaddingInlineEnd:
    case CSSStyleValuePair::KeyKind::PaddingInlineStart:
    case CSSStyleValuePair::KeyKind::MarginInlineEnd:
    case CSSStyleValuePair::KeyKind::MarginInlineStart:
    case CSSStyleValuePair::KeyKind::GridTemplateRows:
    case CSSStyleValuePair::KeyKind::GridTemplateColumns:
        return RequreidStyleResolveStage::kLayout;
    case CSSStyleValuePair::KeyKind::MinWidth:
    case CSSStyleValuePair::KeyKind::MinHeight:
        return RequreidStyleResolveStage::kFrameTreeBuild;
    default:
        return RequreidStyleResolveStage::kStayleResolution;
    }

    return RequreidStyleResolveStage::kStayleResolution;
}

static CSSStyleValuePair stylePaintDataToCSSStyleValue(
    StylePaintData* paintData)
{
    CSSStyleValuePair ret;
    if (paintData->color().isTransparent()) {
        ret.setValueKind(CSSStyleValuePair::ValueKind::None);
    } else {
        ret.setValueKind(CSSStyleValuePair::ValueKind::ColorValueKind);
        ret.setColorValue(paintData->color());
    }
    return ret;
}

static CSSStyleValuePair resolveFlowRelativeInlineProperties(
    CSSStyleValuePair::KeyKind keykind, FrameBox* frame)
{
    // TODO: If 'writing-mode' is supported, the resolved value must be selected
    // using this property as well as 'direction' property.

    CSSStyleValuePair ret;
    if (frame->style()->direction() == DirectionValue::LtrDirectionValue) {
        // margin-inline.
        if (keykind == CSSStyleValuePair::KeyKind::MarginInlineStart) {
            ret.setKeyKind(keykind);
            ret.setValueKind(CSSStyleValuePair::ValueKind::Length);
            ret.setValue(CSSLength(frame->marginLeft()));
            return ret;
        }
        if (keykind == CSSStyleValuePair::KeyKind::MarginInlineEnd) {
            ret.setKeyKind(keykind);
            ret.setValueKind(CSSStyleValuePair::ValueKind::Length);
            ret.setValue(CSSLength(frame->marginRight()));
            return ret;
        }

        // padding-inline.
        if (keykind == CSSStyleValuePair::KeyKind::PaddingInlineStart) {
            ret.setKeyKind(keykind);
            ret.setValueKind(CSSStyleValuePair::ValueKind::Length);
            ret.setValue(CSSLength(frame->paddingLeft()));
            return ret;
        }
        if (keykind == CSSStyleValuePair::KeyKind::PaddingInlineEnd) {
            ret.setKeyKind(keykind);
            ret.setValueKind(CSSStyleValuePair::ValueKind::Length);
            ret.setValue(CSSLength(frame->paddingRight()));
            return ret;
        }

        // border-inline.
        // border-inline-start.
        if (keykind == CSSStyleValuePair::KeyKind::BorderInlineStartColor) {
            ret.setKeyKind(keykind);
            ret.setColorValue(frame->style()->border().left().color());
            return ret;
        }
        if (keykind == CSSStyleValuePair::KeyKind::BorderInlineStartWidth) {
            ret.setKeyKind(keykind);
            ret.setValueKind(CSSStyleValuePair::ValueKind::Length);
            ret.setValue(CSSLength(frame->borderLeft()));
            return ret;
        }
        if (keykind == CSSStyleValuePair::KeyKind::BorderInlineStartStyle) {
            ret.setKeyKind(keykind);
            ret.setValueKind(
                CSSStyleValuePair::ValueKind::BorderStyleValueKind);
            ret.setValue(CSSLength(frame->style()->border().left().style()));
            return ret;
        }
        // border-inline-end.
        if (keykind == CSSStyleValuePair::KeyKind::BorderInlineEndColor) {
            ret.setKeyKind(keykind);
            ret.setColorValue(frame->style()->border().right().color());
            return ret;
        }
        if (keykind == CSSStyleValuePair::KeyKind::BorderInlineEndStyle) {
            ret.setKeyKind(keykind);
            ret.setValueKind(
                CSSStyleValuePair::ValueKind::BorderStyleValueKind);
            ret.setValue(CSSLength(frame->style()->border().right().style()));
            return ret;
        }
        if (keykind == CSSStyleValuePair::KeyKind::BorderInlineStartWidth) {
            ret.setKeyKind(keykind);
            ret.setValueKind(CSSStyleValuePair::ValueKind::Length);
            ret.setValue(CSSLength(frame->borderRight()));
            return ret;
        }
    } else {
        // margin-inline.
        if (keykind == CSSStyleValuePair::KeyKind::MarginInlineEnd) {
            ret.setKeyKind(keykind);
            ret.setValueKind(CSSStyleValuePair::ValueKind::Length);
            ret.setValue(CSSLength(frame->marginLeft()));
            return ret;
        }
        if (keykind == CSSStyleValuePair::KeyKind::MarginInlineStart) {
            ret.setKeyKind(keykind);
            ret.setValueKind(CSSStyleValuePair::ValueKind::Length);
            ret.setValue(CSSLength(frame->marginRight()));
            return ret;
        }

        // padding-inline
        if (keykind == CSSStyleValuePair::KeyKind::PaddingInlineEnd) {
            ret.setKeyKind(keykind);
            ret.setValueKind(CSSStyleValuePair::ValueKind::Length);
            ret.setValue(CSSLength(frame->paddingLeft()));
            return ret;
        }
        if (keykind == CSSStyleValuePair::KeyKind::PaddingInlineStart) {
            ret.setKeyKind(keykind);
            ret.setValueKind(CSSStyleValuePair::ValueKind::Length);
            ret.setValue(CSSLength(frame->paddingRight()));
            return ret;
        }

        // border-inline.
        // border-inline-start.
        if (keykind == CSSStyleValuePair::KeyKind::BorderInlineStartColor) {
            ret.setKeyKind(keykind);
            ret.setColorValue(frame->style()->border().right().color());
            return ret;
        }
        if (keykind == CSSStyleValuePair::KeyKind::BorderInlineStartWidth) {
            ret.setKeyKind(keykind);
            ret.setValueKind(CSSStyleValuePair::ValueKind::Length);
            ret.setValue(CSSLength(frame->borderRight()));
            return ret;
        }
        if (keykind == CSSStyleValuePair::KeyKind::BorderInlineStartStyle) {
            ret.setKeyKind(keykind);
            ret.setValueKind(
                CSSStyleValuePair::ValueKind::BorderStyleValueKind);
            ret.setValue(CSSLength(frame->style()->border().right().style()));
            return ret;
        }
        // border-inline-end.
        if (keykind == CSSStyleValuePair::KeyKind::BorderInlineEndColor) {
            ret.setKeyKind(keykind);
            ret.setColorValue(frame->style()->border().left().color());
            return ret;
        }
        if (keykind == CSSStyleValuePair::KeyKind::BorderInlineEndStyle) {
            ret.setKeyKind(keykind);
            ret.setValueKind(
                CSSStyleValuePair::ValueKind::BorderStyleValueKind);
            ret.setValue(CSSLength(frame->style()->border().left().style()));
            return ret;
        }
        if (keykind == CSSStyleValuePair::KeyKind::BorderInlineStartWidth) {
            ret.setKeyKind(keykind);
            ret.setValueKind(CSSStyleValuePair::ValueKind::Length);
            ret.setValue(CSSLength(frame->borderLeft()));
            return ret;
        }
    }

    return ret;
}

static CSSStyleValuePair resolveFlowRelativeBlockProperties(
    CSSStyleValuePair::KeyKind keykind, FrameBox* frame)
{
    // margin.
    CSSStyleValuePair ret;
    if (keykind == CSSStyleValuePair::KeyKind::MarginBlockStart) {
        ret.setKeyKind(keykind);
        ret.setValueKind(CSSStyleValuePair::ValueKind::Length);
        ret.setValue(CSSLength(frame->marginTop()));
        return ret;
    }
    if (keykind == CSSStyleValuePair::KeyKind::MarginBlockEnd) {
        ret.setKeyKind(keykind);
        ret.setValueKind(CSSStyleValuePair::ValueKind::Length);
        ret.setValue(CSSLength(frame->marginBottom()));
        return ret;
    }

    // border.
    // border-block-start.
    if (keykind == CSSStyleValuePair::KeyKind::BorderBlockStartColor) {
        ret.setKeyKind(keykind);
        ret.setColorValue(frame->style()->border().top().color());
        return ret;
    }
    if (keykind == CSSStyleValuePair::KeyKind::BorderBlockStartStyle) {
        ret.setKeyKind(keykind);
        ret.setValueKind(CSSStyleValuePair::ValueKind::BorderStyleValueKind);
        ret.setValue(CSSLength(frame->style()->border().top().style()));
        return ret;
    }
    if (keykind == CSSStyleValuePair::KeyKind::BorderBlockStartWidth) {
        ret.setKeyKind(keykind);
        ret.setValueKind(CSSStyleValuePair::ValueKind::Length);
        ret.setValue(CSSLength(frame->borderTop()));
        return ret;
    }

    // border-block-end.
    if (keykind == CSSStyleValuePair::KeyKind::BorderBlockEndColor) {
        ret.setKeyKind(keykind);
        ret.setColorValue(frame->style()->border().bottom().color());
        return ret;
    }
    if (keykind == CSSStyleValuePair::KeyKind::BorderBlockEndStyle) {
        ret.setKeyKind(keykind);
        ret.setValueKind(CSSStyleValuePair::ValueKind::BorderStyleValueKind);
        ret.setValue(CSSLength(frame->style()->border().bottom().style()));
        return ret;
    }
    if (keykind == CSSStyleValuePair::KeyKind::BorderBlockEndWidth) {
        ret.setKeyKind(keykind);
        ret.setValueKind(CSSStyleValuePair::ValueKind::Length);
        ret.setValue(CSSLength(frame->borderBottom()));
        return ret;
    }
    return ret;
}

void ComputedStyleCSSStyleDeclaration::updateValue(
    CSSStyleValuePair::KeyKind keyKind, String* customPropertyName)
{
    triggerResolveComputedStyleIfNeeds(keyKind);

    if (m_node->isDocument() || m_node->style() == nullptr) {
        return;
    }

    // Create CSSStyleValuePair and set the resolved style value to
    // CSSStyleValuePair. And add CSSStyleValuePair to m_cssValues.
    // Util perform this at least once, m_cssValues has no items.
    Frame* frame = m_node->frame();
    ComputedStyle* style = m_node->style();

    switch (keyKind) {
#define IGNORE_SHORTHANDS_AND_ETC(Name, ...) \
    case CSSStyleValuePair::KeyKind::Name:   \
        break;
        FOR_EACH_STYLE_ATTRIBUTE_SHORTHAND(IGNORE_SHORTHANDS_AND_ETC)
        IGNORE_SHORTHANDS_AND_ETC(KeyKindSize)
        IGNORE_SHORTHANDS_AND_ETC(Src)
        IGNORE_SHORTHANDS_AND_ETC(Unknown)
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
    case CSSStyleValuePair::KeyKind::BorderBlockStartWidth: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderBlockStartWidth);
        if (frame && frame->isFrameBox()) {
            p = resolveFlowRelativeBlockProperties(
                CSSStyleValuePair::KeyKind::BorderBlockStartWidth,
                frame->asFrameBox());
        } else if (frame && frame->isFrameInline()) {
            InlineNonReplacedBox* inb =
                blockContainer(frame)->firstInlineNonReplacedBox(
                    frame->asFrameInline());
            if (inb != nullptr) {
                p = resolveFlowRelativeBlockProperties(
                    CSSStyleValuePair::KeyKind::BorderBlockStartWidth, inb);
            } else {
                p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
            }
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::BorderBlockEndWidth: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderBlockEndWidth);
        if (frame && frame->isFrameBox()) {
            p = resolveFlowRelativeBlockProperties(
                CSSStyleValuePair::KeyKind::BorderBlockEndWidth,
                frame->asFrameBox());
        } else if (frame && frame->isFrameInline()) {
            InlineNonReplacedBox* inb =
                blockContainer(frame)->firstInlineNonReplacedBox(
                    frame->asFrameInline());
            if (inb != nullptr) {
                p = resolveFlowRelativeBlockProperties(
                    CSSStyleValuePair::KeyKind::BorderBlockEndWidth, inb);
            } else {
                p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
            }
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::BorderInlineStartWidth: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderInlineStartWidth);
        if (frame && frame->isFrameBox()) {
            p = resolveFlowRelativeInlineProperties(
                CSSStyleValuePair::KeyKind::BorderInlineStartWidth,
                frame->asFrameBox());
        } else if (frame && frame->isFrameInline()) {
            InlineNonReplacedBox* inb =
                blockContainer(frame)->firstInlineNonReplacedBox(
                    frame->asFrameInline());
            if (inb != nullptr) {
                p = resolveFlowRelativeInlineProperties(
                    CSSStyleValuePair::KeyKind::BorderInlineStartWidth, inb);
            } else {
                p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
            }
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::BorderInlineEndWidth: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderInlineEndWidth);
        if (frame && frame->isFrameBox()) {
            p = resolveFlowRelativeInlineProperties(
                CSSStyleValuePair::KeyKind::BorderInlineEndWidth,
                frame->asFrameBox());
        } else if (frame && frame->isFrameInline()) {
            InlineNonReplacedBox* inb =
                blockContainer(frame)->firstInlineNonReplacedBox(
                    frame->asFrameInline());
            if (inb != nullptr) {
                p = resolveFlowRelativeInlineProperties(
                    CSSStyleValuePair::KeyKind::BorderInlineEndWidth, inb);
            } else {
                p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
            }
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
        }
        addValuePair(p);
    } break;
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
                    l1 = cb->absolutePoint(
                        m_node->document()->frame()->asFrameDocument());
                    l2 = parent->absolutePoint(
                        m_node->document()->frame()->asFrameDocument());
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
            w.setLengthValue(CSSLength(style->width().specifiedValue(
                LayoutContext::parentContentWidth(frame), m_node)));
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
            bool parentHasFixedHeight =
                LayoutContext::parentHasFixedHeight(frame);
            if (style->height().isDefinite(parentHasFixedHeight)) {
                LayoutUnit parentContentHeight;
                if (parentHasFixedHeight) {
                    parentContentHeight =
                        LayoutContext::parentFixedHeight(frame);
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
            ValueList* val = new ValueList(
                Separator::CommaSeparatorAppendQuoteWhenMeetWhiteSpace);
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
        if (flexBasis.isAuto()) {
            p.setValueKind(CSSStyleValuePair::ValueKind::FlexBasisValueKind);
            p.setValue(FlexBasisValue::AutoFlexBasisValue);
        } else if (flexBasis.isContent()) {
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
    case CSSStyleValuePair::KeyKind::LineClamp: {
        CSSStyleValuePair lc;
        lc.setKeyKind(CSSStyleValuePair::KeyKind::LineClamp);
        lc.setNumberValue(style->lineClamp());
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
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
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
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
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
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
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
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
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
        bgImageValues = new ValueList(Separator::CommaSeparator);

        for (unsigned int i = 0; i < style->backgroundLayerSize(); i++) {
            CSSStyleValuePair item;
            auto imageValue = style->backgroundImage(i);
            if (!imageValue) {
                item.setValueKind(CSSStyleValuePair::None);
            } else if (imageValue->type().isURL()) {
                item.setUrlValue(imageValue->urlValue());
            } else if (imageValue->type().isGradient()) {
                item.setGradientValue(
                    imageValue->gradientValue()->convertToCSSGradientValue());
            } else {
                item.setValueKind(CSSStyleValuePair::None);
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

        bgSizeValues = new ValueList(Separator::CommaSeparator);

        for (unsigned int i = 0; i < style->backgroundLayerSize(); i++) {
            CSSStyleValuePair item;
            if (style->backgroundSizeIsLength(i)) {
                item.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
                ValueList* vals = new ValueList(Separator::SpaceSeparator);
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

        bgAttachmentValues = new ValueList(Separator::CommaSeparator);

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

        bgClipValues = new ValueList(Separator::CommaSeparator);
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

        bgOriginValues = new ValueList(Separator::CommaSeparator);

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
            bgRepeatX.setRepeatStyleValue(style->backgroundRepeatX(0));
            addValuePair(bgRepeatX);
            return;
        }
        bgRepeatX.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);

        ValueList* bgRepeatXValues;

        bgRepeatXValues = new ValueList(Separator::CommaSeparator);

        for (unsigned int i = 0; i < style->backgroundLayerSize(); i++) {
            CSSStyleValuePair item;

            item.setRepeatStyleValue(style->backgroundRepeatX(i));
            bgRepeatXValues->push_back(item);
        }

        bgRepeatX.setValueList(bgRepeatXValues);
        addValuePair(bgRepeatX);

    } break;
    case CSSStyleValuePair::KeyKind::BackgroundRepeatY: {
        CSSStyleValuePair bgRepeatY;

        bgRepeatY.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundRepeatY);
        if (!style->backgroundLayerSize()) {
            bgRepeatY.setRepeatStyleValue(style->backgroundRepeatY(0));
            addValuePair(bgRepeatY);
            return;
        }
        bgRepeatY.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);

        ValueList* bgRepeatYValues;

        bgRepeatYValues = new ValueList(Separator::CommaSeparator);

        for (unsigned int i = 0; i < style->backgroundLayerSize(); i++) {
            CSSStyleValuePair item;
            item.setRepeatStyleValue(style->backgroundRepeatY(i));
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

        bgPositionXValues = new ValueList(Separator::CommaSeparator);

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

        bgPositionYValues = new ValueList(Separator::CommaSeparator);

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
        ValueList* vals = new ValueList(Separator::SpaceSeparator);
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
            ValueList* vals = new ValueList(Separator::SpaceSeparator);
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
        ValueList* vals = new ValueList(Separator::SpaceSeparator);
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
        ValueList* vals = new ValueList(Separator::SpaceSeparator);
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
        if (!style->hasTransforms() || !frame) {
            p.setValueKind(CSSStyleValuePair::ValueKind::None);
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::TransformFunctions);
            FrameBox* box = frame->findNearestAssociateBox();
            SkMatrix m = style->transformsToMatrix(
                box->width(), box->height(), box, style->hasTransforms(frame));

            CSSTransformFunctions* transforms = new CSSTransformFunctions();

            ValueList* values = new ValueList(Separator::CommaSeparator);

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
    case CSSStyleValuePair::KeyKind::BorderBlockStartStyle: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderBlockStartStyle);
        if (frame && frame->isFrameBox()) {
            p = resolveFlowRelativeBlockProperties(
                CSSStyleValuePair::KeyKind::BorderBlockStartStyle,
                frame->asFrameBox());
        }
    } break;
    case CSSStyleValuePair::KeyKind::BorderBlockEndStyle: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderBlockEndStyle);
        if (frame && frame->isFrameBox()) {
            p = resolveFlowRelativeBlockProperties(
                CSSStyleValuePair::KeyKind::BorderBlockEndStyle,
                frame->asFrameBox());
        }
    } break;
    case CSSStyleValuePair::KeyKind::BorderInlineStartStyle: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderInlineStartStyle);
        if (frame && frame->isFrameBox()) {
            p = resolveFlowRelativeInlineProperties(
                CSSStyleValuePair::KeyKind::BorderInlineStartStyle,
                frame->asFrameBox());
        }
    } break;
    case CSSStyleValuePair::KeyKind::BorderInlineEndStyle: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderInlineEndStyle);
        if (frame && frame->isFrameBox()) {
            p = resolveFlowRelativeInlineProperties(
                CSSStyleValuePair::KeyKind::BorderInlineEndStyle,
                frame->asFrameBox());
        }
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
    case CSSStyleValuePair::KeyKind::BorderBlockStartColor: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderBlockStartColor);
        if (frame && frame->isFrameBox()) {
            p = resolveFlowRelativeBlockProperties(
                CSSStyleValuePair::KeyKind::BorderBlockStartColor,
                frame->asFrameBox());
        }
    } break;
    case CSSStyleValuePair::KeyKind::BorderBlockEndColor: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderBlockEndColor);
        if (frame && frame->isFrameBox()) {
            p = resolveFlowRelativeBlockProperties(
                CSSStyleValuePair::KeyKind::BorderBlockEndColor,
                frame->asFrameBox());
        }
    } break;
    case CSSStyleValuePair::KeyKind::BorderInlineStartColor: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderInlineStartColor);
        if (frame && frame->isFrameBox()) {
            p = resolveFlowRelativeInlineProperties(
                CSSStyleValuePair::KeyKind::BorderInlineStartColor,
                frame->asFrameBox());
        }
    } break;
    case CSSStyleValuePair::KeyKind::BorderInlineEndColor: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::BorderInlineEndColor);
        if (frame && frame->isFrameBox()) {
            p = resolveFlowRelativeInlineProperties(
                CSSStyleValuePair::KeyKind::BorderInlineEndColor,
                frame->asFrameBox());
        }
    } break;
    case CSSStyleValuePair::KeyKind::ObjectPosition: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::ObjectPosition);

        if (!style->hasObjectSizing()) {
            p.setValueKind(CSSStyleValuePair::ValueKind::None);
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
            ValueList* vals = new ValueList(Separator::SpaceSeparator);
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
    case CSSStyleValuePair::KeyKind::ClipPath: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::ClipPath);
        p.setUrlValue(style->clipPath());
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::ColumnGap: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::ColumnGap);
        Length gap = style->columnGap();
        if (gap.isFixed()) {
            p.setLengthValue(CSSLength(gap.fixed()));
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
            shadows.setValueList(new ValueList(Separator::CommaSeparator));

            for (auto& sd : *style->textShadow()) {
                CSSStyleValuePair s;
                s.setValueList(new ValueList(Separator::SpaceSeparator));
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
                lengths.setValueList(new ValueList(Separator::SpaceSeparator));

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
            ValueList* list = new ValueList(Separator::SpaceSeparator);
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
                        m_node->starfish(), data->counterStyle()->name()));
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
        p.setValueList(new ValueList(Separator::CommaSeparator));
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
        p.setValueList(new ValueList(Separator::CommaSeparator));
        size_t layerSize = style->transitionLayerSize();
        if (!layerSize) {
            p.multiValue()->emplace_back(CSSStyleValuePair::Time, CSSTime(0));
        } else {
            ValueList* list = new ValueList(Separator::CommaSeparator);
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
        p.setValueList(new ValueList(Separator::CommaSeparator));
        size_t layerSize = style->transitionLayerSize();
        if (!layerSize) {
            p.multiValue()->emplace_back(CSSStyleValuePair::Time, CSSTime(0));
        } else {
            ValueList* list = new ValueList(Separator::CommaSeparator);
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
        p.setValueList(new ValueList(Separator::CommaSeparator));
        size_t layerSize = style->transitionLayerSize();
        if (!layerSize) {
            p.multiValue()->emplace_back(
                CSSStyleValuePair::TimingFunctionPointerKind,
                StyleTransitionData::defaultTimingFunction());
        } else {
            ValueList* list = new ValueList(Separator::CommaSeparator);
            for (size_t i = 0; i < layerSize; i++) {
                p.multiValue()->emplace_back(
                    CSSStyleValuePair::TimingFunctionPointerKind,
                    style->transitionTimingFunction(i));
            }
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::AnimationName: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::AnimationName);
        p.setValueList(new ValueList(Separator::CommaSeparator));
        size_t size =
            style->animation() ? style->animation()->animationNameSize() : 0;
        if (size == 0) {
            p.multiValue()->emplace_back(CSSStyleValuePair::StringValueKind,
                                         String::fromUTF8("none"));
        } else {
            for (size_t i = 0; i < size; i++) {
                p.multiValue()->emplace_back(
                    CSSStyleValuePair::StringValueKind,
                    style->animation()->animationName(i));
            }
        }

        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::AnimationDuration: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::AnimationDuration);
        p.setValueList(new ValueList(Separator::CommaSeparator));
        size_t size =
            style->animation() ? style->animation()->durationSize() : 0;
        if (size == 0) {
            p.multiValue()->emplace_back(CSSStyleValuePair::Time, CSSTime(0));
        } else {
            for (size_t i = 0; i < size; i++) {
                p.multiValue()->emplace_back(CSSStyleValuePair::Time,
                                             style->animationDuration(i));
            }
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::AnimationTimingFunction: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::AnimationTimingFunction);
        p.setValueList(new ValueList(Separator::CommaSeparator));
        size_t size =
            style->animation() ? style->animation()->timingFunctionSize() : 0;
        if (size == 0) {
            p.multiValue()->emplace_back(
                CSSStyleValuePair::TimingFunctionValueKind,
                TimingFunctionValue::TimingFunctionEaseValue);
        } else {
            for (size_t i = 0; i < size; i++) {
                p.multiValue()->emplace_back(
                    CSSStyleValuePair::TimingFunctionPointerKind,
                    style->animation()->timingFunction(i));
            }
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::AnimationDelay: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::AnimationDelay);
        p.setValueList(new ValueList(Separator::CommaSeparator));
        size_t size = style->animation() ? style->animation()->delaySize() : 0;
        if (size == 0) {
            p.multiValue()->emplace_back(CSSStyleValuePair::Time, CSSTime(0));
        } else {
            for (size_t i = 0; i < size; i++) {
                p.multiValue()->emplace_back(CSSStyleValuePair::Time,
                                             style->animationDelay(i));
            }
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::AnimationIterationCount: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::AnimationIterationCount);
        p.setValueList(new ValueList(Separator::CommaSeparator));
        size_t size =
            style->animation() ? style->animation()->iterationCountSize() : 0;
        if (size == 0) {
            p.multiValue()->emplace_back(CSSStyleValuePair::Number, 1.0f);
        } else {
            for (size_t i = 0; i < size; i++) {
                p.multiValue()->emplace_back(CSSStyleValuePair::Number,
                                             style->animationIterationCount(i));
            }
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::AnimationDirection: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::AnimationDirection);
        p.setValueList(new ValueList(Separator::CommaSeparator));
        size_t size =
            style->animation() ? style->animation()->directionSize() : 0;
        if (size == 0) {
            p.multiValue()->emplace_back(
                CSSStyleValuePair::AnimationDirectionValueKind,
                AnimationDirectionValue::AnimationDirectionNormalValue);
        } else {
            for (size_t i = 0; i < size; i++) {
                p.multiValue()->emplace_back(
                    CSSStyleValuePair::AnimationDirectionValueKind,
                    style->animationDirect(i));
            }
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::AnimationPlayState: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::AnimationPlayState);
        p.setValueList(new ValueList(Separator::CommaSeparator));
        size_t size =
            style->animation() ? style->animation()->playStateSize() : 0;
        if (size == 0) {
            p.multiValue()->emplace_back(
                CSSStyleValuePair::AnimationPlayStateValueKind,
                AnimationPlayStateValue::AnimationPlayStateRunningValue);
        } else {
            for (size_t i = 0; i < size; i++) {
                p.multiValue()->emplace_back(
                    CSSStyleValuePair::AnimationPlayStateValueKind,
                    style->animationPlayState(i));
            }
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::AnimationFillMode: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::AnimationFillMode);
        p.setValueList(new ValueList(Separator::CommaSeparator));
        size_t size =
            style->animation() ? style->animation()->fillModeSize() : 0;
        if (size == 0) {
            p.multiValue()->emplace_back(
                CSSStyleValuePair::AnimationFillModeValueKind,
                AnimationFillModeValue::AnimationFillModeNoneValue);
        } else {
            for (size_t i = 0; i < size; i++) {
                p.multiValue()->emplace_back(
                    CSSStyleValuePair::AnimationFillModeValueKind,
                    style->animationFillMode(i));
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
            shadows.setValueList(new ValueList(Separator::CommaSeparator));

            for (auto& sd : *style->boxShadow()) {
                CSSStyleValuePair s;
                s.setValueList(new ValueList(Separator::SpaceSeparator));
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
                lengths.setValueList(new ValueList(Separator::SpaceSeparator));

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
    case CSSStyleValuePair::KeyKind::StopColor: {
        CSSStyleValuePair p = stylePaintDataToCSSStyleValue(style->stopColor());
        p.setKeyKind(CSSStyleValuePair::KeyKind::StopColor);
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::Stroke: {
        CSSStyleValuePair p = stylePaintDataToCSSStyleValue(style->stroke());
        p.setKeyKind(CSSStyleValuePair::KeyKind::Stroke);
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::StrokeOpacity: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::StrokeOpacity);
        p.setValueKind(CSSStyleValuePair::ValueKind::Number);
        p.setNumberValue(style->strokeOpacity());
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
    case CSSStyleValuePair::KeyKind::X1: {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->x1());
        p.setKeyKind(CSSStyleValuePair::KeyKind::X1);
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::Y1: {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->y1());
        p.setKeyKind(CSSStyleValuePair::KeyKind::Y1);
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::X2: {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->x2());
        p.setKeyKind(CSSStyleValuePair::KeyKind::X2);
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::Y2: {
        CSSStyleValuePair p = lengthToCSSStyleValue(style->y2());
        p.setKeyKind(CSSStyleValuePair::KeyKind::Y2);
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
        STARFISH_UNIMPLEMENTED();
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::Cursor);
        // when cursor value type is implemented, add cursor value type instead
        // of string `auto`
        p.setKeywordValue(String::createASCIIString("auto"));
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::MaskImage: {
        CSSStyleValuePair maskImage;
        maskImage.setKeyKind(CSSStyleValuePair::KeyKind::MaskImage);
        if (!style->maskLayerSize()) {
            maskImage.setValueKind(CSSStyleValuePair::None);
            addValuePair(maskImage);
            return;
        }
        maskImage.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);

        ValueList* maskImageValues;
        maskImageValues = new ValueList(Separator::CommaSeparator);

        for (unsigned int i = 0; i < style->maskLayerSize(); i++) {
            CSSStyleValuePair item;
            auto imageValue = style->maskImage(i);
            if (!imageValue) {
                item.setValueKind(CSSStyleValuePair::None);
            } else if (imageValue->type().isURL()) {
                item.setUrlValue(imageValue->urlValue());
            } else if (imageValue->type().isGradient()) {
                item.setGradientValue(
                    imageValue->gradientValue()->convertToCSSGradientValue());
            } else {
                item.setValueKind(CSSStyleValuePair::None);
            }
            maskImageValues->push_back(item);
        }
        maskImage.setValueList(maskImageValues);
        addValuePair(maskImage);
    } break;
    case CSSStyleValuePair::KeyKind::MaskSize: {
        CSSStyleValuePair maskSize;
        maskSize.setKeyKind(CSSStyleValuePair::KeyKind::MaskSize);
        if (!style->maskLayerSize()) {
            maskSize.setValueKind(CSSStyleValuePair::Auto);
            addValuePair(maskSize);
            return;
        }

        maskSize.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        ValueList* maskSizeValues;
        maskSizeValues = new ValueList(Separator::CommaSeparator);
        for (unsigned int i = 0; i < style->maskLayerSize(); i++) {
            CSSStyleValuePair item;
            if (style->maskSizeIsLength(i)) {
                item.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
                ValueList* valueList = new ValueList(Separator::SpaceSeparator);
                LengthSize lengthSize = style->maskSizeLengthValue(i);

                CSSStyleValuePair w = lengthToCSSStyleValue(lengthSize.width());
                valueList->emplace_back(w.valueKind(), w.value());
                CSSStyleValuePair h =
                    lengthToCSSStyleValue(lengthSize.height());
                valueList->emplace_back(h.valueKind(), h.value());
                item.setValue(valueList);
            } else {
                item.setBackgroundSizeValue(style->maskSizeTypeValue(i));
                item.setValueKind(
                    CSSStyleValuePair::ValueKind::BackgroundSizeValueKind);
            }
            maskSizeValues->push_back(item);
        }

        maskSize.setValueList(maskSizeValues);
        addValuePair(maskSize);
    } break;
    case CSSStyleValuePair::KeyKind::MaskPositionX: {
        CSSStyleValuePair positionX;
        positionX.setKeyKind(CSSStyleValuePair::KeyKind::MaskPositionX);
        if (!style->maskLayerSize()) {
            positionX.setPercentageValue(0);
            addValuePair(positionX);
            return;
        }
        positionX.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);

        ValueList* positionXValues = new ValueList(Separator::CommaSeparator);
        for (unsigned int i = 0; i < style->maskLayerSize(); i++) {
            CSSStyleValuePair item;
            item = lengthToCSSStyleValue(style->maskPositionX(i));
            positionXValues->push_back(item);
        }
        positionX.setValueList(positionXValues);
        addValuePair(positionX);
    } break;
    case CSSStyleValuePair::KeyKind::MaskPositionY: {
        CSSStyleValuePair positionY;
        positionY.setKeyKind(CSSStyleValuePair::KeyKind::MaskPositionY);
        if (!style->maskLayerSize()) {
            positionY.setPercentageValue(0);
            addValuePair(positionY);
            return;
        }
        positionY.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);

        ValueList* positionYValues = new ValueList(Separator::CommaSeparator);
        for (unsigned int i = 0; i < style->maskLayerSize(); i++) {
            CSSStyleValuePair item;
            item = lengthToCSSStyleValue(style->maskPositionY(i));
            positionYValues->push_back(item);
        }
        positionY.setValueList(positionYValues);
        addValuePair(positionY);
    } break;
    case CSSStyleValuePair::KeyKind::MaskRepeatX: {
        CSSStyleValuePair repeatX;
        repeatX.setKeyKind(CSSStyleValuePair::KeyKind::MaskRepeatX);
        if (!style->maskLayerSize()) {
            repeatX.setRepeatStyleValue(style->maskRepeatX(0));
            addValuePair(repeatX);
            return;
        }

        repeatX.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        ValueList* maskRepeatXValues;
        maskRepeatXValues = new ValueList(Separator::CommaSeparator);
        for (unsigned int i = 0; i < style->maskLayerSize(); i++) {
            CSSStyleValuePair item;
            item.setRepeatStyleValue(style->maskRepeatX(i));
            maskRepeatXValues->push_back(item);
        }

        repeatX.setValueList(maskRepeatXValues);
        addValuePair(repeatX);
    } break;
    case CSSStyleValuePair::KeyKind::MaskRepeatY: {
        CSSStyleValuePair repeatY;
        repeatY.setKeyKind(CSSStyleValuePair::KeyKind::MaskRepeatY);
        if (!style->maskLayerSize()) {
            repeatY.setRepeatStyleValue(style->maskRepeatY(0));
            addValuePair(repeatY);
            return;
        }

        repeatY.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
        ValueList* maskRepeatYValues;
        maskRepeatYValues = new ValueList(Separator::CommaSeparator);
        for (unsigned int i = 0; i < style->maskLayerSize(); i++) {
            CSSStyleValuePair item;
            item.setRepeatStyleValue(style->maskRepeatY(i));
            maskRepeatYValues->push_back(item);
        }

        repeatY.setValueList(maskRepeatYValues);
        addValuePair(repeatY);
    } break;
    case CSSStyleValuePair::KeyKind::GridTemplateColumns: {
        // FIXME: Fill CSSStyleValuePair with the computed value of owner frame.
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
        p.setValueList(new ValueList(Separator::SlashSeparator));
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
        p.setValueList(new ValueList(Separator::SlashSeparator));
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
        // FIXME: Fill CSSStyleValuePair with the computed value of owner frame.
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
        ValueList* values = new ValueList(Separator::SpaceSeparator);
        Length row = style->gridRowGap();
        Length column = style->columnGap();
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
    case CSSStyleValuePair::KeyKind::GridTemplateAreas: {
        // FIXME: Fill CSSStyleValuePair with the computed value of owner frame.
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
        p.setValueList(new ValueList(Separator::SlashSeparator));

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
            ValueList* v = new ValueList(Separator::SpaceSeparator);
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
            ValueList* v = new ValueList(Separator::SpaceSeparator);
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
            ValueList* list = new ValueList(Separator::CommaSeparator);
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
            p.setValue(new ValueList(Separator::SpaceSeparator));
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
    case CSSStyleValuePair::KeyKind::Filter: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::Filter);
        FilterFunctions* filter = style->filter();
        if (!filter) {
            p.setValueKind(CSSStyleValuePair::ValueKind::None);
        } else {
            filter->toCSSStyleValue(p);
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::MarginBlockStart: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::MarginBlockStart);
        if (frame && frame->isFrameBox()) {
            p = resolveFlowRelativeBlockProperties(
                CSSStyleValuePair::KeyKind::MarginBlockStart,
                frame->asFrameBox());
        } else if (frame && frame->isFrameInline()) {
            InlineNonReplacedBox* inb =
                blockContainer(frame)->firstInlineNonReplacedBox(
                    frame->asFrameInline());
            if (inb != nullptr) {
                p = resolveFlowRelativeBlockProperties(
                    CSSStyleValuePair::KeyKind::MarginBlockStart, inb);
            } else {
                p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
            }
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::MarginBlockEnd: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::MarginBlockEnd);
        if (frame && frame->isFrameBox()) {
            p = resolveFlowRelativeBlockProperties(
                CSSStyleValuePair::KeyKind::MarginBlockEnd,
                frame->asFrameBox());
        } else if (frame && frame->isFrameInline()) {
            InlineNonReplacedBox* inb =
                blockContainer(frame)->firstInlineNonReplacedBox(
                    frame->asFrameInline());
            if (inb != nullptr) {
                p = resolveFlowRelativeBlockProperties(
                    CSSStyleValuePair::KeyKind::MarginBlockEnd, inb);
            } else {
                p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
            }
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::MarginInlineEnd: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::MarginInlineEnd);
        if (frame && frame->isFrameBox()) {
            p = resolveFlowRelativeInlineProperties(
                CSSStyleValuePair::KeyKind::MarginInlineEnd,
                frame->asFrameBox());
        } else if (frame && frame->isFrameInline()) {
            InlineNonReplacedBox* inb =
                blockContainer(frame)->firstInlineNonReplacedBox(
                    frame->asFrameInline());
            if (inb != nullptr) {
                p = resolveFlowRelativeInlineProperties(
                    CSSStyleValuePair::KeyKind::MarginInlineEnd, inb);
            } else {
                p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
            }
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::MarginInlineStart: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::MarginInlineStart);
        if (frame && frame->isFrameBox()) {
            p = resolveFlowRelativeInlineProperties(
                CSSStyleValuePair::KeyKind::MarginInlineStart,
                frame->asFrameBox());
        } else if (frame && frame->isFrameInline()) {
            InlineNonReplacedBox* inb =
                blockContainer(frame)->firstInlineNonReplacedBox(
                    frame->asFrameInline());
            if (inb != nullptr) {
                p = resolveFlowRelativeInlineProperties(
                    CSSStyleValuePair::KeyKind::MarginInlineStart, inb);
            } else {
                p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
            }
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::PaddingInlineEnd: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::PaddingInlineEnd);
        if (frame && frame->isFrameBox()) {
            p = resolveFlowRelativeInlineProperties(
                CSSStyleValuePair::KeyKind::PaddingInlineEnd,
                frame->asFrameBox());
        } else if (frame && frame->isFrameInline()) {
            InlineNonReplacedBox* inb =
                blockContainer(frame)->firstInlineNonReplacedBox(
                    frame->asFrameInline());
            if (inb != nullptr) {
                p = resolveFlowRelativeInlineProperties(
                    CSSStyleValuePair::KeyKind::PaddingInlineEnd, inb);
            } else {
                p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
            }
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::PaddingInlineStart: {
        CSSStyleValuePair p;
        p.setKeyKind(CSSStyleValuePair::KeyKind::PaddingInlineStart);
        if (frame && frame->isFrameBox()) {
            p = resolveFlowRelativeInlineProperties(
                CSSStyleValuePair::KeyKind::PaddingInlineStart,
                frame->asFrameBox());
        } else if (frame && frame->isFrameInline()) {
            InlineNonReplacedBox* inb =
                blockContainer(frame)->firstInlineNonReplacedBox(
                    frame->asFrameInline());
            if (inb != nullptr) {
                p = resolveFlowRelativeInlineProperties(
                    CSSStyleValuePair::KeyKind::PaddingInlineStart, inb);
            } else {
                p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
            }
        } else {
            p.setValueKind(CSSStyleValuePair::ValueKind::Auto);
        }
        addValuePair(p);
    } break;
    case CSSStyleValuePair::KeyKind::CustomProperty: {
        if (style->hasCustomProperty()) {
            Nullable<MutablePropertyValueList*> customPropertyties =
                style->customProperty();
            if (customPropertyties) {
                AtomicString key = AtomicString::createAtomicString(
                    m_node->starfish(), customPropertyName);
                Nullable<String*> value =
                    customPropertyties.value()->property(key);
                if (value) {
                    setCustomProperty(key, value.value());
                }
            }
        }
    } break;
#define ADD_VALUE_PAIR_BORDER_RADIUS(Name1Name2, name1Name2)                  \
    case CSSStyleValuePair::KeyKind::Border##Name1Name2##Radius: {            \
        CSSStyleValuePair p;                                                  \
        p.setKeyKind(CSSStyleValuePair::KeyKind::Border##Name1Name2##Radius); \
        p.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);          \
        ValueList* valueList = new ValueList(Separator::SpaceSeparator);      \
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
        ADD_VALUE_PAIR(StopOpacity, Number, stopOpacity)
        ADD_VALUE_PAIR(BoxDecorationBreak, BoxDecorationBreakValueKind,
                       boxDecorationBreak)
        ADD_VALUE_PAIR(BoxSizing, BoxSizingValueKind, boxSizing)
        ADD_VALUE_PAIR(BoxOrient, BoxOrientValueKind, boxOrient)
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
        ADD_VALUE_PAIR(Appearance, AppearanceValueKind, appearance)
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
} // namespace Starfish
