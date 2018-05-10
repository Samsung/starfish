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

#include "core/style/CSSStyleValue.h"
#include "core/dom/Document.h"

namespace StarFish {

void* CSSStyleValue::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(CSSStyleValue)] = { 0 };
        CSSStyleValue::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(CSSStyleValue));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

ScriptBindingInstance* CSSStyleValue::scriptBindingInstance()
{
    return document()->scriptBindingInstance();
}

// https://drafts.css-houdini.org/css-typed-om/#stylevalue-objects
CSSStyleValue* CSSStyleValue::parse(String* property, String* cssText)
{
    String* prop = String::emptyString;
    if (!property->startsWith("--")) {
        prop = property->toLower();
    }

    // TODO: create CSSStyle* according to property
    return nullptr;
}

GCVector<CSSStyleValue*> CSSStyleValue::parseAll(String* property,
                                                 String* cssText)
{
    // TODO: create CSSStyle* according to property
    return GCVector<CSSStyleValue*>();
}
}
