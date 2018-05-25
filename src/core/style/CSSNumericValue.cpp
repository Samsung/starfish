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

#include "core/style/CSSNumericValue.h"
#include "core/dom/Document.h"

namespace StarFish {

int32_t CSSNumericType::length()
{
    return m_length;
}
void CSSNumericType::setLength(int length)
{
    m_length = length;
}

int32_t CSSNumericType::angle()
{
    return m_angle;
}
void CSSNumericType::setAngle(int angle)
{
    m_angle = angle;
}

int32_t CSSNumericType::time()
{
    return m_time;
}
void CSSNumericType::setTime(int time)
{
    m_time = time;
}

int32_t CSSNumericType::frequency()
{
    return m_frequency;
}
void CSSNumericType::setFrequency(int frequency)
{
    m_frequency = frequency;
}

int32_t CSSNumericType::resolution()
{
    return m_resolution;
}
void CSSNumericType::setResolution(int resolution)
{
    m_resolution = resolution;
}

int32_t CSSNumericType::flex()
{
    return m_flex;
}
void CSSNumericType::setFlex(int flex)
{
    m_flex = flex;
}

int32_t CSSNumericType::percent()
{
    return m_percent;
}
void CSSNumericType::setPercent(int percent)
{
    m_percent = percent;
}

String* CSSNumericType::percentHint()
{
    switch (m_percentHint) {
    case CSSNumericValue::CSSNumericBaseType::Length:
        return String::createASCIIString("length");
    case CSSNumericValue::CSSNumericBaseType::Angle:
        return String::createASCIIString("angle");
    case CSSNumericValue::CSSNumericBaseType::Time:
        return String::createASCIIString("time");
    case CSSNumericValue::CSSNumericBaseType::Frequency:
        return String::createASCIIString("frequency");
    case CSSNumericValue::CSSNumericBaseType::Resolution:
        return String::createASCIIString("resolution");
    case CSSNumericValue::CSSNumericBaseType::Flex:
        return String::createASCIIString("flex");
    case CSSNumericValue::CSSNumericBaseType::Percent:
        return String::createASCIIString("percent");
    default:
        return String::emptyString;
    }
}
void CSSNumericType::setPercentHint(String* percentHint)
{
    String* hint = percentHint->toLower();
    if (hint->equals("length")) {
        m_percentHint = CSSNumericValue::CSSNumericBaseType::Length;
    } else if (hint->equals("angle")) {
        m_percentHint = CSSNumericValue::CSSNumericBaseType::Angle;
    } else if (hint->equals("time")) {
        m_percentHint = CSSNumericValue::CSSNumericBaseType::Time;
    } else if (hint->equals("frequency")) {
        m_percentHint = CSSNumericValue::CSSNumericBaseType::Frequency;
    } else if (hint->equals("resolution")) {
        m_percentHint = CSSNumericValue::CSSNumericBaseType::Resolution;
    } else if (hint->equals("flex")) {
        m_percentHint = CSSNumericValue::CSSNumericBaseType::Flex;
    } else if (hint->equals("percent")) {
        m_percentHint = CSSNumericValue::CSSNumericBaseType::Percent;
    }
}

void* CSSNumericValue::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(CSSNumericValue));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(CSSNumericValue)] = { 0 };
        CSSNumericValue::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(CSSNumericValue));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

ScriptBindingInstance* CSSNumericValue::scriptBindingInstance()
{
    return document()->scriptBindingInstance();
}

CSSNumericValue::CSSNumericValue(Document* document)
    : CSSStyleValue(document)
{
}

CSSNumericValue* CSSNumericValue::parse(String* cssText)
{
    // TODO
    return nullptr;
}
}
