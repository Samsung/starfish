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

#ifndef __StarFishCSSKeywordValue__
#define __StarFishCSSKeywordValue__

#include "binding/ScriptWrappable.h"

#include "core/style/CSSStyleValue.h"

namespace StarFish {

class Document;

class CSSKeywordValue : public CSSStyleValue {
public:
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;
    virtual bool isCSSKeywordValue() const override;
    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    CSSKeywordValue(Document* document, String* value);

    String* value();
    void setValue(String* value);

protected:
    static inline void fillGCDescriptor(GC_word* obj_bitmap)
    {
        CSSStyleValue::fillGCDescriptor(obj_bitmap);
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(CSSKeywordValue, m_value));
    }

private:
    String* m_value;
};
}

#endif
