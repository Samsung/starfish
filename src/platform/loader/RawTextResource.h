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

#ifndef __StarfishRawTextResource__
#define __StarfishRawTextResource__

#include "platform/loader/TextResource.h"

namespace Starfish {

class RawTextResource : public TextResource {
    friend class ResourceLoader;

protected:
    RawTextResource(ResourceURL* url, ResourceLoader* loader,
                    String* preferredEncoding)
        : TextResource(url, loader, preferredEncoding)
        , m_rawData()
    {
    }

public:
    void* operator new(size_t size);
    void clearNativeResources();
    void operator delete(void*)
    {
    }
    void operator delete[](void*) = delete;

    virtual bool isRawTextResource() override
    {
        return true;
    }

    virtual Type type() override
    {
        return Type::RawTextResourceType;
    }

    virtual void didDataReceived(const char* buffer, size_t length) override;

    virtual size_t contentSize() override
    {
        if (!m_rawData.empty()) {
            return m_rawData.size();
        }
        return m_text->contentLength();
    }

    String* text();

    const std::string& rawData() const
    {
        return m_rawData;
    }

    void clearRawData()
    {
        clearNativeResources();
    }

protected:
    std::string m_rawData;
};

} // namespace Starfish

#endif
