/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
#include "platform/loader/RawTextResource.h"
#include "core/page/Window.h"

namespace Starfish {

static void rawTextResourceClear(void* obj, void* cd)
{
    RawTextResource* self = reinterpret_cast<RawTextResource*>(obj);
    self->clearNativeResources();
}

void* RawTextResource::operator new(size_t size)
{
    constexpr static GC_finalizer_closure data = { rawTextResourceClear,
                                                   nullptr };
    return GC_finalized_malloc(size, &data);
}

void RawTextResource::clearNativeResources()
{
    std::string().swap(m_rawData);
}

void RawTextResource::didDataReceived(const char* buffer, size_t length)
{
    if (length != 0) {
        m_rawData.append(buffer, length);
    }
    if (!m_converter) {
        if (m_preferredEncoding->equals(String::emptyString)) {
            m_converter = new TextConverter(
                m_resourceRequest ? m_resourceRequest->responseMimeType()
                                  : String::emptyString,
                m_resourceRequest && m_resourceRequest->executionContext()
                    ? m_resourceRequest->executionContext()->characterSet()
                    : nullptr,
                buffer, length);
        } else {
            m_converter = new TextConverter(m_preferredEncoding);
        }
    }

    Resource::didDataReceived(buffer, length);
}

String* RawTextResource::text()
{
    if (m_text->isEmpty() && !m_rawData.empty()) {
        if (m_converter) {
            m_text =
                m_converter->convert(m_rawData.data(), m_rawData.size(), true);
        } else {
            m_text = String::fromUTF8(m_rawData.data(), m_rawData.size());
        }
    }
    return m_text;
}

} // namespace Starfish
