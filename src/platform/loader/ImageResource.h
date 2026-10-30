/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishImageResource__
#define __StarfishImageResource__

#include "platform/loader/Resource.h"
#include "core/modules/canvas/image/NativeImageData.h"

namespace Starfish {

class NativeImageData;
class MockHTMLIFrameElement;

class ImageResource : public Resource {
    friend class ResourceLoader;
    friend class MockHTMLIFrameElement;

    ImageResource(ResourceURL* url, ResourceLoader* loader)
        : Resource(url, loader)
        , m_isCORSRequest(false)
        , m_isCredentialedCORSRequest(false)
        , m_isCORSSameOrigin(false)
    {
        m_imageData = nullptr;
        m_mockFrameForSVGDocument = nullptr;
        m_devicePixelRatioAtFetch = 1;
    }

public:
    // devicePixelRatio captured when this resource was fetched: decode size
    // (see ImageResource::didLoadFinished) is pinned to that value, so
    // ComputedStyle's same-URL resource-reuse checks must also compare this
    // before reusing a previously fetched resource across a DPR change.
    float devicePixelRatioAtFetch()
    {
        return m_devicePixelRatioAtFetch;
    }
    void setDevicePixelRatioAtFetch(float dpr)
    {
        m_devicePixelRatioAtFetch = dpr;
    }

    virtual void prepare()
    {
        if (m_resourceRequest != nullptr) {
            m_resourceRequest->setRequestHeader(
                String::createASCIIString(HTTPHeaderMap::kAccept,
                                          strlen(HTTPHeaderMap::kAccept)),
                String::createASCIIString("*/*"));
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

    virtual void request(RequestData* requestData, bool allowCache) override
    {
        m_isCORSRequest = requestData->m_mode == RequestMode::CORS;
        m_isCredentialedCORSRequest =
            m_isCORSRequest &&
            requestData->m_credentials == RequestCredentials::Include;
        Resource::request(requestData, allowCache);
    }

    // Whether the loaded image is CORS-same-origin, decided once from the
    // response when the load completed; drawing a CORS-cross-origin image
    // into a canvas taints it.
    bool isCORSSameOrigin()
    {
        return m_isCORSSameOrigin;
    }

    // The image cache key. A CORS request must never be answered with a
    // no-cors response or the reverse, so the CORS mode and credentials are
    // part of it (https://fetch.spec.whatwg.org/#http-network-or-cache-fetch).
    UTF8String cacheKey();

    virtual void didLoadFinished();
    virtual void didCacheHit(Resource* cache)
    {
        m_imageData = cache->asImageResource()->m_imageData;
        m_requestErrorType = cache->asImageResource()->requestErrorType();
        m_isCORSSameOrigin = cache->asImageResource()->m_isCORSSameOrigin;
        Resource::didLoadFinished();
    }

protected:
    NativeImageData* m_imageData;
    MockHTMLIFrameElement* m_mockFrameForSVGDocument;
    float m_devicePixelRatioAtFetch;
    bool m_isCORSRequest : 1;
    bool m_isCredentialedCORSRequest : 1;
    bool m_isCORSSameOrigin : 1;
#if defined(STARFISH_ENABLE_MULTI_THREAD_IMAGE_DECODING)
    void* m_pendingDecodeData{ nullptr };
#endif
};
} // namespace Starfish

#endif
