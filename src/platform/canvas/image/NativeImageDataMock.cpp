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

#include "StarFishConfig.h"

#if defined(PORT_IMAGEDECODER_BACKEND_MOCK)

#include "core/modules/canvas/image/NativeImageData.h"

namespace StarFish {

class NativeImageDataMock : public NativeImageData {
public:
    NativeImageDataMock(String* localImageSrc)
    {
    }

    NativeImageDataMock(const char* buf, size_t len)
    {
    }

    NativeImageDataMock(size_t w, size_t h)
    {
    }

    virtual uint8_t* data()
    {
        return nullptr;
    }

    virtual void clear()
    {
    }

    virtual size_t bufferSize()
    {
        return 0;
    }

    virtual bool hasTransparentPixel()
    {
        return false;
    }

    virtual void* internalSurface()
    {
        return nullptr;
    }

    virtual void* unwrap()
    {
        return nullptr;
    }

    virtual size_t width()
    {
        return 0;
    }

    virtual size_t stride()
    {
        return 0;
    }

    virtual size_t height()
    {
        return 0;
    }

protected:
};

NativeImageData* NativeImageData::create(String* localImageSrc)
{
    NativeImageData* imageData = new NativeImageDataMock(localImageSrc);
    if (imageData->data() == NULL) {
        return NULL;
    }
    return imageData;
}

NativeImageData* NativeImageData::create(const char* buf, size_t len)
{
    NativeImageData* imageData = new NativeImageDataMock(buf, len);
    if (imageData->data() == NULL) {
        return NULL;
    }
    return imageData;
}

NativeImageData* NativeImageData::create(size_t width, size_t height)
{
    return new NativeImageDataMock(width, height);
}
}

#endif
