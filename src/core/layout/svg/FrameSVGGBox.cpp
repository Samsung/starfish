/*
 * Copyright (c) 2025-present Samsung Electronics Co., Ltd
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
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Node.h"
#include "FrameSVGGBox.h"

namespace Starfish {

void* FrameSVGGBox::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FrameSVGGBox));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameSVGGBox)] = { 0 };
        FrameSVGGBox::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameSVGGBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

LayoutRect FrameSVGGBox::boundingRect()
{
    LayoutRect result;

    Frame* child = firstChild();
    while (child) {
        if (child) {
            if (child->isFrameSVGGBox()) {
                result.unite(child->asFrameSVGGBox()->boundingRect());
            } else if (child->isFrameSVGBox()) {
                FrameSVGBox* childBox = child->asFrameSVGBox();
                auto childPath = childBox->path();
                if (childPath) {
                    if (childBox->computedSVGTransform().hasValue()) {
                        auto svgMatrix =
                            *childBox->computedSVGTransform().getValue();
                        Unit::Rect childRect =
                            childPath.getValue()->boundingRect();
                        childRect.setX(childRect.x() +
                                       svgMatrix.getTranslateX());
                        childRect.setY(childRect.y() +
                                       svgMatrix.getTranslateY());
                        result.unite(LayoutRect(childRect.x(), childRect.y(),
                                                childRect.width(),
                                                childRect.height()));
                    }
                }
            }
        }
        child = child->next();
    }
    return result;
}
} // namespace Starfish
