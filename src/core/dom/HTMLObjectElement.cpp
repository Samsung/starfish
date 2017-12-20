/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "StarFish.h"
#include "core/dom/HTMLObjectElement.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"

namespace StarFish {

void* HTMLObjectElement::operator new(size_t size)
{
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
        setNeedsFrameTreeBuild(false);
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
