/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "StarFish.h"
#include "core/dom/HTMLObjectElement.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"

namespace StarFish {

void* HTMLObjectElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLObjectElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLObjectElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLObjectElement, m_content));
        HTMLElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLObjectElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

QualifiedName HTMLObjectElement::name()
{
    return starFish()->staticStrings()->m_objectTagName;
}

void HTMLObjectElement::didAttributeChanged(QualifiedName name, String* old,
                                            String* value,
                                            bool attributeCreated,
                                            bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    if (name == starFish()->staticStrings()->m_type) {
        if (m_content) {
            m_content->unload();
        }

#ifdef STARFISH_ENABLE_AVPLAY
        if (value->toASCIILower()->equals("application/avplayer")) {
            m_content = new AVPlayHTMLObjectElementContent(this);
        } else {
            m_content = new MockHTMLObjectElementContent(this);
        }
#else
        m_content = new MockHTMLObjectElementContent(this);
#endif

        m_content->load();
        setNeedsFrameTreeBuild();
    }
}

void MockHTMLObjectElementContent::drawContent(Compositor* canvas,
                                               const LayoutRect& contentRect,
                                               const LayoutRect& absContentRect)
{
}

#ifdef STARFISH_ENABLE_AVPLAY
void AVPlayHTMLObjectElementContent::drawContent(
    Compositor* canvas, const LayoutRect& contentRect,
    const LayoutRect& absContentRect)
{
    canvas->punchHole(Unit::Rect(contentRect.x(), contentRect.y(),
                                 contentRect.width(), contentRect.height()));
}
#endif
}
