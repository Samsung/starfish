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

#ifndef __StarFishImageResource__
#define __StarFishImageResource__

#include "platform/loader/Resource.h"
#include "core/modules/canvas/image/NativeImageData.h"

namespace StarFish {

class NativeImageData;
class MockHTMLIFrameElement;

class ImageResource : public Resource {
    friend class ResourceLoader;
    friend class MockHTMLIFrameElement;

    ImageResource(ResourceURL* url, ResourceLoader* loader)
        : Resource(url, loader)
    {
        m_imageData = nullptr;
        m_mockFrameForSVGDocument = nullptr;
    }

public:
    virtual void prepare()
    {
        if (m_resourceRequest) {
            m_resourceRequest->setRequestHeader(
                String::createASCIIString(HTTPHeaderMap::kAccept),
                String::createASCIIString(
                    "image/png , image/jpeg , image/gif"));
        }
    }

    virtual bool isImageResource()
    {
        return true;
    }

    NativeImageData* imageData()
    {
        return m_imageData;
    }

    virtual size_t contentSize()
    {
        return m_imageData ? m_imageData->bufferSize() : 0;
    }

    virtual Type type()
    {
        return Type::ImageResourceType;
    }

    virtual void request(ResourceRequestSyncLevel syncLevel,
                         ResourceURL* referrerURL, bool allowCache = false);
    virtual void didLoadFinished();
    virtual void didCacheHit(Resource* cache)
    {
        m_imageData = cache->asImageResource()->m_imageData;
        Resource::didLoadFinished();
    }
#if defined(PORT_CANVAS_BACKEND_EFL)
    static void doLoadFile(void*);
#endif
protected:
    NativeImageData* m_imageData;
    MockHTMLIFrameElement* m_mockFrameForSVGDocument;
};
}

#endif
