/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
