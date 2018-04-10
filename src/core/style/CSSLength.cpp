
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
#include "CSSLength.h"

namespace StarFish {

CSSLength::CSSLength(String* unit, float f)
{
    if (unit->length() == 0 || unit->equals("px")) {
        m_kind = PX;
    } else if (unit->equals("em")) {
        m_kind = EM;
    } else if (unit->equals("ex")) {
        m_kind = EX;
    } else if (unit->equals("in")) {
        m_kind = IN;
    } else if (unit->equals("cm")) {
        m_kind = CM;
    } else if (unit->equals("mm")) {
        m_kind = MM;
    } else if (unit->equals("pt")) {
        m_kind = PT;
    } else if (unit->equals("pc")) {
        m_kind = PC;
    } else if (unit->equals("vw")) {
        m_kind = VW;
    } else if (unit->equals("vh")) {
        m_kind = VH;
    } else if (unit->equals("vmin")) {
        m_kind = VMIN;
    } else if (unit->equals("vmax")) {
        m_kind = VMAX;
    } else if (unit->equals("rem")) {
        m_kind = REM;
    } else if (unit->equals("ch")) {
        m_kind = CH;
    }

    m_value = f;
}

Length CSSLength::toLength() const
{
    if (m_kind == PX) { // absolute length
        return Length(Length::Fixed, m_value);
    } else if (m_kind == CM) {
        return Length(Length::Fixed, convertFromCmToPx(m_value));
    } else if (m_kind == MM) {
        return Length(Length::Fixed, convertFromMmToPx(m_value));
    } else if (m_kind == IN) {
        return Length(Length::Fixed, convertFromInToPx(m_value));
    } else if (m_kind == PC) {
        return Length(Length::Fixed, convertFromPcToPx(m_value));
    } else if (m_kind == PT) {
        return Length(Length::Fixed, convertFromPtToPx(m_value));
    } else if (m_kind == EM) { // font-relative length
        return Length(Length::Em, m_value);
    } else if (m_kind == EX) { // font-relative length
        return Length(Length::Ex, m_value);
    } else if (m_kind == VW) {
        return Length(Length::Vw, m_value);
    } else if (m_kind == VH) {
        return Length(Length::Vh, m_value);
    } else if (m_kind == VMIN) {
        return Length(Length::Vmin, m_value);
    } else if (m_kind == VMAX) {
        return Length(Length::Vmax, m_value);
    } else if (m_kind == REM) { // font-relative length
        return Length(Length::Rem, m_value);
    } else if (m_kind == CH) { // font-relative length
        return Length(Length::Ch, m_value);
    }

    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

String* CSSLength::toString() const
{
    UTF8StringDataNonGCStd stdStr =
        String::fromFloat(m_value)->toUTF8NonGCString();
    if (m_kind == PX) {
        return String::fromUTF8(stdStr.append("px").c_str());
    } else if (m_kind == CM) {
        return String::fromUTF8(stdStr.append("cm").c_str());
    } else if (m_kind == MM) {
        return String::fromUTF8(stdStr.append("mm").c_str());
    } else if (m_kind == IN) {
        return String::fromUTF8(stdStr.append("in").c_str());
    } else if (m_kind == PC) {
        return String::fromUTF8(stdStr.append("pc").c_str());
    } else if (m_kind == PT) {
        return String::fromUTF8(stdStr.append("pt").c_str());
    } else if (m_kind == EM) {
        return String::fromUTF8(stdStr.append("em").c_str());
    } else if (m_kind == EX) {
        return String::fromUTF8(stdStr.append("ex").c_str());
    } else if (m_kind == VW) {
        return String::fromUTF8(stdStr.append("vw").c_str());
    } else if (m_kind == VH) {
        return String::fromUTF8(stdStr.append("vh").c_str());
    } else if (m_kind == VMIN) {
        return String::fromUTF8(stdStr.append("vmin").c_str());
    } else if (m_kind == VMAX) {
        return String::fromUTF8(stdStr.append("vmax").c_str());
    } else if (m_kind == REM) {
        return String::fromUTF8(stdStr.append("rem").c_str());
    } else if (m_kind == CH) {
        return String::fromUTF8(stdStr.append("ch").c_str());
    }
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

} // namespace StarFish
