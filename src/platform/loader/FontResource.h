/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishFontResource__
#define __StarFishFontResource__

#include "platform/loader/Resource.h"
#include "core/modules/canvas/font/Font.h"

namespace StarFish {

class FontResource : public Resource {
    friend class ResourceLoader;

    FontResource(ResourceURL* url, ResourceLoader* loader)
        : Resource(url, loader)
    {
        m_fontFace = nullptr;
    }

public:
    virtual void prepare()
    {
        if (m_resourceRequest) {
            m_resourceRequest->setRequestHeader(
                String::createASCIIString(HTTPHeaderMap::kAccept),
                String::createASCIIString(
                    "application/x-font-ttf,application/x-font-woff"));
        }
    }

    virtual bool isFontResource()
    {
        return true;
    }

    FontFace* fontFace()
    {
        return m_fontFace;
    }

    virtual size_t contentSize()
    {
        return m_fontFace ? m_fontFace->dataSize() : 0;
    }

    virtual Type type()
    {
        return Type::FontResourceType;
    }

    virtual void didLoadFinished();
    virtual void didCacheHit(Resource* cache)
    {
        m_fontFace = cache->asFontResource()->m_fontFace;
        Resource::didLoadFinished();
    }

protected:
    FontFace* m_fontFace;
};
}

#endif
