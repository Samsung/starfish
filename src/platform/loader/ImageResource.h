/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishImageResource__
#define __StarFishImageResource__

#include "platform/loader/Resource.h"
#include "core/modules/canvas/image/ImageData.h"

namespace StarFish {

class ImageData;
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

    ImageData* imageData()
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
                         ResourceURL* referrerURL);
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
    ImageData* m_imageData;
    MockHTMLIFrameElement* m_mockFrameForSVGDocument;
};
}

#endif
