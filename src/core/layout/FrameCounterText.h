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

#ifndef __StarFishFrameCounterText__
#define __StarFishFrameCounterText__

#include "core/layout/FrameText.h"

namespace StarFish {

class CounterContentData;
class CounterStyle;

class FrameCounterText final : public FrameText {
public:
    enum CounterType {
        CounterTypeListOutside,
        CounterTypeListInside,
        CounterTypePseudoContent,
    };

    FrameCounterText(Node* node, CounterType type);

    bool isFrameCounterText() const override
    {
        return true;
    }

    virtual const char* name() override
    {
        return "FrameCounterText";
    }

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(FrameCounterText)] = { 0 };
            FrameCounterText::fillGCDescriptor(desc);
            descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameCounterText));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }

    void* operator new[](size_t size) = delete;

    const AtomicString& counterId() const;
    const CounterContentData* counterContentData() const;

    bool isPseudoContentType() const
    {
        return m_type == CounterTypePseudoContent;
    }

    bool isListType() const
    {
        return m_type != CounterTypePseudoContent;
    }

    bool isListOutsideType() const
    {
        return m_type == CounterTypeListOutside;
    }

    bool isListInsideType() const
    {
        return m_type == CounterTypeListInside;
    }

    void updateCounterText(int32_t index);
    void updateCounterText(std::vector<int32_t>& indice);

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        FrameText::fillGCDescriptor(desc);
    }

    const CounterStyle* counterStyle() const;

protected:
    CounterType m_type;
};
}

#endif
