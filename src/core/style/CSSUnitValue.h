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

#ifndef __StarFishCSSUnitValue__
#define __StarFishCSSUnitValue__

#include "binding/ScriptWrappable.h"
#include "core/style/CSSNumericValue.h"

namespace StarFish {

class Document;

class CSSUnitValue : public CSSNumericValue {
public:
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;
    virtual bool isCSSUnitValue() const override;
    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    CSSUnitValue(Document* document, double value, String* unit)
        : CSSNumericValue(document)
        , m_value(value)
        , m_unit(unit)
    {
    }

    double value()
    {
        return m_value;
    }
    void setValue(double value)
    {
        m_value = value;
    }

    String* unit()
    {
        return m_unit;
    }

protected:
    static inline void fillGCDescriptor(GC_word* obj_bitmap)
    {
        CSSNumericValue::fillGCDescriptor(obj_bitmap);
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(CSSUnitValue, m_unit));
    }

private:
    double m_value;
    String* m_unit;
};
}

#endif
