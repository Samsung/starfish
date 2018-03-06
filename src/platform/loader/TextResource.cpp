/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "StarFish.h"
#include "core/dom/Document.h"
#include "platform/loader/TextResource.h"
#include "core/page/Window.h"

namespace StarFish {

void TextResource::didDataReceived(const char* buffer, size_t length)
{
    if (!m_converter) {
        if (m_preferredEncoding->equals(String::emptyString)) {
            m_converter = new TextConverter(
                m_resourceRequest->responseMimeType(),
                m_resourceRequest->document()->characterSet(), buffer, length);
        } else {
            m_converter = new TextConverter(m_preferredEncoding);
        }
    }
    m_text = m_text->concat(m_converter->convert(buffer, length, true));

    Resource::didDataReceived(buffer, length);
}
}
