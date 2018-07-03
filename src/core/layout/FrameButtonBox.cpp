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

#include "FrameButtonBox.h"

#include "core/dom/Document.h"
#include "core/dom/HTMLInputElement.h"
#include "core/page/Window.h"

namespace StarFish {

void* FrameButtonBox::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FrameButtonBox));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameButtonBox)] = { 0 };
        FrameButtonBox::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameButtonBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

FrameButtonBox::FrameButtonBox(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
}

void FrameButtonBox::inlineLayoutAdditionalPath(LayoutContext& ctx)
{
    bool shouldContinueOperation = true;
    LayoutUnit lineBoxesHeight;
    for (size_t i = 0; i < lineBoxes().size(); i++) {
        auto lineBox = lineBoxes()[i];
        lineBoxesHeight += lineBox->height();
        for (size_t j = 0; j < lineBox->boxes().size(); j++) {
            if (lineBox->boxes()[j]->isFloating()) {
                shouldContinueOperation = false;
                break;
            }
        }
        if (!shouldContinueOperation) {
            break;
        }
    }

    LayoutUnit availableHeight = contentHeight() - lineBoxesHeight;
    if (availableHeight > 0 && shouldContinueOperation) {
        LayoutUnit d;
        for (size_t i = 0; i < lineBoxes().size(); i++) {
            lineBoxes()[i]->setY(availableHeight / 2 + paddingTop() +
                                 borderTop() + d);
            d += lineBoxes()[i]->height();
        }
    }
}
}
