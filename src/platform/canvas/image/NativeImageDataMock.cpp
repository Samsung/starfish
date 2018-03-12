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

#include "StarFishConfig.h"

#if defined(PORT_IMAGEDECODER_BACKEND_MOCK)

#include "core/modules/canvas/image/NativeImageData.h"

namespace StarFish {

class NativeImageDataMock : public NativeImageData {
public:
    void* operator new(size_t size)
    {
        return GC_GENERIC_MALLOC(sizeof(NativeImageDataMock),
                                 NativeImageData::nativeImageDataGCKind());
    }

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
