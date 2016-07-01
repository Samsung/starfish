/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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
#include "ImageData.h"
#include "platform/file_io/FileIO.h"

#include <Elementary.h>

namespace StarFish {

Evas* internalCanvas();

class ImageDataEFL : public ImageData {
public:
    ImageDataEFL(String* imageSrc)
    {
        m_image = evas_object_image_add(internalCanvas());
        evas_object_image_file_set(m_image, PathResolver::matchLocation(imageSrc)->utf8Data(), NULL);
        STARFISH_RELEASE_ASSERT(evas_object_image_colorspace_get(m_image) == EVAS_COLORSPACE_ARGB8888);
        int w, h, err;
        err = evas_object_image_load_error_get(m_image);
        if (err == EVAS_LOAD_ERROR_NONE) {
            evas_object_image_size_get(m_image, &w, &h);
            if (w >= 0 && h >= 0) {
                m_width = w;
                m_height = h;

                reigsterFinalizer();
                return;
            }
        }
        evas_object_del(m_image);
        m_image = NULL;
    }

    ImageDataEFL(const char* buf, size_t len)
    {
        m_image = evas_object_image_add(internalCanvas());
        char format[4] = "";
        evas_object_image_memfile_set(m_image, (void*)buf, (int)len, format, NULL);
        STARFISH_RELEASE_ASSERT(evas_object_image_colorspace_get(m_image) == EVAS_COLORSPACE_ARGB8888);
        int w, h, err;
        err = evas_object_image_load_error_get(m_image);
        if (err == EVAS_LOAD_ERROR_NONE) {
            evas_object_image_size_get(m_image, &w, &h);
            if (w >= 0 && h >= 0) {
                m_width = w;
                m_height = h;

                reigsterFinalizer();
                return;
            }
        }
        evas_object_del(m_image);
        m_image = NULL;
    }

    ImageDataEFL(size_t w, size_t h)
    {
        m_image = evas_object_image_add(internalCanvas());
        m_width = w;
        m_height = h;

        evas_object_image_size_set(m_image, w, h);
        evas_object_image_filled_set(m_image, EINA_TRUE);
        evas_object_image_colorspace_set(m_image, Evas_Colorspace::EVAS_COLORSPACE_ARGB8888);
        evas_object_image_alpha_set(m_image, EINA_TRUE);

        reigsterFinalizer();
    }

    void reigsterFinalizer()
    {
        GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
            // STARFISH_LOG_INFO("ImageDataEFL::~ImageDataEFL\n");
            Evas_Object* m = (Evas_Object*)cd;
            evas_object_hide(m);
            evas_object_del(m);
        }, m_image, NULL, NULL);
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

    virtual void clear()
    {
        void* address = evas_object_image_data_get(m_image, EINA_TRUE);
        size_t end = m_width * m_height * sizeof(uint32_t);
        memset(address, 0, end);
        evas_object_image_data_set(m_image, address);
    }

protected:
    Evas_Object* m_image;
    size_t m_width;
    size_t m_height;
};

ImageData* ImageData::create(String* imageSrc)
{
    ImageData* imageData = new ImageDataEFL(imageSrc);
    if (imageData->unwrap() == NULL)
        return NULL;
    return imageData;
}

ImageData* ImageData::create(const char* buf, size_t len)
{
    ImageData* imageData = new ImageDataEFL(buf, len);
    if (imageData->unwrap() == NULL)
        return NULL;
    return imageData;
}

ImageData* ImageData::create(size_t w, size_t h)
{
    return new ImageDataEFL(w, h);
}

}
