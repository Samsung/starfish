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

#include "core/modules/canvas/image/ImageData.h"

namespace StarFish {

class ImageDataMock : public ImageData {
public:
    ImageDataMock(String* localImageSrc)
    {
    }

    ImageDataMock(const char* buf, size_t len)
    {
    }

    ImageDataMock(size_t w, size_t h)
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

ImageData* ImageData::create(String* localImageSrc)
{
    ImageData* imageData = new ImageDataMock(localImageSrc);
    if (imageData->data() == NULL) {
        return NULL;
    }
    return imageData;
}

ImageData* ImageData::create(const char* buf, size_t len)
{
    ImageData* imageData = new ImageDataMock(buf, len);
    if (imageData->data() == NULL) {
        return NULL;
    }
    return imageData;
}

ImageData* ImageData::create(size_t width, size_t height)
{
    return new ImageDataMock(width, height);
}
}

#endif
