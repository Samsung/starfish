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

#include "FrameOptionBox.h"

#include "core/dom/HTMLOptionElement.h"
#include "core/dom/HTMLSelectElement.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameSelectBox.h"

namespace StarFish {

void* FrameOptionBox::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FrameOptionBox));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameOptionBox)] = { 0 };
        FrameOptionBox::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameOptionBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

FrameOptionBox::FrameOptionBox(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
}

FrameSelectBox* FrameOptionBox::selectBox()
{
    for (Frame* p = parent(); p; p = p->parent()) {
        if (p->node() && (p->node()->isHTMLIFrameElement() ||
                          p->node()->isHTMLFormElement())) {
            return nullptr;
        }

        if (p->isFrameSelectBox()) {
            return p->asFrameSelectBox();
        }
    }

    return nullptr;
}

void FrameOptionBox::layout(LayoutContext& ctx,
                            Frame::LayoutWantToResolve resolveWhat)
{
    if (node() && node()->isHTMLOptionElement() &&
        node()->asHTMLOptionElement()->isDisabled()) {
        return;
    }

    STARFISH_ASSERT(node()->isHTMLOptionElement());
    HTMLOptionElement* optionNode = node()->asHTMLOptionElement();
    HTMLSelectElement* selectNode = optionNode->selectElement();

    if (!selectNode) {
        FrameBlockBox::layout(ctx, resolveWhat);
    } else {
        if (selectNode->displaySize() == 1) {
            if (optionNode->selectedness() &&
                selectBox()->m_drawOptionsCount == 0) {
                if (resolveWhat & ResolveHeight) {
                    selectBox()->m_drawOptionsCount++;
                }
                FrameBlockBox::layout(ctx, resolveWhat);
            }
        } else if (selectNode->displaySize() > 1 &&
                   selectBox()->m_drawOptionsCount <
                       selectNode->displaySize()) {
            if (resolveWhat & ResolveHeight) {
                selectBox()->m_drawOptionsCount++;
            }
            FrameBlockBox::layout(ctx, resolveWhat);
        }
    }
}
}
