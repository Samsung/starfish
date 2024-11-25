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
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Node.h"
#include "FrameSVGClipPathBox.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"

namespace Starfish {

void* FrameSVGClipPathBox::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FrameSVGClipPathBox));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameSVGClipPathBox)] = { 0 };
        FrameSVGClipPathBox::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameSVGClipPathBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void FrameSVGClipPathBox::paintContent(PaintingContext& ctx)
{
    // ‘clipPath’ elements are never rendered directly
}

void FrameSVGClipPathBox::paintSVG(PaintingContext& ctx)
{
    // ‘clipPath’ elements are never rendered directly
}

Optional<Path*> FrameSVGClipPathBox::path()
{
    Optional<Path*> path = nullptr;
    Frame* child = firstChild();
    while (child) {
        if (child && child->isFrameSVGBox()) {
            FrameSVGBox* childBox = child->asFrameSVGBox();
            auto childPath = childBox->path();
            if (childPath) {
                childPath->translate(childBox->x().toInt(),
                                     childBox->y().toInt());
                if (path) {
                    path->append(childPath.value());
                } else {
                    path = childPath;
                }
            }
        }
        child = child->next();
    }
    return path;
}
} // namespace Starfish
