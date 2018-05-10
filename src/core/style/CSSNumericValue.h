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

#ifndef __StarFishCSSNumericValue__
#define __StarFishCSSNumericValue__

#include "binding/ScriptWrappable.h"

#include "core/style/CSSStyleValue.h"

namespace StarFish {

class Document;

class CSSNumericValue : public CSSStyleValue {
public:
    enum CSSNumericBaseType {
        Length,
        Angle,
        Time,
        Frequency,
        Resolution,
        Flex,
        Percent,
        Null,
    };

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;
    virtual bool isCSSNumericValue() const override;
    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    CSSNumericValue(Document* document);
    static CSSNumericValue* parse(String* cssText);

protected:
    static inline void fillGCDescriptor(GC_word* obj_bitmap)
    {
        CSSStyleValue::fillGCDescriptor(obj_bitmap);
    }

private:
};

struct CSSNumericType {
public:
    CSSNumericType()
        : m_length(0)
        , m_angle(0)
        , m_time(0)
        , m_frequency(0)
        , m_resolution(0)
        , m_flex(0)
        , m_percent(0)
        , m_percentHint(CSSNumericValue::CSSNumericBaseType::Null)
    {
    }

    int32_t length();
    void setLength(int length);

    int32_t angle();
    void setAngle(int angle);

    int32_t time();
    void setTime(int time);

    int32_t frequency();
    void setFrequency(int frequency);

    int32_t resolution();
    void setResolution(int resolution);

    int32_t flex();
    void setFlex(int flex);

    int32_t percent();
    void setPercent(int percent);

    String* percentHint();
    void setPercentHint(String* percentHint);

private:
    int32_t m_length;
    int32_t m_angle;
    int32_t m_time;
    int32_t m_frequency;
    int32_t m_resolution;
    int32_t m_flex;
    int32_t m_percent;
    CSSNumericValue::CSSNumericBaseType m_percentHint;
};
}

#endif
