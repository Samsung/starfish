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

#if defined(PORT_IMAGEDECODER_BACKEND_MISC)

#include "core/modules/canvas/image/ImageData.h"
#include "ImageDecoder.h"
#include "platform/file/FileIO.h"

namespace StarFish {

class ImageDataMISC : public ImageData {
public:
    ImageDataMISC(String* localImageSrc)
    {
        ImageDecoder* d = new ImageDecoder(localImageSrc->utf8Data());
        m_image = (void*)d->buffer();
        m_width = d->width();
        m_height = d->height();
    }

    ImageDataMISC(const char* buf, size_t len)
    {
    }

    virtual size_t bufferSize()
    {
        if (m_image) {
            return m_width * m_height * 4;
        } else {
            return 0;
        }
    }

    void reigsterFinalizer()
    {
        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           // STARFISH_LOG_INFO("ImageDataEFL::~ImageDataEFL\n");
                                       },
                                       m_image, NULL, NULL);
    }

    virtual void* unwrap()
    {
        return m_image;
    }

    virtual size_t width()
    {
        return m_width;
    }

    virtual size_t height()
    {
        return m_height;
    }

protected:
    void* m_image;
    size_t m_width;
    size_t m_height;
};

ImageData* ImageData::create(String* localImageSrc)
{
    ImageData* imageData = new ImageDataMISC(localImageSrc);
    if (imageData->unwrap() == NULL) {
        return NULL;
    }
    return imageData;
}

ImageData* ImageData::create(const char* buf, size_t len)
{
    ImageData* imageData = new ImageDataMISC(buf, len);
    if (imageData->unwrap() == NULL) {
        return NULL;
    }
    return imageData;
}
}

#endif
