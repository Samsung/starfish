/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#if defined(PORT_IMAGEDECODER_BACKEND_EFL)
#include "core/modules/canvas/image/NativeImageData.h"
#include "platform/file/File.h"

#include <Elementary.h>

extern Evas* g_internalCanvas;

namespace StarFish {

Evas* internalCanvas();

class NativeImageDataEFL : public NativeImageData {
public:
    void* operator new(size_t size)
    {
        return GC_GENERIC_MALLOC(sizeof(NativeImageDataEFL),
                                 NativeImageData::nativeImageDataGCKind());
    }

    NativeImageDataEFL(String* localImageSrc)
    {
        // STARFISH_LOG_INFO("NativeImageDataEFL::NativeImageDataEFL %s\n",
        // localImageSrc->toUTF8NonGCString().data();
        m_image = evas_object_image_add(internalCanvas());
        m_hasTransparentPixel = true;
        auto utf8Data =
            PathResolver::matchLocation(localImageSrc)->toUTF8NonGCString();
        evas_object_image_file_set(m_image, utf8Data.data(), NULL);
        evas_object_data_set(m_image, "local", "1");
        int w, h, err;
        err = evas_object_image_load_error_get(m_image);
        if (err == EVAS_LOAD_ERROR_NONE) {
            evas_object_image_size_get(m_image, &w, &h);
            if (w >= 0 && h >= 0) {
                m_width = w;
                m_height = h;
                return;
            }
        }
        evas_object_del(m_image);
        m_image = NULL;
    }

    NativeImageDataEFL(const char* buf, size_t len)
    {
        m_image = evas_object_image_add(internalCanvas());
        m_hasTransparentPixel = true;
        evas_object_data_set(m_image, "local", "0");
        char format[4] = "";
        evas_object_image_memfile_set(m_image, (void*)buf, (int)len, format,
                                      NULL);
        int w, h, err;
        err = evas_object_image_load_error_get(m_image);
        if (err == EVAS_LOAD_ERROR_NONE) {
            evas_object_image_size_get(m_image, &w, &h);
            if (w >= 0 && h >= 0) {
                m_width = w;
                m_height = h;
                return;
            }
        }
        evas_object_del(m_image);
        m_image = NULL;
    }

    NativeImageDataEFL(size_t w, size_t h)
    {
        m_image = evas_object_image_add(internalCanvas());
        m_hasTransparentPixel = true;

        evas_object_data_set(m_image, "local", "0");
        evas_object_image_size_set(m_image, w, h);
        evas_object_image_filled_set(m_image, EINA_TRUE);
#ifndef STARFISH_TIZEN_TV
        evas_object_image_colorspace_set(
            m_image, Evas_Colorspace::EVAS_COLORSPACE_ARGB8888);
#endif
        evas_object_image_alpha_set(m_image, EINA_TRUE);
        evas_object_anti_alias_set(m_image, EINA_TRUE);
        STARFISH_RELEASE_ASSERT(evas_object_image_colorspace_get(m_image) ==
                                EVAS_COLORSPACE_ARGB8888);
        m_width = w;
        m_height = h;
    }

    virtual ~NativeImageDataEFL()
    {
        disposeNativeImageData();
    }

    virtual void clear()
    {
        void* address = evas_object_image_data_get(m_image, EINA_TRUE);
        size_t end = m_width * m_height * sizeof(uint32_t);
        memset(address, 0x00, end);
        evas_object_image_data_set(m_image, address);
    }

    virtual uint8_t* data()
    {
        void* address = evas_object_image_data_get(m_image, EINA_FALSE);
        evas_object_image_data_set(m_image, address);
        return (uint8_t*)address;
    }

    virtual size_t bufferSize()
    {
        if (m_image) {
            return m_width * m_height * 4;
        } else {
            return 0;
        }
    }

    void isThereTransparentPixel()
    {
        uint8_t* ptr = data();
        size_t stride = this->stride();

        for (size_t y = 0; y < m_height; y++) {
            uint8_t* b = ptr;
            for (size_t x = 0; x < m_width; x++) {
                if (b[3] != 255) {
                    m_hasTransparentPixel = true;
                    return;
                }
                b += 4;
            }

            ptr += stride;
        }
    }

    virtual void disposeNativeImageData()
    {
        if (g_internalCanvas && m_image) {
            evas_object_hide(m_image);
            evas_object_del(m_image);
        }
        NativeImageData::disposeNativeImageData();
    }

    virtual void* unwrap()
    {
        return m_image;
    }

    virtual void* internalSurface()
    {
        return nullptr;
    }

    virtual size_t width()
    {
        return m_width;
    }

    virtual size_t stride()
    {
        return evas_object_image_stride_get(m_image);
    }

    virtual size_t height()
    {
        return m_height;
    }

    virtual bool hasTransparentPixel()
    {
        return m_hasTransparentPixel;
    }

protected:
    bool m_hasTransparentPixel;
    Evas_Object* m_image;
    size_t m_width;
    size_t m_height;
};

NativeImageData* NativeImageData::create(String* localImageSrc)
{
    NativeImageData* imageData = new NativeImageDataEFL(localImageSrc);
    if (imageData->unwrap() == NULL) {
        return NULL;
    }
    return imageData;
}

NativeImageData* NativeImageData::create(const char* buf, size_t len)
{
    NativeImageData* imageData = new NativeImageDataEFL(buf, len);
    if (imageData->unwrap() == NULL) {
        return NULL;
    }
    return imageData;
}

NativeImageData* NativeImageData::create(size_t width, size_t height)
{
    return new NativeImageDataEFL(width, height);
}
}

#endif
