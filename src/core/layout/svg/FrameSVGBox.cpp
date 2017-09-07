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
    if (resolveWhat & Frame::ResolveWidth) {
        auto styleWidth = style()->width();
        LayoutUnit width;
        if (!styleWidth.isAuto()) {
            width = styleWidth.specifiedValue(cb->width(), ctx.viewportWidth());
        }
        setWidth(width);
    }

    if (resolveWhat & Frame::ResolveHeight) {
        auto styleHeight = style()->height();
        LayoutUnit height;
        if (!styleHeight.isAuto()) {
            height =
                styleHeight.specifiedValue(cb->height(), ctx.viewportHeight());
        }
        setHeight(height);
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
    FrameBox* cb = layoutParent()->asFrameBox();

    LayoutUnit xResult;
    String* x = node()->asElement()->getAttributeOrEmpty(
        node()->starFish()->staticStrings()->m_x);
    if (x->length()) {
        auto s = x->toUTF8NonGCString();
        CSSStyleValuePair pair;
        if (CSSPropertyParser::parseLengthOrNumber(s.data(), false, true,
                                                   &pair)) {
            auto l = pair.lengthValue();
            Length ll = l.toLength();

            ComputedStyle* rootStyle =
                node()->document()->rootElement()->style();
            ll.changeToFixedIfNeeded(style()->fontSize(), rootStyle->fontSize(),
                                     style()->font());
            xResult = ll.specifiedValue(cb->width(), ctx.viewportWidth());
        }
    }

    setX(xResult);

    LayoutUnit yResult;
    String* y = node()->asElement()->getAttributeOrEmpty(
        node()->starFish()->staticStrings()->m_y);
    if (y->length()) {
        auto s = y->toUTF8NonGCString();
        CSSStyleValuePair pair;
        if (CSSPropertyParser::parseLengthOrNumber(s.data(), false, true,
                                                   &pair)) {
            auto l = pair.lengthValue();
            Length ll = l.toLength();

            ComputedStyle* rootStyle =
                node()->document()->rootElement()->style();
            ll.changeToFixedIfNeeded(style()->fontSize(), rootStyle->fontSize(),
                                     style()->font());
            yResult = ll.specifiedValue(cb->height(), ctx.viewportHeight());
        }
    }

    setY(yResult);
}

void FrameSVGBox::paint(PaintingContext& ctx)
{
    ctx.m_canvas->save();

    if (style()->visibility() == VisibilityValue::HiddenVisibilityValue) {
        ctx.m_canvas->setVisible(false);
    } else {
        ctx.m_canvas->setVisible(true);
    }

    ctx.m_canvas->save();
    paintSVG(ctx);
    ctx.m_canvas->restore();
    paintChildrenWith(ctx);

    ctx.m_canvas->restore();
}
}
