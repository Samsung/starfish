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

#include "core/fileapi/BlobPropertyBag.h"

namespace Starfish {

BlobPropertyBag::BlobPropertyBag()
    : m_type(String::emptyString)
{
}

String* BlobPropertyBag::endings() const
{
    if (m_hasEndings) {
        if (m_endings == EndingType::kTransparent) {
            return String::createASCIIString("transparent");
        } else if (m_endings == EndingType::kNative) {
            return String::createASCIIString("native");
        }
    }
    return String::emptyString;
}

void BlobPropertyBag::setEndings(String* endings)
{
    if (endings->equals("transparent")) {
        m_hasEndings = true;
        m_endings = EndingType::kTransparent;
    } else if (endings->equals("native")) {
        m_hasEndings = true;
        m_endings = EndingType::kNative;
    } else {
        m_hasEndings = false;
    }
}

} // namespace Starfish
