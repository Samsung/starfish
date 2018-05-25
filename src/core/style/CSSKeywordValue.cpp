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

#include "core/style/CSSKeywordValue.h"
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"

namespace StarFish {

void* CSSKeywordValue::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(CSSKeywordValue));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(CSSKeywordValue)] = { 0 };
        CSSKeywordValue::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(CSSKeywordValue));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

ScriptBindingInstance* CSSKeywordValue::scriptBindingInstance()
{
    return document()->scriptBindingInstance();
}

CSSKeywordValue::CSSKeywordValue(Document* document, String* value)
    : CSSStyleValue(document)
{
    setValue(value);
}

String* CSSKeywordValue::value()
{
    return m_value;
}

void CSSKeywordValue::setValue(String* value)
{
    STARFISH_ASSERT(value);
    if (value->equals(String::emptyString)) {
        throw new DOMException(document(), DOMException::SCRIPT_TYPE_ERR,
                               "TypeError");
    }

    m_value = value;
}
}
