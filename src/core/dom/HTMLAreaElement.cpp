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
#include "StarFish.h"

#include "core/dom/HTMLAreaElement.h"

#include "core/dom/Document.h"
#include "core/dom/DOMTokenList.h"

namespace StarFish {

void* HTMLAreaElement::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLAreaElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLAreaElement, m_relList));
        HTMLElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLAreaElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

DOMTokenList* HTMLAreaElement::relList()
{
    if (!m_relList) {
        m_relList = new DOMTokenList(this, starFish()->staticStrings()->m_rel);
    }
    return m_relList;
}

String* HTMLAreaElement::referrerPolicy()
{
    return getAttributeOrEmpty(
        document()->starFish()->staticStrings()->m_referrerpolicy);
}

void HTMLAreaElement::setReferrerPolicy(String* policy)
{
    if (ReferrerURL::isValidPolicy(policy)) {
        setAttribute(document()->starFish()->staticStrings()->m_referrerpolicy,
                     policy);
    }
}

QualifiedName HTMLAreaElement::name()
{
    return starFish()->staticStrings()->m_areaTagName;
}
}
