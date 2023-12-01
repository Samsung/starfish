/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#include "StarfishConfig.h"
#include "ResizeObserverOptions.h"

namespace Starfish {

ResizeObserverOptions::ResizeObserverOptions()
    : m_box(ResizeObserverBoxOptions::ContentBox)
{
}

ResizeObserverOptions::ResizeObserverOptions(ResizeObserverBoxOptions option)
    : m_box(option)
{
}

void ResizeObserverOptions::setBox(String* box)
{
    if (box->equals("border-box")) {
        m_box = ResizeObserverBoxOptions::BorderBox;
    } else if (box->equals("device-pixel-content-box")) {
        m_box = ResizeObserverBoxOptions::DevicePixelContentBox;
    } else if (box->equals("content-box")) {
        m_box = ResizeObserverBoxOptions::ContentBox;
    }
}

String* ResizeObserverOptions::box() const
{
    switch (m_box) {
    case ResizeObserverBoxOptions::BorderBox:
        return String::createASCIIString("border-box");
        break;
    case ResizeObserverBoxOptions::DevicePixelContentBox:
        return String::createASCIIString("device-pixel-content-box");
        break;
    case ResizeObserverBoxOptions::ContentBox:
    default:
        break;
    }
    return String::createASCIIString("content-box");
}

} // namespace Starfish
