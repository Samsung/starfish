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
