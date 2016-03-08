#include "StarFishConfig.h"
#include "ImageData.h"

#include <Elementary.h>

namespace StarFish {

Evas* internalCanvas();

class ImageDataEFL : public ImageData {
public:
    ImageDataEFL(String* imageSrc)
    {
        m_image = evas_object_image_add(internalCanvas());
        evas_object_image_file_set(m_image, imageSrc->utf8Data(), NULL);
        STARFISH_RELEASE_ASSERT(evas_object_image_colorspace_get(m_image) == EVAS_COLORSPACE_ARGB8888);

        int w, h;
        evas_object_image_size_get(m_image, &w, &h);
        m_width = w;
        m_height = h;
    }

    ImageDataEFL(size_t w, size_t h)
    {
        m_image = evas_object_image_add(internalCanvas());
        evas_object_image_size_set(m_image, w, h);
        evas_object_image_filled_set(m_image, EINA_TRUE);
        evas_object_image_size_set(m_image, m_width, m_height);
        evas_object_image_colorspace_set(m_image, Evas_Colorspace::EVAS_COLORSPACE_ARGB8888);
        evas_object_image_alpha_set(m_image, EINA_TRUE);
        STARFISH_RELEASE_ASSERT(evas_object_image_colorspace_get(m_image) == EVAS_COLORSPACE_ARGB8888);
        m_width = w;
        m_height = h;

        STARFISH_LOG_INFO("crate ImageData %p", m_image);
    }

    ~ImageDataEFL()
    {
        STARFISH_LOG_INFO("release ImageData %p", m_image);
        evas_object_unref(m_image);
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
    Evas_Object* m_image;
    size_t m_width;
    size_t m_height;
};

class ImageDataNetwork : public ImageData {
public:
    ImageDataNetwork(uint32_t size, void* data)
    {
        m_image = evas_object_image_add(internalCanvas());
        char format[4] = "";
        evas_object_image_memfile_set(m_image, data, size, format, NULL);

        STARFISH_RELEASE_ASSERT(evas_object_image_colorspace_get(m_image) == EVAS_COLORSPACE_ARGB8888);

        int w, h;
        evas_object_image_size_get(m_image, &w, &h);
        m_width = w;
        m_height = h;
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
    Evas_Object* m_image;
    size_t m_width;
    size_t m_height;
};

ImageData* ImageData::create(String* imageSrc)
{
    return new ImageDataEFL(imageSrc);
}

ImageData* ImageData::create(size_t w, size_t h)
{
    return new ImageDataEFL(w, h);
}

ImageData* ImageData::create(uint32_t size, void* data)
{
    return new ImageDataNetwork(size, data);
}

}
