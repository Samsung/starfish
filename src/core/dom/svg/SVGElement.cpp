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
#include "core/dom/svg/SVGElement.h"
#include "StarFish.h"

namespace StarFish {

void* SVGElement::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGElement)] = { 0 };
        Element::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

String* SVGElement::xmlbase()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_xmlBase);
}

void SVGElement::setXmlbase(String* str)
{
    setAttribute(starFish()->staticStrings()->m_xmlBase, str);
}

SVGElement* SVGElement::ownerSVGElement()
{
    // The nearest ancestor ‘svg’ element. Null if the given element is the
    // outermost svg element.
    Element* e = parentElement();

    while (!e->isSVGSVGElement()) {
        e = e->parentElement();
    }

    return (SVGElement*)e;
}

SVGElement* SVGElement::viewportElement()
{
    // The element which established the current viewport. Often, the nearest
    // ancestor ‘svg’ element. Null if the given element is the outermost svg
    // element.
    return ownerSVGElement();
}

void* SVGNamedElement::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGNamedElement)] = { 0 };
        SVGElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGNamedElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}
}
