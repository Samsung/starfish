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

#ifndef __StarFishFrameQuoteText__
#define __StarFishFrameQuoteText__

#include "core/layout/FrameText.h"

namespace StarFish {

class FrameQuoteText : public FrameText {
public:
    FrameQuoteText(Node* node, QuoteValue val);

    bool isFrameQuoteText() const
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameQuoteText";
    }

    QuoteValue quote()
    {
        return m_quote;
    }

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(FrameQuoteText)] = { 0 };
            FrameText::fillGCDescriptor(obj_bitmap);
            descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FrameQuoteText));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }

    void* operator new[](size_t size) = delete;

protected:
    QuoteValue m_quote;
};
}

#endif
