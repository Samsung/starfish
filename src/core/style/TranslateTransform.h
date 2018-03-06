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

#ifndef __StarFishTranslateTransform__
#define __StarFishTranslateTransform__

#include "core/style/Style.h"

namespace StarFish {

class TranslateTransform : public gc {
public:
    TranslateTransform(Length& tx, Length& ty)
        : m_tx(tx)
        , m_ty(ty)
    {
    }

    ~TranslateTransform()
    {
    }

    void setData(Length& a, Length& b)
    {
        m_tx = a;
        m_ty = b;
    }

    Length tx()
    {
        return m_tx;
    }
    Length ty()
    {
        return m_ty;
    }

    bool operator==(const TranslateTransform& o)
    {
        return (this->m_tx == o.m_tx) && (this->m_ty == o.m_ty);
    }

    bool operator!=(const TranslateTransform& o)
    {
        return !operator==(o);
    }

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(TranslateTransform)] = { 0 };
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(TranslateTransform, m_tx));
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(TranslateTransform, m_ty));
            descr =
                GC_make_descriptor(obj_bitmap, GC_WORD_LEN(TranslateTransform));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

    Length m_tx;
    Length m_ty;
};
}

#endif
